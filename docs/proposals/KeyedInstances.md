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

### T2a — `-df-out` (record written pre-commit, at the owner brief)

The DataFlow BB-with-arguments dump, implemented against the §10-
amended emitter pseudocode + spec v3/v3.1/v3.2 pins. Shape: QueryDF
tag-struct operator<< (include/.../DataFlow/Format.h; emitter in
lib/DataFlow/Format.cpp beside the DOT operator); Main.cpp gDFStream
+ -df-out arm on the -dot-out mold, drained post-Program::Build
(TableId populated). Emitter: kind-tagged det_seq-order traversal
(the ten public per-kind iterators in IMPL order, is_dead-skipped —
A1; NEVER the joins-first public ForEachView — L1); PASS-1 seen-
bitset bijection witness (always-on fprintf+abort, raw impl->det_seq
read, N==0-safe); `=>` edge model built from every user's input
columns (p2 bare identity, p3 producer-token .in<K> in join-port
order); iterative Tarjan over the emitter's OWN `=>` edge set (A3 —
never Successors(), which carries INSERT→SELECT materialization
edges) for reachability-exact `; cycle`; p5-p9 grammar (ATTRIBUTES
keyword, byte-52 comments, p7 provenance, p9 join bodies, typed
tokens via `os << *Variable()` — A2 AutoVar_N).

FIRST EMISSION vs the byte-contracts: transitive_closure and
demand-ON demand_tc_witness BYTE-EXACT ON THE FIRST RUN;
symrec_tie_1 byte-exact after its pre-registered ILLUSTRATIVE
tuple-id pin (role bijection verified 1:1 against the FIRM
(role, stratum, table, edge-shape) triples; the det_seq tie-break
arm2=^join.8/arm3=^join.9 and the .in<K> code-read predictions —
tc R2's F3 falsifiability cross-check — all HELD LIVE; pin recorded
in the artifact banner with the old->new id read-through table).

FABLE REVIEW (workflow, 17 agents ~781k tokens): 1 CONFIRMED crash
— the select provenance called QueryIO::From on CONSTANT streams
(clause literals / condition TrueColumn / tags; conditions_to_bools
repro exit 134) — fixed with the IsIO() guard (constant-stream
selects render NO provenance comment pending PIN-1); 1 CONFIRMED
dump defect — compare header rendered [copied, LHS, RHS], a
permutation of the finalized [LHS(,RHS), copied] order — fixed to
QueryView::Columns() order (edge ports likewise); 2 CONFIRMED doc
defects in the symrec pin paragraph (6-edge -> 7-EDGE cycle count;
§2 pre-pin ids now read THROUGH the explicit old->new table) —
fixed; PIN-3 recorded (below); cleanups applied: dead min_port
removed, insert header render deduped, first-char kind dispatch ->
enum indices, ref() checked (DF-REF abort). Recorded not-applied:
the for_each_df_view/Tarjan duplication notes (the impl ForEachView
is private; Stratify's Tarjan is file-static — refactor deferred).

PIN-3 [DISCHARGED 2026-07-23 by the standalone pre-diff, §20(N);
contracts pin3-design.md + pin3-desired-states.md — class= is now
TABLE-LEVEL producer-inclusive; negate_1.df.opt + aggregate_1.df.opt
are the standing fences] (owner, from the review — blocked only
negate-carrying bless):
class= is per-view CanReceiveDeletions; a non-@never NEGATE's own
table is deletion-capable via its crossover while the negate view
does not receive deletions, so a negate block labels its own table
monotone while table-sharing views say differential. Refine
(producer-side / table-level) before any negate-carrying dump is
blessed. In-code comment at attrs_line carries it.

GATES (all green, RE-RUN in full after the review fixes): byte-diff
vs the three contracts EXACT; 5-run dump determinism (3 carriers,
1 hash each, unchanged by the fixes); FULL SUITE PASS (169), zero
stdout churn; 676-row corpus A/B (169×4, exit+.h+.cpp+.ir) BYTE-
IDENTICAL vs the frozen 35b89aab baseline — flag-off invisibility
held exactly; data/ 36-file A/B clean; ctest 3/3; constant-stream +
compare/negate crash repros exit 0; Q5 progsize@128 release SAME-
SESSION INTERLEAVED ABABAB A {150.1,151.1,150.0} vs B {149.8,149.3,
149.6} ms (−0.6% median, noise); no Runtime file touched. The A1
rider (%table eyeball) satisfied by byte-match on contracts carrying
%table:4/8 (tc, symrec) and %table:4/8/12/15/19/23 (demand).

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
          lib/DataFlow/Build.cpp:501 (E-68) iterates const_to_vc
          (unordered_map) into
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
          const_to_vc dormant gap recorded in place — lib/DataFlow/
          Build.cpp:501 per E-68).

      A.2 THE DELTAREL LINEARIZER, post-T2b.0 (LANDED 97d02111,
          §2 record; supersedes §5's pointer-form lines):
            key_of(op) = {lead, stratum, band, op_table_id, sign,
                          ctor}                # DeltaRel.cpp:~3437
            op_table_id = t ? uintptr_t(t->id) + 1u : 0u
                          # t = first non-null of five DROp table
                          # fields else fire_table (six total, E-69);
                          # +1 shift into the
                          # 64-bit key space -> null sentinel 0 is
                          # disjoint BY CONSTRUCTION (survives even
                          # -first-id wraparound minting table id 0
                          # — the Fable review's CONFIRMED catch);
                          # t->id == the .ir's %table:<id>
            pinned_order = Kahn topo sort over non-loop-carried dep
                          edges, ready-set tie-broken by key_less
                          (argmin loop :3731-3748; E-67)
            consumers   = validators (:3830-4000; V-LINEAR :3834 ..
                          V-OLD-EQUIV(order) :3995) + the NEVER-READ
                          body_ops/output_ops substrate loop
                          (:3813-3824); EMISSION READS NEITHER
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

## 8. §7 re-verification record (2026-07-19, tip 35b89aab; the
## eleventh fleet run — 4 seed-unread derivation lanes (3 opus + 1
## sonnet harness) + 4 seed-read adversarial verifiers + xhigh
## consolidator; 9 agents ~679k tokens, 223 tool uses; record
## COMMITTED as KeyedInstances.artifacts/s7-fleet-consolidated.md —
## raw lane/verify reports stayed in the session scratchpad)

Baseline re-confirmed this session before the fleet: both presets
green (incremental), ctest 3/3, SUITE PASS (169), binaries frozen
to session scratchpad baseline-bin/ at 35b89aab.

THE §7 SEED HELD (third consecutive clean core): all four verifiers
independently SEED-HOLDS-WITH-ERRATA; every load-bearing A.1/A.2/
A.4 mechanism and every (B) T2a code-ASSUMPTION CODE-CONFIRMED (the
consolidated record's §3 per-claim table; (B)'s design choices were
out of the fleet's scope — the emitter-pseudocode critique round is
the next step). The E-62 tripwire was re-grepped by four verifiers
+ the consolidator: CLEAN — zero emission-path readers of
pinned_order/body_ops/output_ops. NO STOP finding; T2a UNBLOCKED.

### Errata E-67..E-69 (consolidator-adjudicated; all cosmetic,
### fixed in place in §5/§7 this session)

- E-67 (STALE-ANCHOR): §7 A.2's linearizer anchors drifted AND
  transposed at the boundary (inherited from §5, not re-floated
  after T2b.0 shifted surrounding code). Corrected: Kahn ready-set
  argmin loop DeltaRel.cpp:3731-3748; body_ops/output_ops
  substrate-fill loop :3813-3824 (pushes :3817/:3821); validators
  :3830-4000 (V-LINEAR :3834 .. V-OLD-EQUIV(order) :3995; the fn
  closes :4001). The seed's ":3817-3981 = validators" landed :3817
  INSIDE the substrate loop — a T2b implementer harvesting
  ValidatorFail literals by that span starts in the wrong loop. The
  E-62 tripwire itself (a NAME re-grep, not a line lookup) is
  unaffected.
- E-68 (SEED-DEFECT cosmetic): E-63's bare "Build.cpp:501" is
  ambiguous — lib/DataFlow/Build.cpp AND lib/ControlFlow/Build/
  Build.cpp both have a line 501. The const_to_vc gap is
  lib/DataFlow/Build.cpp:501 (AllConstantsView; decl :83); the
  ControlFlow :501 is an unrelated DataTable line.
- E-69 (SEED-DEFECT cosmetic): "first non-null of six DROp table
  fields else fire_table" double-counts — the chain is FIVE
  ternary-guarded fields (table_op_table, product_table, agg_table,
  negate_table, ingest_table) whose final else IS fire_table, six
  total (DeltaRel.cpp:3394-3402). The +1-shift sentinel logic is
  unaffected and re-confirmed.

REJECTED candidate recorded: verify-linearizer's "lane mis-cite —
the never-read comment is only at DeltaRel.h:592" is FALSE — the
comment appears at BOTH :584-586 (body_ops) AND :592 (output_ops);
the lane's cite was correct. Two further lane nits logged as
non-issues in the consolidated record.

## 9. FUTURE-EPOCH CANDIDATE (owner-directed, recorded 2026-07-19):
## eager-web DR-lowering — NOT this epoch's scope

CANDIDATE: model the eager descent as DR-IR ops and lower it from
the flow graph, retiring the last major hand-coded emission
generator. Scope: BuildEagerInsertionRegions/BuildEagerRegion ff.
(lib/ControlFlow/Build/Build.cpp — the syntax-directed walk that
fills the LowerIngestFold hole with the acyclic monotone push path:
TUPLE forwards, MAP functor calls, CMP filters, JOIN index-probe
loops, NEGATE gates, terminal inserts/vector appends, plus the two
plumbing arms E-95 restored to the enumeration: plain-MERGE unions
and SELECT rebinds — the Build.cpp:1084 dispatch has EIGHT monotone
arms) plus the E-42 VECTORLOOP shim (ExtendEagerProcedure,
Procedure.cpp — today minted from NO DR-IR op).

RATIONALE (the 2026-07-19 owner discussion): the ControlFlow IR is
currently generated by TWO generators over one semantic source —
DR-ops (stratum machinery + every fold) and a direct DataFlow walk
(the eager web + scaffolding) — interleaved at the contract-guarded
hole seam. The walk-generator's plan exists only as its execution
trace: the validators/census/dump cannot see its INTERIOR (only the
boundary — INGEST-CURSOR-SHAPE, V-INGEST-XCHECK Site 5), the F17/
F18 class of defect historically lived in exactly this style of
surface, and the T2b dump will have a hole where the descent is BY
CONSTRUCTION. Migrating it completes the strangler-fig arc every
epoch has advanced and makes the whole emission plan validated +
observable data. The D1 ruling (§0.6.6: SUBGRAPH_INSTANTIATE fully
DR-lowered, "no hand-coded web", LowerGroupUpdate mold) is the
in-scope precedent; this candidate is the same argument applied to
the original web.

GATING (pre-registered for whichever epoch takes it): pure
modeling + lowering-in-place — ZERO emission change, id-stream
identity preserved (the hole contract retires; an E-42 op is
minted); full-corpus byte-identity in all 4 modes vs a frozen
binary; the T3 IR goldens + retained N-run sweep are the acceptance
instrument (a principal motivation for landing T2/T3 first); census
extended to the new op kinds day one; the E-62-style tripwire
discipline applies to any new pinned_order consumer. Expect
per-epoch slicing (folds retired the same way): descent step kinds
migrated one at a time, gates between.

SEQUENCING: after this epoch's charter (T2a→T2b→T3→P1→D1→D2→D3→
D4-design). Candidate competes at the next epoch-open re-rank
alongside pass-harness P2-P5.

## 10. T2a round-2 record (2026-07-19, tip 6c833fe7; emitter
## pseudocode build-out + 4 adversarial critics + 4 fresh contract
## re-verifiers + xhigh consolidator, 10 agents ~932k tokens, 320
## tool uses; consolidated record committed as
## KeyedInstances.artifacts/t2a-round2-consolidated.md, emitter
## pseudocode stays session-scratch — the CODE is its artifact)

VERDICT: GO for T2a. The pseudocode spine is code-correct (raw
det_seq access via public Node::impl + lib-private Query.h include;
blocks ascend det_seq by traversal-sharing; QueryDF tag-struct
coexists with the DOT operator<<; T2b.0 confirmed landed). FIVE
mandatory emitter amendments adjudicated (fold at implementation):

- A1: the emitter's ForEachDFView walk must explicitly skip
  `v.impl->is_dead` — the PUBLIC DefinedNodeIterator does NOT skip
  dead views (DefUse.h:1055-1058) while the impl stamp walk does;
  comment states the true mechanism (RemoveUnusedViews leaves the
  public lists dead-free today; Pass 1's s>=N is the tripwire).
- A2: column tokens render via `os << *c.Variable()` (the
  ParsedVariable operator<< maps unnamed vars to AutoVar_<n>,
  Parse/Format.cpp:14-22) — never var->Name() (drops AutoVar_N),
  never the optional operator (prints _MissingVar).
- A3: `; cycle` SCC/reachability runs over the emitter's OWN `=>`
  edge map, NEVER QueryView::Successors() — Link.cpp:343-346 wires
  INSERT→SELECT materialization edges into successors that the `=>`
  grammar never renders; Successors would over-mark
  materialization-closed recursion.
- A4: the iterative Tarjan body is pinned concretely (resume-frame
  child-cursor form), seeded in ForEachDFView order.
- A5: two anchor fixes (HasNeverHint Query.h:800; MergedViews :736).

Contract re-verification: every PREDICTED residual adjudicated —
tc R1 insert ids + R2 .in<K> stay CONFIRMED-AS-PREDICTED (code-read
at first bless, with the F3 falsifiability caveat: .in<K> must be
cross-checked as a FUNCTION of joined_views order at bless, not
matched-to-output); symrec arm assignment + det_seq tie-break
CONFIRMED; demand 20 blocks / 24 edges / 13 cycle marks CONFIRMED
(valid only under A3's edge set); average_weight within-band order
under the +1 key CONFIRMED (monotone shift), never-minted-t36-vec
note CONFIRMED, deps section stays PREDICTED/floor. REJECTED loudly:
two critics' claim that runall.sh:248/:284 anchors had drifted
(both read a stale tip; :248/:284 are CORRECT at head).

### Errata E-70/E-71 (orchestrator-caught AFTER the fleet — the
### cross-contract grammar audit the fleet lacked)

- E-70 (ARTIFACT defect, LOAD-BEARING, T2a-blocking): the four
  committed byte-exact contracts rendered FOUR DIFFERENT GRAMMARS —
  identity `=>` maps dst=src (symrec/demand) vs bare (tc, the
  ratified p2); producer-side .in<K> as dst=src role maps (demand)
  vs producer tokens (tc), with symrec's tuple.2 edge in producer-
  column order vs tc/demand's join-port order; ATTRIBUTES keyword
  omitted (demand) vs present; hand-written prose annotation
  comments (symrec ~12, demand `; terminal`) vs none; comment
  columns 38..52; join bodies 4-space/aligned/`  }` vs
  2-space/plain/`}`; INSERT tokens untyped + attributes present
  (tc) vs typed + attributes MISSING (demand insert.19). Root
  cause: the fa2bc7c5 revision applied each critique's fixes
  per-artifact; the v3.1 pins were minted from the tc critique and
  never back-applied. One emitter cannot match four grammars —
  gate 8 was UNSATISFIABLE as committed. RESOLUTION: spec v3.2
  session pins (p3-order, p5-p9) + all four §1 blocks re-rendered
  under them in this commit (banners on each artifact; graph facts
  untouched; C-TC-1 and C-AW-1/2 folded into the same re-render;
  demand insert.19 gains its ATTRIBUTES line from the artifact's
  own §2.1 table).
- E-71 (FLEET-METHOD): the round-2 contract critics verified graph
  facts IN ISOLATION and issued CONTRACT-HOLDS verdicts blind to
  grammar nonconformance (demand's "all 24 => lines byte-exact"
  while every identity map violated ratified p2). Standing method
  note: a byte-contract verification fleet needs an explicit
  GRAMMAR-CONFORMANCE lane that diffs the artifacts against the
  PINS and against EACH OTHER, not only against the graph.

### Owner items for the T2a pre-commit brief

- PIN-1 (constant-column token): DOT renders a constant column's
  literal value (do_const, Format.cpp:60-71); the pseudocode
  renders c<id>:<type>. No contract witnesses a constant column.
  Owner rules (value vs c<id>) + a constant-column contract case
  before any such bless. Does NOT block T2a wiring (tc/symrec/
  demand carry none).
- PIN-2 (T3 h-surface plumbing): `h` has no named-path stream
  (-cpp-out writes <dbname>.h into a dir; #database renames it) —
  run_irgold special-cases h via a post-copy from a cpp.<mode>/
  dir. T3 item, logged so T3 does not stall.

NEXT: implement T2a (emitter per the amended pseudocode + v3.2
pins), gates: byte-diff vs the three re-rendered contracts, full
suite 169, corpus flag-off A/B vs the frozen 35b89aab baseline,
ctest, Q5 ABABAB, %table eyeball in all three carriers; then the
Fable review -> owner brief (PIN-1/PIN-2 attached) -> commit.
[T2a LANDED e6264b54 — §2 record.]

## 11. T2b round-3 record (2026-07-19, tip e6264b54; the pre-code
## grammar adjudication the E-71 rule mandates — 1 opus adjudicator
## + 1 adversarial verifier, 2 agents ~259k tokens; pinned grammar =
## session scratchpad t2b-grammar.md, 806 lines; verifier verdict
## SOUND with two LOW corrections, both folded)

E-62 tripwire re-grepped at T2b implementation time by the
orchestrator: CLEAN (zero readers of pinned_order/body_ops/
output_ops outside lib/DeltaRel; the one Stratum.cpp hit is a
comment). The dump becomes pinned_order's FIRST legitimate reader.

FINDINGS (all code-verified):
- PINNED COMMENT SET = ∅ (pin p10): the deltarel surface has NO
  derivable comment class; every §1 `;;`/`;` line was decorative or
  prose. Spec v3.3 pins p10 (zero comments) + p11 (section layout)
  + p12 (no-source fields never render).
- LOUD no-source flags, all confirmed by the verifier at code: J-2
  (%index ids in the join section-walk are ControlFlow DataIndex
  ids — NOT in the DR-IR at the emit point; subline UNRENDERABLE),
  SP-1/AR-1 (functor-name glosses — PlanNode carries no functor
  field), IG-1 (receive=<recv ...> — no primitive), B-1 (DRBranch
  has no join index — emitter matches path.back()==join_view), S-1
  (`·` has no glyph map — implementer hardcode, multibyte), ST-1/
  BD-1 (stratum/band are recomputed lambdas DeltaRel.cpp:3276/3354,
  not DROp fields — hoist), EF-1 (no statecell field on DREffect —
  confirm backing at bless).
- F-9 CONFIRMED AT CODE (the second determinism hole): dep_edges
  is appended from TWO unordered_map traversals (by_vec :3634,
  by_flag :3662) — hash-order, unstable, NOT fixed by T2b.0. The
  emitter MUST canonically sort by (from,to,kind); spec §2.3's
  "VECTOR ORDER" corrected in v3.3.
- CONFIG-INVARIANCE PRE-AUDIT: CLEAN — zero NDEBUG-gated members
  render (the .df producer= trap does not recur).
- Verifier corrections: G-1 — the lane's "mutable renders as mut"
  rationale was BACKWARDS (the live .ir renders
  `mutable(new_weight_i32)` verbatim; the contract byte was
  CORRECT — struck before it caused a wrong fix); G-2 — the
  multibyte byte-verify note extends to the `—` spine glyph.

RE-RENDER R-1..R-10 APPLIED to the contract §1 this session
(banner on the artifact): comments stripped (bulk), section-walk
subline dropped, op.51 publish_target=false added, op.52
args/spine corrected, op.8/9 spine gloss stripped (true shape
pinned at first emission), branch= composites dropped, census
flattened to enum order, p11 layout applied. Ids stay ILLUSTRATIVE
(first-emission pinning, the symrec precedent); deps stays the
F-9/F-10 canonical-sort floor.

## 12. OWNER DIRECTIVES (2026-07-19, the subgraph-functor
## discussion) — D1 design constraints, recorded as H11 in
## witness-deltarel-target.md

Context: functors have NO Database access and never will — every
functor body is a pure driver-supplied free function receiving
bound values (the ADL/functor-surface contract; purity is
load-bearing for call placement, the differential machinery, and
the WASM ABI direction). For keyed instances, an instance owns its
key α; nested tables ELIDE α from row storage; so consumers inside
the instance (functor bound args, join keys, negate gates, insert
projections) must load α from the instance key, not the row.

DIRECTIVE 1 — "the binding source is a MODELED ATTRIBUTE, not
codegen cleverness": D1's op family carries, per bound argument
slot, a binding source `row-slot | instance-key-slot | config-slot`
as a DR-IR attribute (access-plan-spine extension). The wiring is
model-carried (censusable, dumpable in -deltarel-out,
validator-checkable), consistent with the §0.6.6 "no hand-coded
web" ruling. The functor ABI stays closed: instance-key args
arrive as leading plain values, the config-column mold
(config_agg_1/2 precedent).

DIRECTIVE 2 — "the elision decision and the wiring decision are
the SAME decision": one rule for ALL consumers — α-columns resolve
to the instance key, never to row storage. A validator aborts on
any row-slot resolution of an α-column (a row-slot resolution
silently duplicates α per row and forfeits the nested lowering's
storage win). Functors are just the consumer where the wrong
answer is least visible.

## 13. FUTURE CANDIDATES + a STANDING FENCE PRINCIPLE (owner-
## directed 2026-07-19, the lattice-recursion discussion)

STANDING PRINCIPLE — "condition fences on deletion-capability":
feature fences must be scoped to the DIFFERENTIAL case when the
hazard is differential, never class-wide. The two existing fences
are drawn INCONSISTENTLY: the on-cycle product fence is CORRECT
(keys on CanReceiveDeletions() AND ViewSelfReachable — monotone
on-cycle products compile); C-4 is OVER-BROAD (aggregates over
induction-owned inputs rejected regardless of deletability). The
differential machinery is the expensive semantics; the monotone
fragment must not inherit its restrictions.

CANDIDATE A (near-term liftable gap) — THE C-4 MONOTONE SLICE:
count-over-completed-tc (a stratified, terminating, insert-only
aggregate over a recursion's OUTPUT) is semantically trivial yet
rejected today. The C-2 pre-pass comment states the true reason:
no frontier provisioning exists off an induction's output — a
PLUMBING absence stated as a feature gap. The monotone lift is
small: drain the induction's output into the band-(a) net-addition
frontier AFTER the SCC closes (no counters, no claim gates, the
existing GROUP_UPDATE machinery unchanged). The differential case
(aggregate over a DELETABLE recursion) stays fenced. Gate shape:
new corpus case (count-over-tc) + oracle golden; C-4's reject
narrows to CanReceiveDeletions inputs only.

CANDIDATE B (future-epoch, beside §9) — REFINEMENT-MONOTONE
LATTICE RECURSION (the Ascent-lattice gap, the one comparative
expressiveness loss with real workloads: shortest path, dataflow/
abstract-interpretation lattices): insert-only lattice recursion
with IDEMPOTENT ACC merges is sound Kleene iteration in a product
order — every deep hazard (retraction under absorption, inverses,
rescan-per-round) is differential-only. NOT free even so: value
refinement = row REPLACEMENT at the relational level, which the
current monotone fragment (insert-only, sealed watermarks) cannot
express — it is a THIRD propagation class between monotone and
differential: overwrite-and-refire, no counters/claims/rederive
(an upward refinement never needs un-refining), the StateCell
sealed/working pair already the right cell. Requires: (1) a NEW
algebra pragma for idempotent merges (@invertible/@recompute
classify invertibility, NOT idempotence — sum must stay excluded
from recursion even monotone: re-derivation double-counts); (2)
the ACC/termination obligation carried by the pragma; (3) the
Flix-style downstream type discipline (a lattice column infects
its consumers — else (X,5) and (X,3) coexist as rows); (4) the
unstratified-aggregation reject relaxes ONLY for the new pragma
class, never independently. Competes at the next epoch-open
re-rank with §9 and pass-harness P2-P5.

## 14. LONG-RANGE TARGET (owner direction 2026-07-19): a
## CodeQL-EQUIVALENT — the capability roadmap, and the newtype/
## representation-seam MERGE ruling

ASSESSMENT (recorded from the 2026-07-19 discussion): no
impossibility-class gaps stand between Dr. Lojekyll and a
CodeQL-equivalent semantics. TWO designed-feature gaps are on the
critical path; the rest is engineering.

GAP 1 — DETERMINISTIC ENTITY CONSTRUCTION (the newtype slot).
CodeQL's libraries lean on newtype everywhere (dataflow nodes,
summaries, path nodes). RULING: this gap MERGES INTO the owner's
data-structure-representation goal, with the seam's contract
widened one notch — REPRESENTATIONS OWN INSERT SEMANTICS, not
just layout. One contract family, three instances:
  merge-on-collision = mutable(merge_fn)  (LANDED — KV/StateCell)
  mint-on-miss       = interned relation  (-> the newtype surface)
  union-on-insert    = eqrel              (equivalence classes)
Determinism holds: mint-on-miss under the (F)-deterministic
schedule yields deterministic ids (content-addressing is the
alternative). LIFECYCLE RULE PINNED NOW: the intern pool is
MONOTONE FOREVER — ids never reclaimed or reused; facts ABOUT an
entity are differential, its identity is not. Refcounted entity
death is REFUSED (dangling-reference hell inside the counter
machinery for zero analysis value). Residual language-side
halves: the sum-type surface (tag discipline over minted ids,
foreign-types-adjacent) and pattern dispatch — front-end work.

GAP 2 — DEMAND BREADTH. A CodeQL-scale stdlib lives on demand
specialization: multi-adornment per name (D3, chartered) plus
demand THROUGH negation/aggregates (beyond the current
clean-reject slice). Width, not depth — SLDMagic-as-transformation
is the right mechanism.

ENGINEERING (not semantics): storage scale (in-memory runtime vs
on-disk code DBs — the spill question the data-structures epoch
never faced); compile-time scale (Q5 progsize@128 ~150ms; the
relevant number is progsize@10k, unmeasured); ordered aggregates
(rank/ordered-concat — a sorted-fold arm the algebra taxonomy
lacks); TC fast paths (asymptotics only); the front-end
elaboration layer (classes/modules/signatures — a different
language by design).

NOT REQUIRED: lattice recursion (§13 CANDIDATE B) — CodeQL global
dataflow is deliberately SET-BASED (bounded access paths,
summaries as facts); the set fragment + GAP 1 + GAP 2 is
semantically sufficient for the dataflow/taint libraries.

GRAPHS (code/SSA are cyclic) — THE RELATIONAL DISSOLUTION: cyclic
graphs are only hard as VALUES (terms are finite trees; cyclic
terms do not exist — why even Soufflé's ADTs keep graphs
relational). CodeQL never represents a graph as a value: nodes
are opaque entities, edges are binary relations; the cyclic
reference structure lives in edge ROWS while entity construction
stays well-founded (a phi node mints from (var, block) — exactly
the interned-relation FD; the loop back-edge is an edge row, not
part of anyone's identity). Dominance is expressible set-based
(reaches-without-passing-through under stratified negation). So
cyclic code graphs need exactly two things: node identity (GAP 1)
and edges-as-rows (day one). No terms, no lattices, no cyclic
values.

STRATEGIC ADVANTAGES over the target (why the goal fits this
substrate): native incrementality (per-snapshot DBs are CodeQL's
permanent wish-list item; message-driven differential maintenance
IS "file changed -> findings delta"); byte-determinism ((F) —
reproducible findings as a compliance feature); keyed instances =
context-sensitivity as instantiation (the kcfa corpus cases are
literally this); demand = per-query specialization. The epoch's
direction is convergent with the code-analysis use case.

## 15. T2b first-emission adjudication (2026-07-19; record written
## pre-commit, at the owner brief — the -deltarel-out landing)

IMPLEMENTATION (one opus implementer agent against the §11-pinned
grammar; ~242k tokens): lib/DeltaRel/Format.cpp NEW (~860 lines —
the emitter + the EIGHTEEN enum spelling tables, the sole spelling
authority; pre-guarded SetDeltaRelDumpStream sink), DeltaRel.h +9,
public ControlFlow/Format.h +7 (the Main.cpp-reachable decl),
Stratum.cpp +6 (drain at the :2167 flow ref before the no-phase
early return), Main.cpp +26 (-deltarel-out arm; sink installed
BEFORE Program::Build — the drain fires inside it). E-62 tripwire
was re-grepped CLEAN pre-implementation (§11); the dump is
pinned_order's first legitimate reader.

FIRST EMISSION vs the §11-re-rendered contract: CONTENT-EXACT
modulo four adjudicated classes (orchestrator-verified at the
tree, not taken on report):
- IDS: only the 10 SEED_FOLDs permuted (join-pivot folds mint
  FIRST); branch bijection; vec + other-op ids IDENTITY. Pinned
  from the live emission (the symrec precedent), bijections
  verified against structural content.
- p13 (v3.4): mechanical whitespace pinned (single-space, no
  padding, one-line effects) — the artifacts' hand alignment was
  unpinned + internally inconsistent.
- p14 (v3.4): deps sort widened to (from,to,kind,scope,carried) +
  exact-duplicate dedup (242 -> 174 rows, 0 dups). The 36-edge
  F-9/F-10 floor is a strict subset of the live 174 — the
  "not-certified-complete" caveat RETIRES (this IS the full
  enrollment).
- 4a/4b/4c rulings (each per pin p12, no guessing): agg= =
  derivable functor name (kKv: merge functor — edge_weight was an
  unrenderable relation-name guess); vec def=[] = faithful render
  (the model registers NO defs for overdelete/addition/net/
  join-pivot vecs — recorded as a model-fidelity improvement
  candidate, surfaced via re-bless when added); spine — uniform on
  all four join-pivot seeds.

CONFIG-INVARIANCE: PROVEN DIRECTLY — the debug and release
binaries produce BYTE-IDENTICAL dumps (the audit the (a3) ruling
mandated before any deltarel bless; the round-3 static pre-audit
predicted it, the A/B confirms it).

FABLE REVIEW (workflow, 26 agents ~1.09M tokens) + fixes, all
gates RE-RUN after: 2 CONFIRMED dump defects fixed — the p11
empty-section double blank (98/169 corpus dumps violated the pin;
separators now section-guarded; post-fix corpus sweep 0/158) and
the kPivotAssemble index-less `($join-pivots)` (op_pivot_vec now
reads the op's own pivot_vec_index — a multi-join SCC dump was
AMBIGUOUS across pivot vecs and would have frozen a lossy golden);
the 18 spelling tables' silent `?`/alias fallback arms replaced
with the loud abort idiom (a forgotten table update must crash the
compile, never print a plausible wrong golden); the census gains a
sum==ops.size() cross-check (a 16th DROpKind cannot silently
vanish); the deps comment states the true 5-field key; the
OpStratum hand-copy HOISTED to one shared authority (DROpStratum
in DeltaRel.{h,cpp}, called by BOTH key_of and the emitter — the
dump can no longer silently lie about the schedule; bands are
key_of kind constants, no helper needed); the kGroupUpdate
input-table scan deduped; CLAUDE.md's DeltaRel boundary line
updated (the SetDeltaRelDumpStream decl on ControlFlow's public
Format.h is the one public seam). Recorded-not-applied: the
ColRender/DataFlow-helper duplication (cross-target surgery
deferred); ACCEPTED RESIDUAL: the p14 dedup hides a hypothetical
future double-enrollment from the DUMP only — the validators read
the raw undeduped dep_edges, so the regression class stays
abort-visible.

GATES (all green, RE-RUN in full post-fix): average_weight bytes
UNCHANGED through every fix (the pinned contract holds); 5-run +
3-run dump determinism 1 hash; p11 corpus sweep 0/158; FULL SUITE
PASS (169), zero stdout churn; 676-row corpus A/B BYTE-IDENTICAL
vs the frozen e6264b54 baseline (the DROpStratum hoist proved
behavior-neutral); data/ 36-file A/B clean; ctest 3/3;
CONFIG-INVARIANCE re-proven post-fix (debug==release
byte-identical dumps); Q5 progsize@128 release SAME-SESSION
INTERLEAVED ABABAB A {152.7,162.8,153.8} vs B {154.3,156.0,156.7}
ms (+1.4% median, inside the noise band, A-side 162.8 outlier in
the same run); no Runtime file touched. Contract §1 re-rendered to
the adjudicated emission with the full pin banner; spec at v3.4
(p13/p14).

## 16. T3 landing record (2026-07-19; the IR-golden sidecar
## machinery + the permanent (F) gate — and erratum E-72, the
## config-variance latent the T3 audit caught before any golden
## baked it in)

MACHINERY (runall.sh, spec §3 exactly): run_irgold in the --one
worker beside run_oracle — sidecar-guarded, ONE compile per pinned
mode emitting all four surfaces (df/deltarel/ir via named streams;
h via the PIN-2 post-copy out of cpp.<mode>/), strict cmp against
goldens/<name>.<surface>.<mode>.golden, IRGOLD-FAIL/-MISSING/
-DIVERGE all grep-visible at the :284 summary; the --bless mirror
iterates the SIDECAR and hard-errors on a missing produced file
(the F5 no-skip-idiom discipline), guarded [ -d ] per E-65.
Sidecars: demand_tc_witness (h+ir+df+deltarel, opt, demand-ON via
its .drflags — THE restored permanent (F) gate) and symrec_tie_1
(ir+df, opt). Sidecars + goldens in the SAME commit (atomicity).

- E-72 (REAL-DEFECT, caught by the reviewed-truth ritual's
  config-invariance check BEFORE the first bless): the .ir dump
  AND the generated datalog.h were config-VARIANT on every
  recursive program — Induction.cpp:661 minted the INDUCTION
  region's "set N depth M" comment under #ifndef NDEBUG, and the
  comment renders into both surfaces. The spec's standing "the
  .deltarel/.ir/.h are all config-invariant" assertion (the a3
  producer rationale) was FALSE for ir/h; a debug-blessed golden
  would have failed under the release preset. The four other
  ControlFlow NDEBUG sites are proper assert blocks (no output).
  FIX: the mint is now UNCONDITIONAL (values are deterministic —
  InductionGroupId + merge depth); debug output is byte-UNCHANGED
  (guard removal is a debug no-op, proven corpus-wide by the
  post-fix 676-row A/B), release now aligns with debug.

REVIEWED-TRUTH ritual before bless: both df goldens BYTE-EXACT vs
their committed contracts (re-verified post-bless from the golden
files); 3-run all-surface determinism 1 hash per carrier;
config-invariance debug==release on ALL FOUR surfaces of BOTH
carriers (post-E-72); the h golden stays policy-limited to the
single demand witness.

GATES (all green): filtered red run = exactly the 6 expected
IRGOLD-MISSING; bless = 10 goldens (the 4 stdout/oracle/monotone
re-blesses byte-identical — zero churn, confirmed by git); FULL
SUITE PASS (169) with the irgold arms live; 676-row corpus A/B
BYTE-IDENTICAL vs frozen e6264b54 (E-72's debug no-op proven);
ctest 3/3; Q5 progsize@128 release ABABAB A {153.2,154.4,152.7}
vs B {154.5,152.2,151.1} ms (−0.7% median, noise). goldens/ grows
by 6; suite count stays 169.

SWEEP RETIREMENT (per the ratified (c) ruling): with the
demand_tc_witness + symrec_tie_1 sidecars blessed and green, the
scripted 8-run -ir-out sweep RETIRES as the routine per-commit
step; it is RETAINED as the acceptance gate for any
determinism-substrate-touching change.

## 17. P1 landing record (2026-07-19; record written pre-commit, at
## the owner brief — the pass-harness slice, judged before code)

JUDGE ROUND (3 judges + xhigh consolidator, ~261k tokens; pinned
contract = scratchpad p1/p1-pinned.md; verdict GO-WITH-AMENDMENTS):
three DRAFT errors corrected as hard pins — (1) the df alias is the
ENUMERATED {df.cse,df.canon,df.dfe,df.sink}, NEVER df.* (df.simplify
+ df.demand run OUTSIDE the optimize guard in all 4 golden modes);
(2) the wholesale-skip branches are PRESERVED via
AnyBodyOptionalEnabled (enter-and-gate-all is NOT byte-equal — the
region-list .Sort(depth_cmp) calls inside the cf sweep are
emission-visible; and the cf alias must reach BOTH Optimize call
sites); (3) "~10 driver sites" corrected to 3 outer guards + interior
gate names, library-internal, with the interior gates DEFAULT-ALLOW
(inert at default config) and -opt-counter/-passes deferred to P3/P4.

IMPLEMENTATION: include/drlojekyll/Util/PassPolicy.h + lib/Util/
PassPolicy.cpp (glob matcher: prefix-star + exact only; the single
cross-level monotone bisect counter; allow-path PURE — no NDEBUG
logic, the E-72 lesson); PassPolicy REPLACES the optimize bool in
both public Build signatures (callers: Main.cpp ×2 + the Oracle,
which passes DisableDataFlowOpt()); the 3 outer guards; interior
gates df.simplify, df.cse (per do_cse), df.canon (PER ROUND inside
the Canonicalize fixpoint), df.dfe (EliminateDeadFlows ONLY —
RemoveUnusedViews is REQUIRED hygiene, never gated), df.sink,
cf.regionopt (per sweep), cf.procdedup; Main.cpp arms -opt-disable=/
-opt-only=/-opt-bisect-limit= with the legacy flags as factory-exact
aliases; per-module bisect reset.

FABLE REVIEW (20 agents ~841k tokens): four CONFIRMED
silent-acceptance gaps, all fixed + probed loud — (1)
-opt-bisect-limit= with an empty value silently installed limit 0
(strtoll no-conversion now rejected); (2) THE BIG ONE: the gated
df.demand silently NEUTERED an explicit -demand under -opt-only/
-opt-disable/bisect, dropping its clean diagnostics (the
demand_multi_adorn_1 reject class would have vanished) — RULING:
-demand is SEMANTICS, not an optimization; df.demand is UN-GATED in
P1 (name RESERVED, absent from the registry; a future stage defines
LOUD composition semantics); (3) shape-valid but unmatchable globs
(df., bare df, typos) were silent no-ops — and under -opt-only a
typo silently inverted to disable-everything — fixed with the
MatchesAnyKnownPass parse-time registry check; (4) an empty
-opt-only= inverted to run-everything — empty glob lists now
diagnose. Cleanups: PassPolicy.cpp moved to its lib/Util home;
public Program.h made self-contained. Probes: the multi-adornment
reject diagnostic now SURVIVES -opt-only (the fix's witness);
-opt-disable=df.ces and -opt-bisect-limit= and -opt-only= all emit
colored diagnostics.

GATES (all green, re-run in full post-fix): [G1] SUITE PASS (169)
with the irgold arms live; [G2] 676-row corpus A/B BYTE-IDENTICAL
vs the frozen e6264b54 baseline (default config = the exact current
pipeline); [G6a] 676-row legacy-vs-new-vocabulary A/B
BYTE-IDENTICAL; [G6b] the FULL suite (stdout + all 6 irgold
surfaces) PASSES under the new-vocabulary flags_of (temporarily
patched, restored — flags_of("opt") stays ""); [G7] demand
orthogonality (legacy==new under -demand on all 4 surfaces; the df
alias preserves the demand pass); [G3] ctest 3/3; [G5] data/ 36-row
A/B clean; [G4] Q5 progsize@128 release ABABAB A {151.5,151.1,
150.1} vs B {147.1,147.0,150.2} ms (−2.0% median, inside the noise
band); bisect -1 prints its index stream to stderr only. The
all-4-modes byte-identity prediction (spec §4/A8) HELD.

STATUS: T1 + (F) + T2b.0 + T2a + T2b + T3 + P1 landed — the T2/T3/
P1 program is COMPLETE. Next: the D1 design fleet (§7(B); targets
witness-deltarel-target.md + tc-four-adornment-target.md, holes
H1-H11; the §12 binding-source directives; the minus-before-plus
band-(a) DR-IR edge pin; instance death as its OWN op). P2-P5 of
the pass harness remain next-epoch candidates per §0.6.1.

## 18. Whole-program checkpoint (2026-07-19, end of the T2/T3/P1
## session; tip 0af322a2 + this ledger commit): the ARCHITECTURE AS
## PSEUDOCODE with T2a/T2b/T3/P1 AS-LANDED, and the PATH FORWARD AS
## DIFFS. SINGLE-PASS record by this session (the §5/§7 blocks it
## incorporates were fleet-verified THREE times — §4, §6, §8; the
## NEW blocks are this session's writing, backed by the §10/§11/§15/
## §16/§17 per-diff records). A new session starts HERE; re-verify
## per the house precedent (continue errata at E-73), leaning on the
## committed fleet records so only the DELTA needs fresh lanes.

    (A) AS-LANDED SURFACES:

      A.1 COMPILE PIPELINE (post-P1; §5(A)'s "main(argv)"/"Query::
          Build tail"/"DETERMINISM SUBSTRATE" blocks stand with
          these deltas):
            main(argv):                    # bin/drlojekyll/Main.cpp
              parse flags -> streams (gDOTStream/gDFStream/
                gDeltaRelStream/gDRStream/gIRStream), gDemand,
                gFirstId, gPassPolicy (-opt-disable=/-opt-only=/
                -opt-bisect-limit= + the legacy flags as
                factory-EXACT aliases; empty values, malformed
                globs, and REGISTRY-unmatchable globs are colored
                diagnostics — never silent)
              CompileModule:
                gPassPolicy.bisect_counter = 0        # per-module
                query = Query::Build(module, log, gPassPolicy,
                                     gDemand)
                SetDeltaRelDumpStream(gDeltaRelStream) # BEFORE
                                                       # Program::Build
                program = Program::Build(query, log, gFirstId,
                                         gPassPolicy)
                gIRStream? << program; emit C++; gDOTStream? <<
                query; gDFStream? << QueryDF{query}   # both
                                                      # post-Build
            Query::Build tail (lib/DataFlow/Build.cpp):
              ... -> if policy.Gate("df.simplify") Simplify
              -> ConnectInsertsToSelects
              -> ApplyDemandTransform(module, log, demand_mode)
                 # df.demand UN-GATED: -demand is SEMANTICS (§17)
              -> if policy.AnyBodyOptionalEnabled(kDataFlow)
                   Optimize(log, policy)   # wholesale-skip
                                           # PRESERVED (§17 pin 1)
              -> LinkViews -> IdentifyInductions (det_seq last
                 stamp) -> FinalizeDepths/FinalizeColumnIDs/... ->
                 Stratify   # view-neutral tail, thrice-verified
            QueryImpl::Optimize(log, policy): do_cse gated df.cse
              (3 calls); Canonicalize gated df.canon PER ROUND;
              EliminateDeadFlows gated df.dfe (RemoveUnusedViews
              NEVER gated); do_sink gated df.sink (dormant body).
            ProgramImpl::Optimize(policy): sweep loop gated
              cf.regionopt PER SWEEP (the emission-visible
              .Sort(depth_cmp) region reorders live INSIDE it —
              why enter-and-gate-all is not byte-equal);
              cf.procdedup gates the dedup tail. BOTH Program::Build
              call sites guarded by AnyBodyOptionalEnabled(kCF).
            PassPolicy (include/drlojekyll/Util/PassPolicy.h,
              lib/Util/PassPolicy.cpp): prefix-star+exact glob
              matcher; Enabled = only-select then disable-subtract;
              Gate = Enabled -> tick -> bisect-limit check (allow
              path PURE, no NDEBUG logic — the E-72 law); the
              7-name registry {df.simplify, df.cse, df.canon,
              df.dfe, df.sink, cf.regionopt, cf.procdedup} is the
              parse-time reachability authority.

      A.2 THE DUMP SURFACES (the T2 instruments — D1's designers
          hand-author desired states against REAL output now):
            -df-out (lib/DataFlow/Format.cpp, QueryDF tag):
              PASS-1 seen-bitset bijection witness (always-on,
              raw impl->det_seq); kind-tagged det_seq-order
              traversal (impl kind order, is_dead-skipped; NEVER
              the joins-first public ForEachView); `=>` edge model
              (p2 bare identity, p3 producer-token .in<K> in
              join-port order); iterative Tarjan over the
              emitter's OWN =>-edge set for `; cycle`; grammar
              pins p1-p9 (spec v3.4 §1.3).
            -deltarel-out (lib/DeltaRel/Format.cpp): 18 enum
              spelling tables (sole authority); vecs mint order /
              branches+joins / ops labeled MINT INDEX in
              pinned_order / bare rounds: / deps sorted+deduped on
              (from,to,kind,scope,carried) / census in enum order
              with the sum==ops.size() abort; DROpStratum is the
              ONE stratum authority (key_of + emitter share it);
              pre-guarded SetDeltaRelDumpStream sink declared on
              ControlFlow/Format.h; pins p10-p14. CONFIG-
              INVARIANCE PROVEN debug==release (E-72 fixed: the
              Induction.cpp comment mint is unconditional).

      A.3 THE GOLDEN MACHINERY (T3): run_irgold in runall.sh
          --one (one compile per pinned mode emits all four
          surfaces; h via the cpp.<mode>/ post-copy; strict cmp;
          IRGOLD-FAIL/-MISSING/-DIVERGE); sidecar-driven
          hard-error bless. BLESSED: demand_tc_witness
          h+ir+df+deltarel @opt demand-ON (THE permanent (F)
          gate) + symrec_tie_1 ir+df @opt. The 8-run sweep is
          RETIRED to substrate-change acceptance duty.

      A.4 STANDING GATES (every future diff): SUITE PASS (169,
          irgold live); 676-row corpus A/B vs a frozen baseline;
          data/ A/B; ctest 3/3; Q5 SAME-SESSION INTERLEAVED
          ABABAB; config-invariance (debug==release dumps) on any
          dump-touching diff; the E-62 tripwire re-grep at any
          DeltaRel diff; deltarel config-invariance audit before
          any deltarel bless; Fable review -> owner brief before
          every emission commit.

    (B) THE PATH FORWARD AS DIFFS:

      D1   DESIGN+JUDGE FLEET (no code; the ratified §0.6
           decisions 2/3/5/6 + §12 + §17-era rulings bind it).
           INPUTS: witness-deltarel-target.md (H1-H11; H11 = the
           §12 binding-source directive) + tc-four-adornment-
           target.md + demand_neighborhood_witness.dr (PICK-A,
           compiles both ways) + epoch-diffs.md §D1 + §3(F') with
           E-46/E-52/E-53 folded. DELIVERABLES: (1) the
           SUBGRAPH_INSTANTIATE DR-IR op family in the
           BuildGroupUpdateOps mold (fully DR-lowered, no
           hand-coded web) incl. INSTANCE-DEATH AS ITS OWN OP
           (demand-retract = whole-instance death, never per-row
           counters); (2) the release-surviving per-guard-site
           annotation (three GuardSite kinds, two demand sides —
           E-46/E-52); (3) the binding-source attribute
           (row-slot | instance-key-slot | config-slot) on the
           access-plan spine, α resolves to the instance key for
           ALL consumers, validator-enforced (§12); (4) the
           minus-before-plus band-(a) ordering PINNED by an
           explicit DR-IR edge (H9/H10 — precondition of the
           equivalence gate); (5) the R-A frozen-pair store
           surface vs Runtime reality; (6) the dual-lowering
           equivalence gate design (§0.6.3: one .dr + one
           .batches under both knobs vs the SAME oracle golden);
           (7) the D2 fence list (acyclic-DEMAND fence,
           mid-stream monotone-edge-add fenced, differential
           inputs fenced). METHOD: desired-output-state artifacts
           for the PICK-A witness under BOTH lowerings, authored
           against REAL -df-out/-deltarel-out dumps of the flat
           lowering (the T2 instruments as ground truth — new
           since the R-A paper); byte-contract fleets carry an
           explicit GRAMMAR-CONFORMANCE lane (the E-71 rule).
           Judge rounds before any ratification; owner brief with
           the open decisions at the end.
      D2   EMISSION: the nested lowering behind a knob (flat stays
           default, §0.6.2); demand_neighborhood_witness enters
           the suite WITH batches + oracle goldens at D2 (never
           unblessed); census from the query-side annotation
           count; the equivalence gate + standing gates.
      D3   MULTI-ADORNMENT LIFT design (>1 binding pattern per
           name; REJECT-18 scope; demand_multi_adorn_1
           disposition — §0.6.6).
      D4   SEAMS DESIGN-ONLY (implicit asynchrony; emission gated
           on the termination judge + a measured-profitable
           witness; E-50: VecRole + kVecAppend/kVecDrain, B-10
           precedent).
      NEXT-EPOCH CANDIDATES (compete at epoch-open re-rank):
           pass-harness P2-P5 (P2 print-after now has all three
           dumps to wire), §9 eager-web DR-lowering, §13 lattice
           candidates A/B, §14 CodeQL-target items.

    (C) STATUS at session end (2026-07-19): T1 + (F) + T2b.0 +
    T2a + T2b + T3 + P1 LANDED (tip 0af322a2, pushed); ledger
    §8-§17 this session; errata through E-72; spec v3.4 (pins
    p1-p14); all four dump contracts pinned to live emissions;
    owner pins OPEN: PIN-1 (constant-column token — blocks only
    constant-carrying bless), PIN-3 (negate class refinement —
    blocks only negate-carrying bless). NO D1/D2 code exists.
    Next session: re-verify §18 (E-73+), then the D1 design
    fleet.

## §19. D1 DESIGN PINNED — whole-program checkpoint (2026-07-20, tip 1aaca896)

THE ENTRY POINT for the next session. Status: the D1 design is COMPLETE,
JUDGED (unanimous GO-WITH-AMENDMENTS), and PINNED — implementation is
BLOCKED ON OWNER RATIFICATION of the decision list in d1-pinned.md §4.
NO D1/D2 code exists; the tree at this checkpoint is docs-only ahead of
1aaca896. SINGLE-PASS (the house precedent): the next session re-verifies
this section before building on it; errata continue at E-81.

(A) §18 RE-VERIFIED (the thirteenth run of the precedent): SEED-HOLDS,
    GO. Four seed-unread derivation lanes + four seed-read adversarial
    verifiers + an xhigh consolidator; orchestrator personally re-grepped
    the E-62 tripwire (sole hit = Stratum.cpp:1073, a comment) and the
    bisect-counter readers (PassPolicy.cpp:85 tick, Main.cpp:65 reset,
    :452-461 parse). Zero load-bearing defects; four COSMETIC errata:
    E-73 §18:1763-4 "BOTH Program::Build call sites guarded" — the two
         guarded calls are the ProgramImpl::Optimize invocations INSIDE
         lib/ControlFlow/Build/Build.cpp (:1372-3, :1381-2), not two
         public Program::Build calls (there is one, Main.cpp:79-80).
         Mechanism pinned is correct; phrase loose.
    E-74 §16:1606's E-72 anchor is stale: the mint is at
         lib/ControlFlow/Build/Induction.cpp:671 (post-fix banner starts
         :661), and bare "Induction.cpp" is ambiguous across three files
         (lib/ControlFlow/Build/ is authoritative — the ambiguity induced
         a lane's wrong-file grep).
    E-75 §18 A.2 folds "E-72 fixed" under the -deltarel-out bullet; E-72
         was an .ir/datalog.h defect — deltarel bytes were always
         config-invariant (§16 scopes it correctly).
    E-76 §15:1509 "~860 lines" for lib/DeltaRel/Format.cpp; true figure
         at tip = 821.
    Coverage gaps carried for the NEXT re-verify: the view-neutral tail
    order was leaned on (thrice-verified, not re-derived); process-history
    gates not statically checkable; -dr-out surface existence-checked only.

(B) GROUND TRUTH — the real flat dumps (artifact d1-ground-truth-nbhd.md,
    GT-1..GT-6, dump bytes embedded). The two load-bearing divergences
    from the paper-era target: GT-1 the fabricated demand table lowers
    MONOTONE (no frontier/claim/commit machinery; instance death's
    trigger is UNREACHABLE as landed — the retraction surface became
    owner decision OD-1); GT-2 the ENTIRE witness is monotone (two
    kIngestFolds + empty rounds/deps — there is no differential tail to
    ride). Also: GT-3 the raw_seed/d_reader CSE fold is LIVE (demand_side
    must be stamped pre-CSE); GT-4 the real %table map (8=demand,
    11=edge, 15=guarded copy, 4=answer); GT-5 the flat demand flow is
    carried ENTIRELY by the hand-coded eager web — SUBGRAPH_INSTANTIATE
    replaces EAGER-WEB emission, not DR ops. witness-deltarel-target.md
    carries its SUPERSEDED-IN-PART banner accordingly (E-80).

(C) THE D1 DESIGN (artifacts, precedence stack top-down):
    d1-pinned.md (the judge contract: HP-1..HP-18 hard pins, the owner
    list, overruled findings) WINS over d1-design-consolidated.md (the
    adjudicated architecture §A + diff sequence §B + 27 critique findings
    dispositioned at code) WINS over d1-desired-states.md (the four
    adjudicated dump blocks + C-1..C-12 residuals). The architecture as
    pseudocode DIFFS against the §18(A) as-landed pipeline:

      Query::Build tail (D1.a — DataFlow-side, inert):
        ApplyDemandTransform(...)        // landed, un-gated (df.demand=semantics)
       +  at the two stamp sites (Demand.cpp step-7 :960 body/d_reader,
       +  step-8 :1002 query-projection/raw_seed):
       +    guard_annotations.push({kind, demand_side, role,
       +        is_instance_key /*D3 recursive-subgoal marker ONLY*/,
       +        instance_key, guarded_read, demanded_view, forcing_index})
       +    guard_annotation_of[view]=idx  // lookup-ONLY map; ordered
       +        // consumption ALWAYS from the DefList/det_seq walk (HP-9)
       +  per forcing in query.DemandForcings():   // recognition unit =
       +    recognized_subgraphs.push({...})       //   the FORCING (X-9)
        CopyDifferentialAndGroupIdsTo(that)  // View.cpp:557 — the choke pt
       +  TransferGuardAnnotation(loser, survivor)  // migrates exactly
       +      // where group_ids migrate (covers RAUW, both Join
       +      // self-canon sites, CSE, the ~12 hand call sites);
       +      // incompatible fold = LOUD abort

      BuildDRInventory (D1.b — enums/validators/grammar land, mint OFF):
       +  3 DROpKinds after kStateSeal: kSubgraphInstantiate (BIRTH/
       +      REBUILD + band-(b) publish; SOLE pub deriver), kInstanceDeath
       +      (OWN op, §18(B); ZERO-COUNTER signature: drain kNetRemoval +
       +      kInstanceDemand + kInstanceOld + kInstanceRebuild(-1), NO
       +      fold/counter/append; minted ONLY when
       +      TableIsDifferential(demand) — mint-predicate-false at D2,
       +      HP-17), kInstanceSeal (band 11, self-lowered carrier HP-1)
       +  5 EffKinds: kInstanceRebuild/kInstanceEmit/kInstanceOld/
       +      kInstanceDemand(frozen,NO-hazard HP-8)/kInstanceSealSwap —
       +      kInstanceEmit/Old justify-or-collapse vs kStateEmit/Old
       +      BEFORE spelling rows (HP-11)
       +  effect multisets REGIME-SPLIT on DIFF:=TableIsDifferential(pub):
       +      always {drain(demand,kNetAddition), kInstanceDemand,
       +      kFlagRead(input,Present,kSeed), kInstanceRebuild(+),
       +      kInstanceEmit, kInstanceOld}; DIFF adds 2×kCounter(±)+
       +      2×kInIReadFrozen+2×kVecAppend; !DIFF adds 1×kCounter(+)
       +  minus-before-plus: BOTH ops op_table_id=pub_table, death
       +      sign=-1/inst sign=+1 → the band key's sign tie-break fires
       +      (HP-3); NO mint-time DRDep (the mandated explicit edge is
       +      provably CIRCULAR under emit_waw key-forcing — OD-2);
       +      V-INST-ORDER (always-on) is the enforcement
       +  validators: V-INST-EFFECT/-SOLE/-PAIR(2-way R-MONO, 3-way
       +      R-DIFF)/-ORDER/-DRAIN(HP-2)/-EMITTED(all THREE kinds, HP-1);
       +      V-ALPHA arms A/B (α-elision = wiring; MAP/NEGATE/AGG in a
       +      demanded body → recognizer REFUSAL until extended, HP-4);
       +      linearizer default: → ValidatorFail
       +  dump grammar: kInstance* spelling rows (loud-abort fallback),
       +      3 census counters, instances:/DRInstance section, ik:/row:
       +      binding-source tags; THE ONE CHURN = demand_tc_witness
       +      census line +3 counters — DIRECT DIFF bless (one line, enum
       +      order, census-abort green) — NOT permcheck (HP-14: permcheck
       +      EXECUTED, FAILS on census-line change; boundary_re)

      Lowering (D2.b — the knob goes live):
       +  eager walk: recognized boundary = chain-breaker (GROUP_UPDATE
       +      precedent); flat guard-join eager web NOT emitted under
       +      -demand-instance (GT-5); OD-7 provisioning gives the
       +      monotone demand table its net-additions frontier — which
       +      FORCES a monotone kCommitSweep on it (X-DS-2; census
       +      kCommitSweep=1 under demand-only provisioning, OD-4)
       +  LowerSubgraphInstance (Stratum.cpp GroupUpdates-loop mold):
       +      drain demand frontier → FindOrAddInstance → V-INST-FRESH
       +      (current empty, fprintf+abort, survives NDEBUG) →
       +      [R-DIFF: death arm FIRST] → Rederive rescan of the monotone
       +      input from the op-BODY PlanTree → TryAdd into current →
       +      band-(b) two-scan publish: born=(F,T)→+pub_row,
       +      dropped=(T,F)→-pub_row (provably EMPTY under R-MONO, HP-7;
       +      partition semantics pinned in the p-rule, HP-6);
       +      pub_row = concat(KeyAt(iid) at ik: slots, r at row: slots)
       +      — V-ALPHA arm B: the α value is NEVER row-projected
       +  V-INST-EMITTED multiset-compares emitted regions vs enrollment

      Runtime (D2.a — inert): InstanceStore<Key,RowT> = the StateCellStore
        transpose (dense iid monotone-forever; frozen/current TABLE PAIR
        per instance; nested tables INDEX-FREE — flat's idx_38 disappears;
        Seal = per-touched pointer swap + Reset + occupancy snapshot);
        RowStore::Reset() (Arena-safe: no allocator entry point — H6).
        DrTest unit ships with the header. HP-16: one-shot
        -DDRLOJEKYLL_BENCH_COUNTERS ON-build compile+suite gate.

      Retraction (STAGED, OD-1): D2 = R-MONO-a (birth + demand-flap
        rebuild only); D3.a = R-DIFF via NEW opt-in -demand-retract
        (@differential fabrication at the Parse mint + suppressed-
        message-preserving unask ABI), carried by witness .drflags.
        Knobs -demand-instance/-demand-retract are OFF the PassPolicy
        registry (the P1 "demand is semantics" ruling).

      Equivalence gate (D2.c): run_eqgate in runall.sh --one — flat
        stdout == nested stdout == blessed golden (ANSWER identity,
        never generated bytes). The oracle is the orthogonal closure
        check on the FLAT arm only; demand-SCOPING correctness rests on
        the blessed .stdout + .ir structural gate (HP-5 — the witness
        graph MUST contain out-of-neighborhood edges); DEATH is
        oracle-blind forever (eqgate-F2). Three per-forcing fences:
        (i) cyclic-demand, (ii) mid-stream monotone-input-add [scope =
        OD-3], (iii) differential-summarized-input; witnesses land with
        D2.c per OD-8 (suite 169→170→173).

    DIFF SEQUENCE (d1-pinned §3, amendments folded): D1.a annotation+
    registry → D1.b op family+validators+grammar → D2.a runtime store →
    D2.b excision+lowering+codegen → D2.c witness+eqgate → D3.a R-DIFF
    (next epoch; HP-15's surface×mode re-bless matrix pre-registered).
    Every diff under the standing gates G1-G7 + Fable review + owner
    brief before emission commits.

(D) OWNER DECISIONS PENDING (full statements in d1-pinned.md §4; blocking
    map): OD-1 retraction staging (REC staged; dissent recorded; blocks
    D3.a only) · OD-2 §18(B)(4) mechanism substitution (REC ratify;
    blocks D1.b) · OD-3 fence-(ii) scope — the §13 judgment call (blocks
    fence-ii code + its witness only) · OD-4 OD-7 provisioning blast
    radius (REC demand-only; blocks D2.b provisioning + nested census
    bless) · OD-5 seal carrier = self-lowered (veto point; blocks D2.b
    wiring) · OD-6 three-op family conditioned on HP-11 (blocks D1.b) ·
    OD-8 fence witnesses with D2.c (REC yes) · OD-10 witness .irgold
    timing (REC defer to post-D3.a; dissent recorded). Veto-only:
    OD-R1..R9. Implementer-at-D1.b: OD-I1..I4. PIN-1/PIN-3 remain open
    (no carriers arose).

(E) ERRATA E-77..E-80 (the judge round's candidates, adopted):
    E-77 the permcheck mis-citation — design decision #9 named a referee
         that categorically REJECTS the change it was pinned to referee
         (census line = structural boundary under permcheck.py:62's
         boundary_re; executed twice, exit=1). Two critique rounds
         rubber-stamped it; caught only when EXECUTED. Corrected in
         HP-14. HOUSE RULE minted: a referee named in a binding pin must
         be RUN against the exact change it is pinned to referee.
    E-78 design §A.3.4's three-exit emit_inst_seal wiring SUPERSEDED by
         HP-1/OD-5 (seal self-lowered from kInstanceSeal's dispatch;
         V-INST-EMITTED enrolls the seal).
    E-79 §18(B)(4)'s "explicit DR-IR edge" minus-before-plus mechanism is
         pending owner SUBSTITUTION (OD-2): the mandated edge is provably
         circular (emit_waw key-forces edge direction, DeltaRel.cpp:
         3540-3556; V-LINEAR checks against the same key that oriented
         it). RULED 2026-07-20: substitution RATIFIED — see (I).
    E-80 witness-deltarel-target.md superseded-in-part (banner applied
         this checkpoint; see (B)).

(F) ASAN — OWNER DIRECTIVE (2026-07-20, STANDING): "you should be
    compiling and running the code with asan." Adopt from the next
    session on, BEFORE D1.a lands: (1) a build/asan tree (Debug +
    -fsanitize=address -fno-omit-frame-pointer, linker
    -fsanitize=address — the CLAUDE.md coverage-build pattern); (2)
    ctest under it; (3) an OptDiff suite run with DR=build/asan/bin/
    drlojekyll — this exercises the COMPILER under ASAN on all 169
    cases; (4) price and decide the second surface: compiling the
    GENERATED code + drivers with ASAN inside diffrun (exercises the
    Runtime/Allocator) — likely via an env-selected CXXFLAGS arm, never
    a fifth golden mode. First sweep's findings go to FINDINGS.md/errata
    LOUDLY. ASAN builds are never timed (no Q5/bench); never rebuild
    mid-suite applies unchanged. Whether ASAN-suite becomes a standing
    per-diff gate or a per-epoch sweep is decided from the first run's
    cost — bring the number to the owner.

(G) STATUS at session end (2026-07-20): tip 1aaca896 + this docs-only
    checkpoint; suite 169 irgold-live; ~3.2M subagent tokens across 4
    workflows (29 agents) this session; fleet reports live only in the
    session scratchpad — the four committed artifacts above are the
    binding record (scratchpad is disposable). NEXT SESSION: (0) ASAN
    sweep per (F); (1) re-verify THIS section (errata E-81+); (2) obtain
    the owner rulings on (D); (3) implement D1.a → D1.b → D2.a → D2.b →
    D2.c ONE DIFF AT A TIME under the standing gates (G1-G7 of
    d1-design-consolidated §B, as amended by d1-pinned §3), Fable review
    + owner brief before every emission commit.

(H) OWNER DIRECTION (2026-07-20, post-checkpoint): §9 EAGER-WEB
    DR-LOWERING IS PROMOTED to the top next-epoch candidate, REFRAMED as
    "DeltaRel → Rel" — completing the IR into the single relational
    authority for ALL flow, with differentialness an op ATTRIBUTE/regime
    (the direction D1's regime-split op family already takes), not the
    IR's identity. Motivating facts (owner-observed, measured at tip):
    .deltarel is 12-16 lines on monotone programs vs 419-427 on
    differential ones (symrec_tie_1 12 / demand_tc_witness 16 vs
    fixpoint_stress_1 419 / average_weight 427) because the monotone
    interior — the ORIGINAL Push-method eager web — has no op kinds and
    is invisible to the model (GT-5). ACCEPTANCE CRITERION for §9 = the
    TWO-AUTHORITY SEAM IS DELETED: the ingest-fold hole contract, the
    replicated cut-successor predicates + the §7d role/walk
    divergence-abort, V-INGEST-XCHECK, and the E-42 hand-minted
    VECTORLOOP shim all become internal invariants of one mint+lower
    path; every program gets a proportional dump and a whole-program
    census; eager-per-row vs frontier-batch becomes a LOWERING CHOICE
    on modeled monotone ops (opens the access-plan/WCOJ consumer).
    NOT folded into D2 — the D1 contract is judged against the current
    boundary; §9 runs at next epoch-open under the migration ritual
    (byte-identity A/B per step; structural gates for any one-time
    emission-shape change; the DeltaRel→Rel rename follows the lib/DR→
    lib/DeltaRel ritual). RECOMMENDATION DELTAS this direction induces
    on (D) — the decisions themselves still await the owner:
    - OD-4 REC FLIPS to MECHANISM-NATURAL provisioning: the coherent
      stop-vs-provision split is scaffolding §9 tears down, and the edge
      frontier gains consumers post-Rel; cost = one epoch of dead
      per-edge-add frontier work. Consequence if taken: the pinned
      nested .deltarel block amends by d1-desired-states §C-2's
      enumerated deltas (census kCommitSweep=2, edge ingest gains
      kVecAppend, +1 WAW edge).
    - OD-10 defer-REC STRENGTHENED: §9 schedules a SECOND wholesale
      .deltarel churn (monotone ops appear corpus-wide) — pin no new
      deltarel/irgold surfaces on the witness until the substrate
      stabilizes.
    - OD-3 retirement path RE-HOMED: fence (ii) still ships at D2 as a
      labeled feature gap; its plumbing (input-triggered rebuild via
      input frontiers) lands with/after Rel, where it is the natural
      generalization; the witness flips diagnostic→golden then
      (aggregate_1 precedent).
    - OD-R7 (effect-only frontier, no first-class DRVec) STANDS for D2;
      §9 revisits when monotone frontiers get modeled consumers.
    NEW OWNER FORK OD-11 — NEXT-EPOCH ORDERING: §9-Rel BEFORE D3.a
    (R-DIFF/multi-adornment) or after. REC: Rel first — D3's plumbing
    (fence-ii retirement, input frontiers, R-DIFF lowering) all get
    cheaper on the unified substrate. COST: the death op / V-INST-ORDER
    executing-coverage residual (HP-17) extends one more epoch;
    mitigations: the D2.a DrTest covers the store's death half, and the
    vacuous-green V-INST-ORDER corpus line stays standing.

(I) RATIFICATION RECORD (owner, 2026-07-20): "I ratify all the ODs per
    your recommendations, including OD-11." IMPLEMENTATION IS UNBLOCKED.
    The rulings, binding:
    OD-1  STAGED retraction: D2 = R-MONO-a (birth + demand-flap rebuild;
          death op designed, minted-off); R-DIFF via opt-in
          -demand-retract (@differential fabrication + suppression-
          preserving unask ABI) at D3.a — whose epoch slot follows OD-11.
    OD-2  §18(B)(4) mechanism SUBSTITUTED: same-table_id band key
          (death table_op_sign=-1 / instantiate +1, op_table_id=
          pub_table for both) + always-on V-INST-ORDER + the D1.b
          negative-space test REPLACE the mandated explicit DR-IR edge
          (circular per E-79). The mandate's INTENT (minus-before-plus,
          phantom pairs impossible) is unchanged and now checkably so.
    OD-3  FENCE-NOW-PLUMB-LATER: fence (ii) ships at D2.b as a clean
          diagnostic EXPLICITLY LABELED a feature gap; witness
          demand_midstream_edge_1 lands at D2.c; the plumbing
          (input-triggered rebuild via input frontiers) lands with/after
          Rel, and the witness then flips diagnostic→golden
          (aggregate_1 precedent).
    OD-4  MECHANISM-NATURAL provisioning (the (H) flip): the landed
          cut-successor test provisions ALL monotone boundary inputs —
          the edge table too. CONSEQUENCE, binding on D2.b + the
          desired states: d1-desired-states §B.4 amends per its §C-2
          enumeration — census kCommitSweep=2, the edge ingest fold
          gains kVecAppend(edge, kNetAddition), a second monotone
          sweep op, +1 WAW edge; the demand-only census pin
          (kCommitSweep=1) is SUPERSEDED. One epoch of dead
          per-edge-add frontier appends is the accepted cost; consumers
          arrive with Rel.
    OD-5  Seal carrier = option (iii) self-lowered from kInstanceSeal's
          own dispatch; V-INST-EMITTED enrolls all three kinds (HP-1).
    OD-6  Three-op family CONFIRMED (kSubgraphInstantiate /
          kInstanceDeath / kInstanceSeal, publish-on-instantiate);
          kInstanceEmit/kInstanceOld membership decided by the HP-11
          gate at D1.b (owner nudge on record: lean collapse into
          kStateEmit/kStateOld unless a distinct hazard/census earns
          the new members).
    OD-8  Fence witnesses land WITH D2.c: suite 169→170 (witness)
          →173 (three fence diagnostics, fence-ii included per OD-3);
          runall.sh alternation + CLAUDE.md updated same commit.
    OD-10 Witness .irgold DEFERRED past BOTH scheduled substrate
          churns (D3.a differential flip AND §9-Rel dump reshape);
          correctness at D2.c is carried by .stdout + oracle + eqgate.
    OD-11 REL FIRST: the next epoch after D2 is §9 "DeltaRel → Rel"
          per (H); D3.a (R-DIFF + multi-adornment) follows it. HP-17's
          executing-coverage residual extends accordingly — carried
          LOUD in the ledger until D3.a retires it.
    OD-R1..R9 stand un-vetoed (ratified as consolidated). OD-I1..I4
    remain implementer pins at D1.b, sequenced by HP-11. PIN-1/PIN-3
    remain open (unrelated; no carriers).
    Blocking map now: NOTHING blocks D1.a/D1.b/D2.a/D2.b/D2.c. The
    per-diff design ritual ((3) in (G), i.e. pseudocode → diff-on-
    pseudocode → critique → desired IR states vs real dumps → implement
    → Fable review → owner brief) still gates each landing.

(J) §19 RE-VERIFICATION RECORD + THE (F) ASAN SWEEP (2026-07-20, tip
    99f211f5 — the fourteenth run of the precedent): SEED-HOLDS-WITH-
    ERRATA, GO. Fleet: 4 seed-unread derivation lanes (demand /
    deltarel / cfbuild opus + harness sonnet) + 4 seed-read adversarial
    verifiers (incl. a GROUND-TRUTH REPRODUCTION lane) + xhigh
    consolidator; 9 agents ~722k tokens, 221 tool uses; consolidated
    record in session scratchpad fleet-s19/ (disposable; this entry is
    the record). Orchestrator personal duties per the E-77 house rule:
    E-62 tripwire re-grepped CLEAN (sole hit = Stratum.cpp:1073, a
    comment); the HP-14 referee EXECUTED — permcheck on the exact
    three-counter census append FAILS ("segment 3: boundary line
    differs", exit=1) exactly as pinned, so the D1.b census bless
    stays a DIRECT-DIFF bless. GROUND TRUTH RE-PROVEN AT TIP: every
    d1-ground-truth-nbhd.md appendix (A-G) reproduces BYTE-FOR-BYTE
    with the tip binary; 3-run determinism 1 hash; debug==release on
    the demand-ON surfaces. Zero load-bearing defects; five errata,
    all cosmetic/stale-anchor:
    E-81 d1-design-consolidated §0.1-F2/§A.1.3 enumerate 11 hand call
         sites of CopyDifferentialAndGroupIdsTo; a 12th exists
         (lib/DataFlow/Merge.cpp:924). The "~12" hedge was right; the
         enumeration was short one. Harmless — the D1.a hook lives at
         the View.cpp:557 choke point, covering :924 automatically.
    E-82 d1-design-consolidated F-OPS-4 files demand_forcings storage
         ":1133" under the PUBLIC DataFlow/Query.h (1107 lines); the
         member is lib/DataFlow/Query.h:1133 (internal). §A.1.2 and
         §19 are correct; file misattribution only.
    E-83 d1-pinned HP-4's body-walk reject cite "Demand.cpp:601-604"
         is :602-605 at tip (AsNegate||AsAggregate :602, reject
         :603-605, else-reject :606-607). Mechanism intact.
    E-84 d1-ground-truth-nbhd Appendix G header mislabeled the donor
         dump "demand-ON"; it is the FLAG-OFF dump (the donor's
         differential tail comes from @differential, and the donor
         REJECTS under -demand). Header corrected in place this
         commit; bytes were and are exact.
    E-85 runall.sh:32-33 comment ("inert .main.cpp (never compiled)")
         over-claims: nonascii_1/truncated_decl_1 have no .main.cpp
         at all (169 .dr / 167 .main.cpp). Harmless; recorded.
    THE (F) ASAN SWEEP EXECUTED (both surfaces), ZERO FINDINGS:
    build/asan (Debug + -fsanitize=address -fno-omit-frame-pointer,
    tests ON) — ctest 3/3 PASS under ASAN (108s; the e2e test programs
    themselves are ASAN-compiled); FULL SUITE PASS (169) with
    DR=build/asan (2m03s wall, 8 jobs); SECOND SURFACE priced and RUN:
    full suite with the tip debug compiler + an env-selected CXX
    wrapper (clang++ -fsanitize=address ... "$@" — no harness change,
    never a fifth mode) so generated code + drivers +
    Runtime/Allocator.cpp are sanitized — SUITE PASS (169), zero
    reports, 2m16s wall. CADENCE RECOMMENDATION brought to the owner:
    PER-DIFF for both surfaces (~4.5 min combined — surface 1 on every
    diff, surface 2 at least on every emission/Runtime-touching diff);
    proceeding on that recommendation pending the brief. ASAN runs are
    never timed as benchmarks; nothing enters FINDINGS.md (no
    findings).

(K) D1.a LANDED (2026-07-20; the annotation + recognition registry,
    DataFlow-side, INERT — record written pre-commit). Design ritual
    per (I): the binding adjudicated contract is COMMITTED as
    KeyedInstances.artifacts/d1a-design.md (designer + 3 fresh critics
    + xhigh adjudicator, 5 agents ~538k tokens; verdict
    GO-WITH-AMENDMENTS, 8 amendments folded, 0 rejected). TWO OWNER
    RATIFICATIONS obtained at the pre-implementation brief (recorded
    in the artifact's §2.3/§2.4 banners):
    RAT-1 the per-view-field mechanism swap — d1-design-consolidated
          §A.1.2's guard_annotation_of map + §A.1.3's
          impl->TransferGuardAnnotation is UN-IMPLEMENTABLE at the
          choke point (View.cpp:557/575/602 take only `that`;
          QueryViewImpl has no QueryImpl back-pointer — orchestrator-
          verified). RATIFIED: per-view
          QueryViewImpl::guard_annotation_index{~0u} (the group_ids/
          det_seq precedent), CLEAR-ON-MOVE in the transfer, NO map
          at all (removes HP-9's only map-iteration hazard by
          construction).
    RAT-2 the STEP-8 kind — the query-projection guard has no
          GuardSite record; RATIFIED: stamped kReadAtTuple (the
          direct-read shape), disambiguated by role=kQueryProjection
          + demand_side=kRawSeed; a static_assert couples
          GuardAnnotation::Kind to GuardSite::Kind by value.
    E-86 (erratum, from the ritual's FLAG-2): d1-design-consolidated
          §0.1 F6-annot mis-attributes demand_tc_witness's
          kReadAtTuple to the RECURSIVE BODY; empirically (tc.df +
          classifier + the witness's own comment) the recursive body
          is kPushDown, the base body kBaseAtom, and the sole
          kReadAtTuple is the query-projection stamp (per RAT-2).
    AS LANDED (+205 lines, 4 files): GuardAnnotation +
    RecognizedSubgraph public structs + GuardAnnotations()/
    RecognizedSubgraphs() accessors (include/.../DataFlow/Query.h,
    bodies in Demand.cpp beside DemandForcings()); QueryImpl storage
    (guard_annotations / recognized_subgraphs /
    guard_annotation_folded_count — the counter dormant until D3);
    the two stamps (STEP 7 per GuardSite pre-rewire, STEP 8) keyed on
    the GUARD JOIN (the guarded read is shared and the demand-side
    child CSE-folds — GT-3, so neither can carry the record; verified
    live: the nbhd raw_seed fold runs the loser-unannotated no-op
    case); the per-forcing registry push (X-9); the pre-Optimize-ONLY
    debug census (dead-flow elimination deletes annotated views
    outright with no orphan bucket — the census never re-runs
    post-Optimize); the choke-point transfer with CLEAR-ON-MOVE.
    PREDICTIONS ALL HELD LIVE: P-D1a.1 flag-off mints nothing (G2
    proof); P-D1a.2 tc = 3 stamps {kBaseAtom, kPushDown,
    kReadAtTuple}, 1 subgraph == 1 forcing, census green; P-D1a.3
    nbhd = 2 stamps {kBaseAtom, kReadAtTuple}, 1 subgraph, key
    {Start}; P-D1a.4 zero dump churn. FABLE REVIEW (workflow, 7
    agents ~384k tokens): 3 candidates, ALL REFUTED, 0 confirmed —
    the one real-mechanism note carried LOUD for D1.b/D2.b: the
    stored QueryView handles DANGLE after dead-flow elimination
    erases a view from its DefList; any post-Optimize consumer of
    guarded_read/demanded_view/pub_view must tolerate or pre-filter
    dead/erased views (the census is pre-Optimize by design for this
    reason; first consumer lands D1.b — re-examine there).
    GATES ALL GREEN: SUITE PASS (169, irgold live); 676-row corpus
    A/B + data/ 36-row A/B BYTE-IDENTICAL vs the frozen 99f211f5
    baseline; ctest 3/3; G5 4-surface 3-run 1-hash AND debug==release
    on demand_tc_witness demand-ON; HP-9 re-grep clean (zero sorts,
    no map exists); ASAN BOTH surfaces (ctest 3/3 + suite PASS under
    DR=asan; suite PASS with ASAN-compiled generated code+drivers+
    Runtime) zero reports — the per-diff cadence executed; Q5
    progsize@128 release SAME-SESSION INTERLEAVED ABABAB A {133,135,
    134} vs B {134,132,134} ms (0.0% median, noise; round-0 cold
    outlier discarded); PassPolicy untouched; no Runtime edit; G6
    N/A (no DeltaRel touch). NEXT: D1.b.

(L) D1.b LANDED (2026-07-20; the DR-IR instance op family + validators
    + dump grammar, MINT GATED OFF — record written pre-commit).
    Design ritual: adjudicated contract in session scratchpad
    d1b/d1b-design.md (designer xhigh + 3 critics + xhigh adjudicator;
    GO-WITH-AMENDMENTS, 9 amendments folded, 0 rejected; the designer
    agent died emitting its structured return AFTER completing the
    894-line doc — recovered by stubbing the stage and resuming the
    workflow, no work lost). THE HP-11 DECISION (owner nudge honored,
    adjudicator-upheld): kInstanceEmit → kStateEmit(read_table=
    pub_table), kInstanceOld → kStateOld(pub_table), kInstanceSealSwap
    → kStateFold(pub_table, sign=0) — all three COLLAPSE (no distinct
    hazard target or census freight; semantically exact for the
    InstanceStore transpose: current==working, frozen==sealed; the
    kStateSeal peer already realizes its swap as kStateFold(0)).
    KEPT: kInstanceRebuild (±1 structural regime discriminant +
    TryAdd/Recycle selector) and kInstanceDemand (census-load-bearing
    frozen key read, HP-8 no-hazard). NET NEW EffKinds = 2.
    CONSEQUENCE flagged for the D2.b desired-state refresh: the
    collapsed seal's kStateFold ENROLLS a write hazard on pub_table —
    d1-desired-states §B.4's "no seal edge" is WRONG (stale), corrected
    at the mandatory D2.b re-derivation alongside its OD-4 amendments.
    OWNER RULING obtained this round (RAT-3): HP-3's "ships a
    negative-space test" = the PERMANENT death test at D1.b —
    V-INST-ORDER's core factored as pure CheckInstanceOrder(const
    DRFlowGraph&), NEW ctest target tests/DeltaRelValidators (DrTest +
    fork/waitpid; death arm asserts WIFSIGNALED && WTERMSIG==SIGABRT
    specifically; fflush-before-fork; fork-failure loud; ctest 3→4) —
    not the run-once probe the design first proposed.
    AS LANDED (+716 lines, 5 files + the new test dir): 3 DROpKinds
    after kStateSeal (kSubgraphInstantiate / kInstanceDeath /
    kInstanceSeal, band 11 for the seal); DROp instance payload riding
    table_op_table=pub_table + table_op_sign ∓1 (HP-3/OD-2 — the sign
    tie-break fires on equal table_id); DRInstance descriptor (incl.
    pub_view + precomputed forcing_name per the render amendments);
    BuildSubgraphInstanceOps gated on Context::demand_instance_enabled
    {false} (structurally unreachable — no flag exists until D2.b);
    effect builders regime-split per §A.2.3 as HP-11-collapsed;
    DROpStratum instance cases ValidatorFail-on-miss (a deliberate
    strengthening over kGroupUpdate's return-0u); the effect-hazard
    switch gains kInstanceRebuild(write)/kInstanceDemand(no-hazard)
    cases AND default → ValidatorFail("unhandled EffKind");
    V-INST-EFFECT/SOLE/PAIR (3-way arm inert per HP-17) +
    V-INST-ORDER always-on in LinearizeAndValidateDRFlow; Format.cpp
    spelling rows (loud-abort idiom), 3 census counters via kAllKinds,
    instances: section (p11 empty-guard), 3 op p-rules (i# in header,
    store=I# in args; ik:/row: tags via pub_view.Columns(); abort on
    unresolvable). Census recount knob-gated — RecognizedSubgraphs()
    handles NEVER dereferenced at D1.b (the §19(K) caveat); the
    adjudicator's ABA warning is BINDING ON D2.b: the mint's identity
    scheme must be deref-free and ABA-safe (re-resolve by forcing/
    message identity, never raw pointer) — carried LOUD.
    HP-17 DISCHARGED OBSERVABLY: temporary probe (deleted before
    commit) showed V-INST-ORDER RAN on every probed flow
    (average_weight ops=53, fixpoint_stress_1 ops=40, tc ops=2,
    symrec ops=1; 0 instance ops, 0 aborts) — the vacuous-green line.
    THE ONE CHURN, blessed per HP-14's three-point DIRECT-DIFF
    referee (executed: (i) exactly one changed line — census line 16;
    (ii) delta exactly " kSubgraphInstantiate=0 kInstanceDeath=0
    kInstanceSeal=0" appended in kAllKinds order; (iii) census-sum
    abort green); the --bless re-wrote 7 goldens, git shows exactly
    ONE file one line changed (the other 6 byte-identical — zero
    stray churn, P-D1b.2 exact).
    FABLE REVIEW (workflow, 9 agents ~638k tokens): 3 CONFIRMED, ALL
    in the NEW TEST FILE (none in the compiler diff), all fixed +
    re-verified — fflush-before-fork (buffered-stdout duplication
    under ctest pipes, empirically reproduced then gone),
    SIGABRT-specific death assert, loud fork-failure arm.
    GATES ALL GREEN (re-run/re-verified post-fix): SUITE PASS (169)
    with the blessed line; single pre-registered IRGOLD-DIVERGE
    before bless, nothing else; 676-row corpus A/B + data/ A/B
    BYTE-IDENTICAL vs frozen 99f211f5 (P-D1b.1 — emission untouched);
    ctest 4/4 debug + DeltaRelValidators green under ASAN; G5 3-run
    1-hash + debug==release on BOTH irgold carriers (tc 4 surfaces,
    symrec 2); G6 E-62 tripwire re-grepped CLEAN (sole external hit
    the Stratum.cpp:1073 comment; the one new pinned_order reader is
    V-INST-ORDER itself — a validator, the sanctioned class); ASAN
    both surfaces SUITE PASS zero reports; P-D1b.3 held (default-
    abort trips nothing corpus-wide); Q5 ABABAB A {148,148,145} vs
    B {144,147,146} ms (−1.4% median, noise); golden re-verified
    byte-exact post-rebuild. NEXT: D2.a.

(M) D2.a LANDED (2026-07-20; the Runtime InstanceStore + RowStore::
    Reset + the DrTest unit, INERT — record written pre-commit).
    Design ritual: adjudicated contract in session scratchpad
    d2a/d2a-design.md (designer xhigh + 2 critics + xhigh adjudicator;
    GO-WITH-AMENDMENTS, 4 folded — the consequential one:
    hand-rolled configure lines need -DDRLOJEKYLL_ENABLE_TESTS=ON or
    the HP-16 gate passes VACUOUSLY [tests/ default-OFF outside the
    presets]). OWNER RATIFICATIONS this round:
    RAT-4 the `monotone` ctor bool (default true) gates the HP-7
          frozen⊆current seal belt — the belt CANNOT be unconditional
          (the death-half unit and D3.a R-DIFF legitimately shrink
          current); one bool beyond §A.3.1's literal surface.
    RAT-5 the belt-fires NEGATIVE ships NOW (overriding the design's
          defer-to-D3.a): a fork/waitpid death arm in the unit (the
          RAT-3 mold verbatim — fflush-before-fork, loud fork-fail,
          SIGABRT-specific), #ifndef NDEBUG'd so release SKIPS it.
    AS LANDED: include/drlojekyll/Runtime/InstanceStore.h (NEW, 333
    lines) — the StateCellStore transpose: dense-iid monotone-forever
    namespace, Vec<Table*> frozen/current pairs, FindOrAddInstance
    (open-addressing mold), TouchCurrent/Touched (sort-unique),
    KeyAt, WorkingOccupied==NumRows>0 (the N-1 working_count drop
    recorded in the header — revisit at R-DIFF), Seal (.Set-triple
    pointer swap — Vec::operator[] is const, std::swap ill-formed;
    Reset on the new current; sealed_occupied snapshot; the RAT-4
    belt), RecycleCurrent unconditional-idempotent, DebugValidate;
    bench-counter seam reuses ONLY enumerated
    HYDE_RT_BENCH_COUNTER_FIELDS names (zero new). Table.h +27:
    protected RowStore::Reset (Truncate + in-place slot loop, NO
    allocator entry point — the H6 Arena discharge) chained by public
    Table::Reset (sealed=0). tests/InstanceStore/ (NEW, 5th ctest
    target per P-D2a.2): 8 arms — mint/find/collision-growth,
    Touch/Touched, KeyAt, Seal semantics, Recycle idempotence
    (twice==once), H6 quantitative Arena regression, the death half
    (Recycle→re-add — the store's only pre-D3 execution), the RAT-5
    belt-fires fork negative. Intent-communicating asserts
    throughout. FABLE REVIEW (workflow, 4+ agents ~341k tokens): 1
    CONFIRMED (MED — the fork helper compiled-but-unused under
    NDEBUG: -Wunused-function in release; fixed by moving helper+enum
    inside the #ifndef NDEBUG; NDEBUG syntax-only re-check clean),
    rest refuted. HP-16 GATE EXECUTED with teeth: build/benchcount
    (Debug + -DDRLOJEKYLL_BENCH_COUNTERS + ENABLE_TESTS=ON) full
    build green, instance_store_test COMPILES under the ON define
    (proves every counter name enumerated), ctest 5/5, FULL SUITE
    PASS (169) with the counters-ON compiler AND counters-ON drivers
    (via the env CXX wrapper — the harness's quoted "$CXX" cannot
    take a multi-word value; the wrapper is the established seam).
    GATES ALL GREEN: SUITE PASS (169) with drivers compiled against
    the new Table.h; 676-row + data A/B BYTE-IDENTICAL vs frozen
    99f211f5 (P-D2a.1); ctest 5/5 debug + 5/5 under ASAN (both fork
    death arms green under ASAN); release unit 7/7 (death arm
    NDEBUG-skipped by design); ASAN both surfaces SUITE PASS zero
    reports; no DeltaRel touch (E-62 N/A); PassPolicy untouched; Q5
    ABABAB A {139,139,141} vs B {140,139,141} ms (0.0% median,
    noise). NEXT: D2.b — carrying LOUD: the ABA-safe deref-free mint
    identity, the §B.4 desired-state re-derivation (OD-4
    mechanism-natural + the collapsed-seal kStateFold WAW edge), and
    HP-13(b)'s end-to-end real-dump review before any nested bless.

(N) D2.b LANDED (2026-07-21; the -demand-instance nested lowering
    end-to-end — recognizer excision → DR-IR mint → CF lowering →
    codegen; record written pre-commit). DESIGN RITUAL (the epoch's
    largest): designer + desired-states writer in parallel (the
    ds-writer re-derived the nested .deltarel from REAL tip dumps,
    SUPERSEDING d1-desired-states §B.4 IN WHOLE — wrong on FOUR axes:
    OD-4 mechanism-natural, the HP-11 collapse spellings, the
    collapsed seal's write hazard, and DEPS ARE SIX EDGES NOT TWO) +
    3 critics + xhigh adjudicator; GO-WITH-AMENDMENTS, 9 folded
    (ADJ-C1 the full-rescan collapse — TouchCurrent does NOT reset,
    freshness comes from the prior Seal's Reset; ADJ-C2 fence-i also
    rejects recursive content; ADJ-P1 V-ALPHA arm B three-part;
    ADJ-G1 the a2 effects fork). THE RITUAL'S BIG CATCH (ADJ-C3,
    code-proven): OD-1's "demand-flap rebuild" wording is a NO-OP
    under R-MONO (if-crossed idempotent — a re-asserted demand never
    re-seeds); a1-only silently drops edge-after-demand forever.
    OWNER RULINGS this round:
    RAT-6 a1-ONLY, BIRTH-ONLY witness (ruling-consistent with OD-3's
          plumbing-lands-with-Rel): the flap narrative RETIRED, the
          D2.c witness ENFORCED birth-only with edges hard-ordered
          before every demand, edge-after-demand a LOUD LABELED gap,
          the mechanism-natural edge frontier provisioned-undrained
          (the OD-4 accepted pricing).
    RAT-7 NO runtime band-(b) partition assert: the HP-6 guardians
          post-HP-11-collapse are the SITE-3 real-codegen review
          (EXECUTED by the orchestrator this landing — the emitted
          band-(b) is genuinely frz.Find==kNoRow (F,T)-gated, the
          (T,F) scan correctly absent under R-MONO, V-INST-FRESH
          always-on, Row_0 carries NO α column — the elision is
          real) + the D2.c eqgate; revisit at D3.a.
    OWNER-BRIEF DEVIATION (from OD-3's literal wording, implementer-
    proven): fence-(ii) CANNOT be a compile diagnostic — mid-stream
    edge-add is a BATCH-ORDERING property indistinguishable at
    compile time from the accepted witness program itself; it ships
    as the DOCUMENTED labeled feature gap (Build.cpp comment;
    CLAUDE.md naming at D2.c) — consequently demand_midstream_edge_1
    has no diagnostic to witness and the D2.c fence-witness set
    becomes {demand_cyclic_1, demand_recursive_content_1 (OWN-4/
    ADJ-C2), demand_diff_input_1}; suite arithmetic 169→170→173
    holds with the recomposed set. Also: fence-iii's shape is
    pre-empted upstream by the demand body-walk rejects (the fence
    stands as the D3.a belt); the spine renders the section-walk
    TARGET plan while D2.b codegen full-scans-with-key-filter (the
    review REFUTED this as a defect — ratified model/emission
    layering; the keyed rescan is the deferred perf refinement, .ir
    uncertified at D2.b by design).
    AS LANDED (+1039 lines, 16 files): ResolveLiveRecognition — the
    ABA-SAFE DEREF-FREE identity (the §19(K)/(L)/(M) carry
    DISCHARGED): stored handles empirically DEAD post-Optimize
    (live_rs=0); resolution walks LIVE guard JOINs via the
    CSE-migrating GuardAnnotationIndex stamp, tables keyed off parse
    identities; the census recount UN-GATED and live under the knob.
    New public QueryView::GuardAnnotationIndex(). The knob
    (-demand-instance, implies -demand, OFF PassPolicy); GT-5
    excision (the flat guard web + %table:15 machinery NOT emitted
    under the knob — the generated header carries NO Row15 at all);
    OD-4 mechanism-natural provisioning (both boundary frontiers;
    census kCommitSweep=2); V-ALPHA arms A + B(i/ii/iii),
    V-INST-DRAIN, V-INST-EMITTED (all three kinds), HP-4 refusal;
    LowerSubgraphInstance (band-a1 drain → FindOrAddInstance →
    V-INST-FRESH → full-rescan → TryAdd; band-b (F,T) publish; seal
    self-lowered per OD-5/HP-1); SUBGRAPHINSTANCE region + codegen
    against the real InstanceStore (monotone=true). FIRST NESTED
    COMPILE IN HISTORY matches the re-derived ds contract EXACTLY —
    op set, effect multisets, line order, census, ALL SIX dep edges;
    the op-id map came out IDENTITY vs the illustrative labels.
    Nested .df == flat .df BYTE-IDENTICAL (Alt-A held). BIRTH PROBE:
    flat==nested stdout byte-equal (nb 1: 2 3 — NOT 9, HP-5's
    out-of-neighborhood discrimination held; nb 9: 9; nb 5 empty).
    FABLE REVIEW (workflow, 10 agents ~1.02M tokens): 1 CONFIRMED
    (MED, probe-REPRODUCED silent miscompile — pub_table resolved by
    declaration NAME only, ignoring arity; a legal same-name/
    different-arity sibling with a table-modeled answer INSERT bound
    the WRONG pub table and flowed to codegen silently; FIXED to
    full declaration identity Id()==Id(), the design's own §2.1
    mandate; the repro probe now binds the correct-width table and
    the witness dumps are byte-UNCHANGED by the fix), 5 refuted
    (incl. the spine-honesty and V-ALPHA-arm-A-scope candidates).
    GATES ALL GREEN (post-fix re-runs incl.): SUITE PASS (169)
    knob-off ×3 runs (debug, ASAN, post-fix); 676-row + data A/B
    BYTE-IDENTICAL vs frozen 99f211f5 (P-D2b.1); ctest 5/5 debug +
    5/5 ASAN; G5 debug==release + multi-run 1-hash on all four
    NESTED surfaces (HP-13(b)'s config-invariance precondition — the
    nested goldens themselves stay UNBLESSED at D2.b per OWN-5/
    OD-10); G6 E-62 re-grepped CLEAN; ASAN both surfaces + the
    nested compile under the ASAN binary (the ABA walk sanitized)
    zero reports; HP-12's no-uncovered-α-consumer line checked (the
    two α consumers on the witness — the rescan filter and the
    pub_row ik: slot — both covered; review fences lens concurred);
    Q5 progsize@128 release SAME-SESSION INTERLEAVED ABABAB A
    {152,152,154} vs B {154,152,151} ms (0.0% median, noise; round-0
    cold outlier discarded); post-fix SUITE PASS (169) + ctest 5/5
    re-run. NEXT: D2.c (witness + eqgate + the recomposed
    fence-witness set enter the suite).

(O) D2.c LANDED (2026-07-21; the witness + equivalence gate + fence
    witnesses enter the suite, 169→173 — record written pre-commit).
    ZERO compiler change (tests/docs only; the drlojekyll binary is
    byte-unchanged, so the 676-row A/B and Q5 are unchanged BY
    CONSTRUCTION — verified via zero lib/bin/include churn + no
    recompile). Design ritual: designer + 2 critics (who EMPIRICALLY
    ran both arms ×4 modes, the oracle/monotone arms, an
    intentionally over-materializing "bad" driver proving HP-5's
    discrimination has teeth, and all three fence probes) + xhigh
    adjudicator; GO-WITH-AMENDMENTS, 5 folded, 1 rejected. OWNER
    RULINGS:
    RAT-8  the BLESS-BOOTSTRAP house rule (E-77 family, STANDING):
           "never bless a red case green" means never bless a case
           that DIVERGED from an EXISTING golden; seeding a
           first-ever golden from a REVIEWED run (outputs checked
           against the design contract before --bless, then re-run
           to SUITE PASS) is the sanctioned exception.
    RAT-9  the eqgate's nested arm runs ALL FOUR optimization modes
           (the [ADJ:H1] catch: opt-only left nested nodf/nocf/none
           unchecked while -demand-instance drives cf lowering the
           cf-opt modes reshape); each mode byte-compared to the
           .stdout golden; flat==nested follows transitively via
           diffrun's standing flat==golden; FLAT-NESTED-DIVERGE
           retired for NESTED-GOLDEN-DIVERGE.
    RAT-10 demand_recursive_content_1's .drflags is BARE -demand
           (the [ADJ:H3] de-lump: it rejects UPSTREAM in the
           plain-demand body-walk — the Build.cpp recursive-content
           fence is SHADOWED at tip, documented; the case is the
           body-walk witness, not a nested fence).
    NOTABLE DESIGNER EMPIRICAL CORRECTIONS: demand_diff_input_1's
    minimal shape PASSES the demand transform and reaches the REAL
    fence-iii (its own diagnostic — a stronger witness than the
    predicted upstream-reject); the witness driver carries EXPLICIT
    HP-5 asserts (the tc mold had only golden-compare; note driver
    asserts are live — diffrun compiles drivers -g without -DNDEBUG).
    AS LANDED: cases/demand_neighborhood_witness.{dr (RAT-6
    birth-only header), drflags(-demand), batches (add_edge only,
    two edge batches BEFORE all probes — both seal surfaces
    exercised per [ADJ:W2]), eqgate, main.cpp (HP-5 asserts; sorted
    keyed drains)} + demand_cyclic_1 + demand_recursive_content_1 +
    demand_diff_input_1 (each .dr/.drflags/.main.cpp; diagnostics:
    "Recursive demand relations are not yet supported under
    -demand-instance" / "Unsupported rule-body shape under -demand"
    / "Demanded subgraphs over deletable (differential) inputs are
    not yet supported under -demand-instance"); goldens +3
    (.stdout "nbhd 1: 2 3 4 / nbhd 3: 5 6 / nbhd 9: 9 / nbhd 5:" +
    .oracle.stdout + .monotone.stdout — each REVIEWED byte-exact vs
    the design contract before bless per RAT-8; ZERO existing-golden
    churn, git-verified); runall.sh run_eqgate (sidecar-guarded in
    --one between run_oracle and run_irgold; $NAME.eqgate.<mode>
    layout; NESTED-GOLDEN-DIVERGE / EQGATE-*-FAIL /
    EQGATE-GOLDEN-MISSING all summary-grep-visible) + the
    three-name alternation edit; CLAUDE.md (173, the corrected
    diagnostic attributions, the eqgate arm, the edge-after-demand
    feature gap named, the witness described birth-only).
    FABLE REVIEW (5 agents ~300k tokens): 1 CONFIRMED (LOW — a
    stale driver comment contradicting the RAT-10 bare sidecar;
    fixed), 1 refuted; orchestrator's own sweep additionally caught
    4 MISSING copyright headers on the new drivers (added; the
    review's copyright lens had passed them — recorded as a
    review-lens miss). GATES: SUITE PASS (173) with the eqgate arm
    live (all four nested modes OK) — ×3 runs (implementer, ASAN,
    final post-fix); ASAN BOTH surfaces PASS at 173 zero reports
    (the nested eqgate drivers ran ASAN-compiled); ctest 5/5;
    bash-3.2 compat scan of the runall diff clean; the witness's
    four golden modes byte-agree; binary byte-unchanged (A/B + Q5
    N/A by construction). THE D1→D2 IMPLEMENTATION CHARTER IS
    COMPLETE: D1.a → D1.b → D2.a → D2.b → D2.c all landed under the
    ritual. NEXT: the §20 epoch-close checkpoint + the §9
    "DeltaRel → Rel" epoch-open.

## §20. EPOCH-CLOSE CHECKPOINT (2026-07-21, tip a77b8bea) — the D1→D2 charter COMPLETE; THE ENTRY POINT for the next session (single-pass: re-verify per the house precedent, errata E-87+)

THE ENTRY POINT for the next session. The D1→D2 keyed-instances charter is
DONE: the nested (`-demand-instance`) lowering runs end-to-end, knob-off
INERT, behind the standing gates. SINGLE-PASS (the house precedent): the
next session re-verifies THIS section before building on it; errata continue
at E-87. The next epoch is §9 "DeltaRel → Rel" per OD-11 — its epoch-open
brief is KeyedInstances.artifacts/rel-epoch-open-brief.md.

(A) STATUS — WHAT LANDED THIS EPOCH.
    Prior sessions (ledger §8-§19(J)): T1 + the (F) determinism landing +
    T2a/T2b/T2b.0 (the -df-out/-deltarel-out dump instruments) + T3 (the
    irgold golden machinery) + P1 (the PassPolicy pass-harness). This
    session, the five D-diffs of the §19 charter, ONE DIFF AT A TIME under
    the per-diff design ritual (pseudocode → diff → critique → desired-IR-
    states-vs-real-dumps → implement → Fable review → owner brief), each a
    landing record:
      §19(K) D1.a  DataFlow annotation + recognition registry (INERT).
      §19(L) D1.b  DR-IR instance op family + validators + dump grammar
                   (MINT GATED OFF); ctest 3→4 (tests/DeltaRelValidators).
      §19(M) D2.a  Runtime InstanceStore + RowStore::Reset + unit (INERT);
                   ctest 4→5 (tests/InstanceStore).
      §19(N) D2.b  THE NESTED LOWERING — recognizer excision → DR-IR mint →
                   CF lowering → codegen against the real InstanceStore;
                   knob-off byte-identical, knob-on the first nested compile
                   in history.
      §19(O) D2.c  witness + equivalence gate + fence witnesses enter the
                   suite; ZERO compiler change.
    SUITE 169→173 (demand_neighborhood_witness + demand_cyclic_1 +
    demand_recursive_content_1 + demand_diff_input_1). ctest 3→5. Errata
    through E-86 (E-86 the D1.a F6-annot kind fix; E-81..E-85 the §19(J)
    re-verify cosmetics). Owner pins PIN-1/PIN-3 remain OPEN (no carriers
    arose). The ten owner ratifications, binding:

      RAT   DIFF   STATEMENT (one line; full text in the cited §19 record)
      ----  -----  -----------------------------------------------------
      RAT-1 D1.a   per-view QueryViewImpl::guard_annotation_index REPLACES
                   the pinned guard_annotation_of map (un-implementable at
                   the View.cpp:557 choke point) — no map, no HP-9 hazard.
      RAT-2 D1.a   the step-8 query-projection guard is stamped kReadAtTuple
                   (role=kQueryProjection + demand_side=kRawSeed; a
                   static_assert couples GuardAnnotation::Kind to GuardSite).
      RAT-3 D1.b   HP-3's negative-space test = the PERMANENT death test:
                   pure CheckInstanceOrder + tests/DeltaRelValidators fork/
                   waitpid SIGABRT (not a run-once probe).
      RAT-4 D2.a   a `monotone` ctor bool (default true) gates the HP-7
                   frozen⊆current seal belt (it CANNOT be unconditional —
                   the death half + R-DIFF legitimately shrink current).
      RAT-5 D2.a   the belt-fires NEGATIVE ships NOW (fork/waitpid death
                   arm, #ifndef NDEBUG) — overriding the design's defer.
      RAT-6 D2.b   a1-ONLY BIRTH-ONLY: OD-1's demand-flap rebuild is a NO-OP
                   under R-MONO (code-proven); the flap narrative RETIRED,
                   edge-after-demand a LOUD LABELED gap, the mechanism-
                   natural edge frontier provisioned-undrained.
      RAT-7 D2.b   NO runtime band-(b) partition assert — the SITE-3 real-
                   codegen review (EXECUTED: (F,T)-gated, α-elision real,
                   Row_0 carries no α) + the eqgate are the guardians;
                   re-opens at D3.a.
      RAT-8 D2.c   the BLESS-BOOTSTRAP house rule (STANDING, E-77 family):
                   seeding a first-ever golden from a REVIEWED run is the
                   sanctioned exception to "never bless a red case green".
      RAT-9 D2.c   the eqgate's nested arm runs ALL FOUR optimization modes,
                   each byte-compared to the .stdout golden (the opt-only
                   hole was real); FLAT-NESTED-DIVERGE → NESTED-GOLDEN-DIVERGE.
      RAT-10 D2.c  demand_recursive_content_1's .drflags is BARE -demand: it
                   rejects UPSTREAM in the plain-demand body-walk (the
                   Build.cpp recursive-content fence is SHADOWED at tip,
                   documented) — a body-walk witness, not a nested fence.

(B) THE AS-LANDED NESTED-LOWERING ARCHITECTURE (source of truth = the five
    committed adjudicated contracts; POINT, don't restate).
    Section-by-section authority:
      - D1.a annotation + registry .... KeyedInstances.artifacts/d1a-design.md
      - D1.b op family/validators ...... d1b-design.md
      - D2.a Runtime store ............. d2a-design.md
      - D2.b lowering + codegen ........ d2b-design.md (+ d2b-desired-states.md,
                                         the REAL-dump re-derivation superseding
                                         d1-desired-states §B.4 IN WHOLE)
      - D2.c witness + eqgate .......... d2c-design.md
    THE ~15-LINE END-TO-END FLOW (knob = -demand-instance, implies -demand,
    OFF the PassPolicy registry — "demand is semantics"):
        -demand-instance
          → Query::Build: ApplyDemandTransform mints the demand relation,
            then D1.a stamps a GuardAnnotation at each guard JOIN (Demand.cpp
            step-7 / step-8, keyed on the GUARD JOIN — GT-3) and pushes one
            RecognizedSubgraph per DemandForcing (X-9); guard_annotation_index
            rides CopyDifferentialAndGroupIdsTo (View.cpp:557), CLEAR-ON-MOVE
          → Program::Build: ResolveLiveRecognition (ABA-SAFE, DEREF-FREE —
            stored handles are DEAD post-Optimize; walks LIVE guard JOINs via
            the CSE-migrating GuardAnnotationIndex stamp, tables keyed off
            parse identity; census recount live under the knob)
          → BuildSubgraphInstanceOps mints the three-op family (Context::
            demand_instance_enabled): kSubgraphInstantiate (BIRTH/REBUILD +
            band-(b) publish, SOLE pub deriver) / kInstanceDeath (OWN op,
            minted-OFF at R-MONO, HP-17) / kInstanceSeal (band 11, self-
            lowered per OD-5/HP-1); payload rides table_op_table=pub_table +
            table_op_sign ∓1 (the same-table_id band-key sign tie-break
            REPLACES the circular explicit edge — OD-2)
          → LowerSubgraphInstances (Procedure.cpp:249, GroupUpdates mold —
            the mold itself lives in Stratum.cpp; E-93): band-a1
            drain demand frontier → FindOrAddInstance → V-INST-FRESH →
            full-rescan → TryAdd (BIRTH-ONLY per RAT-6 — no a2 rebuild) →
            band-b (F,T)→+pub_row publish (the (T,F) drop scan EMPTY under
            R-MONO); seal self-lowered
          → codegen: SUBGRAPHINSTANCE region against the real InstanceStore
            (monotone=true; the flat guard web EXCISED — %table:15's create
            line SURVIVES but loses its guard index and is never scanned,
            per d2b-desired-states; E-94 corrected the "no Row15" claim —
            struct Row15/table_15 remain in the nested header)
        The eqgate (run_eqgate, runall.sh --one) is the STANDING referee:
        flat stdout == nested stdout (all four modes) == blessed .stdout
        golden (ANSWER identity, never generated bytes); the oracle closes
        the flat arm; DEATH is oracle-blind forever.

(C) LOUD RESIDUALS carried forward (harvested from §19(K)-(O)).
    - EDGE-AFTER-DEMAND — DISCHARGED BY R-a2 (the first Rel slice, OD-12;
      landing record §20(G)). Band-(a2) drains the OD-4 edge frontier and
      full-rescans live-demanded keys (the ADJ-C1 collapse); the witness is
      BIRTH-AND-REBUILD and its golden trio re-blessed via the ritual. The
      CLAUDE.md + Build.cpp gap labels retired in the same diff. [Original
      residual text retired 2026-07-21; RAT-6's witness enforcement lifted
      per the OD-12-sanctioned reversal path.]
    - HP-17 the death-op / V-INST-ORDER EXECUTING-COVERAGE residual. kInstance
      Death is minted-OFF at R-MONO and V-INST-ORDER's corpus line is
      vacuous-green (0 instance ops); no test EXECUTES an ordered instance-
      op pair through the emitter. Per OD-11 this EXTENDS PAST Rel — D3.a
      (R-DIFF) retires it. Mitigations standing: the D2.a DrTest death half +
      the RAT-3 permanent CheckInstanceOrder negative. CARRIED LOUD.
    - OWN-3 (d2b-design §OWN-3 / ADJ-C4): the View.cpp:585-587 record-
      comparing incompatible-fold diagnostic must be PROMOTED always-on — a
      HARD D3 PRECONDITION before recursive/multi-guard demanded content is
      admitted (both halves, per d2b-design §OWN-3 — E-96).
    - THE Build.cpp RECURSIVE-CONTENT FENCE is SHADOWED UPSTREAM by the
      plain-demand body-walk reject (RAT-10). The Build.cpp fence stands as
      the D3.a belt; demand_recursive_content_1 witnesses the upstream
      body-walk reject, not the nested fence.
    - THE SPINE SECTION-WALK is the MODELED TARGET plan; D2.b codegen full-
      scans-with-key-filter (review-RATIFIED as model/emission layering, not
      a defect). The keyed rescan is the DEFERRED perf refinement; .ir is
      uncertified at D2.b by design.
    - NESTED GOLDENS / .irgold UNBLESSED (OD-10/OWN-5): deferred past BOTH
      scheduled substrate churns (the D3.a differential flip AND the §9-Rel
      dump reshape). Correctness at D2.c rests on .stdout + oracle + eqgate.
    - N-1 the InstanceStore WorkingOccupied==NumRows>0 working_count drop —
      recorded in the header, revisit at R-DIFF.
    - THE BAND-(b) PARTITION-ASSERT question RE-OPENS at D3.a (RAT-7): the
      (T,F) drop scan is empty under R-MONO; whether a runtime partition
      assert is owed returns when R-DIFF makes drops reachable.
    - PIN-1 (constant-column token) / PIN-3 (negate class refinement) still
      OPEN — no carriers arose this epoch.

(D) THE PATH FORWARD.
    NEXT EPOCH = §9 "DeltaRel → Rel" per OD-11 (one relational authority for
    ALL flow; differentialness = op attribute/regime). Its epoch-open brief
    is KeyedInstances.artifacts/rel-epoch-open-brief.md (§19(H)'s acceptance
    criterion, the migration ritual, what D2.b added to the Rel case, the
    first-slice recommendation). D3.a (R-DIFF + multi-adornment) FOLLOWS Rel;
    D4 (seams design-only) defers with it. NEXT SESSION: (0) ASAN sweep per
    §19(F); (1) re-verify THIS section (errata E-87+); (2) epoch-open re-rank
    (Rel is the ranked top per OD-11; pass-harness P2-P5, §13 lattice, §14
    CodeQL compete); (3) open the chosen epoch under its design ritual.
    "DeltaRel → Rel" epoch-open brief (OD-11: Rel precedes D3.a).

(E) REL-EPOCH-OPEN RE-VERIFICATION RECORD (2026-07-21, tip 5813ab8a;
    the §20/§(D) NEXT-SESSION items (0)+(1) EXECUTED).
    (0) THE ASAN STANDING SWEEP, BOTH SURFACES, ZERO FINDINGS: build/asan
        rebuilt at tip; ctest 5/5 under ASAN (119.8s); SUITE PASS (173)
        with DR=build/asan; SUITE PASS (173) with the debug compiler + the
        env-CXX clang-asan wrapper (generated code + drivers + Runtime
        sanitized); zero sanitizer reports; nothing enters FINDINGS.md.
    (1) THIS SECTION + rel-arch-pseudocode.md + rel-epoch-open-brief.md
        FLEET-RE-VERIFIED: 3 seed-UNREAD opus derivation lanes (eager web
        Build.cpp:841-1170 / DR mint+linearizer / nested path — code only,
        docs forbidden) + 4 seed-read adversarial verifiers (every anchor)
        + 1 xhigh consolidator adjudicating EVERY candidate at the code
        (8 agents, ~804k tokens). The E-62 tripwire re-grepped by the
        ORCHESTRATOR personally: CLEAN (sole out-of-lib hit = the
        Stratum.cpp:1073 comment, the standing baseline; in-lib hits =
        the substrate-fill writers, validators, and the T2b-sanctioned
        dump reader). VERDICT: SOUND-WITH-ERRATA — eleven errata
        E-87..E-97, ALL MED OR BELOW, zero code or design defects, ALL
        APPLIED IN PLACE (each site carries its E-tag):
          E-87 MED  pseudocode §1: Authority-B driver is
                    BuildEntryProcedure (Procedure.cpp:725), not the
                    nonexistent "BuildEagerProcedures".
          E-88 MED  pseudocode §2: the deletion-side fold body IS the
                    queue VECTORAPPEND (Stratum.cpp:1962-1975), cursor
                    DISCARDED (Procedure.cpp:54-59); the EMPTY-body HOLE
                    + INGEST-CURSOR-SHAPE guard are MONOTONE-only
                    properties (Procedure.cpp:79-93).
          E-89 LOW  pseudocode §2: kAddQueue park predicate restored its
                    parent != parent_ conjunct (Build.cpp:867-868).
          E-90 MED  pseudocode §3: the A/B corpus gate is the FROZEN
                    676-row (169×4) baseline (KeyedInstances.md:442) —
                    "692" was a naive 173×4 recompute; the brief's 676
                    was CORRECT (the opposite verifier claim REFUTED).
          E-91 COSM pseudocode §1: Program::Build 5th arg demand_instance.
          E-92 LOW  pseudocode §2: BuildEagerInductiveRegion feeds
                    induction input vecs; the fixpoint ROUND SHELLS are
                    Authority A (LowerDRRounds, Stratum.cpp:1651ff).
          E-93 MED  §20(B): LowerSubgraphInstances lives at
                    Procedure.cpp:249, NOT Stratum.cpp (the GroupUpdates
                    MOLD lives there — the confusion source).
          E-94 MED  §20(B): "no Row15 in the header" was FALSE —
                    consolidator re-compiled the witness nested: struct
                    Row15/table_15 SURVIVE; only the guard index + scans
                    are excised (matches d2b-desired-states:528-540).
          E-95 LOW  §9 + brief §5: the monotone step-kind enumeration is
                    EIGHT arms (Build.cpp:1084) — plain-MERGE union +
                    SELECT rebind restored to both docs.
          E-96 LOW  OWN-3 scope re-unified to "recursive/multi-guard"
                    (both halves, d2b-design §OWN-3) in pseudocode + (C).
          E-97 COSM brief: "verbatim-faithful" → "faithful" (elaboration,
                    not quotation).
        VERIFIED-CLEAN (held): the four seam artifacts S1-S4 + the
        is_recog_guard twin; cut-successor (Build.cpp:965-972) ==
        AnyCutSuccessorDR (DeltaRel.cpp:130-147) lock-step via the §7d
        cross-check (DeltaRel.cpp:3721-3745); the OD-4 append predicate;
        the dispatch table; RAT-1..RAT-10 vs records AND code; the five
        artifacts + supersedes banner; 173/5 counts; the witness nested
        census (kSubgraphInstantiate=1, kInstanceDeath=0, kInstanceSeal=1,
        kIngestFold=2, kCommitSweep=2) + the provisioned-UNDRAINED edge
        frontier live; the .deltarel measurement 12/16/419/427 EXACT;
        kInstanceDeath gate = TableIsDifferential (DeltaRel.cpp:1137);
        V-INST-ORDER vacuous-green; the shadowed recursive-content fence
        reproduced; spine full-scan-vs-modeled-target matches code; R-a2
        ADJ-C1 framing matches d2b-design. Full adjudication record:
        session scratchpad e87-errata.md (disposable; THIS entry is the
        binding record). Errata continue at E-98.

(F) OD-12 — THE EPOCH-OPEN RE-RANK RULING (owner, 2026-07-21): the §9
    "DeltaRel → Rel" epoch OPENS with R-a2 FIRST — the input-frontier
    drain for SUBGRAPH_INSTANTIATE (band-(a2) full-rescan keyed on the
    edge frontier, the ADJ-C1 collapsed mechanism) — then the witness's
    two-join monotone web step kinds (TUPLE/INSERT before JOIN, per
    brief §5 as amended by E-95). Alternatives (step-kinds-first,
    pass-harness P2-P5, §13/§14) were tabled and declined. LINEAGE:
    this is the sanctioned reversal path RAT-6/OD-3 pre-registered —
    the a1-only birth-only enforcement was the D2.b slice's ruling
    WITH the provision that the rebuild plumbing lands with Rel; the
    witness's REBUILD batch un-retires under the golden-bless ritual,
    and the edge-after-demand gap labels (CLAUDE.md + the Build.cpp
    comment) retire WITH the landing, not before. R-a2 proceeds under
    the full per-slice design ritual.

(G) R-a2 LANDED (2026-07-21) — THE FIRST REL-EPOCH SLICE: the
    SUBGRAPH_INSTANTIATE input-frontier drain; EDGE-AFTER-DEMAND CLOSED.
    Binding contracts COMMITTED: KeyedInstances.artifacts/ra2-design.md
    (ADJ-R1..R9) + ra2-desired-states.md (DS-ADJ-1..4). THE MECHANISM:
    band-(a2) drains the OD-4-provisioned edge kNetAdditions frontier —
    project the edge row's key cols, FindInstance (non-adding; a stray
    undemanded edge SKIPS silently, pinned by the new InstanceStore unit
    arm per ADJ-R4), !TouchedFlag dedup ([D-COLLAPSE]: one rescan per
    key per epoch, two drain sources), then the SAME full-rescan as a1
    via the shared emit_instance_rescan emitter ([9]-fold, generated .h
    proven BYTE-NEUTRAL). Mint: InstantiateEffects pushes the second
    kVecDrain(input, kNetAddition) ([D-DRAIN-SECOND] = push-order
    determinism per ADJ-R9); the edge frontier threads into the flow
    proc as a param in id-order (vec25 BEFORE vec29 — DS-ADJ-1, the
    set_intersection order). V-INST-EFFECT is SOURCE-AWARE (one demand
    drain + one input drain, roles checked — the Fable review's [4],
    discharging ADJ-R8's "no wiring guarantee" residual); V-INST-DRAIN
    checks input provisioning; a key-arity belt (always-on) aborts on
    any rescan-spine/store-key mismatch ([5], the zero-fill probe
    hazard, not constructible at tip). OBSERVABILITY: the .ir
    SUBGRAPHINSTANCE line now renders "input $net_additions:N" ([3],
    the review's headline — the ADJ-R5 hand-review surface can no
    longer pass on a mis-wired frontier). THE WITNESS is
    BIRTH-AND-REBUILD: STAGE-D red→green EXECUTED (frozen 4a5fbfc4
    binary SIGABRTs at the first rebuild probe printing only the 4
    birth lines = DS-ADJ-4; the landed binary prints the 8 predicted
    lines; flat==nested byte-equal); golden trio re-blessed via the
    ritual AFTER raw-output review (cross-mode single hash; oracle
    1640 assertions/6 batches; monotone 11 facts — all DS-ADJ-3
    byte-pins). RAT-6's witness enforcement LIFTED per the OD-12
    reversal path; gap labels retired same-diff (CLAUDE.md
    heading+anchor, Build.cpp fence-(ii) comment, §20(C) above).
    EVERY pre-registered [BYTE]/[STRUCT] prediction MATCHED, incl. the
    census (kSubgraphInstantiate=1, kInstanceDeath=0 — HP-17 carried,
    kCommitSweep=2) and deps=6 a2-invariance. FABLE REVIEW (14-agent
    workflow): 11 verified findings — 7 FIXED pre-commit ([3] printer,
    [4] source-aware validator, [5] belt, [0/1/6]+[2/7]+[10] doc
    retirements, [9] dedup), 1 ACCEPTED residual ([8] the contract-
    pinned duplicated V-INST-DRAIN check). GATES (final tree): SUITE
    PASS (173) eqgate-live; 676-row knob-off A/B 0-diverged vs frozen
    4a5fbfc4 (run TWICE: post-impl + post-review-fixes); data/ 36-file
    ×4 modes 0-diverged; ctest 5/5; ASAN BOTH surfaces ×2 sweeps zero
    reports; E-62 CLEAN; config-invariance 3-run + debug==release
    single hash 61ad8bdf; Q5 progsize@128 release SAME-SESSION
    INTERLEAVED ABABAB A warm {146,149,149} vs B {146,151,148,149}
    ms (−0.3% median, noise). RESIDUALS OPENED: [8] accepted above;
    the a2 WorkingOccupied belt is R-MONO-unreachable (review LOW-3 —
    a green witness is not evidence it has teeth; D3.a). NEXT: the
    R1..Rk step kinds per brief §5 (TUPLE/INSERT first; the JOIN slice
    carries the pre-registered pivot-equality-belt fold candidate,
    NOT RULED).

(H) R1 LANDED (2026-07-22) — THE FIRST STEP-KIND MIGRATION: the monotone
    eager web's TUPLE-forward + terminal-INSERT dispatch arms are MODELED
    DR-IR OPS. Binding contracts COMMITTED: r1-design.md (ADJ-S1..S14) +
    r1-desired-states.md (DS-ADJ-1..7). THE MECHANISM: kEagerForward(18)/
    kEagerInsert(19) — EFFECT-FREE, KNOB-INDEPENDENT walk-position
    markers minted at the dispatch site (Build.cpp) and lowered IN PLACE
    by thin LowerRelStep wrappers CALLING the untouched
    BuildEagerTupleRegion/BuildEagerInsertRegion (Tuple.cpp/Insert.cpp
    byte-unchanged; id-stream identity MECHANICAL). Enrollment appends
    to flow.ops STRICTLY AFTER the ingest folds (ADJ-S2 BINDING pin —
    folds keep op.0/op.1); lead-0 off-lattice key_of; effect-free ⇒
    ZERO dep edges ⇒ invisible to every hazard validator (the exclusion
    generalizing the kIngestFold precedent). Helpers .find()-guarded
    (ADJ-S13/S14 — operator[] on publish_vecs/view_to_model FORBIDDEN);
    message extracted ONCE at the mint site (review [3]). RENDER:
    dedicated Format cases (no reads/effects/spine sublines; table=
    only when non-null per ADJ-S3 — tid() has no null guard); census
    18→20 DAY ONE; the A.6(c) structural recount (kind↔view-kind +
    MERGED-MODEL table match; NO count oracle per ADJ-S12 — the
    ADJ-S10 bless-time count read compensates; the chain-breaker arm
    DELETED as tautologically dead per review [1], honest note in
    place). THE ORDER LAW (ADJ-S1, corrected pre-code): pinned_order =
    (op_table_id, sign, ctor) — table-less eager ops LEAD the dump;
    sign-0 eager ops precede each table's sign-+1 ingest fold (symrec's
    op.0 fold renders LAST — verified live). RENDER AUTHORITY (DS-ADJ-7,
    post-bless): eager table= = the union-find MERGED model, never the
    .df per-view attribute. GOLDENS: demand_tc_witness.deltarel
    re-blessed + symrec_tie_1.deltarel FIRST-EVER seeded (ADJ-S9 second
    carrier; RAT-8 lineage) via the FULL ritual — pre-bless divergence
    EXACTLY the two pre-registered (tc IRGOLD-DIVERGE + symrec
    IRGOLD-MISSING, nothing else); ADJ-S7 same-workroot referee RUN
    (purely additive eager blocks, folds byte-identical at op.0/op.1,
    census tail-append only); ADJ-S10 counts tc 12F/2I + symrec 7F/1I
    within floors; DS-ADJ-4 sign=· FIRST-PINNED c2 b7 by hexdump;
    git-verified ONLY the two sanctioned goldens changed. DS-ADJ-1:
    census mode-stability holds ONLY across the controlflow axis
    (df-axis growth EXPECTED — CSE-skip enlarges the walk). FABLE
    REVIEW (11 agents): 5 verified findings, ALL handled pre-commit
    ([0] untracked-golden staged; [1] dead arm deleted; [2] → DS-ADJ-7;
    [3] single extraction; [4] enum comment de-transient-ed); fixes
    proven DUMP-NEUTRAL. GATES (final tree): SUITE PASS (173) ×3 +
    post-fix; 676-row knob-off A/B 0-diverged vs frozen 6d695aec ×2;
    post-baseline-4 A/B (incl. nested ×4 modes) 20 rows 0-diverged;
    data/ 144 rows 0-diverged; ctest 5/5 (no fixture edits); ASAN both
    surfaces ×2 zero reports; E-62 CLEAN; config-invariance 3-run +
    debug==release single hash; Q5 progsize@128 release SAME-SESSION
    INTERLEAVED ABABAB A warm {149,149,147} vs B {149,152,148,147} ms
    (−0.3% median, noise — the ADJ-S8 MEASURED gate). RESIDUALS OPENED:
    publish-* sink spellings + stream message= arm corpus-UNWITNESSED
    (ADJ-S5 — a publishing-demanded-insert case is a future candidate);
    the ClassifyEagerSink replica ships with no cross-check vs the
    emission branch (ADJ-S4, retired at the R-final direction-flip);
    the eager census counts have NO independent oracle (ADJ-S12 —
    R-final's reachability flip owes one). NEXT: R2 per brief §5 order
    (CMP/MAP next, JOIN last with the NOT-RULED pivot-belt fold
    candidate).
(I) R2-OPEN RE-VERIFICATION RECORD (2026-07-22, tip a4b807dc; the
    session-open items (0)+(1) EXECUTED per §20(G)/(H)'s NEXT).
    (0) Frozen A/B baselines RE-SNAPSHOTTED from tip into the session
        scratchpad BEFORE any code (debug 971d88ad… / release
        f6ab49a1…; both presets rebuilt clean). ASAN cadence stands
        per-diff (§19(F)/(J)); the tip is docs-only atop 8fa156bc,
        whose ×2 both-surface sweeps are the standing green.
    (1) §20(G)/(H) + rel-arch-pseudocode.md §4/§5 FLEET-RE-VERIFIED
        (house precedent): 3 seed-UNREAD opus derivation lanes
        (Build.cpp walk-side mint/dispatch; DeltaRel.cpp ctors/
        enrollment/key_of/recount + Format render; the R-a2 lowering)
        + 3 seed-read adversarial verifiers (§4/§5 mold, §20(H),
        §20(G)) + 1 mechanical golden/census lane + 1 xhigh
        consolidator (8 agents, ~708k tokens; every candidate
        adjudicated AT the code). The E-62 tripwire re-grepped by the
        ORCHESTRATOR personally: CLEAN (zero out-of-lib body_ops/
        output_ops readers; Format.cpp dump reader + substrate-fill
        writers in-lib; standing Stratum.cpp:1073 comment + the RAT-3
        InstanceOrderTest fixture are the sanctioned pinned_order
        neighbors). The carrier-golden referee EXECUTED by the
        orchestrator personally: both .deltarel carriers regenerate
        BYTE-IDENTICAL at tip (mechanical lane extended to all six
        pinned surfaces: 6/6 byte-identical; counts tc 12F/2I +
        symrec 7F/1I; census 20 kinds; sign glyph c2 b7 re-confirmed).
        VERDICT: SOUND-WITH-ERRATA — zero code or design defects;
        four errata, ALL LOW/COSM, applied IN PLACE (each site tagged):
          E-98  COSM pseudocode §4: INSERT message extraction "ONCE"
                     anchor is :1233 (helper :1113); ":1112" was the
                     helper doc-comment line. Substance held.
          E-99  COSM pseudocode §4 M7: EagerSinkName is Format.cpp:126
                     (:125 is its doc-comment); loud-abort shape exact.
          E-100 COSM pseudocode §4 R-a2 block: the band-(a2) drain
                     CODEGEN (FindInstance/skip/dedup/rescan +
                     emit_instance_rescan) lives in Generator::
                     EmitSubgraphInstance (Database.cpp:2369-2403 /
                     :2308-2346); LowerSubgraphInstances
                     (Procedure.cpp:252-320) wires the memoized
                     input-frontier param + the key-arity belt
                     (:292-298). One cross-lane contradiction resolved
                     AT CODE in the doc's favor: input_front (vec25)
                     threads BEFORE the demand vec (vec29) — id-order,
                     as §20(G) states.
          E-101 LOW  pseudocode §5 R2 block (ORCHESTRATOR-caught, the
                     carrier-coverage audit the fleet lacked): NEITHER
                     .deltarel carrier contains a CMP or MAP view
                     (verified in both .df goldens), so R2's golden
                     churn on the existing carriers is CENSUS-LINE
                     ONLY (census renders zero-count kinds, 20->22);
                     "+ blocks" requires a THIRD carrier (map_3 /
                     fibonacci_iterative candidates, RAT-8 seeding)
                     or R2's blocks land corpus-unwitnessed (the
                     ADJ-S5 residual class). Materially re-scopes the
                     R2 bless plan; folded into the slice re-rank.
        VERIFIED-CLEAN (held): enum 18/19 + effect-free ctors
        (DeltaRel.cpp:1279/:1290); mint sites Build.cpp:1225/:1234 +
        LowerRelStep :1138/:1146; enrollment tail-append
        (DeltaRel.cpp:2389-2396, folds keep op.0/op.1); the A.6(c)
        recount (:3405-3440) + merged-model render authority
        (DS-ADJ-7); the order-law key (:4326-4390) incl. the golden-
        verified table-less-lead + sign-0-before-fold layout;
        ADJ-S13/S14 .find() discipline; §5's remaining arms confirmed
        hand-coded (Compare/Generate/Union/Negate/Join.cpp); E-42
        shim persists op-less; MERGE-inductive round shells Authority
        A; ra2 band mechanics + V-INST-EFFECT source-awareness +
        always-on key-arity belt + the .ir input-frontier render.
        Full record: session scratchpad fleet/consolidated-e98.md
        (disposable; THIS entry is binding). Errata continue at E-102.

(J) R2 LANDED (2026-07-22) — THE SECOND STEP-KIND MIGRATION: the monotone
    eager web's CMP-filter + MAP functor-call dispatch arms are MODELED
    DR-IR OPS. Binding contracts COMMITTED: r2-design.md (ADJ-R2-0..8,
    Fable-review record in banner) + r2-desired-states.md (DS-R2-1..9,
    stage-(d) critique record in banner). THE MECHANISM: kEagerCompare(20)/
    kEagerGenerate(21) — EFFECT-FREE, knob-independent walk-position
    markers on the R1 M1-M8 mold, minted at the Build.cpp dispatch site
    (the MAP mint in the IsPure() TRUE arm ONLY — ADJ-R2-3; impure maps
    reject upstream pre-walk) and lowered IN PLACE by LowerRelStep_Compare/
    _Generate wrappers calling the UNTOUCHED builders (Compare.cpp/
    Generate.cpp byte-unchanged; the CMP wrapper forwards neither pred_view
    nor last_table — the builder's own signature). NO NEW PAYLOAD FIELD
    (the headline): the CMP operator and MAP functor are PURE FUNCTIONS of
    the stored eager_view, re-derived at Format time (the agg_name
    precedent) — EmittedEagerOp unchanged, round-trip lossless by
    construction. EAGER_WEB re-invocation now a 4-way switch WITH a
    loud-abort default (ADJ-R2-8a); enrollment stays strictly after the
    ingest folds (ADJ-S2); key_of admits both kinds into the lead-0
    off-lattice band via the review-minted ONE IsEagerMarkerKind predicate
    (shared with the A.6(c) guard); A.6(c) recount extended 4-way
    (IsCompare/IsMap + the unchanged DS-ADJ-7 merged-model table match),
    restructured as a switch with an honest loud-abort default. RENDER
    (E-71 pre-code-adjudicated productions): kEagerCompare header token
    cmp=<eq|neq|lt|gt> via the new ComparisonOperatorName loud-abort table
    (the .df house spelling; totality = by-construction + abort tail,
    -Wswitch is warning-only); kEagerGenerate args token
    functor=<name>/<arity> (decl arity, the message= idiom; NO positivity
    token — an unwitnessed spelling declined). Census 20→22 DAY ONE.
    OWNER RULINGS (pre-code): kEagerGenerate over kEagerMap;
    functor=<name>/<arity> over the codegen <name>_<pattern> key;
    map_3.irgold pins deltarel-opt ONLY. THE THIRD CARRIER: map_3
    (owner-ruled; acyclic monotone, TABLE-LESS ingest kIngestFold=0,
    compare×1 + map×3 opt) — goldens/map_3.deltarel.opt.golden FIRST-EVER
    seeded (RAT-8) via the FULL ritual. STAGE-(d) HEADLINE: the blind
    inventory re-derivation + the orchestrator's personal all_cols_match
    verification UPGRADED the render-order prediction to [BYTE]-derived
    going-in (no union rule reaches map_3's maps/compare — subset-width
    tuples defeat the only inbound passthrough path) — and the landed dump
    MATCHED Candidate A line-for-line; the general "MAP/COMPARE can never
    carry table=" claim was REFUTED (a full-width passthrough TUPLE would
    union — corpus-UNWITNESSED, future-carrier note). EVERY pre-registered
    [BYTE]/[STRUCT] prediction MATCHED: tc/symrec census-line-ONLY diffs
    (exact predicted lines); map_3 opt = 10 ops (3G/1C/3F/3I), nodf/none
    18; pre-bless reds EXACTLY the 3 pre-registered (tc+symrec
    IRGOLD-DIVERGE, map_3 IRGOLD-MISSING, literal run_irgold format);
    git-status exactly 3 goldens + 1 sidecar. Referee steps EXECUTED
    PERSONALLY (E-77): ADJ-S7 same-workroot direct-diff (census-tail-only,
    bless-source byte-identical to the reviewed dump); ADJ-S10 count read
    (per-VISIT semantics noted); DS-ADJ-4 hexdump (sign= c2 b7, cmp=eq,
    functor=add_i32/3 first-pinned); DS-ADJ-1 manual knob compiles
    (opt==nocf 10, nodf==none 18 — the sidecar drives opt only). FABLE
    REVIEW (10 agents): 4 verified findings, ZERO correctness — [1]
    IsEagerMarkerKind unification FIXED, [2] A.6(c) switch + loud-abort
    default FIXED, [4] stale note FIXED, [3] the triple-copied eq/neq/lt/gt
    spelling ACCEPTED-DEFERRED (a .df-emitter-touching hygiene diff, not
    mid-slice); fixes proven DUMP-NEUTRAL + post-fix suite/A-B green.
    GATES (final tree): SUITE ×3 pre-bless (exact reds) + PASS (173) ×3
    post-bless + post-fix; 676-row knob-off A/B 0-diverged vs frozen
    a4b807dc ×3 (post-impl ×2 + post-fix); post-baseline-4 16 rows +
    nested witness ×4 modes clean; data/ 144 rows 0-diverged (NOTE
    surfaced: data/self_testing_examples/evm_array_parse.dr SIGABRTs
    IDENTICALLY at baseline and tip, all 4 modes — a PRE-EXISTING
    clean-diagnostic-then-abort, record-only, out of R2 scope); ctest 5/5
    debug + 5/5 ASAN; ASAN BOTH surfaces ×2 sweeps PASS zero reports;
    config-invariance 3-run + debug==release single hash ×3 carriers;
    E-62 re-grep CLEAN (no new reader; edited sites widened branches
    only); Q5 progsize@128 release SAME-SESSION INTERLEAVED ABABAB A warm
    {148,152,151} vs B {155,150,150,150} ms (−0.7% median, noise — ADJ-S8
    MEASURED, A1 cold discarded). RESIDUALS: neq/lt/gt + any positivity
    spelling corpus-UNWITNESSED (only eq pinned — ADJ-S5-analog); the
    full-width-passthrough MAP-with-table= shape corpus-UNWITNESSED; the
    [3] spelling unification deferred; publish-*/message= + the eager
    count oracle + ClassifyEagerSink replica carry unchanged (R-final).
    NEXT: R3..Rk per brief §5 as amended (MERGE-union/SELECT/NEGATE, then
    R-JOIN with the NOT-RULED pivot-belt fold, R-E42, R-final).

(K) R3-OPEN RE-VERIFICATION RECORD (2026-07-22, tip b8314dc0; the
    session-open items (0)+(1) EXECUTED per §20(J)'s NEXT).
    (0) Frozen A/B baselines RE-SNAPSHOTTED from tip into the session
        scratchpad BEFORE any code (debug c0a8a819… / release
        958ddf8b…; both presets rebuilt clean) — the a4b807dc-era
        snapshots retired as STALE post-R2. ASAN cadence stands
        per-diff (§19(F)/(J)); the tip is docs-only atop 056d2f96,
        whose ×2 both-surface sweeps are the standing green.
    (1) §20(I)/(J) + rel-arch-pseudocode.md §4/§4.1/§5 FLEET-RE-
        VERIFIED (house precedent): 3 seed-UNREAD opus derivation
        lanes (Build.cpp dispatch/mints+wrappers; DeltaRel.cpp ctors/
        IsEagerMarkerKind/EAGER_WEB/key_of/A.6(c); Format.cpp render +
        golden conformance) + 3 seed-read adversarial verifiers
        (§4+§4.1; §5+§20(J); §20(I)+R-a2) + 2 sonnet mechanical lanes
        (carrier counts/census/hexdump pins; the M9 corpus-wide
        reachability sweep) + 1 xhigh consolidator (9 agents, ~636k
        tokens; every candidate adjudicated AT the code). The E-62
        tripwire re-grepped by the ORCHESTRATOR personally: CLEAN
        (zero out-of-lib body_ops/output_ops readers; pinned_order
        neighbors = the standing Stratum.cpp:1073 comment + the RAT-3
        InstanceOrderTest fixture). The carrier-golden referee
        EXECUTED by the orchestrator personally: ALL EIGHT pinned
        golden surfaces regenerate BYTE-IDENTICAL at tip (tc
        h/ir/df/deltarel, symrec ir/df/deltarel, map_3 deltarel).
        VERDICT: SOUND-WITH-ERRATA — zero code or design defects;
        four errata, ALL LOW/COSM, ALL anchor-line drift confined to
        §4's R1-era body (cause: R2 inserted the IsMap/IsCompare
        dispatch arms and the two EAGER_WEB cases, shifting lines;
        §4.1's own anchors were stamped at the R2 tip and HELD),
        applied IN PLACE (each site tagged):
          E-102 LOW  §4 M4: EAGER_WEB enrollment tail-append is
                     DeltaRel.cpp:2424-2446; the cited :2389-2396 now
                     lands in the UNRELATED DRRound test_vec build
                     (misdirection, not a slip — the worst of the
                     four).
          E-103 COSM §4 R1 dispatch: mint sites :1254 (TUPLE) /
                     :1263 (INSERT); MessageOfInsertOrNull :1262
                     (+29 drift; E-98's own landed ":1233" was
                     re-staled by R2 — an erratum can rot).
          E-104 COSM §4 M6: A.6(c) recount head = :3454/:3461;
                     ":3405" sits INSIDE the count-expect() lambda.
          E-105 COSM §4 M7: EagerSinkName def = Format.cpp:128
                     (:125-127 its doc-comment; E-99's :126 re-
                     settled).
        §20(I) carries the same drift family but self-declares tip
        a4b807dc (pre-R2, correct as-written) — NOT renumbered; §4.1
        + this entry are the live anchors. THE M9 SWEEP (209 files /
        194 dumps, dataflow-layer proxy — CF InductionGroupId
        unprobed, caveat carried): plain-MERGE union — the fleet's
        "ZERO witnesses" statement was WRONG (E-106, ORCHESTRATOR-
        caught post-consolidation: the sweep's category (a) filtered
        class=monotone ONLY — an orchestrator prompt-spec defect the
        whole fleet inherited, the E-101 lesson repeating one layer
        up). CORRECT statement: zero TABLE-BACKED-monotone acyclic
        merges (74/74 class=monotone merges carry `; cycle`
        successors), but TABLE-LESS acyclic merges are PLENTIFUL —
        47 corpus hits (merge_1..6, compare_6 ×3, select_2/4,
        fibonacci_iterative, the transitive_closure family, …) +
        data/ twins — and a `.df class=table-less` merge is never
        deletion-capable, hence never cut, hence walk-reached
        (orchestrator-verified at merge_1's ^merge.18 + the dispatch
        arm). The R3 union arm needs NO new .dr — an .irgold sidecar
        on an existing green case (RAT-8 first-ever .deltarel
        seeding) suffices. E-107 (R3 stage-(d), the blind lane's
        ModelTableOrNull instrumentation probe, adjudicator-
        re-executed): E-106's RENDER framing was INVERTED — `.df
        class=table-less` is a DATAFLOW attribute; at the
        ControlFlow DataModel layer these merges are typically
        model-table-BACKED (equivalence-set sharing: merge_2's 5
        unions render table=%table:4 ×3 / %table:8 ×2, select_2's 2
        render %table:4), so InTryInsert FOLDS there and the
        WITNESSED kEagerUnion arm is the table-BACKED one; the
        table-LESS union render (bare args:) is the opt-UNWITNESSED
        residual (nodf/none only, unpinned). E-106's REACHABILITY
        half stands unchanged. SELECT rebind arm = WITNESSED by
        the unit-condition
        shape (booleans, booleans_diff, elim-cond-cycle-simple,
        prove_constant + data/ conditions_to_bools; raw select-node
        counts OVERSTATE — recv selects are walk roots); NEGATE =
        plentiful (17 monotone hits, negate_1..6 et al.) but @never
        THIN (sole witness negate_6, rendered as prose "; never
        negates", no @never token) + PIN-3 stands; compare spellings
        eq=61/lt=33/neq=24/gt=3 (gt scarcest: compare_6 table-less +
        reconverge_1 differential; only eq golden-pinned).
        FORWARD-LOOKING CODE NOTES (recorded, out of doc-set scope,
        R3-slice inputs): Build.cpp:1224 impure-MAP arm is a bare
        assert(false) (defense-in-depth behind the real :1373-1375
        upstream reject — NDEBUG-silent if that reject regressed);
        Format.cpp:927 per-op generic render default silently
        mis-renders an unhandled kind (unlike the EAGER_WEB/A.6(c)/
        DROpKindName/census abort discipline) — the next marker
        slice MUST add its dedicated render case per M7; three stale
        in-code comment line-refs (Format.cpp:864, Procedure.cpp:
        264-265, DeltaRel.cpp:2413-2415's carrier-specific "op.0/
        op.1" phrasing — the unconditional invariant is "tail-append
        shifts nothing"). VERIFIED-CLEAN (held): the four mint arms +
        effect-free ctors :1279/:1290/:1317/:1328; MAP pure-arm-only
        mint; CMP wrapper forwards neither pred_view nor last_table;
        M2' no-payload-field (EmittedEagerOp = {kind,view,table,sink,
        message}); M6' IsEagerMarkerKind :1306 with EXACTLY two
        callers (:3462 + :4424); EAGER_WEB 4-way + loud-abort;
        A.6(c) 4-way switch + honest abort + DS-ADJ-7 merged-model
        match; census kAllKinds[22] zero-count-rendering + totality
        abort; counts tc 12F/2I + symrec 7F/1I + map_3 3F/3I/1C/3G;
        DS-ADJ-1 cross-knob (tc 16/16/24/24, symrec 9/9/14/14, map_3
        10/10/18/18); sign=· c2 b7 + cmp=eq + functor=add_i32/3
        hexdump-confirmed; ADJ-S13/S14 .find() discipline; Union.cpp
        49 lines / Negate.cpp 105 lines / SELECT arm assert-IsInsert
        :1239; MERGE-inductive round shells Authority A (LowerDRRounds
        Stratum.cpp:1801); R2 commit = exactly 3 goldens + 1 sidecar,
        Compare.cpp/Generate.cpp byte-unchanged; triple-copy
        eq/neq/lt/gt at DeltaRel/Format.cpp:150 + DataFlow/Format.cpp
        :222/:1351; E-100's R-a2 attributions EXACT; E-42 shim
        persists op-less (Procedure.cpp:94-104). Full record: session
        scratchpad fleet-r3open/consolidated.md (disposable; THIS
        entry is binding; the M9 plain-merge paragraph above is
        CORRECTED in place per E-106 and E-107). Errata continue at
        E-108.

(L) R3 LANDED (2026-07-22) — THE THIRD STEP-KIND MIGRATION: the monotone
    eager web's MERGE-union + SELECT-rebind dispatch arms are MODELED
    DR-IR OPS. Binding contracts COMMITTED: r3-design.md (owner rulings
    ADJ-R3-1..10 + Fable-review record in banner) + r3-desired-states.md
    (DS-R3-1..9, stage-(d) blind-probe record in banner). THE MECHANISM:
    kEagerUnion(22)/kEagerSelect(23) — EFFECT-FREE, knob-independent
    walk-position markers on the M1-M9 mold, minted at the Build.cpp
    dispatch site (the union mint ONLY on the does-NOT-own-an-
    InductionGroupId leg — the owning-merge leg is Authority A round
    shells, mint-free; the ADJ-R2-3 one-leg precedent) and lowered IN
    PLACE by LowerRelStep_Union (calling the UNTOUCHED
    BuildEagerUnionRegion — Union.cpp byte-unchanged) and
    LowerRelStep_Select (calling BuildEagerSelectRegion, the owner-ruled
    EXTRACT-AND-WRAP: the inline rebind block moved VERBATIM out of the
    dispatch — a byte-move minting zero impl->next_id, id-stream
    identity mechanical). NO payload field on either kind (M2';
    EmittedEagerOp closed). A.6(c): the union arm is the mold's FIRST
    STRENGTHENED arm (owner-ruled: re-checks IsMerge AND
    !InductionGroupId — the view-kind alone is ambiguous between
    inductive and plain, a discrimination the Compare/Map precedent
    never had); the select arm strict. IsEagerMarkerKind 6-way (key_of +
    A.6(c) ride free); EAGER_WEB 6-way + loud-abort; census 22→24 DAY
    ONE (both ADJ-S11 count-in-comment bumps); dedicated render cases =
    the kEagerForward shape exactly (no extra token — owner-declined
    cond=/rel=). THE E-106→E-107 CARRIER STORY (two orchestrator/fleet
    corrections mid-ritual, both committed as errata BEFORE code): the
    M9 sweep's class=monotone filter missed the plentiful `.df
    class=table-less` acyclic merges (E-106 — reachability half STANDS);
    then the stage-(d) BLIND lane's ModelTableOrNull instrumentation
    probe INVERTED the render prediction (E-107): those merges are
    ControlFlow-DataModel table-BACKED, so the WITNESSED kEagerUnion arm
    renders table=%table:N and the table-LESS union render is the
    opt-UNWITNESSED residual. CARRIERS (owner-ruled): merge_2 (PRIMARY
    union — 5 markers, %table:4 ×3 + %table:8 ×2) + booleans (PRIMARY
    select — 1 marker, table=%table:4 via the SELECT<->pred-INSERT
    model union) + elim-cond-cycle-simple (the NEGATIVE guard: its
    induction-owned merge minted ZERO unions — the mint-guard witness;
    select @%table:5) — three .irgold sidecars pinning deltarel opt,
    three FIRST-EVER goldens seeded via RAT-8 from the reviewed
    suite-pre1 workroot. EVERY pre-registered [BYTE]/[STRUCT] prediction
    MATCHED: censuses (merge_2 20 ops 10F/5I/5U; booleans 11 ops
    6F/2I/2fold/1S; elim 9 ops 6F/2I/1S), the three existing carriers'
    census-tail-only churn (` kEagerUnion=0 kEagerSelect=0`, verified 6
    diff lines total), pre-bless reds EXACTLY the six pre-registered
    (tc/symrec/map_3 IRGOLD-DIVERGE + 3 IRGOLD-MISSING) ×3 runs,
    git-status exactly 3 modified + 3 new goldens + 3 sidecars.
    REFEREES EXECUTED PERSONALLY (E-77): ADJ-S7 same-workroot
    direct-diff (6/6 bless-sources SAME-AS-REVIEWED); ADJ-S10 count
    read (per-visit semantics, ADJ-R3-9); DS-ADJ-4 hexdump (sign=· c2
    b7; the args-table bytes first-pinned); DS-ADJ-1 manual knob
    compiles (opt==nocf / nodf==none on all three new carriers: 20/20/
    53/53, 11/11/18/18, 9/9/24/24 — df-axis growth expected). FABLE
    REVIEW (10-agent workflow): 3 verified findings, ZERO live
    correctness — [1] the Induction.cpp:996 SECOND BuildEagerUnionRegion
    caller (DEAD at tip behind the NeedsInductionCycleVector TODO
    short-circuit) is a LABELED coverage hole outside the marker model
    (the strengthened A.6(c) arm forbids minting there): loud comment
    landed at the call site, recorded residual, re-visit at R-final;
    [2]/[3] four→six comment drift FIXED; [R1] shared render-case
    labels REFUTED (dedicated blocks contract-specified). Fixes
    comment-only, DUMP-NEUTRAL ×6 carriers + post-fix suite/A-B green.
    GATES (final tree): SUITE ×3 pre-bless (exact six reds) + PASS
    (173) ×3 post-bless + post-fix; 676-row knob-off A/B 0-diverged vs
    frozen c0a8a819/958ddf8b ×3 (post-impl ×2 + post-fix); post-
    baseline-4 20 rows (incl. nested witness ×4) + data/ 144 rows
    0-diverged ×3 (evm_array_parse identical-SIGABRT baseline stands);
    ctest 5/5 debug + 5/5 ASAN; ASAN BOTH surfaces PASS (173) zero
    reports; config-invariance 3-run + debug==release SINGLE-HASH ×6
    carriers; E-62 re-grep CLEAN (edited sites widened branches only);
    Q5 progsize@128 release SAME-SESSION INTERLEAVED ABABAB A warm
    {139,141,139} vs B {139,139,139,137} ms (0.0% median — ADJ-S8
    MEASURED, A1 cold discarded; generated headers byte-identical).
    RESIDUALS: the table-LESS kEagerUnion render (bare args:) is
    opt-UNWITNESSED (nodf/none only, unpinned — ADJ-S5-analog); the
    Induction.cpp dead-branch coverage hole (review [1], labeled,
    R-final); neq/lt/gt + positivity spellings, publish-*/message=,
    the eager count oracle + ClassifyEagerSink replica carry unchanged.
    NEXT: R4 NEGATE per §5 (PIN-3 class= refinement is the standing
    bless blocker — rule at the R4 ritual head [DISCHARGED as the
    §20(N) standalone pre-diff]), then R-JOIN (the
    NOT-RULED pivot-belt fold), R-E42, R-final.

(M) R4-OPEN RE-VERIFICATION RECORD (2026-07-23, tip 1492adbf; the
    session-open items (0)+(1) EXECUTED per §20(L)'s NEXT).
    (0) Frozen A/B baselines RE-SNAPSHOTTED from tip into the session
        scratchpad BEFORE any code (debug f69574b8… / release
        56da82ab…; both presets rebuilt clean) — the c0a8a819/
        958ddf8b-era snapshots retired as STALE post-R3. ASAN cadence
        stands per-diff (§19(F)/(J)); the tip is docs-only atop
        7982db9a, whose both-surface sweeps are the standing green.
    (1) §20(K)/(L) + rel-arch-pseudocode.md §4/§4.1/§4.2/§5 FLEET-RE-
        VERIFIED (house precedent): 3 seed-UNREAD opus derivation
        lanes (Build.cpp dispatch/mints/wrappers incl. the extracted
        BuildEagerSelectRegion + all-caller sweeps; DeltaRel.cpp
        ctors/IsEagerMarkerKind/EAGER_WEB/key_of/A.6(c); Format.cpp
        render + six-golden conformance) + 3 seed-read adversarial
        verifiers (§4+§4.1; §4.2+§5; §20(K)+(L)) + 1 sonnet
        mechanical lane (11-surface regen, counts, census 24,
        hexdump pins, cross-knob compiles) + 1 WORKTREE-ISOLATED
        opus R4-input lane (the M12-sanctioned fprintf walk/model
        probe on its own build — the pristine tree never touched) +
        1 xhigh consolidator (9 agents, ~818k tokens; every
        candidate adjudicated AT the code). The E-62 tripwire
        re-grepped by the ORCHESTRATOR personally: CLEAN (zero
        out-of-lib body_ops/output_ops readers; pinned_order
        neighbors = the standing Stratum.cpp:1073 comment + the
        RAT-3 InstanceOrderTest fixture). The carrier-golden referee
        EXECUTED by the orchestrator personally: ALL ELEVEN pinned
        golden surfaces regenerate BYTE-IDENTICAL at tip (tc
        h/ir/df/deltarel, symrec ir/df/deltarel, map_3/merge_2/
        booleans/elim-cond-cycle-simple deltarel — the sidecars are
        the surface-count authority; "nine" in the session brief was
        a miscount). VERDICT: SOUND-WITH-ERRATA — zero code or
        design defects; the M1-M13 mold holds exactly; four errata,
        applied IN PLACE (each site tagged):
          E-108 MED  §5 R4 block STARTING-STATE CAVEAT (the fleet's
                     headline, orchestrator-re-verified at code):
                     NEGATE is NOT un-modeled — an EAGER kNegateGate
                     op ALREADY exists, minted per-negate from
                     query.Negations() (DeltaRel.cpp:2299-2321,
                     ctx=kEager) with a REAL kFlagRead effect (the
                     R1-R3 markers are effect-FREE), inventory-only
                     (no LowerNegateGate; BuildEagerNegateRegion is
                     the sole CHECKMEMBER emitter), OVER-ENUMERATING
                     the walk on 9 corpus cases (the walk-cut
                     differential negates; the mint comment's
                     "every negate is eager-walk-reached" premise is
                     false there — its "Build.cpp:1048-1051" anchor
                     is also a stale code comment, an R4-implementer
                     nit). R4 is therefore a DIFF/RE-SOURCE of an
                     existing op (option A: move the mint to the
                     walk dispatch) or a FOLD/KEEP + separate marker
                     (option B) — ruled at the R4 ritual head; the
                     effect asymmetry must be accounted for either
                     way; V-NEG-CTX (DeltaRel.cpp:3057) must survive;
                     DeltaRel.cpp:417-422 pre-anticipates the
                     re-source ("R1d+").
          E-109 LOW  "negate_6 is the sole @never witness" sharpened
                     to sole @never-NEGATE witness (map_5.dr:17's
                     `@never is_even` is @never-on-a-#functor -> a
                     negated MAP, kNegateGate=0, not a QueryNegate;
                     two other grep hits are comment text). Both §5
                     sites.
          E-110 COSM §4.2 M13 "Induction.cpp:997-1005" ->
                     ":996-1005" (the loud label opens at :996,
                     matching §5's anchor).
          E-111 LOW  the §4/§4.1 R1/R2-body anchor re-base at tip
                     1492adbf (banner-pre-declared drift from R3's
                     insertions): mint sites :1294/:1303, INSERT
                     msg :1302; M4 enroll :2437-2478; M6 recount
                     :3486-3560 (guard :3494, switch :3505,
                     table-match :3556); M7 EagerSinkName
                     Format.cpp:130; §4.1 CMP :1276-1281, MAP
                     :1260-1274, ctors :1319/:1330, render
                     :903/:915, ComparisonOperatorName :150,
                     A.6(c) guard :3494, key_of :4475; EAGER_WEB
                     re-counted 6-way at tip (was 4-way at the R2
                     tip). Definition anchors :1138/:1146/:1157/
                     :1166/:1113/:1123/:1306/:1279/:1290 HELD
                     exactly (orchestrator spot-verified).
        ADJUDICATED NON-DEFECT (recorded, no erratum): the
        cross-knob nodf/none op totals for tc/symrec/map_3 are
        26/15/21 at tip — §20(K)'s 24/14/18 self-declares the
        pre-R3 tip and stands as-written; the surplus is EXACTLY
        the R3 kEagerUnion markers on un-CSE'd merges (dataflow
        axis only; opt==nocf and nodf==none held; no golden pins
        those modes — DS-ADJ-1-consistent).
        R4-INPUT (adjudicated at the correct layers per M12,
        consolidator-spot-checked, recorded in the §5 R4 block):
        walk-cut discriminator = CanReceiveDeletions
        (Build.cpp:970); ~18 dispatch fires opt / 22 none; EVERY
        reached negate has group_id=none (InductionGroupId is the
        WRONG discriminator — F22); M10 strengthened arm =
        !v.CanReceiveDeletions() (non-dead via d5_recursive_negate);
        M13 DISCHARGED (sole live caller Build.cpp:1312); PIN-3
        MANIFESTED at the model layer (negate_1 ^negate.5 .df
        class=monotone at that tip while model %table:4 DIFFERENTIAL,
        CanProduceDeletions=1 — cured by the §20(N) pre-diff);
        negate_6's @never leg MODE-SPLITS
        (%table:7 opt / null none — the E-107 shape); recommended
        golden trio negate_6 + negate_1 + d5_recursive_negate (the
        zero-mint NEGATIVE guard). VERIFIED-CLEAN (held): the full
        §20(K)-style list in the consolidation record — six kinds
        18-23 effect-free, single-authority ctors, tail-append
        enrollment shifting nothing, M2' no-payload, M10/M11/M12
        exact, census 24 totality-guarded, six render cases, 11/11
        goldens, E-106/E-107 internally consistent, R2/R3 residuals
        stand as declared. Full record: session scratchpad
        fleet-r4open/consolidated.md (disposable; THIS entry is
        binding). Errata continue at E-112.

(N) THE PIN-3 PRE-DIFF LANDED (2026-07-23) — class= IS A TABLE
    PROPERTY; the T2b-era PIN-3 (this file:431-437) DISCHARGED as the
    OD-12-style ruled pre-diff before R4. Binding contracts COMMITTED:
    pin3-design.md (stage-(a)-(c) ritual + owner rulings Q1-Q4 +
    Fable-review record in banner) + pin3-desired-states.md
    (DS-PIN3-1..12; the stage-(d) three-way blind-convergence record
    in banner). OWNER RULINGS: Q1 = candidate (ii) TABLE-LEVEL
    (class= renders the TABLE's deletion-capability: differential iff
    SOME live member view of the %table:<id> has CanReceiveDeletions
    || CanProduceDeletions — the (i)≡(ii) zero-bystander equivalence
    held corpus-wide, ruled on the category-error/peer-of-table=/
    R4-invariant-coupling ground); Q2 = TWO standing fences blessed
    in-slice (negate_1.df.opt + aggregate_1.df.opt — the FIRST
    producer-carrying .df goldens; the A/B gate is .df-blind, so
    without them no suite gate would catch a class= regression);
    Q3 = standalone pre-diff (the churn premise REFUTED: the two
    pre-existing .df pins are producer-free and proven byte-stable);
    Q4 = loud fprintf+abort checked lookup (DF-CLASS, the
    DF-REF/DF-JOIN idiom) + explicit crd||cpd (crd⟹cpd,
    Differential.cpp:114-117). THE MECHANISM: a deterministic
    pre-pass in the .df emitter (lib/DataFlow/Format.cpp, PASS 2
    head) OR-folds each %table:N's member deletion-capability over
    the same dead-skipping for_each_df_view the emission drives
    (domain-match ⇒ the abort never false-fires; integer-keyed map,
    never iterated — order-free by construction); attrs_line renders
    the table's class. Dump-only: NO emission-path change. STAGE-(d)
    THREE-WAY CONVERGENCE (the blind-lane precedent extended): the
    author lane's hand-predicted golden bytes == the BLIND worktree
    prototype's empirical dumps == the comparator's mechanical
    tip-flip, for BOTH new fences; the 46-flip inventory (21 opt /
    25 none over 18 cases: 17 negates + 2 agg + 2 kv at opt)
    tuple-for-tuple identical across lanes and MATCHED EXACTLY at
    implementation. negate_1's ^negate.5 @%table:4 now renders
    differential, consistent with its table-sharing tuple/insert —
    the PIN-3 lie is gone; R4's negate-carrying blesses UNBLOCKED.
    F24 RECORDED (FINDINGS.md round 10): the intended-flip referee's
    ONE non-flip diff — conflicting_constants nodf/none .df
    run-to-run instability (AutoVar column order; 3 hashes / 8 runs)
    — reproduces IDENTICALLY on the frozen 1492adbf baseline
    (pre-existing, the F20-family .df-surface survivor; record-only,
    out of slice scope). FABLE REVIEW (8-agent workflow): 2 verified
    findings, ZERO live correctness — [1] the §20(N) cross-refs were
    dangling pre-commit (CONFIRMED; discharged by THIS entry landing
    in the same commit); [2] the pre-pass try_emplace four-liner
    simplified to operator[] |= (the M8 checked-lookup discipline is
    the EMISSION-side rule; contract pins the predicate spelling,
    not map mechanics) — fix proven DUMP-NEUTRAL on all 10 pinned
    surfaces + post-fix suite/A-B green. GATES (final tree): suite
    pre-bless EXACTLY the 2 pre-registered IRGOLD-MISSING reds →
    RAT-8 bless (sources byte-verified same-as-reviewed; the bless
    arm's oracle/monotone/stdout rewrites were byte-identical no-ops)
    → SUITE PASS (173) ×3 + post-fix; intended-flip referee
    (orchestrator-EXECUTED, E-77) 21 opt / 25 none EXACT with
    pure-flip shape across 674 comparisons; A/B vs frozen f69574b8/
    56da82ab: 692 corpus rows + 4 nested rows + 144 data/ rows
    0-DIVERGED (+346-row post-fix re-run clean); ctest 5/5 debug +
    5/5 ASAN; ASAN BOTH surfaces SUITE PASS (173) zero reports;
    config-invariance single-hash (3-run debug + release) on both
    fence carriers × opt+none; E-62 re-grep clean (no DeltaRel
    touch); Q5 progsize@128 release SAME-SESSION INTERLEAVED ABABAB
    A warm {128,128,130} vs B {129,126,129,128} ms (0.0% median,
    A1 cold discarded; generated headers byte-identical). DOC
    DISCHARGES same-commit: this file :431-437 + §20(L)/(M) live
    status lines; rel-arch §5 R4 block (blocker → discharged);
    t2-dump-spec.md CLASS SEMANTICS clause (table-level,
    producer-inclusive, normative); Format.cpp comment. NEXT: the
    OD-13 model-set observability diff (owner-directed 2026-07-23:
    DF dumps — at least DOT — document each view's DataModel
    union-find set; DOT floor zero-golden; textual model= token =
    its own E-71 ruling + mini re-bless), THEN R4 NEGATE under
    option (A) diff/re-source per §20(M)/E-108 with the ruled
    carrier trio negate_6 + negate_1 + d5_recursive_negate.

(O) OD-13-OPEN RE-VERIFICATION RECORD (2026-07-23, tip 527d9dd4; the
    session-open items (0)+(1) EXECUTED per §20(N)'s NEXT).
    (0) Frozen A/B baselines RE-SNAPSHOTTED from tip into the session
        scratchpad BEFORE any code (debug 0e017e89… / release
        5090a5d2…; both presets rebuilt clean) — the f69574b8/
        56da82ab-era snapshots retired as STALE post-PIN-3. ASAN
        cadence stands per-diff (§19(F)/(J)); the tip is docs-only
        atop 57aef93d, whose both-surface sweeps are the standing
        green.
    (1) §20(M)/(N) + rel-arch-pseudocode.md §4/§4.1/§4.2/§5/§6
        FLEET-RE-VERIFIED (house precedent): 3 seed-UNREAD opus
        derivation lanes (the six-marker machinery + the PIN-3
        attrs_line/pre-pass; kNegateGate/V-NEG-CTX/Negate.cpp
        end-to-end incl. live census-vs-walk counts; the partition/
        dump surfaces incl. BuildEquivalenceSets determinism
        EMPIRICS) + 3 seed-read adversarial verifiers (§4/§4.1/§4.2;
        §5+§20(M); §6+§20(N)) + 1 sonnet mechanical lane (13-surface
        regen, census 24, hexdump pins, cross-knob compiles) + 1
        xhigh consolidator (8 agents, ~695k tokens; every candidate
        adjudicated AT the code). The E-62 tripwire re-grepped by
        the ORCHESTRATOR personally: CLEAN (sole out-of-lib hits =
        the standing Stratum.cpp:1073 comment + the RAT-3
        InstanceOrderTest fixture). The carrier-golden referee
        EXECUTED by the orchestrator personally: ALL THIRTEEN pinned
        golden surfaces regenerate BYTE-IDENTICAL at tip (tc
        h/ir/df/deltarel, symrec ir/df/deltarel, map_3/merge_2/
        booleans/elim-cond-cycle-simple deltarel, negate_1.df,
        aggregate_1.df — the irgold sidecars are the count
        authority). VERDICT: SOUND-WITH-ERRATA — zero code or
        design defects; the M1-M13 mold, the E-108 caveat, the
        PIN-3 pre-diff, and the §6 one-union-site partition hold
        EXACTLY; five errata, ALL COSM/LOW doc drift, applied IN
        PLACE (each site tagged):
          E-112 COSM §4 M6: ":3405" is the GROUP_UPDATE key setup
                     (add_gu_key :3407-3416), NOT inside the count
                     lambdas — count_kind :3425 / expect :3434.
          E-113 COSM §4 R1: the ADJ-S13 binding text is
                     Build.cpp:1088; :1092 is the Fable-review R1
                     [3] single-extraction note (both inside the
                     ClassifyEagerSink doc-comment :1085-1094).
          E-114 LOW  §4.2 M12: InTryInsert resolves its table
                     INLINE via view_to_model[]->FindAs
                     (Build.cpp:808-809), not the ModelTableOrNull
                     helper — same equivalence-set table; a
                     pre-existing builder outside the M8 mint-path
                     discipline, not a violation. M12's thesis
                     unchanged.
          E-115 COSM §6: dump placement = Main.cpp:118-132 (the
                     abstraction-break comment :118-120, gDOTStream
                     :121-124, gDFStream :126-132); ":110" was an
                     unrelated cpp-out line.
          E-116 LOW  §6 D1: EQ SET coverage is COMPLETE over view
                     nodes (Format.cpp:65 UNCONDITIONAL; negate_1
                     7/7) vs TABLE-id on the stamped subset (5);
                     the "10 TABLE-prefixed cells" counted HTML
                     <TABLE> label-markup tags, not do_table cells.
        OD-13 STAGE-(a) INPUTS RESOLVED BY THE FLEET (L3 empirics,
        consolidator-adjudicated, folded into §6 D1): (1) EQ SET
        VALUES ARE DETERMINISTIC — EquivalenceSetId = Find()->id,
        min-id-wins over COUNTER-MINTED ids in ForEachView order
        (EquivalenceSet.h; NOT pointer-ordered); byte-identical
        across 3 runs on negate_1 + merge_2; raw ids are (F)-safe,
        D3's dense renumber is leak hygiene not necessity. (2) the
        DOT WHOLE-FILE dump is NOT byte-reproducible — node names
        v<N>/t<N> embed raw pointers via UniqueId (3 runs -> 3
        hashes, semantics identical after node-id strip) — the DOT
        floor stays ZERO-GOLDEN unless node ids canonicalize first.
        (3) EQ SET already renders on EVERY view node — D1
        "possibly nothing to mint" empirically supported. (4) the
        textual .df renders NO partition token (live grep) — D2 is
        the real work, churning all FOUR pinned .df goldens.
        R4-INPUT NITS (code-side, recorded for the option-A
        re-source): stale mint-block comments DeltaRel.cpp:2331/
        :2335 (beyond E-108's :2294; live anchors = cut
        Build.cpp:970, boundary append :999-1006); the kNegateGate
        EFFECT ASYMMETRY is the R4 crux — off-lattice for stratum
        ordering (key_of lead-0, V-READY skip :4991-5000) YET
        dep-edge-bearing via kFlagRead (FlagAccess :4295-4296 ->
        RAW/WAR; negate_1 op.7->op.6/op.9 WAR) — any re-source must
        preserve the gate's read effect while an effect-free marker
        (if any) stays effect-free; d5_recursive_negate.dr's header
        comment misattributes the cut to CanProduceDeletions
        (actual: CanReceiveDeletions; both hold there — fix when d5
        becomes the R4 zero-mint NEGATIVE guard). VERIFIED-CLEAN
        (held): the full §20(M)-style list in the consolidation
        record — six kinds effect-free w/ single-authority ctors,
        tail-append enrollment, IsEagerMarkerKind :1306 exactly-two-
        callers, A.6(c) 6-way + strengthened union arm, census 24
        totality, 13/13 goldens, counts tc 12F/2I + symrec 7F/1I +
        map_3 3G/1C/3F/3I + merge_2 10F/5I/5U + booleans 6F/2I/1S +
        elim 6F/2I/1S, cross-knob opt==nocf / nodf==none ×6, sign=·
        c2 b7 + cmp=eq + functor=add_i32/3, kNegateGate inventory-
        only ctx=kEager + V-NEG-CTX :3057 + 9-case over-enumeration
        (merge_5=3 / cond_both_polarities=2 / d5=1 live), PIN-3
        pre-pass + DF-CLASS abort + crd⟹cpd Differential.cpp:
        114-117, one-union-site Build.cpp:242 + Data.cpp:252 stamp,
        both .df fences byte-identical. Full record: session
        scratchpad fleet-od13open/consolidated.md (disposable; THIS
        entry is binding). Errata continue at E-117. NEXT: the
        OD-13 slice under the full ritual (§6 D1-D4 as amended;
        the E-71 grammar adjudication + owner spelling ruling at
        the head), then R4 NEGATE option (A). [EXECUTED — §20(P).]

(P) OD-13 LANDED (2026-07-23) — THE .df TABLE-SHARING PARTITION
    TOKEN; the §6 textual observability gap CLOSED (the owner-
    directed model-set observability diff, §20(N) NEXT item one).
    Binding contracts COMMITTED: od13-design.md (stage-(b)/(c)
    adjudication verbatim + owner rulings Q-A/Q-B/Q-C in banner) +
    od13-desired-states.md (DS-OD13-1..10 + the stage-(d) three-way
    record + the Fable-review record). OWNER RULINGS: Q-A =
    `eqset=` (over model= — names the computed quantity, matches
    the DOT "EQ SET" cell: one concept, one name, one value on
    both DataFlow surfaces); Q-B = RAW EquivalenceSetId (over
    dense — literal DOT cross-surface identity, zero machinery;
    a dense renumber, if ever, lands on BOTH surfaces together);
    Q-C (POST-REVIEW, superseding the stage-(b) INSERT-omit
    adjudication) = eqset= RENDERS ON INSERT (table= stays
    omitted — the Fable review REFUTED the "recoverable from any
    table-sibling" ground on demand_tc_witness: insert.19 is
    %table:4's SOLE table-stamped rendered block, its set
    rendering elsewhere only as table-less eqset=10 on
    tuple.9/join.16). THE MECHANISM (dump-only, ZERO emission
    change): attrs_line (lib/DataFlow/Format.cpp) renders
    ` eqset=<raw EquivalenceSetId>` unconditionally on EVERY
    block, positioned after table=/before class= (the DOT
    TABLE-then-EQ-SET pair-order precedent); bare integer, no
    sigil; the DF-EQSET ~0u belt (unreachable — BuildEquivalence-
    Sets runs unconditionally at DataFlow Build.cpp:2640); the
    PIN-3 pre-pass untouched. DOT FLOOR: NOTHING minted (D1
    proven — Format.cpp:65 renders EQ SET unconditionally on
    every view node, values deterministic (min-id over
    ForEachView-order counter mints, (F)-safe raw), whole-file
    DOT stays zero-golden on pointer node names). t2-dump-spec.md
    gains the normative PARTITION TOKEN SEMANTICS clause
    (cross-surface identity + pair-order precision per review
    [4]). STAGE-(d) THREE-WAY CONVERGENCE (PIN-3 precedent
    extended): author hand-simulation (DOT-blind) == blind
    worktree prototype == DOT-derived mechanical comparator,
    BYTE-IDENTICAL ×4 carriers; the pristine implementation
    reproduced the converged bytes exactly (four-way); the Q-C
    delta = exactly the 4 INSERT lines (values insert.19=10 /
    insert.11=11 / insert.6=4 / insert.5=4, each verified by the
    table-consistency law + the DOT cell). CHURN as landed: 45
    ATTRIBUTES lines in place (tc 20 / symrec 12 / negate_1 7 /
    aggregate_1 6), zero non-ATTRIBUTES changes, line counts
    unchanged — the four .df pins re-blessed via the ritual TWICE
    (pre-Q-C 41 + Q-C 4; both cycles: pre-bless reds EXACTLY the
    four .df IRGOLD-DIVERGE, bless sources byte-verified
    same-as-reviewed). FABLE REVIEW (11-agent workflow, high): 5
    verified findings, ZERO live code-correctness — [1] the
    INSERT-omit premise refutation ESCALATED -> owner ruling Q-C
    (fixed by the second bless cycle); [2] §20(P)/record
    cross-refs discharged by THIS entry + the records landing
    same-commit (the PIN-3 [1] family); [3] the Fable record
    appended to od13-desired-states.md; [4] the "same order"
    claim tightened to PAIR order; [5] the kDFInsert call-site
    comment refreshed. GATES (final tree): SUITE pre-bless
    EXACTLY 4 reds ×2 cycles -> PASS (173) ×3 + ×2; A/B vs frozen
    0e017e89/5090a5d2: 840 rows (692 corpus ×4 modes + 4 nested +
    144 data/) 0-DIVERGED + 346-row post-Q-C subset clean
    (evm_array_parse identical-SIGABRT baseline stands); ctest
    5/5 debug ×2 + 5/5 ASAN ×2; ASAN BOTH surfaces SUITE PASS
    (173) ×2 cycles, zero sanitizer reports; config-invariance
    SINGLE-HASH ×8 (3-run debug + release, opt+none, 4 carriers)
    ×2 runs; E-62 re-grep CLEAN (no DeltaRel touch; same 5
    sanctioned hits); Q5 progsize@128 release SAME-SESSION
    INTERLEAVED ABABAB A warm {110,110,110} vs B {110,110,110}
    ms (0.0% median, colds discarded; generated headers
    byte-identical). DOC DISCHARGES same-commit: rel-arch §5
    OD-13 block DONE + §6 gap-closed paragraph + banner; the M12
    worktree-probe ritual RETIRES for the partition question
    (R4's fleet reads eqset= from the dump). NEXT: R4 NEGATE
    under option (A) diff/re-source per §20(M)/E-108 with the
    ruled carrier trio negate_6 + negate_1 + d5_recursive_negate
    (the §20(O) R4-input nits stand: stale mint comments
    DeltaRel.cpp:2331/:2335, the kFlagRead effect asymmetry as
    the crux, the d5 fixture header misattribution). [EXECUTED —
    §20(Q).]

(Q) R4 LANDED (2026-07-23) — THE FOURTH STEP-KIND MIGRATION: the
    NEGATE gate, executed as the owner-ruled OPTION (A)
    DIFF/RE-SOURCE (E-108/§20(M)/(O)). Binding contracts COMMITTED:
    r4-design.md (stage-(a)-(c) adjudication verbatim + owner
    rulings + Fable-review record + lane-file scoping note in
    banner) + r4-desired-states.md (DS-R4-1..10 + the stage-(d)
    three-way record). THE MECHANISM: the kNegateGate mint MOVED
    from the query.Negations() inventory loop (DELETED, with its
    stale E-108 comment block) to the Build.cpp IsNegate walk
    dispatch — MakeEagerNegateOp single-authority ctor +
    LowerRelStep_Negate thin wrapper calling the UNTOUCHED
    BuildEagerNegateRegion; EAGER_WEB re-invocation case;
    RecordEagerDispatch sources rec.view/table from the gate_*
    fields. THE FORCED MOLD DELTA (the §20(O) crux, ruled):
    kNegateGate is EFFECT-BEARING — the ctor reconstructs its
    kFlagRead{negated_table, NegateGatePred(kEager,hint)}
    identically at walk mint and re-invocation (M2' extended to an
    effect); the op populates ONLY gate_* (never eager_view/
    table_op_table) so it stays OUT of IsEagerMarkerKind —
    REQUIRED, the marker recount would false-fire on null fields;
    op_table_id=0 ⇒ table-less lead-0, the gate LEADS the dump;
    dep edges are EFFECT-determined so the WAR set is byte-stable
    under the ruled M4 tail-append (enrollment position need not
    be preserved — no golden pinned any negate case). M10 per-op
    recount at the V-NEG-CTX site: abort iff ctx==kEager &&
    gate_negate->CanReceiveDeletions() (the EXACT Build.cpp:970
    cut criterion, NOT InductionGroupId — F22; the ctx guard
    scopes it off the reserved standalone seed/fixpoint forms —
    Fable [1]). OWNER RULINGS: @never renders IMPLICIT via reads:
    Present vs InI (a POSITIVE E-71 ruling — no token, no
    dedicated render case, the generic fallback stands; the R2
    "declined unwitnessed" basis was VOID once negate_6 entered
    the carrier set); the differential-input @never shape stays a
    LABELED LATENT GAP (DS-R4-10, no directed fence). THE 9-CASE
    OVER-ENUMERATION DROPPED (orchestrator census referee, E-77,
    full corpus ×4 modes vs frozen b1c95bac: EXACTLY the 9
    predicted cases changed — 8 → 0 all modes, disassemble 2→1
    the SOLE mixed case, merge_5 df-axis 3opt/4nodf → 0; corpus
    opt total 30→18; the referee also caught the orchestrator's
    own transient non-ASCII § in the d5 comment edit — the lexer
    fence works). THE NORMATIVE COUNT ORACLE (DS-R4-5, the merged
    C1-F1+C2-F3 correction): POST census == BuildEagerNegateRegion
    DISPATCH count, per-VISIT (kEagerForward=12 vs 11 TUPLE views
    proves per-visit≠per-view) and EMISSION-SIDE — verified by the
    blind lane's lldb hit-counts 11/11. STAGE-(d) THREE-WAY
    CONVERGENCE: author hand-prediction (labels/edges/census
    re-derived under the order law; negate_6 two-gate order
    derived from walk-DFS successor structure — NO SWAP) == blind
    worktree prototype == pristine implementation, BYTE-IDENTICAL
    ×3 carriers; generated C++ byte-identical to the pristine tip
    on all 11 probe cases (ZERO emission change). CARRIERS:
    negate_1 (1→1, the PIN-3 differential-table carrier) +
    negate_6 (2→2, the @never carrier) + d5_recursive_negate
    (1→0, the ZERO-MINT NEGATIVE guard) — three FIRST-EVER negate
    .deltarel.opt goldens (RAT-8) + sidecars; the d5 fixture
    header misattribution FIXED (CanReceiveDeletions, the §20(O)
    nit). FABLE REVIEW (12-agent workflow, high): 4 verified
    findings, ZERO live-today correctness — [1] the recount ctx
    guard (latent false-abort on the reserved seed/fixpoint gate
    forms) FIXED; [2] EAGER_WEB effect-free/six-kinds + Build.h
    comments refreshed for the effect-bearing seventh kind; [3]
    CLAUDE.md re-pointed (NINE .deltarel goldens; unmodeled arms
    JOIN + E-42); [4] the r4-design.md lane-file scoping note;
    1 REFUTED (the view_to_model[] mint idiom = the live :735
    precedent). Fixes proven DUMP-NEUTRAL on all 16 pinned
    surfaces + post-fix suite/A-B green. GATES (final tree):
    SUITE pre-bless EXACTLY the 3 pre-registered IRGOLD-MISSING
    reds → RAT-8 bless (sources byte-verified same-as-reviewed;
    the other 6 rewrites byte-identical no-ops) → PASS (173) ×3 +
    post-fix; A/B vs frozen b1c95bac (debug 49f23d78 / release
    eed45d2e): 692 corpus + 4 nested + 144 data/ rows 0-DIVERGED
    + post-fix subset clean (evm_array_parse identical-SIGABRT
    stands); ctest 5/5 debug + 5/5 ASAN; ASAN BOTH surfaces SUITE
    PASS (173) zero reports (+ final-tree re-run post-fix);
    config-invariance SINGLE-HASH ×6 (trio × opt+none, 3-run
    debug + release); E-62 re-grep CLEAN (LIVE this diff — same 5
    sanctioned hits); Q5 progsize@128 release SAME-SESSION
    INTERLEAVED ABABAB A warm {120,120,120} vs B {120,120,120} ms
    (0.0% median, A1 cold discarded; generated headers
    byte-identical). DOC DISCHARGES same-commit: rel-arch §5 R4
    block DONE + banner; CLAUDE.md Rel-epoch paragraph. RESIDUALS: DS-R4-10 the
    differential-input @never labeled gap; the R1-R3 residual
    set carries unchanged (table-less kEagerUnion render,
    Induction.cpp:996 dead caller, neq/lt/gt spellings,
    publish-*/message=, eager count oracle + ClassifyEagerSink
    replica → R-final). NEXT: R-JOIN per §5 (the NOT-RULED
    pivot-equality-belt fold candidate rules at its head), then
    R-E42, R-final.

(R) R-JOIN-OPEN RE-VERIFICATION RECORD (2026-07-24, tip f60379c3; the
    session-open items (0)+(1) EXECUTED per §20(Q)'s NEXT).
    (0) Frozen A/B baselines RE-SNAPSHOTTED from tip into the session
        scratchpad BEFORE any code (debug 5f30847f… / release
        e69068d0…; both presets rebuilt clean) — the b1c95bac-era
        snapshots retired as STALE post-R4. ASAN cadence stands
        per-diff (§19(F)/(J)); the tip is docs-only atop 2aa23b3f,
        whose both-surface sweeps are the standing green.
    (1) §20(P)/(Q) + rel-arch-pseudocode.md §4/§4.1/§4.2/§4.3/§5/§6
        FLEET-RE-VERIFIED (house precedent): 3 seed-UNREAD opus
        derivation lanes (the 7-kind walk-mint machinery incl. the
        effect-bearing kNegateGate as landed; the JOIN/PRODUCT
        subsystem Join.cpp + Product.cpp END TO END incl.
        ContinueJoinWorkItem — the R-JOIN stage-(a) seed; the
        eqset=/partition dump surfaces) + 3 seed-read adversarial
        verifiers (§4-§4.3; §5+§6 incl. the R-JOIN anchors; §20(P)/
        (Q)+contract banners) + 1 sonnet mechanical lane (16-surface
        regen, census 24, hexdump pins, cross-knob compiles, the
        DS-R4-4 9-case sweep) + 1 xhigh consolidator (8 agents,
        ~762k tokens; every candidate adjudicated AT the code). The
        E-62 tripwire re-grepped by the ORCHESTRATOR personally:
        CLEAN (sole out-of-lib hits = the standing Stratum.cpp:1073
        comment + the RAT-3 InstanceOrderTest fixture). The
        carrier-golden referee EXECUTED by the orchestrator
        personally: ALL SIXTEEN pinned golden surfaces regenerate
        BYTE-IDENTICAL at tip (tc h/ir/df/deltarel, symrec
        ir/df/deltarel, map_3/merge_2/booleans/elim-cond-cycle-
        simple/negate_6/d5_recursive_negate deltarel, negate_1
        df+deltarel, aggregate_1 df — the irgold sidecars are the
        count authority). VERDICT: SOUND-WITH-ERRATA — zero code or
        design defects; the M1-M15 mold, the M14 effect
        byte-reconstruction, the §5 R-JOIN starting-state caveat,
        and the §6 partition facts hold EXACTLY (mechanical: 16/16
        surfaces, opt==nocf / nodf==none ×9 carriers, sign= c2 b7 +
        cmp=eq + functor=add_i32/3, DS-R4-4 post-state exact —
        disassemble=1 sole nonzero, corpus opt kNegateGate=18); five
        errata, ALL anchor/arity/label drift, applied IN PLACE (each
        site tagged):
          E-117 MED  §4 M6: E-112's own correction anchors went
                     stale AND slid onto the confusable add_gu_key
                     region (:3424-3439) — count_kind :3442 /
                     expect :3451 at tip.
          E-118 LOW  §4/§4.1/§4.2 Build.cpp mint/wrapper anchors
                     drifted +7..+22 (R4 insertions): mints
                     Union :1265 / Generate :1290 / Compare :1301 /
                     Select :1308 / Forward :1316 / Insert :1325 /
                     Negate :1339; wrappers :1145/:1153/:1164/
                     :1173/:1207/:1216/:1232. Helper anchors
                     :1113/:1088/:1092/:1123 HELD.
          E-119 LOW  DeltaRel.cpp recount/caller anchors drifted
                     +17: A.6(c) guard :3511, switch :3522,
                     table-match :3573, key_of caller :4492.
          E-120 COSM EAGER_WEB switch is 7-WAY after R4 (gate case
                     :2470-2472), not "6-way"; IsEagerMarkerKind
                     itself stays 6 kinds (gate excluded).
          E-121 COSM §6: "tc insert.19" spelled out to
                     demand_tc_witness (the house shorthand made
                     precise at the sole-stamped-INSERT witness).
        REFUTED (held, grounds in the consolidation record): the
        "per-op recount" naming candidate (house term, criterion
        exact); the A.6(c) dead IsAggregate cut-check (code-
        documented, Fable R1 [1]); the gate_table/negate_table
        near-collision (two field families, op_table_id correctly
        falls to 0); class= OR-fold + DOT non-reproducibility
        (deliberate, code-commented, §6-recorded). R-JOIN STAGE-(a)
        DIGEST (consolidator-adjudicated, L2==§5 with §5
        UNDERSTATING the caveat; full record in the fleet
        consolidation): (1) Build.cpp:1246-1253 dispatches JOIN/
        PRODUCT with NO mint/wrapper — the only two walk arms (plus
        E-42) with no DROp; BuildEagerJoinRegion (Join.cpp:707-783)
        at the walk moment only InTryInsert's the PREDECESSOR
        (:715-716, in place), mints pivot_vec + ContinueJoinWorkItem
        on first visit (:759-764), and appends parent to
        join_action->inserts (:767) — the in-place nested-loop else
        (:774-777, BuildNestedLoopJoin :179-294) is DEAD behind
        `true ||` (:744) + assert(false) (:775). (2) The TABLEJOIN
        mints ONCE at ContinueJoinWorkItem::Run (Join.cpp:558-704,
        BuildJoin :601 for_delta=false; TABLEJOIN Create :313) in
        WORK-ITEM-DRAIN order — CompleteProcedure (Build.cpp:
        1009-1022) stable_sorts by ContinueJoinOrder (Join.cpp:9-42;
        kContinueJoinOrder Build.h:81, induction inversion :82-83)
        every iteration — so join next_id fires in drain order, NOT
        walk order; the M3 identity argument does not transfer (the
        LEAD ruling = marker referent: per-visit dispatch record vs
        once-per-join emission; a zero-next_id marker ctor is
        id-stream-safe either way). (3) JOIN views are TABLE-LESS at
        the model layer (join_1 verified) — a marker's table= would
        be absent like the gate's lead-0; whether it carries
        pred-table kFlagReads (M14 effect-bearing) is the R4-crux
        analog. (4) The pivot-belt TUPLECMP (Join.cpp:317-321,
        !for_delta only; filled :513-519) is TAUTOLOGICALLY
        REDUNDANT — Index::First is exact full-key (Table.h:790-805
        slot.key == key; Next :807-810 walks the per-exact-key
        chain; the Join.cpp:513-516 "approximate" comment is STALE)
        — BUT it is also the body anchor ({let,parent} chain) and
        the provenance-hider (col_id_to_var remap :532-534); folding
        relocates BOTH roles = an EMISSION-shape change (structural
        gates, never byte A/B). (5) M13 sweep: BuildEagerJoinRegion/
        BuildEagerProductRegion each have ONE caller (Build.cpp:
        1249/:1251); ContinueJoinWorkItem is ALSO created at
        Induction.cpp:726 (products :774); BuildJoin has TWO callers
        — Join.cpp:601 (eager, for_delta=false) and Stratum.cpp:1532
        (DR-round delta, for_delta=true) — the ONE region factory
        SHARED by hand-coded and modeled paths (touching it risks
        the modeled path). (6) CARRIERS: NO JOIN/PRODUCT eager
        marker exists today (kPivotAssemble is the DISJOINT
        differential Path #2, not an eager op); join_1 = the clean
        acyclic pivot-join candidate (2 joins, table-less views,
        table-backed sides); symrec/tc witness the deferred/
        inductive interplay; NO existing .deltarel golden covers a
        PRODUCT (an acyclic-@product carrier must be ADDED).
        [E-122, the stage-(b) adjudicator's correction, ORCHESTRATOR-
        verified at the golden: the digest's "kPivotAssemble/
        kFixpointFire 0 in all 9 goldens / Path #2 entirely golden-
        unwitnessed" was FALSE — d5_recursive_negate.deltarel.opt.
        golden carries kPivotAssemble=1 kFixpointFire=2, so the
        differential-JOIN delta path through the shared
        BuildJoin(for_delta=true) IS carrier-covered; only the
        differential-PRODUCT path remains golden-unwitnessed.] Full record: session
        scratchpad fleet-rjoinopen/consolidated.md (disposable; THIS
        entry is binding). Errata continue at E-122. NEXT: the
        R-JOIN slice under the full ritual (stage-(a) pseudocode
        build-out from the digest; ritual-head rulings: slice scope,
        marker referent, effect-free vs effect-bearing (M14), and
        the NOT-RULED pivot-equality-belt fold — an OWNER ruling).
        [EXECUTED — §20(S).]

(S) R-JOIN LANDED (2026-07-24) — THE FIFTH STEP-KIND MIGRATION: the
    pivot-JOIN + @product dispatch arms are MODELED DR-IR OPS. Binding
    contracts COMMITTED: rjoin-design.md (stage-(b)/(c) adjudication
    ADJ-RJ-1..16 + the four OWNER RULINGS in banner) +
    rjoin-desired-states.md (DS-RJ-1..10 + the stage-(d) three-way +
    Fable-review records). OWNER RULINGS (all four recommendations
    ratified at the ritual head): (i) SCOPE = JOIN+PRODUCT, ONE slice
    (the R1-R3 sibling-pair precedent; pivots-only rejected — census
    double-churn for no isolation); (ii) REFERENT = the PER-VISIT
    dispatch edge (normative: a kEagerJoin op marks that the eager
    walk reached a pivot-join at one (pred_view -> join_view)
    dispatch edge and lowered it in place via the UNTOUCHED deferral
    machinery — a reachability record, NOT the once-per-join deferred
    TABLEJOIN emission, which stays hand-coded in
    ContinueJoinWorkItem::Run drain order and is owed to R-final
    with its own emission op carrying the ContinueJoinOrder key —
    ADJ-RJ-14, the M16 mold delta); (iii) EFFECT-FREE (M14 does NOT
    transfer — no pre-existing eager DR op to re-source;
    effect-bearing-ness is the DEP-EDGE an op contributes, not the
    runtime reads of its lowered region; both kinds join
    IsEagerMarkerKind, now 8 kinds); (iv) the pivot-equality-belt
    fold DECLINED this slice (the epoch-brief §5 NOT-RULED candidate,
    RULED: REDUNDANCY-HOLDS — Index::First is exact full-key
    slot.key==key, Table.h:801, Next walks the per-exact-key chain,
    the no-index path dead — but the TUPLECMP is ALSO the body
    anchor and the provenance-hider (Join.cpp:532-534), so folding
    is an EMISSION-shape change touching the SHARED
    BuildJoin(for_delta) — its own gated follow-up at R-final;
    REVISES the brief's "belongs WITH the JOIN migration"
    pre-registration on principled grounds). THE MECHANISM:
    kEagerJoin(24)/kEagerProduct(25) minted at the Build.cpp IsJoin
    dispatch (pivot-count split), lowered IN PLACE by
    LowerRelStep_Join/_Product wrappers calling the UNTOUCHED
    BuildEagerJoinRegion/BuildEagerProductRegion (Join.cpp/
    Product.cpp/Stratum.cpp/Induction.cpp byte-unchanged); ctors
    clone MakeEagerUnionOp (zero next_id — id-stream identity holds
    ACROSS the deferral because the ctor mints no id and the builder
    is untouched, ADJ-RJ-7); A.6(c) arms M10-STRENGTHENED on the
    pivot-count discriminant; census 24→26 DAY ONE; render = the
    bare kEagerForward production (NO new token, NO E-71 lane).
    STAGE-(a) CORRECTIONS mid-ritual (both committed as errata
    BEFORE code): E-122 — the digest's "Path #2 golden-unwitnessed /
    kPivotAssemble=0 in all 9" was FALSE (d5's golden carries
    kPivotAssemble=1 kFixpointFire=2, so the shared
    BuildJoin(for_delta=true) delta path IS carrier-covered);
    ADJ-RJ-1 — "demand_tc is a zero-mint case" REFUTED by lldb
    (8 dispatches; Induction.cpp:726 only pre-creates the work item,
    the walk still dispatches). THE M12 HEADLINE (stage-(d) probe,
    ADJ-RJ-16): the uniform table-less prediction REFUTED on
    demand_tc_witness — TWO of its four join views are
    model-table-BACKED (tables 4/15), so FOUR marker blocks render
    table= and sort into their tables' bands (the FIRST JOIN
    witnesses of the E-107 shape); the author lane independently
    derived the same pair from the OD-13 eqset= tokens (the §20(P)
    partition-visibility payoff — derivation without a worktree
    probe, the probe as confirmation). STAGE-(d) THREE-WAY
    CONVERGENCE (now FIVE slices deep): author hand-prediction
    (dump-blind; full predicted dumps for both new goldens +
    unified diffs for all 9 re-blessed) == blind worktree prototype
    == pristine implementation, BYTE-IDENTICAL on ALL 44 SURFACES
    (11 carriers × 4 modes; orchestrator-executed cmp per E-77).
    THE COUNT ORACLE (M15): census == BuildEagerJoinRegion/
    BuildEagerProductRegion lldb dispatch hit-counts, 44/44 rows,
    per-VISIT + emission-side (join_1 = 4 markers over 2 join
    views). CARRIERS: join_1 (kEagerJoin=4) + optimize_2 (the FIRST
    product golden, kEagerProduct=2; the existing case, sidecar
    only — Design C's product_conds candidate REFUTED, its
    conditions desugar to unit pivot-JOINS) seeded RAT-8; four
    existing carriers gained REAL blocks (demand_tc 8 / symrec 4 /
    booleans 4 / elim 2 — the widest existing-carrier churn of the
    epoch, ADJ-RJ-3); d5_recursive_negate = the ZERO-MINT join
    NEGATIVE guard (third golden role: R4 zero-mint negate + Path
    #2 delta-join carrier + join walk-cut). FABLE REVIEW (18-agent
    workflow): 12 findings, TEN confirmed (one adjudicated by the
    ORCHESTRATOR personally after its verifier lane died at the
    StructuredOutput cap — recovered from the journal, never
    respawned), ZERO live correctness — all doc/comment drift,
    ALL FIXED PRE-COMMIT (DeltaRel.h field comment + EAGER_WEB
    "six"→"eight" + census-guard "25th"→"27th" + the pre-existing
    Build.cpp:743 "six"→"eight" + DS-RJ-4 four-blocks rewording +
    CLAUDE.md re-point: eight markers, ELEVEN goldens, unmodeled
    arm = E-42 only), proven DUMP-NEUTRAL (18/18 pinned surfaces
    post-fix) + post-fix suite/A-B green; 2 REFUTED (the enum
    "carries two" counts VIEWS; the :179/:180 cite nit). GATES
    (final tree): SUITE pre-bless EXACTLY the 11 pre-registered
    reds (2 IRGOLD-MISSING + 9 IRGOLD-DIVERGE, nothing else) →
    RAT-8 bless (sources 11/11 byte-verified same-as-reviewed; all
    other bless rewrites byte-identical no-ops) → PASS (173) ×3 +
    post-fix; A/B vs frozen f60379c3 (debug 5f30847f / release
    e69068d0): 840 rows × 2 pairs (692 corpus + 4 nested + 144
    data/) 0-DIVERGED + post-fix 64-row subset clean
    (evm_array_parse identical-SIGABRT stands); ctest 5/5 debug +
    5/5 ASAN; ASAN BOTH surfaces SUITE PASS (173) zero reports;
    config-invariance SINGLE-HASH ×22 (11 carriers × opt+none,
    3-run debug + release); E-62 re-grep CLEAN (LIVE this diff —
    same sanctioned hits, zero new pinned_order consumers); Q5
    progsize@128 release SAME-SESSION INTERLEAVED ABABAB A warm
    {155,154,155} vs B {158,155,154,155} ms (0.0% median, A1 cold
    discarded; generated headers byte-identical — the ADJ-S8
    MEASURED gate). RESIDUALS OPENED (DS-RJ-10): R-final owes
    the per-join EMISSION op (drain-order key) — the per-visit
    marker is the reachability HALF; the fold (redundancy holds,
    stale "approximate" comment Join.cpp:513-516 lingers
    deliberately); the side_key_eqs delta fold = a separate third
    diff (Database.cpp :2788/:2791/:2862/:2896 — the brief's
    ":2740-2744" was stale, ADJ-RJ-16); dead BuildNestedLoopJoin
    (Join.cpp:179-294, assert-disabled) labeled, not removed; no
    differential-PRODUCT kProductArm golden (d5 covers the join
    delta path). The R1-R4 residual set carries unchanged. NEXT:
    R-E42 per §5 (the VECTORLOOP shim minted from an op — S4
    retires), then R-final (direction flip + the emission op + the
    fold + count oracle + ClassifyEagerSink replica retirement +
    the DeltaRel→Rel rename ritual).

(T) R-E42-OPEN RE-VERIFICATION RECORD (2026-07-24, tip 429f14f4; the
    session-open items (0)+(1) EXECUTED per §20(S)'s NEXT).
    (0) Frozen A/B baselines RE-SNAPSHOTTED from tip into the session
        scratchpad BEFORE any code (debug e6eb2e3e… / release
        035720ac…; both presets rebuilt clean) — the 5f30847f/
        e69068d0-era snapshots retired as STALE post-R-JOIN. ASAN
        cadence stands per-diff (§19(F)/(J)); the tip is docs-only
        atop 18026049, whose both-surface sweeps are the standing
        green.
    (1) §20(R)/(S) + rel-arch-pseudocode.md §4-§6 (incl. §4.4 and the
        §5 R-E42 block) FLEET-RE-VERIFIED (house precedent): 3
        seed-UNREAD opus derivation lanes (the 9-kind walk-mint
        machinery incl. kEagerJoin/kEagerProduct as landed; the
        INGEST subsystem END TO END — ExtendEagerProcedure,
        MakeStageOneIngestFolds, MakeMonotoneIngestFold,
        LowerIngestFold, V-INGEST-XCHECK Site 5, the table-less
        VECTORLOOP arm — doubling as the R-E42 stage-(a) seed; the
        dump/census surfaces) + 3 seed-read adversarial verifiers
        (§4-§4.4; §5 incl. the R-E42 block + §6; §20(R)/(S) +
        rjoin-design/desired-states) + 1 sonnet mechanical lane
        (18-surface regen, census 26, marker counts, hexdump pins,
        cross-knob compiles, the kIngestFold=0 quad, the demand_tc
        four-table-backed check) + 1 xhigh consolidator (8 agents,
        ~710k tokens; every candidate adjudicated AT the code). ONE
        lane death (the §20(R)/(S) verifier, at the structured-output
        cap) — RECOVERED FROM ITS DISK FILE per the R-JOIN precedent,
        never respawned: its written verdict was SOUND with every
        mechanism/carrier/anchor claim confirmed (four frozen-tip
        rjoin-design interior anchors noted COSM, correct-as-pinned);
        the consolidator independently swept §20(R)/(S) and found no
        gap. The E-62 tripwire re-grepped by the ORCHESTRATOR
        personally: CLEAN (sole out-of-lib hits = the standing
        Stratum.cpp:1073 comment + the RAT-3 InstanceOrderTest
        fixture; zero out-of-lib body_ops/output_ops readers). The
        carrier-golden referee EXECUTED by the orchestrator
        personally: ALL EIGHTEEN pinned golden surfaces regenerate
        BYTE-IDENTICAL at tip (tc h/ir/df/deltarel, symrec
        ir/df/deltarel, negate_1 df+deltarel, aggregate_1 df, map_3/
        merge_2/booleans/elim-cond-cycle-simple/negate_6/
        d5_recursive_negate/join_1/optimize_2 deltarel — the irgold
        sidecars are the count authority). VERDICT: SOUND-WITH-ERRATA
        — zero code or design defects; the M1-M16 mold, BOTH §5
        R-E42 starting-state caveats, and the §6 partition facts hold
        EXACTLY (§5 R-E42 + §6 drew ZERO candidates); seven errata,
        ALL anchor/arity/count drift in the §4-§4.3 mold bodies
        (last re-based at f60379c3 = pre-R-JOIN), applied IN PLACE
        (each site tagged):
          E-123 MED  §4.1 M6'/§4.2: IsEagerMarkerKind is 8 KINDS
                     after R-JOIN (kEagerJoin/kEagerProduct joined;
                     the R4 gate stays excluded). Def :1306 HELD;
                     callers now :3542/:4546.
          E-124 MED  §4.1/§4.2/§4.3: EAGER_WEB switch is 9-WAY
                     (Join :2495 / Product :2498; gate case
                     :2501-2503; loud-abort default :2504-2509); the
                     M4 enrollment region is :2457-2510.
          E-125 MED  §4.3: MakeEagerNegateOp ctor is :1396 (decl
                     .h:1041) — the old ":1371" anchor now COLLIDES
                     with MakeEagerJoinOp (R-JOIN inserted Join:1371/
                     Product:1379 between Select and Negate).
          E-126 MED  §4.2: kAllKinds is Format.cpp:1054 with 26
                     kinds (was ":1040 (24)"); totality guard :1077.
          E-127 LOW  §4/§4.1/§4.2/§4.3: every Build.cpp MINT anchor
                     +30 (R-JOIN inserted LowerRelStep_Join/_Product
                     :1246/:1255 + the IsJoin arm :1269-1283): Union
                     :1295, Generate :1320, Compare :1331, Select
                     :1338, Forward :1346, Insert :1355, Negate arm
                     :1361-1371 (mint :1369). Wrappers (:1145/:1153/
                     :1164/:1173/:1207/:1216/:1232/:1246/:1255),
                     helpers (:1088/:1092/:1113/:1123), and
                     BuildEagerSelectRegion :1190 all HELD.
          E-128 LOW  DeltaRel.cpp recount region +31 / key_of+V-READY
                     +54: add_gu_key :3455ff, count_kind :3473,
                     expect :3483, base batch :3491-3497, A.6(c)
                     guard :3542, switch :3554, union :3574, select
                     :3588, join :3593, product :3606, table-match
                     :3628, kNegateGate recount :3090, key_of gate
                     :4527, key_of marker :4546, V-READY skips
                     :5063/:5067.
          E-129 COSM Format.cpp render/name anchors +2: EagerSinkName
                     :132, ComparisonOperatorName :152, render
                     Compare :905 / Generate :917 / Union :937 /
                     Select :947. DROpKindName def HELD.
        NON-DEFECT CODE NOTE (recorded, no erratum; a future
        code-comment sweep or the R-E42 diff itself may fix it):
        Stratum.cpp:2099's Site-5 header comment reads
        "V-PRED-XCHECK Site 5" while the :2150 abort string and the
        docs say V-INGEST-XCHECK — the doc matches the authoritative
        abort string. R-E42 STAGE-(a) SEED DIGEST (consolidator-
        adjudicated at code; full record in the fleet consolidation):
        (1) ARM SELECTION (Procedure.cpp:39-110): Arm A
        deletion-capable :50-60 (two stage-1 folds, no descent); Arm
        B monotone table-bearing :70-93 (one fold, UPDATECOUNT
        descent cursor, INGEST-CURSOR-SHAPE guard :87-93); Arm C =
        E-42, table-less monotone :94-106 (hand VECTORLOOP + VARs,
        no fold, no table touch), descent :108 with table=null; sole
        caller BuildEntryProcedure :820. (2) THE ID-ALLOC CRUX: Arm
        C allocates EXACTLY 1+arity next_ids (VECTORLOOP :96, then
        one VAR per receive column :101) — IDENTICAL in shape and
        order to LowerIngestFold's monotone arm (Stratum.cpp:1941/
        :1951; UPDATECOUNT and VECTORAPPEND consume NO next_id), so
        the hole-contract lower-in-place shape reproduces the id
        stream mechanically; the marker zero-next_id trick does NOT
        apply (M16 pin-the-referent). (3) WHY TABLE-LESS (in-code
        :62-69): the receive head is induction-owned — the descent's
        InTryInsert emits the fold under an induction; FillDataModel
        forces no table on a plain monotone receive. (4) EFFECTS:
        vec-only (no table, no kCounter; MonotoneIngestRoleDR/
        EmissionDerivClass both NEED a table and do not apply).
        (5) EXTENSION POINTS: enrollment currently enrolls NOTHING
        table-less (guard DeltaRel.cpp:2400, ack comment :2365-2366);
        count-expect += 0 (:3423/:3429); Site 5's enrolled-filter
        SKIP (:2130-2132) is defensively DEAD today and becomes live
        coverage under a modeled op; LowerIngestFold hard-asserts
        table!=nullptr (:1930) — structurally incapable of Arm C
        today (relax vs sibling lowering = a stage-(b) decision).
        (6) RITUAL-HEAD RULINGS OWED: op FAMILY (ingest-fold sibling
        vs marker — the :2366 comment already says "not an ingest
        fold"; the layer is INGEST, not a walk-dispatch arm), payload
        (message stored vs re-derived, M2'), the PER-RECEIVE
        count-law referent (M15 re-derived for the ingest layer), and
        the Site-5/count-expect/key-multiset extension. (7) M9
        CARRIERS: the kIngestFold=0 quad (map_3, merge_2,
        elim-cond-cycle-simple, join_1) are the candidates; census
        26->27 churns ALL ELEVEN .deltarel pins; whether each quad
        member actually HAS a table-less receive is re-verified at
        stage (a) at the walk/model layers per M12. VERIFIED-CLEAN
        (held; compressed): IsEagerMarkerKind 8 kinds excl. gate;
        EAGER_WEB 9-way tail-append (folds keep op.0/op.1); all nine
        ctors id-neutral, single-authority; the M14 gate kFlagRead
        reconstruction; A.6(c) strengthened arms (union
        !InductionGroupId, join pivots>0, product pivots==0);
        kAllKinds 26 + totality; MECH 25/25 checks PASS (18-surface
        regen, census lines, marker counts incl. join_1 kEagerJoin=4
        / optimize_2 kEagerProduct=2 / d5 zero-mint kPivotAssemble=1
        kFixpointFire=2, hexdump pins sign=· c2 b7 + cmp=eq +
        functor=add_i32/3, demand_tc four table-backed join blocks
        op.7/op.17=%table:4 + op.14/op.16=%table:15, cross-knob
        opt==nocf / nodf==none ×11 carriers); §5 R-E42 anchors EXACT
        (Procedure.cpp:14-111/:820/:96/:101/:62-69, DeltaRel.cpp
        :1191/.h:996, :1235/.h:1006, :2369/:2366, :3403, :3633,
        Stratum.cpp:1909/:1930/:1980/:2101ff). Full record: session
        scratchpad fleet-re42open/consolidated.md +
        R-E42-seed-digest.md (disposable; THIS entry is binding).
        Errata continue at E-130. NEXT: the R-E42 slice under the
        full ritual (stage-(a) pseudocode build-out from the seed
        digest; ritual-head rulings: op family, payload/effects, the
        PER-RECEIVE count law, the Site-5 extension, and the
        id-stream argument — the hole-contract lower-in-place shape).
        [EXECUTED — §20(U).]

(U) R-E42 LANDED (2026-07-27) — THE SIXTH SLICE and the FIRST
    INGEST-LAYER migration: the table-less monotone receive's
    VECTORLOOP shim (Arm C, S4 — the LAST emission surface with zero
    model representation) is a MODELED DR-IR op. Binding contracts
    COMMITTED: re42-design.md (the stage-(b)/(c) adjudicated design,
    A1-A6, + the TEN ritual-head OWNER RULINGS RH-1..RH-10 ratified
    2026-07-27) + re42-desired-states.md (the stage-(a) record + the
    author predictions + the three-way/gates/Fable records). OWNER
    RULINGS (all ten ratified as recommended): RH-1 kIngestLoop(26),
    a new INGEST-FAMILY sibling kind (the 27th; NEVER an eager
    marker — out of IsEagerMarkerKind and EAGER_WEB; the marker
    family was REJECTED-BY-CONSTRUCTION: markers mint zero next_id,
    Arm C MUST mint 1+arity); RH-2 the sibling LowerIngestLoop (the
    byte-move of Arm C — VECTORLOOP next_id++ then one VAR per
    receive column, returning the loop as the descent cursor) + a
    new always-on INGEST-LOOP cursor-shape guard mirroring Arm B's;
    RH-3 payload = ingest_message + ingest_receive, EFFECT-FREE
    (vec-only read; zero kCounter-first edits — the A1 adjudication
    struck the draft's false ":3177 REQUIRED edit"); RH-4 TAIL-APPEND
    enrollment via a dedicated query.IOs()×Receives() re-derivation
    AFTER EAGER_WEB (Arm C is not reachability-gated, so per-receive
    == per-visit == per-emission and enrollment needs no walk
    stream; every existing op.N label stays byte-stable — the
    in-loop alternative would have renumbered the four quad goldens
    wholesale); RH-5 the PER-RECEIVE count law under a SEPARATE
    counter + expect(kIngestLoop) + a table-less sibling key
    multiset (never absorbed into exp_ingest — the kIngestFold=0
    quad stands); RH-6 Site 5 gains a SIBLING emitted-vs-enrolled
    multiset over the new Context::emitted_ingest_loops, WITH both
    hygiene riders (the dead table-less filter DELETED; the
    V-PRED-XCHECK header renamed V-INGEST-XCHECK); RH-7 MARKER-SHAPE
    render (header + args: only); RH-8 message=<name>/<arity>
    reused — NO new spelling, NO E-71 lane (the referent shift
    documented in the ruling); RH-9 a dedicated lead-0 key_of arm
    (sign=+1, oi; merged with the fold arm post-review); RH-10 NO
    new carrier — the kIngestFold=0 quad (map_3 +1 / merge_2 +3 /
    elim-cond-cycle-simple +1 / join_1 +2) gains real blocks, all
    ELEVEN pins re-blessed. §4.5 = the M17 ingest-family
    lower-in-place precedent. MID-SLICE EVENTS (all recorded):
    TWO owner-directed diffs landed BETWEEN stages (c) and (d) —
    the aggregate multiplicity-semantics documentation (1ccc9be3)
    and the projected-column LINT + agg_distinct_1 witness
    (c8888e44, SUITE 173→174, ErrorLog gains the first-class
    advisory-warning arm) — so the frozen A/B baselines were
    re-snapshotted mid-slice (debug 0ed9cb3a / release e238bb17)
    and every gate arithmetic moved to 174; the FIRST stage-(d)
    prototype dispatch was REFUSED BY ITS OWN LANE (the
    auto-provisioned worktree materialized 99 commits stale; the
    lane stopped at its verify-the-tip step — the ritual working)
    and was re-dispatched on a hand-provisioned worktree verified
    at c8888e44; the AUTHOR lane died at its structured-output cap
    and was RECOVERED FROM ITS COMPLETE DISK FILE (the R-JOIN
    precedent, never respawned); the ASAN gate lane returned a
    placeholder and was ORCHESTRATOR-EXECUTED. STAGE-(d) THREE-WAY
    CONVERGENCE (the SIXTH slice): author hand-prediction
    (dump-blind; full predicted bodies for the quad + census-append
    diffs for the seven) == blind worktree prototype == pristine
    implementation, BYTE-IDENTICAL 11/11 (orchestrator-executed cmp,
    E-77). FABLE REVIEW (15-agent workflow): 6 findings, ZERO live
    correctness — [1] a §7b-sibling duplicate-receive guard was
    MISSING for the loop family (every other check re-derives from
    the same IOs×Receives; FIXED, a new always-on enrolled-side
    guard) and [2] kIngestLoop fell to the per-op validation
    default (FIXED, a new always-on ctor-contract arm) — both
    validator ADDITIONS; [0]/[6] comment drift FIXED (the ADJ-S2
    tail-position wording; the INGEST_FOLD cross-ref); [3] key_of
    arm merged; [4] IngestLoopKeyOf minted as the one flow-side key
    spelling; 2 REFUTED. Fixes proven DUMP-NEUTRAL (18/18) +
    post-fix suite/ctest/A-B/ASAN green. GATES (final tree): suite
    pre-bless EXACTLY the 11 pre-registered IRGOLD-DIVERGE
    (single-hash ×3, 0 MISSING, agg_distinct_1 green) → bless
    (11/11 sources byte-verified same-as-converged BEFORE bless) →
    PASS(174) ×3 + post-fix; A/B vs frozen c8888e44: 844 rows
    (696+4+144) 0-DIVERGED + 64-row post-fix subset
    (evm_array_parse identical-SIGABRT stands); ctest 5/5 debug +
    5/5 ASAN; ASAN BOTH surfaces SUITE PASS(174) zero reports ×2;
    config-invariance SINGLE-HASH ×22; E-62 re-grep CLEAN (live);
    M15 count oracle 44/44 per-RECEIVE + emission-side (lldb
    LowerIngestLoop == census; the cross-knob law held ×11 incl.
    the four nodf B→C flips); Q5 progsize@128 release ABABAB A warm
    {154,157,153} vs B {155,155,153,155} ms (~0.6% median, noise;
    headers byte-identical). DOC DISCHARGES same-commit: rel-arch
    §4.5 (M17) + §5 R-E42 DONE + banner; CLAUDE.md re-point (NO
    unmodeled arm remains; the descent is the ONLY hand-coded
    emission surface; census 27). RESIDUALS: the R1..R-JOIN set
    carries unchanged (per-join emission op + pivot-belt fold +
    side_key_eqs fold + count oracle + ClassifyEagerSink replica →
    R-final; DS-R4-10; dead BuildNestedLoopJoin; spellings). NEXT:
    R-final per §5 (the direction flip + the per-join emission op +
    the fold + the eager count oracle + ClassifyEagerSink
    retirement + the DeltaRel→Rel rename ritual). Errata continue
    at E-130.

(V) R-FINAL-OPEN RE-VERIFICATION RECORD (2026-07-27, tip a7dde012; the
    session-open items (0)+(1) EXECUTED per §20(U)'s NEXT).
    (0) Frozen A/B baselines RE-SNAPSHOTTED from tip into the session
        scratchpad BEFORE any code (debug 04c759c9… / release
        b3e3cd58…; both presets rebuilt clean) — the c8888e44-era
        snapshots retired as STALE post-R-E42. ASAN cadence stands
        per-diff (§19(F)/(J)); the tip is docs-only atop 535a6621,
        whose ×2 both-surface sweeps are the standing green.
    (1) §20(T)/(U) + rel-arch-pseudocode.md §4-§6 (incl. §4.5/M17 and
        the §5 R-FINAL block) FLEET-RE-VERIFIED (house precedent): 3
        seed-UNREAD opus derivation lanes (the walk-mint/record/
        EAGER_WEB machinery + the S2 replicas + §7d — the direction-
        flip stage-(a) seed; the ContinueJoinWorkItem deferral
        subsystem END TO END — the emission-op seed; the dump/census/
        key_of + ingest surfaces at census 27) + 3 seed-read
        adversarial verifiers (§4-§4.5 mold; §5 R-final block + §6;
        §20(T)/(U) + re42 contracts) + 1 sonnet mechanical lane
        (18-surface regen, census 27, kIngestLoop quad 1/3/1/2,
        hexdump pins, cross-knob ×11, the demand_tc four-table-backed
        check — 6/6 checks PASS) + 1 xhigh consolidator (8 agents,
        ~858k tokens; every candidate adjudicated AT the code). One
        lane digest degenerated at the structured-output layer (L2);
        its DISK file was complete and the consolidator used it (the
        R-E42 recovery precedent — never respawned). The E-62
        tripwire re-grepped by the ORCHESTRATOR personally: CLEAN
        (sole out-of-lib hits = the standing Stratum.cpp:1073 comment
        + the RAT-3 InstanceOrderTest fixture). The carrier-golden
        referee EXECUTED by the orchestrator personally: ALL EIGHTEEN
        pinned golden surfaces regenerate BYTE-IDENTICAL at tip.
        VERDICT: SOUND-WITH-ERRATA — zero code or design defects; the
        M1-M17 mold, the §5 R-FINAL block's five head items, and the
        §6 partition facts hold EXACTLY; eight errata, ALL anchor/
        count drift from the R-E42 insertions (last re-base 429f14f4
        = pre-R-E42), applied IN PLACE (each site tagged):
          E-130 MED  §4.2: kAllKinds is Format.cpp:1075 with 27 kinds
                     (the one COUNT drift; guard msg reads "28th").
          E-131 LOW  §4 M2 (+§4.1/§4.2/§4.3): marker ctors +32
                     (MakeIngestLoopOp:1274/IngestLoopKeyOf:1290
                     inserted above; Forward:1311…Negate:1428,
                     decl .h:1075).
          E-132 LOW  §4 M6 (+M6'): census/recount +99..+100 (base
                     expect 3590-3601, A.6(c) guard 3642, switch
                     3653, table-match 3727); IsEagerMarkerKind
                     :1306→:1338, callers :3642/:4697 (still 8 kinds,
                     2 callers).
          E-133 LOW  §4 M4: EAGER_WEB enrollment ~2491-2547; the
                     ABSOLUTE tail is now the INGEST_LOOP block
                     :2549-2578 (M4's tail-append HOLDS; EAGER_WEB no
                     longer the final family).
          E-134 LOW  §4.3: kNegateGate recount :3155; key_of gate
                     :4669; V-READY skips :5214/:5218.
          E-135 LOW  §4.1/§4.2: renders Format.cpp:926/:938 +
                     :958/:968; ComparisonOperatorName :153;
                     EagerSinkName :133.
          E-136 LOW  §6: ModelTableOrNull DeltaRel.cpp:1267→:1299.
          E-137 COSM §5 head item (4): the identifier-sweep rename
                     bucket names tests/DeltaRelValidators/ (ctest
                     target, #include "DeltaRel.h").
        STAGE-(a) SEED DIGESTS (consolidator-adjudicated at code;
        recorded in the rel-arch banner + the fleet consolidation;
        they SHARPEN the §5 head items): (A) FLIP — walk op.N order
        is a WORK-LIST SCHEDULER artifact (CompleteProcedure
        stable_sort DESC + pop_back LIFO), NOT a graph DFS: order-
        reproduction means scheduler replay, so the structural-gate
        re-bless of the eleven pins is the realistic path; the
        recorded eager identity is VIEW-valued while the op referent
        is the per-visit EDGE (EmittedEagerOp has no pred_view) —
        walk-consume needs a counted key or a NEW per-edge key, a
        head ruling §5 does not yet name; the flip retires
        RecordEagerDispatch/emitted_eager_ops/EAGER_WEB-replay/§7d
        and RELOCATES ClassifyEagerSink (walk-only, no DR replica);
        NOTE the stale AnyCutSuccessorDR code comments (DeltaRel.cpp
        :127/:2383 cite "Build.cpp:857-858"; the cut test is :970) —
        code hygiene for the flip diff. (B) EMISSION OP — the
        deferred TABLEJOIN mints once per (proc, join_view,
        path-form) in the SHARED BuildJoin (Join.cpp:313; eager
        :601 for_delta=false / delta Stratum.cpp:1532); drain order
        = (ContinueJoinOrder ASC, REVERSE walk-insertion index) —
        the order key ALONE collides (equal depth/induction-class,
        and vs products), so the op must ALSO carry the walk-
        insertion seq; the per-join next_id budget is NOT a
        structural constant (TABLEINDEX mints only for NEW column
        specs — cross-join REUSE makes the count emission-ORDER-
        sensitive), so any id-stream contract must carve the index
        stream out or canonicalize provisioning. (C) CENSUS/INGEST —
        kIngestLoop is the ONE effect-free lead-0 op NOT id-neutral
        at lower (mints 1+arity; the load-bearing ground it stays
        out of IsEagerMarkerKind); the 8 markers still have NO
        scalar count expect (the M15/ADJ-S12 oracle R-final owes);
        kNegateGate rides the generic render, no dedicated case
        (surface-rework input). MID-SESSION OWNER DIVERGENCE (landed
        between (1) and R-final stage (a), its own commit): the `:-`
        strict-order clause separator (owner-directed 2026-07-27) —
        suite 174→175 (barrier_neck_1), all gates in that commit's
        record. Full fleet record: session scratchpad
        fleet-rfinalopen/ (disposable; THIS entry is binding).
        Errata continue at E-138. NEXT: R-final stage (a) per §5 as
        sharpened by the three seed digests (ritual-head rulings:
        order-of-diffs; flip order-reproduction-vs-rebless + gate
        families + the per-edge identity question; emission-op
        referent/payload/count law + the id-budget carve; the two
        fold adopt-vs-defer rulings; rename scope + surface name).
        [EXECUTED same-session: stages (a)-(c) ran as three fleets
        (stage-(a) build-out ~838k tokens; stages (b)/(c) design+
        critique+adjudication ~1.30M tokens, 22 findings adjudicated,
        4 HIGH amended, zero escalations); the owner RATIFIED R1-R6
        as recommended (rfinal-ruling-brief.md ratification record);
        Fold A landed — §20(W).]

(W) R-FINAL SLICE 1 — FOLD A LANDED (2026-07-27): the pivot-belt TUPLECMP
    RETIRED — the first R-final diff under the ratified R1-R6 rulings
    (rfinal-ruling-brief.md, owner "ratify all" in-session; ruled order
    Fold A → Fold B → emission op → flip SD-1..SD-4 → rename). Binding
    contracts COMMITTED: rfinal-design.md (the stages-(b)/(c) adjudicated
    design for ALL FIVE slices — 22 critique findings adjudicated at code,
    4 HIGH all amended pre-implementation: the flip's constant-TUPLE second
    SET root, the delta kJoinEmit enrollment-after-DeriveDRStrata move, the
    join Site-5 closing-block placement, the CMake live-line rename catch)
    + rfinal-ruling-brief.md (R1-R6 + the ratification record) +
    rfinal-desired-states.md (the per-slice DS ledger; the Fold A three-way
    record). THE MECHANISM (design §1, amended foldA-F1..F4): BuildJoin's
    eager arm no longer mints the TUPLECMP that re-checked each scanned
    side's key columns against the pivot — Index::First/Next is FULL-KEY
    EXACT (slot.key == key over every key column; Next walks the per-key
    Add-chain), so the belt re-checked what the probe guarantees; the
    TUPLECMP's two other roles re-home: body-anchor → the TABLEJOIN itself
    (callers set parent=join), provenance-hider col_id_to_var →
    join->col_id_to_var (the foldA-F1 INTENTIONAL same-key last-write-wins
    over the pivot-loop write — write-order byte-correct, pinned
    in-comment; never convert to insert-if-absent); BuildJoin returns bare
    TABLEJOIN* (pair signature + the vacuous delta cmp==nullptr assert
    dropped); the delta path emission-UNTOUCHED (side_key_eqs stays until
    Fold B); the Runtime CONTRACT PIN lands at Table.h:789ff (full-key
    exactness under the Key's memberwise operator== — VALUE equality, the
    Fable-review re-wording — scoped to the monotone body path until Fold
    B; stale "approximate" comments retired across Join.cpp/Build.h/
    Program.h/Database.cpp incl. the review-caught PUBLIC-API reader).
    STAGE-(d) THREE-WAY CONVERGENCE (the SEVENTH slice): dump-blind author
    hand-predictions (source diff + full unified golden diffs + the
    DONT-CHANGE set + exact red vocabulary) == blind worktree prototype
    (5 files; churn EXACTLY the pre-registered 58/175 opt headers;
    19-surface regen = 3 changed + 16 identical; e2e stdout 12/12) ==
    pristine implementation (converged patch orchestrator-applied; 3
    surfaces BYTE-IDENTICAL to the prototype outputs, 16 pinned surfaces
    byte-identical to committed goldens — orchestrator-executed cmp, E-77);
    the author's hand-predicted hunks byte-match the empirical diffs.
    FABLE REVIEW (18-agent workflow, high): 13 confirmed findings
    collapsing to SIX root causes, ZERO live correctness — the Table.h
    NOTE re-scoped (the no-re-check claim is body-path-only until Fold B)
    and re-worded to operator== VALUE equality (the float −0.0/NaN
    byte-equality catch); the missed PUBLIC-API reader
    include/ControlFlow/Program.h:977; EmitJoin's side_key_eqs eager
    clause; the Join.cpp comparison-guard rationale; the rel-arch §5
    fold-block discharge (dead :317-321 anchors) — ALL FIXED PRE-COMMIT,
    proven DUMP-NEUTRAL (19/19 pinned surfaces) + post-fix SUITE
    PASS(175). GATES (final tree): pre-bless suite reds EXACTLY the 3
    pre-registered IRGOLD-DIVERGE (demand_tc h.opt + ir.opt, symrec
    ir.opt; zero stdout/oracle/eqgate reds anywhere) → bless (3 goldens
    changed, 8 sibling rewrites byte-identical no-ops; sources
    byte-verified same-as-converged BEFORE bless) → SUITE PASS(175) ×3 +
    post-fix; ANSWER IDENTITY: the whole corpus's stdout goldens stand
    unchanged under the folded compiler (the 175×4 suite IS the answer
    A/B) + data/ 144 rows behavior-identical vs frozen a7dde012 (exit
    codes + diagnostics; evm_array_parse identical-SIGABRT stands); ctest
    5/5 debug (MiniDisassembler/PointsTo re-verify real join programs
    end-to-end) + 5/5 ASAN; ASAN BOTH surfaces SUITE PASS(175) zero
    reports; config-invariance SINGLE-HASH ×4 surfaces (3-run debug +
    release on tc ir/h + symrec ir/h); Q5 progsize@128 release
    SAME-SESSION INTERLEAVED ABABAB A warm {161,160} vs B {166,162,162}
    ms (~0.6% median, noise; A1 cold discarded; progsize headers
    byte-identical — a chain program, no joins) + the tc_random engine
    A/B (per-epoch medians ~200.5µs vs ~200.7µs, noise; its header ALSO
    byte-identical — tc_random's joins are DELTA-path, outside this
    fold's eager surface, itself confirming the churn analysis; the
    fold's eager-join engine win has NO bench flagship carrier — honest
    note, ADJ-S8 MEASURED). RESIDUALS: Fold B NEXT (cf16_2 the ruled
    witness — the only multi-column side_key_eq carrier; its irgold
    seeding is the gate precondition, rfinal-design.md §2); the
    R1..R-E42 residual set carries unchanged. NEXT: Fold B, then the
    emission op, the flip, the rename — per rfinal-design.md §2-§5.
    [EXECUTED — §20(X).]

(X) R-FINAL SLICE 2 — FOLD B LANDED (2026-07-27): side_key_eqs RETIRED —
    the differential join sections emit NO per-side key-equality
    conjuncts (codegen-only; Database.cpp: the vector decl + comment, the
    key_eq build loop, and the `side_key_eqs[i] << " && "` conjunct
    prefix all deleted; indexed_cols/side_reads/scan arms stay). The
    ruled R5 ADOPT-gated-on-witness: cf16_2's h.opt pin was SEEDED
    PRE-FOLD in its own commit (RAT-8; red EXACTLY IRGOLD-MISSING; the
    only corpus carrier of the MULTI-COLUMN crossed-key conjunction —
    r53_1.c1==v54 && r53_1.f==v55 against the swapped Find({v55,v54})
    key), so the fold landed as a reviewed IRGOLD-DIVERGE re-bless.
    Fold B also LIFTS the Table.h contract NOTE to full join generality
    (body + sections) and updates both Program.h "sections still
    conjoin" clauses — the Fable-review-[0]-anticipated riders.
    STAGE-(d) THREE-WAY (compact — the design §2 carried the after-shape):
    ORCHESTRATOR-authored dump-blind predictions (the 3-cut source diff;
    cf16_2 = EXACTLY 2 predicate lines rewritten in place, both post-fold
    lines given VERBATIM; churn EXACTLY 28/175; the one pre-bless red) ==
    blind worktree prototype (Database.cpp only; churn 28 EXACT — the
    lane itself caught a stale-baseline confound, first count 85 =
    FoldA+B compound, rebuilt the Fold-A-only reference; e2e cf16_2 4/4
    modes + oracle + monotone identical) == pristine implementation —
    cf16_2's header BYTE-IDENTICAL, the author's predicted lines matched
    VERBATIM (orchestrator cmp, E-77). FABLE REVIEW (13-agent, high): 4
    findings, ZERO live correctness in the fold — [0] the rel-arch
    "anywhere" claim OVERBROAD (EmitTableScan's full-scan fallback
    re-checks are LOAD-BEARING — never fold them; sentence re-scoped to
    join emission); [1] the design F3 corner-model WRONG (first-match
    pivot_for_col: the old belt emitted the first pivot's equality TWICE
    and never protected the duplicate-column corner — reverting Fold B
    would NOT restore protection; design corrected in place); [2] the
    foldB-F3 invariant made STRUCTURAL — a new always-on JOIN-KEY-DUP
    fprintf+abort in EmitJoin (indexed key columns pairwise distinct per
    side; survives NDEBUG); [3] BuildMaybeScanPartial's TUPLECMP scan
    belt DISCOVERED as the same probe-redundant pattern — the FOLD C
    candidate, its own witness + gate family owed (the "slightly faulty"
    comment replaced with the honest contract + deferral note). Fixes
    proven emission-NEUTRAL (21/21 pinned surfaces byte-identical
    post-fix) + post-fix SUITE PASS(175). GATES (final tree): pre-bless
    red EXACTLY the 1 pre-registered IRGOLD-DIVERGE → bless (source
    byte-verified same-as-converged; 3 sibling rewrites no-ops) → SUITE
    PASS(175) ×3 + post-fix; churn EXACTLY 28/175 delta-carrying .h-only
    (133/161 compiled byte-identical, 14 diagnostics headerless); data/
    144 rows behavior-identical vs frozen ea72d97a; ctest 5/5 debug +
    5/5 ASAN; ASAN BOTH surfaces SUITE PASS(175) zero reports;
    config-invariance SINGLE-HASH (cf16_2 h, 3-run debug + release); Q5
    progsize@128 ABABAB ~0.7% noise + tc_random engine medians 0.0-0.1%
    noise — tc_random's header BYTE-IDENTICAL under Fold B too (its
    joins are INDUCTIVE round-shell emission, not the acyclic
    differential sections): NEITHER fold has a bench flagship carrier
    (honest ADJ-S8 note; an acyclic-differential-join workload is the
    future witness). RESIDUALS OPENED: Fold C (the scan-partial belt) +
    the standing set. NEXT: the per-join EMISSION op (kJoinEmit/
    kProductEmit per rfinal-design.md §3 as amended emit-HIGH-1/2 +
    MED-1/2 — the E-71 grammar lane for form=/order=/seq= rules
    pre-code), then the flip SD-1..SD-4, then the rename.
    [EXECUTED — §20(Y).]

(Y) R-FINAL SLICE 3 — THE PER-JOIN EMISSION OPS LANDED (2026-07-27):
    kJoinEmit(27)/kProductEmit(28), census 27→29 — the ADJ-RJ-14 debt
    DISCHARGED: the once-per-join deferred TABLEJOIN emission is a
    MODELED DR-IR op. The ruled R3 shape as amended (emit-HIGH-1/2,
    MED-1/2, NOTE-1/2): referent = one op per (proc, join_view,
    form∈{eager,delta}) — 1:1 with a BuildJoin call; payload
    emit_join_view/form/order_key/walk_seq/stratum, where walk_seq is
    the NEW WorkItem base-ctor counter (Context::work_item_seq — the
    drain-order tie-break ContinueJoinOrder LACKS, one edit covering
    all six emplace sites) and the eager record stream
    (Context::emitted_join_events) is captured at work-item CREATION
    (view_to_join_action dedups → once per (proc, view)), replayed
    into eager ops AFTER INGEST_LOOP (every prior op.N byte-stable);
    the DELTA form enrolls in its OWN step AFTER DeriveDRStrata (real
    join_stratum, wired into DROpStratum) filtered by the file-scope
    AllSidesSameScc shared with the lowering skip; V-JOIN-EMIT-XCHECK
    Site 5 = a closing multiset block; CARVE-3 (TABLEINDEX outside the
    id contract — GetOrCreateIndex reuse is emission-order-sensitive,
    a false constant); LowerJoinEmit wraps the UNTOUCHED BuildJoin at
    BOTH callers (eager Join.cpp + delta Stratum.cpp:1532 — M13
    discharged), LowerProductEmit is the Site-5 RECORDER at the inline
    TABLEPRODUCT mint; render = form= header + args table= (E-107,
    ctor-stored via ModelTableOrNull — the dump has no impl) +
    order=/seq= EAGER-ONLY. EMISSION BYTE-IDENTITY was the hard gate:
    175/175 generated headers byte-identical vs frozen 6107fd3d.
    STAGE-(d) THREE-WAY (both lanes xhigh): the dump-blind author
    hand-predicted ALL ELEVEN pins' census tails EXACT + the 7
    block-gaining carriers' op.N labels EXACT (join_1 op.20/21,
    optimize_2 op.18, demand_tc op.24-28, symrec op.13/14, booleans
    op.15/16, elim op.12, d5 op.60 the DELTA carrier — the
    differential path's first modeled emission event) with an honest
    E-F ledger naming the only empirical-fill integers == the blind
    worktree prototype (full implementation +465/−22 across 8 files;
    21-surface regen EXACTLY 11 DIVERGE + all h/ir/df identical;
    175-case .h A/B 0-diverged; census==blocks per pin; cross-knob ×3;
    one self-caught linkage fix; four design-truth adaptations all
    adjudicated consistent) == the pristine implementation — 11/11
    dumps BYTE-IDENTICAL (orchestrator cmp, E-77). FABLE REVIEW
    (14-agent, high): 11 findings → SEVEN root causes, NONE a live
    miscompile, ALL FIXED PRE-COMMIT: [0] the Site-5 closing block was
    DEAD on monotone-only programs (the pre-existing no-phase-work
    early return — the check now runs on BOTH BuildStratumPhases
    exits); [1] Product.cpp's appends-empty bail converted to the
    always-on loud abort (the ctor-recorded event made the old silent
    NDEBUG no-op an enrolled-but-never-emitted hazard — abort at the
    cause, not the cross-check); [2] the per-op ctor-contract
    validator arm added for both kinds (the kIngestLoop precedent);
    [3] the delta Site-5 key gains the join VIEW's identity
    (model-sharing distinct-but-equal views at one stratum keyed
    apart; pointer-derived is safe — never rendered, in-process
    multiset only); [4] the CLAUDE.md re-point applied (census 29, the
    deferral modeled); [5] order= made EAGER-ONLY (the delta form's
    order=0 was meaningless — d5's golden re-blessed, EXACTLY the
    predicted one-line diff); [6] the copy-paste kProductEmit render
    arm collapsed into the shared case. Post-fix: suite red EXACTLY
    the predicted d5 IRGOLD-DIVERGE → re-bless (byte-verified) →
    SUITE PASS(175); ASAN REBUILT + BOTH surfaces PASS(175) + ctest
    5/5 (the pre-rebuild spot run's stale red identified and
    discarded). GATES (final tree): E-62 re-grep CLEAN (LIVE); pre-
    bless reds EXACTLY the eleven .deltarel IRGOLD-DIVERGE → bless
    (UNFILTERED; git-verified EXACTLY the 11 sanctioned goldens
    changed, 276 rewrites byte-identical no-ops; sources 11/11
    byte-verified) → PASS(175) ×3 + post-fix; emission byte-identity
    175/175 + data/ 144 rows behavior-identical vs frozen 6107fd3d;
    ctest 5/5 debug + 5/5 ASAN ×2; config-invariance SINGLE-HASH
    (join_1 + d5, 3-run debug + release); Q5 progsize@128 ABABAB ~1%
    noise. RESIDUALS: the standing set + Fold C. NEXT: the flip
    SD-1..SD-4 (rfinal-design.md §4 as amended flip-HIGH-1 — the
    constant-TUPLE second SET root), then the rename (§5, NARROW +
    .rel lockstep). [EXECUTED — §20(Z).]

(Z) R-FINAL SLICE 4 — THE DIRECTION FLIP LANDED (2026-07-27): the
    §19(H) SEAM DELETION is substantively COMPLETE — SD-1
    IsCutSuccessorDR is the ONE cut authority (S2's replication
    retired; the walk and the DR derivation call the same function);
    SD-2 ClassifyEagerSink/MessageOfInsertOrNull relocated DR-side
    (the ADJ-S4 replica retired; enrollment is their sole caller);
    SD-3 the FLIP CORE — RecordEagerDispatch, emitted_eager_ops, the
    nine LowerRelStep_* wrappers, the walk-side MakeEager*Op mints,
    and the EAGER_WEB replay switch ALL RETIRED; BuildDREagerInventory
    derives the eager marker SET flow-side (roots = receive
    successors + all-constant TUPLEs per flip-HIGH-1, cut at the one
    authority, transparent through inductive merges) and enrolls in
    (Depth, DeterministicOrder) — THE DUMP ORDER IS NOW A PURE GRAPH
    FUNCTION, the work-list schedule retired from the observable
    surface; the kJoinEmit event capture + INGEST_LOOP/JOIN_EMIT
    enrollment + §7d (KEPT this slice per the ruling) + A.6(c) +
    key_of survive; SD-4 the always-on SET-agreement oracle
    (ADJ-S12 discharged at the set layer), proven LIVE by worktree
    perturbation (drop the constant root → exit-134 with the
    SD-4/ADJ-S12 abort text on elim-cond-cycle-simple → revert →
    green). COUNTED-CONSUME, honestly recorded: the ruled R2(d)
    protocol is realized as a CONSTRUCTION-PAIRED census — the
    Fable-review-hardened CensusEagerMarkerAndBuild couples every
    census increment to its region-builder call in one helper, and
    enrollment reads that census for multiplicity; post-flip no
    independent second count source exists to check at runtime (the
    SET has the SD-4 oracle; the COUNT has the pairing + the eleven
    pins). ALL ELEVEN pins re-blessed under the ruled STRUCT gate:
    emission byte-identity 642/642 across ALL FOUR modes (worktree)
    + 0/175 (pristine opt) vs frozen d3f5746a; census-line
    byte-equality ×11; the per-band order-free multiset referee
    CONTENT-DIVERGE=0, EMPIRICALLY 7 pure-relabel / 4 band-reorder —
    ADJUDICATED over the a3 4/7 coarse estimate (the band's
    (table_id, oi) sort hides enrollment-order changes after
    canonical relabel; prediction granularity, not correctness).
    STAGE-(d) THREE-WAY: the dump-blind author (four substrate facts
    F-KEY/F-SORT/F-COUNT/F-ORDER; a FULL map_3 hand-derivation whose
    block sequence is byte-identical post-relabel) == the blind
    worktree prototype (4 files +376/−294; per-sub-diff BYTE gates;
    the perturbation-liveness proof) == pristine, 11/11 dumps
    BYTE-IDENTICAL (orchestrator cmp, E-77). FABLE REVIEW (26-agent,
    high): 10 findings, none a live miscompile — [0] the count-copy-
    not-counted-consume gap FIXED by the pairing helper (+ the honest
    ledger framing above); [1] the duplicated all-constant-root
    predicate EXTRACTED (IsAllConstantTupleDR, the SD-1 one-authority
    pattern, both callers rewired); [2] the stale A.6(c)
    "declines-the-oracle" header REFRAMED; [3] the dead cross-TU
    ctor seam CLOSED (the nine MakeEager*Op decls + ModelTableOrNull
    removed from the header, definitions file-static — a walk-side
    re-mint is now a compile error); [4]-[8] retirement doc-drift
    fixed (the SD-2 "walk still calls them" falsehood, the
    MakeEagerNegateOp EAGER_WEB tail, the kIngestLoop/kJoinEmit
    membership phrasings, the M10 recount comment); [9] six
    Build.cpp:970→:969 off-by-one anchors fixed. INCIDENT (recorded):
    the first post-fix verification ran against a STALE binary (the
    seam-closing edit broke the build — ModelTableOrNull used before
    its now-static definition — and the green suite lines were
    invalid evidence); caught by reading the build output, fixed
    with a forward decl, and the ENTIRE post-fix battery re-run
    against the genuinely rebuilt binaries: 21/21 dump-neutral,
    SUITE PASS(175) debug + ASAN (rebuilt, 0 errors), ctest 5/5.
    GATES (final tree): E-62 re-grep CLEAN (LIVE); pre-bless reds
    EXACTLY the eleven .deltarel IRGOLD-DIVERGE → bless (11/11
    sources byte-verified same-as-converged; git-verified 11 goldens
    only) → SUITE PASS(175) ×3 + post-fix; emission byte-identity +
    data/ 0 diverged; ctest 5/5 debug + 5/5 ASAN; ASAN both surfaces
    PASS(175); config-invariance SINGLE-HASH (map_3 + join_1, 3-run
    debug + release); Q5 progsize@128 ABABAB A warm {147,147} vs B
    {149,149,150} ms (~1.4% median — within the session's observed
    same-binary variance band; MEASURED, ADJ-S8). SEAM ACCEPTANCE
    (§19(H)): S1 retired (R-E42-era hole contract now interior), S2
    retired (one cut authority), S3 interior (both Site-5 multisets
    are one-path invariants), S4 retired (R-E42) — the two-authority
    seam is DELETED; ONE mint+lower path; §7d retained as a
    belt-not-authority cross-check per the ruling. RESIDUALS: Fold C;
    the standing spelling/witness set. NEXT: the RENAME ritual (§5
    ruling R6 — NARROW + the .deltarel→.rel lockstep, its own
    commit, LAST). [EXECUTED — §20(AA).]

(AA) R-FINAL SLICE 5 — THE RENAME RITUAL LANDED (2026-07-27); THE
    R-FINAL PROGRAM IS COMPLETE. The ruled R6 NARROW scope + the
    dump-surface lockstep, ONE commit, ZERO semantic churn (proven,
    not asserted): lib/DeltaRel→lib/Rel (DeltaRel.{h,cpp}→Rel.{h,cpp};
    CMake target DeltaRel→Rel incl. the rename-HIGH-1 live
    target_include_directories line; lib/CMakeLists add_subdirectory;
    ControlFlow's dep); every #include "DeltaRel.h"→"Rel.h";
    tests/DeltaRelValidators→tests/RelValidators (all FIVE
    rename-MED-2 target refs; ctest NAME RelValidators); the flag
    -deltarel-out→-rel-out (clean break) + SetDeltaRelDumpStream→
    SetRelDumpStream + the sink identifier family (gRelDumpStream/
    DumpRelIfEnabled); THE LOCKSTEP: runall.sh:275 (flag + surface
    filename) + ELEVEN .irgold sidecar tokens 'deltarel opt'→'rel
    opt' + ELEVEN golden files git-mv'd *.deltarel.opt.golden→
    *.rel.opt.golden (content UNTOUCHED); the two rename-LOW-4
    pre-classified comment tokens fixed (InstanceStoreTest path,
    Induction.cpp .rel); CLAUDE.md live statements re-pointed.
    EXCLUSIONS held (verified by grep post-edit): -dr-out/gDRStream/
    dr_out ("Dr. Lojekyll" amalgamation) + dr_define_static_library
    untouched; DR* IDENTIFIERS deliberately retained (NARROW, not the
    ~970-site FULL sweep — LowerRelStep names already harmonized and
    are now retired anyway); the proposal docs + historical ledger
    prose untouched (journal, not renamed retroactively). DELIBERATE
    RESIDUAL (recorded): the in-dump `deltarel` header token (Format
    .cpp:402) KEPT — it is dump GRAMMAR, its respelling is an E-71
    question the R6 ruling did not reach; a future mini-diff may
    rule it. GATES: the rename proven CONTENT-EMPTY (old binary
    -deltarel-out vs new binary -rel-out: dumps + generated headers
    byte-identical on 6 carriers incl. the demand witness); SUITE
    PASS(175) with the renamed surface LIVE (irgold reads rel.opt
    goldens); ctest 5/5 debug (RelValidators passing under its new
    name) + 5/5 ASAN; ASAN BOTH surfaces PASS(175) (asan tree
    reconfigured for the moved dir); release reconfigured + built
    clean. THE R-FINAL CLOSE: all five ruled slices landed IN ONE
    SESSION under the full per-slice ritual — Fold A (§20(W)), Fold
    B (§20(X)), the emission ops (§20(Y)), the flip (§20(Z)), the
    rename (THIS) — the §19(H) acceptance is MET: the two-authority
    seam is deleted (S1/S2/S4 retired, S3 interior), ONE relational
    authority derives all flow reachability, every program's dump is
    proportional (census 29), the eager count/set oracle is live,
    and the library is named for what it is. RESIDUALS carried
    forward: Fold C (the scan-partial belt — own witness + gates);
    the in-dump header token; DS-R4-10; the spelling/witness set
    (neq/lt/gt, publish-*/message=, table-less kEagerUnion render,
    dead BuildNestedLoopJoin); F20/F24 record-only; HP-17 → D3.a.
    NEXT EPOCH candidates (owner re-ranks): D3.a (R-DIFF +
    multi-adornment, per OD-11 it follows Rel); the agent-substrate
    direction (the owner's 2026-07-27 design note + my composition
    assessment — Phase-0 V-series if it firms); Fold C; the
    pass-harness P2-P5 / §13 / §14 backlog.

(AB) EPOCH-OPEN RE-VERIFICATION RECORD (2026-07-28, tip 4d3ae315; the
    session-open items (0)+(1) EXECUTED per the §20(AA) close + the §7
    SINGLE-PASS banner).
    (0) Frozen A/B baselines RE-SNAPSHOTTED from tip into the session
        scratchpad BEFORE any code (debug 11490b83… / release
        e8809f6e…; both presets rebuilt clean, error-grep 0) — the
        prior session's snapshots gone with its scratchpad. ASAN
        cadence stands per-diff (§19(F)/(J)); the tip was docs-only
        atop 19f16652, whose both-surface sweeps are the standing
        green. [Post-errata the snapshots were REFRESHED from the
        errata commit — see the gates paragraph below.]
    (1) §7 (rel-arch-pseudocode.md:1313-1459) + §20(W)-(AA) + the
        three rfinal contracts FLEET-RE-VERIFIED (house precedent):
        3 seed-UNREAD opus derivation lanes (the post-flip
        one-authority pipeline end to end; the kJoinEmit/kProductEmit
        emission layer; the dump/census/key_of + codegen contracts at
        census 29) + 3 seed-read adversarial verifiers (§7;
        §20(W)/(X)/(Y) + rfinal-design §1-§3/§6; §20(Z)/(AA) +
        rfinal-design §4-§5 + ruling brief + desired-states) + 1
        sonnet mechanical lane + 1 xhigh consolidator (8 agents,
        ~739k tokens; every candidate adjudicated AT the code; zero
        lane deaths). The E-62 tripwire re-grepped by the
        ORCHESTRATOR personally, TWICE (pre- and post-errata): CLEAN
        (sole out-of-lib hits = the standing Stratum.cpp:1073 comment
        + the RAT-3 InstanceOrderTest fixture, now under
        tests/RelValidators). The carrier-golden referee EXECUTED by
        the orchestrator personally: ALL TWENTY pinned golden
        surfaces regenerate BYTE-IDENTICAL at tip (11 rel + 5 df +
        2 h + 2 ir; the 14 .irgold sidecars the count authority) —
        and the MECH lane's independent second regen agreed 20/20,
        plus 11/11 census-29 lines byte-identical (all carrying
        kIngestLoop=/kJoinEmit=), per-carrier op-line counts ==
        census summaries (154 counts, 0 mismatch), map_3 hexdump
        pins (c2 b7 middle dot, cmp=eq, functor=add_i32/3), the
        cross-knob law opt==nocf / nodf==none on all 11 rel
        carriers (33 extra compiles rc=0), d5's op.60 form=delta
        stratum=6 with NO order=/seq=, and demand_tc's 4/8
        table=-carrying kEagerJoin split (2 x %table:4 + 2 x
        %table:15). VERDICT: SOUND-WITH-ERRATA — ZERO behavior
        defects, ZERO false live-section claims; §7's every anchor
        and §20(W)-(AA)'s every as-landed claim held. FOUR errata,
        ALL cosmetic CODE-TEXT residuals of the NARROW rename /
        stale comments (not doc falsities), applied in place:
          E-138 LOW  Main.cpp: the -rel-out arm's LOCAL FileStream
                     still spelled deltarel_out — renamed rel_out
                     (4 sites :284/:374/:375/:379); flag/help/
                     gRelStream/SetRelDumpStream were already clean.
          E-139 LOW  tests/RelValidators/InstanceOrderTest.cpp: the
                     two DrTest TEST(DeltaRelValidators, ...) suite
                     labels -> TEST(RelValidators, ...) (:87/:95);
                     dir/CMake target/ctest NAME were already clean.
          E-140 COSM Stratum.cpp:1451 comment: AllSidesSameScc is
                     HEADER-SHARED (Rel.h:1143 decl, cross-TU
                     callers), not "file-scope" — comment respelled;
                     the one-authority substance holds.
          E-141 COSM Rel.cpp:1680 comment: the all-constant root
                     dispatch is Procedure.cpp:823 (":826" was
                     3-line drift; NOT a §7 anchor).
        REFUTED (grounds in the consolidation record): the
        gDeltaRelStream-vs-gRelDumpStream doc conflict (two DISTINCT
        identifiers — Main.cpp driver-local gRelStream vs the
        lib-side gRelDumpStream/DumpRelIfEnabled family — both
        landed, neither doc misstates its object); the dead
        census-free second BuildEagerUnionRegion caller as a
        one-authority falsity (§7 claims pairing-indivisibility and
        merge-transparency, not sole-callership; the Induction.cpp
        :1005 caller is provably DEAD behind NeedsInductionCycle-
        Vector's unconditional true and honestly labeled in-code);
        the census totality-guard message nit (the ordinal lives in
        a comment, never the emitted string). RECORD-ONLY forward
        notes: (i) D3.a INPUT — SD-4 set-oracle COMPLETENESS rests
        on NeedsInductionCycleVector's unconditional-true merge
        short-circuit (Induction.cpp:12-13): if that TODO is ever
        relaxed for R-DIFF, the union region on an induction-owning
        merge must be MODELED BEFORE the relax (else an emitted
        region the .rel dump cannot show, and SD-4 stays blind —
        both sides exclude induction-owning merges); (ii) the COUNT
        dimension has no independent runtime oracle by design
        (§20(Z) honest framing re-confirmed; construction pairing +
        Site-5 + emission A/B are the nets); (iii) the DELTAREL-
        DUMP:/DELTAREL: stderr diagnostic prefixes + proposal-doc
        name refs in code comments are deliberate NARROW-rename
        residue (unranked hygiene). GATES on the errata diff (all
        run against genuinely rebuilt binaries, error-grep 0):
        20/20 pinned surfaces DUMP-NEUTRAL post-fix; SUITE PASS(175)
        (irgold + eqgate live); ctest 5/5 debug + 5/5 ASAN (asan
        tree rebuilt); E-62 re-grep CLEAN (live — the Rel.cpp edit
        is a comment); frozen A/B snapshots refreshed from the
        errata commit. MID-SESSION OWNER IDEATION (recorded, its own
        proposal doc, same commit): @ephemeral on #foreign types —
        values that MUST NOT be persisted (re-run the functor to
        read); docs/proposals/EphemeralTypes.md is the binding
        record (two readings kept separate; type-level primary —
        dissolves taint propagation; the equality/hash exemption;
        the reject-only V-EPHEMERAL slice-1; the @declassify knob;
        placement: agent-substrate strongest synergy, WASM manifest
        slot, recompute-on-read sequenced after D3.a; NOT ranked —
        a mini-diff-class candidate note for the re-rank). Full
        fleet record: session scratchpad fleet-newopen/ (disposable;
        THIS entry is binding). Errata continue at E-142. NEXT: the
        epoch re-rank — an OWNER RULING at the head (candidates per
        §7's path-forward block: D3.a R-DIFF+multi-adornment; the
        agent-substrate direction; Fold C; the header-token E-71
        mini-diff; P2-P5/§13/§14; + the EphemeralTypes.md candidate
        note) — brief presented in-session, code waits on the
        ruling. [RULED — §20(AC).]

(AC) OD-14 — THE EPOCH RE-RANK RULING (owner, 2026-07-28, in-session)
    + THE HEADER-TOKEN E-71 MINI-DIFF LANDED. THE RULING: "ratify
    D3.a with the header-token mini-diff riding along" — D3.a
    (R-DIFF + multi-adornment, the OD-11-ranked follower) is the
    next epoch; the in-dump header-token respell rides along as its
    own pre-epoch commit; the alternatives (agent-substrate Phase-0,
    Fold C, P2-P5/§13/§14) tabled per the brief; agent-substrate
    Phase-0 remains available as a bounded measurement fleet at
    any seam without opening an epoch; @ephemeral (EphemeralTypes
    .md) remains a recorded mini-diff-class candidate note whose
    recompute half sequences after D3.a. THE MINI-DIFF (this
    commit): the §20(AA) DELIBERATE RESIDUAL discharged — the
    in-dump header token `deltarel` -> `rel` (lib/Rel/Format.cpp
    :402ff, the E-71 grammar ruling made BY the OD-14 ratification;
    the token now matches the -rel-out flag + the .rel golden
    surface; the sole live emitter site, zero readers parse it —
    grep-verified). GATES: pre-bless suite red EXACTLY the eleven
    pre-registered `irgold rel.opt IRGOLD-DIVERGE` (nothing else
    red across 175 cases), every produced dump differing from its
    golden by EXACTLY the line-1 header (orchestrator per-golden
    diff, 11/11 one-line) -> FILTERED bless (git-verified EXACTLY
    the 11 sanctioned goldens changed, 11 insertions/11 deletions;
    the in-filter stdout/oracle/monotone/h/ir/df rewrites all
    byte-identical no-ops) -> SUITE PASS(175); emission UNCHANGED
    (dump-only: the h/ir/df pins passed in BOTH suite runs —
    generated headers byte-identical, so Q5 is VACUOUS-BY-BYTE-
    IDENTITY and was deliberately not run, recorded not asserted);
    ctest 5/5 debug + 5/5 ASAN (debug/release/asan trees all
    rebuilt, error-grep 0); config-invariance SINGLE-HASH x2
    carriers (map_3 + join_1, 3-run debug + release); E-62 re-grep
    CLEAN (live — same 5 sanctioned hits). Frozen A/B baselines
    refreshed from THIS commit in the session scratchpad. NEXT:
    D3.a OPENS under the full per-slice ritual — stage (a)
    pseudocode build-out of the InstanceStore / instance-op /
    demand-admission subsystem + the differential machinery it
    activates (seed-unread derivation lanes, the §7 idiom), with
    the §20(AB) forward note (NeedsInductionCycleVector) + the
    standing D3.a inputs (HP-17, RAT-7 re-open, N-1, OWN-3,
    DS-R4-10, -demand-retract, >1 adornment) as the charter seed.
    [EXECUTED — §20(AD).]

(AD) D3.a STAGE (a) COMPLETE (2026-07-28, tip 428dae76): THE
    SUBSTRATE PSEUDOCODE COMMITTED —
    KeyedInstances.artifacts/d3a-substrate.md (the §7-idiom
    whole-program map of the InstanceStore / instance-op /
    demand-admission / differential subsystems as landed, every
    load-bearing statement file:line-anchored at tip). Produced by
    a 4-lane seed-UNREAD derivation fleet + xhigh seed-READ
    consolidator (~683k tokens, zero deaths); the orchestrator
    personally re-verified the five most load-bearing anchors at
    code pre-commit. TWO CONTRACT-VS-CODE DRIFTS ADJUDICATED (code
    is authority, both aspirational-R-DIFF prose that never
    landed): XC-1 — the band-(b) (T,F) drop scan is ABSENT from
    EmitSubgraphInstance (not "emitted inert" as d2b-design
    :722-727/:826 reads); XC-2 — the SUBGRAPHINSTANCE region is
    minted TWO-ARG (Procedure.cpp:284; d2b-design:689-690's third
    TableIsDifferential ctor arg is UNLANDED — the region does not
    know its diff-ness). HEADLINE STRUCTURAL FACTS the map
    establishes: the store's algebra slot is FIXED monotone
    (Table<RowT>, no signed nested representation);
    RecycleCurrent — the whole-instance retraction primitive — is
    BUILT AND UNIT-PINNED with ZERO codegen callers; a minted
    kInstanceDeath today is FATAL-not-inert (the lowering enrolls
    only instantiate+seal, so V-INST-EMITTED multiset-aborts);
    DeathEffects assumes a demand kNetRemoval frontier NOTHING
    provisions or validates; the whole demand chain is monotone by
    construction (fabricated message never @differential, injector
    mints no del_vec); V-INST-SOLE FORBIDS a differential input
    outright. §5 = the 17-gap ledger (G-DIFF-TABLE, G-ROW-RETRACT,
    G-OCCUPANCY/N-1, G-BELT-FLIP, G-TF-PUBLISH, G-DIFF-REGION,
    G-DEATH-LOWER, G-DEMAND-NEG, G-RETRACT, G-INPUT-NEG, G-STALE,
    G-REBIRTH, G-FRESH-BELT, G-OWN3, G-NEVER-DIFF,
    G-INDUCTION-UNION, G-ADORN). §6 = the TWELVE ritual-head open
    questions (OQ-MODEL the pivotal rescan-vs-incremental fork on
    which every other OQ is contingent; OQ-AXES demand-vs-content
    differentialness; OQ-RETRACT-POLICY ref-counted vs set-demand
    + batch netting; OQ-N1; OQ-BELT the HP-7 replacement; OQ-
    PUBLISH-ORDER; OQ-DEATH-VS-REBUILD; OQ-INPUT; OQ-ADORN-KEY;
    OQ-OWN3 mechanism; OQ-NEVER reject-vs-promote; OQ-INDUCTION-
    UNION). NEXT: the D3.a RITUAL-HEAD RULING BRIEF (the twelve
    OQs with recommendations) — an OWNER RULING; stage (b) design
    lanes open only under the ratified rulings. [RULED — §20(AE).]

(AE) OD-15 — THE D3.a RITUAL-HEAD RULINGS RATIFIED (owner,
    2026-07-28, in-session): "ratify all" — the twelve §6 OQs +
    the sub-slice order, AS RECOMMENDED; binding record =
    KeyedInstances.artifacts/d3a-ruling-brief.md (ratification
    record at tail). THE FRAME: OQ-MODEL = FULL-RESCAN (the store
    stays a predicate-free set island; differentialness = the
    frozen-vs-current DIFF AT PUBLISH; D3.a is a pub-boundary +
    lifecycle problem — moots G-DIFF-TABLE/G-ROW-RETRACT/
    G-OCCUPANCY, N-1 CLOSES moot-under-rescan with a recorded
    contingency). The rest in one line each: two staged axes
    (demand D3.a.1 / input D3.a.2); SET-demand + batch SET
    netting via the fabricated message going @differential (the
    injector's del_vec arm exists); RAT-7 YES — the band-(b)
    partition belt lands WITH the (T,F) scan (born+carried==cur,
    dropped+carried==frz), HP-7 stays armed for monotone stores;
    publish order = signed deltas into pub's own machinery, drop
    scan before born scan per iid; death = full (T,F) retract +
    RecycleCurrent with the PINNED three-way coupling (netting
    kills same-batch flap; TouchedFlag suppresses dead-key a2;
    V-INST-FRESH unchanged); differential input YES at D3.a.2
    (FENCE (iii) + V-INST-SOLE lift; removal-triggered
    Recycle+rescan; G-STALE subsumed); multi-adornment = N
    disjoint stores keyed (query, BindingPattern), the pass loops
    per adornment; OWN-3 promotes always-on (record-comparing
    abort + census equation) as D3.a.0 hygiene; DS-R4-10 = REJECT
    fence + witness; recursive demand DEFERRED all of D3.a (the
    §20(AB) precondition binds any future toucher). SUB-SLICE
    ORDER: D3.a.0 (OWN-3 + inert diff-selector plumbing) ->
    D3.a.1 (differential demand: -demand-retract, death ON, (T,F)
    scan, partition belt, demand net-removals frontier) -> D3.a.2
    (differential input) -> D3.a.3 (multi-adornment); each under
    the full per-slice ritual; the eqgate's flat==nested answer
    identity is the standing cross-lowering oracle (flat -demand
    is already differential-capable). NEXT: stage (b) design
    lanes for D3.a.0 under these rulings. [EXECUTED — §20(AF).]

(AF) D3.a.0 STAGES (b)/(c) COMPLETE (2026-07-28, tip 42100428):
    THE BINDING DESIGN COMMITTED —
    KeyedInstances.artifacts/d3a0-design.md. Fleet: 2 xhigh design
    lanes (b1 OWN-3 promotion; b2 inert diff-selector plumbing) +
    2 FRESH adversarial critics + 1 xhigh adjudicator (~555k
    tokens; 12 findings CONFIRMED and folded, 1 not-material,
    ZERO OD-15 escalations); the orchestrator re-verified every
    load-bearing anchor at code before commit. SUB-DIFF (i) OWN-3:
    View.cpp:588's dormant assert becomes the always-on
    record-comparing CheckGuardAnnotationFold (pure free function,
    RAT-3 CheckInstanceOrder idiom; compatibility predicate =
    forcing_index + instance_key EQUAL — the two Equals-INVARIANT
    identity fields; site stamps kind/demand_side/role
    legitimately differ across a valid fold); mechanism (B) = a
    QueryImpl* back-pointer on QueryViewImpl under INV-OWN3-Q
    (non-null iff annotated; propagated on move, cleared paired;
    always-on null-guard before the record deref — the MED-2
    NDEBUG-SIGSEGV catch); guard_annotation_folded_count gains its
    SOLE writer (the compatible-fold arm, dormant on corpus); the
    Demand.cpp census promotes always-on IN PLACE with the
    dead-flow-orphan question RESOLVED AT CODE (dead-flow deletes
    annotated views ONLY post-Optimize; the census is pinned
    PRE-Optimize at Build.cpp:2587-before-2599 — never relocate
    without orphan accounting); NEW negative test =
    tests/DataFlowValidators/GuardAnnotationFoldTest.cpp
    (fork/waitpid SIGABRT death arm + positive arm; the LOW-1
    catch: {}-init makes both records compatible — the death arm
    MUST set forcing_index explicitly). SUB-DIFF (ii) plumbing:
    ONE authority DRInstance.differential =
    TableIsDifferential(pub) stamped at the mint (Rel.cpp:1058),
    feeding the SUBGRAPHINSTANCE region ctor (third arg;
    Hash/Equals UNTOUCHED — proven safe: the bit is a pure
    function of pub_table which Equals already keys on) + the
    ProgramInstanceStore descriptor + a new always-on
    V-INST-DIFF-COHERENCE (stamped bit == live predicate, at the
    lowering); the store-ctor emitter appends ", false" ONLY when
    differential (emit-when-differential CHOSEN over
    always-explicit: the eqgate nested arm's generated text stays
    byte-for-byte tip); ZERO render change (an .ir token would be
    an E-71 lane — out of scope). CROSS-SLICE (§3): the L3
    predicate rider is FIRST-CLASS for D3.a.1 (store bit keys
    pub-diff, death mint keys demand-diff — reconcile before the
    bit goes live); coherence/death-test liveness by perturbation
    lands with D3.a.1. GATES pre-registered (§4): expected reds
    NONE, zero golden churn, [BYTE] on every pinned surface (both
    sub-diffs), eqgate nested-arm generated-text delta EXACTLY
    ZERO, suite PASS(175) debug+release+ASAN (release newly runs
    the always-on aborts — must pass silently), ctest +1 (the new
    DataFlowValidators death test). NEXT: stage (d) — compact
    desired-states + the BLIND worktree prototype (hand-
    provisioned, tip-verified) converging with the pristine
    implementation; then Fable review; then land as ONE commit.
    [EXECUTED — §20(AG).]

(AG) D3.a.0 LANDED (2026-07-28) — THE FIRST D3.a SLICE: the OWN-3
    promotion + the inert diff-selector plumbing, per d3a0-design.md
    as amended by the Fable review; desired-states record =
    d3a-desired-states.md SLICE 0. MECHANISM AS LANDED: (i) OWN-3 —
    GuardAnnotationsCompatible/CheckGuardAnnotationFold pure free
    functions (View.cpp; predicate = forcing_index + instance_key,
    the Equals-invariant identity fields; record-printing
    fprintf+abort, survives NDEBUG); the QueryImpl* back-pointer on
    QueryViewImpl under INV-OWN3-Q (stamped at both Demand.cpp
    sites, propagated on move, cleared paired; BOTH halves always-on
    — the loser-side null guard AND the review-[2] survivor-side
    owner guard); guard_annotation_folded_count gains its sole
    writer; the Demand.cpp census promoted always-on PINNED
    PRE-Optimize; NEW tests/DataFlowValidators (ctest 5→6,
    fork/waitpid death arm SIGABRT + positive arm, EINTR-safe per
    review [3]). (ii) plumbing — DRInstance.differential =
    TableIsDifferential(pub) stamped at mint; the SUBGRAPHINSTANCE
    region ctor third arg (Hash/Equals untouched); the
    ProgramInstanceStore descriptor bit + IsDifferential() accessor;
    always-on V-INST-DIFF-COHERENCE at the lowering; the store-ctor
    emitter appends ", false" ONLY when differential. STAGE-(d)
    THREE-WAY: design §4 pre-registered predictions == blind
    worktree prototype (hand-provisioned at 118d723a, own build,
    zero substantive deviations) == pristine (patch applied VERBATIM
    after orchestrator spec review) — every prediction MATCHED incl.
    the EXACTLY-ZERO nested-arm generated-text delta. FABLE REVIEW
    (15-agent, high): 13 findings → 7 distinct, ZERO live
    miscompiles — [2] survivor-side guard + [3] waitpid + [4]/[6]
    doc comments FIXED pre-commit (proven dump-neutral post-fix);
    [5] REFUTED-as-designed (the §2.2 ctor-arg adjudication);
    [0]+[1] ACCEPTED AS THE LABELED PREDICATE RESIDUAL — a BINDING
    D3.a.3 PRECONDITION (in-code at the View.cpp predicate + here):
    before multi-guard folds first go live, re-derive the
    compatibility predicate against REAL fold shapes with directed
    witnesses in BOTH directions (survivorship: the surviving
    record's role is load-bearing in ResolveLiveRecognition —
    Rel.cpp:960 derives input_table only from kBody; invariance:
    proxy-TUPLE-migrated annotations may legally fold with differing
    instance_key — a false-abort hazard; the two findings pull
    opposite ways, which is why the re-derivation waits for
    witnesses). GATES (final tree, all against genuinely rebuilt
    binaries, error-grep 0 ×3 trees ×2 rounds): SUITE PASS(175) ×2
    (pre-fix + post-fix) with ZERO reds and ZERO golden churn; ctest
    6/6 debug + 6/6 ASAN ×2; nested-arm datalog.h BYTE-IDENTICAL to
    the pre-patch reference ×2 (+ prototype ×2); 20/20 pinned
    surfaces regen ×2 (+ prototype ×2 + the blind lane's own);
    E-62 CLEAN (5 sanctioned); config-invariance SINGLE-HASH
    (demand_tc .rel + .h, 3-run debug + release); Q5
    VACUOUS-BY-BYTE-IDENTITY (recorded, not asserted). RESIDUALS
    OPENED: the [0]/[1] predicate re-derivation (D3.a.3
    precondition); the §3 inheritance set stands (region bit goes
    live at D3.a.1; the L3 pub-diff-vs-demand-diff predicate
    reconciliation FIRST-CLASS at D3.a.1; coherence/death-test
    liveness by perturbation at D3.a.1). NEXT: D3.a.1 — differential
    demand (-demand-retract, kInstanceDeath ON + its lowering, the
    (T,F) scan, the RAT-7 partition belt, the demand net-removals
    frontier + V-INST-DRAIN extension) under the full per-slice
    ritual, stage (a) seeded by d3a-substrate.md §5's
    G-DEATH-LOWER/G-DEMAND-NEG/G-RETRACT/G-TF-PUBLISH/G-REBIRTH/
    G-FRESH-BELT + the OD-15 rulings.

(AH) D3.a.1-OPEN RE-VERIFICATION RECORD (2026-07-28, tip 0d33bdca;
    the §7 SINGLE-PASS banner discharged — the errata commit atop is
    docs+comments only). (0) Frozen A/B baselines RE-SNAPSHOTTED from
    tip into the session scratchpad BEFORE any code (debug + release +
    oracle binaries, nested-arm reference dumps SHA-256-pinned; all
    three trees rebuilt clean, error-grep 0 x2 rounds — pre- and
    post-errata); SUITE PASS(175) against the FROZEN A binary; ctest
    6/6 debug + 6/6 ASAN. (1) §7 + §20(AD)-(AG) + the d3a contracts
    FLEET-RE-VERIFIED (house precedent): 3 seed-UNREAD opus
    derivation lanes (OWN-3/INV-OWN3-Q + census; the diff-selector
    plumbing end to end; the death/retract substrate) + 2 seed-READ
    adversarial verifiers (§7+§20(AD)-(AG); the d3a0-design/ruling-
    brief/desired-states contracts) + 1 sonnet mechanical lane + 1
    xhigh consolidator (7 agents, ~614k tokens, zero deaths, zero
    truncations). The carrier-golden referee EXECUTED by the
    orchestrator personally: 20/20 pinned surfaces regenerate
    BYTE-IDENTICAL at tip (the 14 .irgold sidecars the count
    authority); the MECH lane's independent regen agreed 20/20, plus
    census-29 x 11 rel carriers (all kIngestLoop=/kJoinEmit=), the
    nested witness clean (kInstanceDeath=0, two-arg store ctor), the
    cross-knob law 11/11 (opt==nocf, nodf==none), ctest 6/6 both
    trees. The E-62 re-grep EXECUTED by the orchestrator personally:
    CLEAN — zero body_ops/output_ops readers outside lib/Rel/; the 5
    sanctioned pinned_order hits (Stratum.cpp:1073 comment + 4x the
    RAT-3 InstanceOrderTest fixture). VERDICT: SOUND-WITH-ERRATA —
    ZERO false claims (verifier A), ZERO code-deviations from the
    amended design (verifier B), three independent seed-unread maps
    converge on the doc structure; the D3.a.1 CHARTER IS UNCHANGED
    (no d1-d7 premise false; the L3 two-axis question stands exactly
    as posed — pub @Rel.cpp:1055/1059 vs demand @:1139). SIX ERRATA
    (LOW/COSM), applied by the orchestrator per the consolidator's
    exact edit specs, LINE-COUNT-PRESERVING in Rel.cpp so every
    standing doc anchor stays valid (verified: the :1139 gate, :2041
    mint call, :3996 census expect unmoved):
      E-142 LOW  Rel.cpp:1008-1018 stale "GATED OFF at D1.b /
                 unconditionally false" header above
                 BuildSubgraphInstanceOps — respelled live-since-D2.b
                 (the flag lands at Build.cpp:1468; mech proved the
                 body runs: kSubgraphInstantiate=1 on the witness).
      E-143 LOW  Rel.cpp:2036-2040 same stale D1.b prose at the mint
                 call site — respelled.
      E-144 LOW  Rel.cpp:3968-3974 same stale D1.b prose at the
                 census recount — respelled (recount re-derives
                 counts INDEPENDENTLY of the mint, the cross-check).
      E-145 COSM d3a0-design.md x3 (:349/:554/:658) death-gate
                 anchor Rel.cpp:1137 -> :1139 (load-bearing for the
                 D3.a.1 L3 rider).
      E-146 COSM d3a-substrate.md:247 "Rel.cpp:1137-1149" ->
                 ":1138-1150; the if at :1139" (§1-§6 pre-slice-0
                 anchor, fixed because D3.a.1-seeding).
      E-147 COSM d3a0-design.md:162 stale debug assert in the §1.3
                 block — [AS LANDED superseded by Fable fix [2], the
                 always-on survivor guard View.cpp:668-674] note
                 appended; historical block preserved.
    REFUTED (grounds in the consolidation record): the HP-7-belt
    "always-on" mismatch (the belt IS #ifndef NDEBUG at
    InstanceStore.h:177-194 and NO in-scope doc claims otherwise —
    "armed" is accurate; the always-on family is V-INST-*/OWN-3
    only, a d5/d7 perturbation-gate design note); the ruling-brief
    1137 citation (it contains none); the "ships INERT — HP-17" mint
    comment vs "FATAL-not-inert" (different antecedents: unminted-
    dormant vs counterfactual-mint, both true). ADVISORY for stage
    (b): lane 3's code-derived missing-pieces checklist M1-M6 (death
    lowering/enrollment closing V-INST-EMITTED; EmitInstanceDeath
    draining a demand kNetRemovals frontier + RecycleCurrent + the
    (T,F) drop scan; frontier provisioning + V-INST-DRAIN extension;
    monotone=false store + the N-1 contingency check; injector
    del_vec downstream completeness; the V-INST-FRESH same-epoch
    death+refire ordering) seeds the D3.a.1 design checklist; the
    death abort point is LATE (V-INST-EMITTED at CF lowering — every
    Rel.cpp validator passes a minted death) and NO pre-pass fence
    guards a differential demand message. GATES post-errata (rebuilt
    x3 trees, error-grep 0): SUITE PASS(175) zero churn; ctest 6/6
    debug + 6/6 ASAN; nested-arm datalog.h/.rel/.ir BYTE-IDENTICAL
    to the frozen pre-errata reference (comment-only proven
    dump-neutral); baselines refreshed from the errata commit. NEXT:
    D3.a.1 stage (a) — the slice-scoped pseudocode build-out (the
    retract channel end to end; the death path; the netting/
    TouchedFlag/V-INST-FRESH coupling), then the d2 L3 ritual-head
    question BEFORE stage-(b) lanes; the landing record will be
    §20(AI) (the §20(AG) NEXT said (AH); this record took the
    letter). [EXECUTED — §20(AI); the landing record shifts again,
    to §20(AJ).]

(AI) D3.a.1 STAGE (a) COMPLETE + THE d2/L3 RULING (2026-07-28, tip
    a87aad5f): THE SLICE SUBSTRATE COMMITTED —
    KeyedInstances.artifacts/d3a1-substrate.md (§1 retract channel /
    §2 death path / §3 instance runtime+band / §4 frontiers+pub
    interface+coupling timeline / §5 the 18-gap ledger mapped to
    d1-d7+M1-M6 / §6 the L3 fact base / §7 the d2 adjudication).
    Fleet: 3 seed-UNREAD opus lanes (retract channel; instance
    runtime+band; frontiers+netting) + fleet1's lane3-death map as a
    fourth lane (re-anchored at a87aad5f, no drift) + xhigh seed-READ
    consolidator (~513k tokens, zero deaths); the ORCHESTRATOR
    personally re-verified all ten load-bearing anchor families at
    code pre-commit. HEADLINE STRUCTURAL FACTS: the d1 toggle is ONE
    field (ParsedMessageImpl::differential_attribute, Parse.h:388,
    never set by fabrication); the retract DATA channel is plumbed
    end-to-end but WRITER-LESS (injector del_vec Build.cpp:416-440
    passed `// Empty.`, never a VECTORAPPEND target; handler NETBATCH
    Procedure.cpp:570-575 -> NetBatch Vec.h:176-218 = the OQ3 SET
    netting, engaged only when a remove side exists); XC-3 (the ONE
    substantive lane correction, both halves orchestrator-verified):
    under a naive d1 flip the FIRST abort is V-INST-DRAIN
    (Rel.cpp:4509-4512) — the eager boundary append is gated
    !TableIsDifferential (Build.cpp:999) so a differential demand
    table loses its kNetAdditions provisioning, and ValidateDROps
    (Stratum.cpp:2186) runs BEFORE the differential frontier vecs are
    minted in LowerDRFlow/LowerDRRounds (:2477/:2489) —
    V-INST-EMITTED is the SECOND abort; hence d1 IS NOT INDEPENDENTLY
    LANDABLE (co-design d1+d3(+d4) or interpose a temporary fence).
    THE d2/L3 RULING (substrate §7; adjudicated at code by the
    orchestrator, NO owner brief — the charter's escalation rule:
    §6 proves the two spellings extensionally EQUAL on accepted
    D3.a.1 programs, so no observable behavior turns on the choice):
    **CO-ACTIVATION — P-STORE stays TableIsDifferential(pub) as
    landed; the explicit disjunction REJECTED.** A @differential
    demand message makes TableIsDifferential(pub) TRUE under BOTH
    lowerings (the lib/DataFlow/Differential.cpp closure :56-65/
    :114-140; -demand-instance is ControlFlow-only, Main.cpp:69 vs
    :82, so the Query-graph bits are shared); the converse is fenced
    (FENCE (iii) + Demand.cpp:624-627 + impure-MAP). Grounds: D3.a.2
    divergence (diff input: P-STORE true/P-DEATH false) is handled
    CORRECTLY by the pub-keyed spelling with no edit; one-authority
    preserved (all three sinks read the one Rel.cpp:1055 `diff`
    local); V-INST-DIFF-COHERENCE keeps its designed stamp-vs-live
    meaning. BINDING for stage (b): the death gate KEEPS its own
    demand-keyed predicate (Rel.cpp:1139) — extensional equality is a
    THEOREM of the fence set, not an invariant; do NOT fold the two
    predicates into a shared helper. Also recorded: any published
    output over the demanded closure must itself be @differential or
    the DataFlow faithfulness check hard-errors (Differential.cpp
    :174-179) — a d6 witness-design constraint. Docs only; no gates
    beyond anchor verification (no code moved). NEXT: stage (b) —
    design lanes for the slice under this substrate + the ruling
    (suggested split per the session charter: b1 retract channel +
    SET netting; b2 death mint/lowering/enrollment + the demand
    kNetRemovals frontier + V-INST-DRAIN extension; b3 the (T,F)
    drop scan + RAT-7 partition belt + the selector going live; b4
    witness/fixture + the DS-R4-10 fence), honoring XC-3's
    co-landability constraint; then critics + adjudicator ->
    d3a1-design.md; landing record = §20(AJ). [EXECUTED — §20(AJ);
    the landing record shifts to §20(AK).]

(AJ) D3.a.1 STAGES (b)/(c) COMPLETE (2026-07-28, tip 95251825): THE
    BINDING DESIGN COMMITTED — d3a1-design.md + the four annexes
    d3a1-b{1,2,3,4}-design.md (the design binds where it amends; a
    lane's edit spec is adopted verbatim where it is silent). Fleet:
    4 xhigh design lanes + 4 FRESH adversarial critics (pipelined
    per lane) + 1 xhigh adjudicator (~1.59M tokens, zero deaths);
    20 findings CONFIRMED at code and folded (0 refuted, 0
    escalations — §8 OWNER-ESCALATION: NONE); the orchestrator
    personally re-verified all ten load-bearing anchor families at
    code pre-commit. THE MERGED SLICE = ONE commit, four sub-diffs:
    (i) -demand-retract (implies -demand; orthogonal to
    -demand-instance + the 4 modes; OFF PassPolicy) -> the
    fabricated demand message gains a synthetic @differential token
    -> the closure flips via the ORDINARY machinery (zero new
    differential code flat); a SECOND kQueryMessageInjector + the
    generated hidden friend <name>_<pattern>_retract(db, log,
    functors, bound...) writes the del_vec; netting = the landed
    NETBATCH arm. Driver contract RULED TOTAL+IDEMPOTENT (R-4:
    SubExplicit structural no-op, AddExplicit kExplicit-bit
    idempotence = the SET-demand second leg). (ii) death lowering =
    band-(a0) INSIDE the SUBGRAPHINSTANCE region (R-1,
    seal-precedent; epoch position is the operative constraint),
    removal_frontier UseRef + {sid,kInstanceDeath} enrollment
    closing V-INST-EMITTED, FindInstance+RecycleCurrent-only
    emitter (kNoInstance silent-skip R-6), V-INST-DRAIN
    REGIME-SPLIT + pure CheckInstanceDeathFrontier (new TEST in the
    existing rel_validators_test binary, EINTR-safe mold; ctest
    stays 6/6 binaries, R-5) + V-INST-DEATH-COHERENCE; ZERO
    ControlFlow pre-provisioning (the differential machinery
    provisions the demand sextet before ValidateDROps);
    ClassifyVector's kSubgraphInstance arm extended for EVERY new
    region vector (A2.1 + the adjudicator's del/add-queue
    extension); orphan-mint fences at every memoized vec fetch
    (A2.6/§3.3). (iii) band-(b) (T,F) drop scan (OVERDELETE-first
    per iid; SubDerivation + DelQueue append), born arm ->
    AddDerivation, always-on GENERATED V-INST-PARTITION belt,
    IsDifferential()/DemandTable()/queue accessors, d5 ", false"
    ctor by co-activation, PLUS the adjudicator-folded BAND-(a2)
    DEMAND-LIVENESS GATE (R-3 = the b3-critique HIGH-1
    zombie-rebirth catch, the slice's one genuine interface
    mismatch X8: a dead key still binds an iid and TouchedFlag
    resets at Seal, so a later edge would re-materialize the dead
    neighborhood — the diff-arm a2 gate gains demand-table
    Find+Present conjuncts; model-covered by the ALREADY-declared
    kInstanceDemand read Rel.cpp:800-803, no effect-set change; NOT
    an iid tombstone — OD-15 honored). (iv) witness growth (the
    @differential nbhd_out tap; retract/dead-key-edge/rebirth/
    second-death phases; send_expect_silent NORMATIVE with abort
    teeth), the DS-R4-10 fence MOVED POST-FIXPOINT with the
    is_dead||!negated_view filter (A4.1) + negate_never_diff_1
    (all-4-modes diagnostic; suite 175->176) + FINDINGS.md F25 (the
    @never-over-differential mode-split latent accept, caught
    pre-code). LOUD RIDER R-2: the merged design REORDERS XC-3's
    counterfactual abort chain — under the regime-split
    V-INST-DRAIN a d1-only build's FIRST abort is V-INST-EMITTED
    (verdict unchanged: one commit, no temporary fence; the
    substrate's V-INST-DRAIN-first wording is superseded as a
    counterfactual description, observed at the L2 checkpoint).
    EQGATE UPGRADED: answer + sorted published-delta identity (the
    tap). GATES PRE-REGISTERED (§6): pre-bless red set EXACTLY 10
    lines (witness x4 GOLDEN-DIVERGE + oracle + monotone + eqgate
    x4; any 11th red or byte off prediction = STOP); golden churn
    EXACTLY 3 witness-owned files via --bless; [BYTE] everywhere
    else (174 stdouts x4, 20 pinned surfaces, diagnostics, data/
    36x4); witness header/rel/ir [STRUCT] with line-level
    predictions re-anchored at the stage-(d) (d0) TAP-FUL PRE-d1
    BASELINE (binding protocol addition); Q5 MUST RUN (ABABAB,
    bytes move — the D3.a.0 waiver is void); ASAN hard-gated x2
    sweeps (death path = use-after-free terrain); d7
    liveness-by-perturbation table L1-L10 consolidated (incl. both
    V-INST-DIFF-COHERENCE polarities with the TRUE-bit carrier —
    the §20(AF) §3.6 obligation lands here). NEXT: stage (d) — (d0)
    tap-ful baseline, compact desired-states, the dump-author
    prediction lane + the BLIND worktree prototype (hand-
    provisioned, tip-verified) converging with pristine; then Fable
    review; then land as ONE commit. Landing record = §20(AK).

(AK) D3.a.1 LANDED (2026-07-28) — DIFFERENTIAL DEMAND: the retract
    channel, death, the (T,F) publish, and the witness family, per
    d3a1-design.md as amended by the Fable review; desired-states
    record = d3a-desired-states.md SLICE 1; substrate =
    d3a1-substrate.md (incl. §7 the d2 CO-ACTIVATION ruling).
    MECHANISM AS LANDED: (i) -demand-retract (implies -demand, OFF
    PassPolicy) -> FabricateDemandMessage stamps a SYNTHETIC
    @differential token (kPragmaDifferential) -> the closure flips
    via the ORDINARY machinery; a second kQueryMessageInjector + the
    generated <name>_<pattern>_retract hidden friend write del_vec;
    netting = the landed NETBATCH arm; driver contract
    TOTAL+IDEMPOTENT (R-4). (ii) death = band-(a0) INSIDE the
    SUBGRAPHINSTANCE region (R-1): removal_frontier UseRef +
    RemovalFrontier(), {sid,kInstanceDeath} enrollment closing
    V-INST-EMITTED, FindInstance+RecycleCurrent emitter (its FIRST
    codegen caller; kNoInstance silent-skip R-6); V-INST-DRAIN
    REGIME-SPLIT + the pure CheckInstanceDeathFrontier (death-tested
    in rel_validators_test) + the G-8 source check +
    V-INST-DEATH-COHERENCE + orphan-mint fences at EVERY memoized
    vec fetch (incl. the review-[B] band-(a1) fence); ClassifyVector
    extended for every new region vector. (iii) band-(b) gains the
    (T,F) drop scan (OVERDELETE-first per iid; SubDerivation +
    DelQueue) + the AddDerivation born arm + the ALWAYS-ON generated
    V-INST-PARTITION belt; band-(a2) gains the DEMAND-LIVENESS gate
    (R-3: demand Find+Present nested inside the iid check per
    review-[I]); IsDifferential()/DemandTable()/DelQueue()/AddQueue()
    accessors; the store constructs monotone=false by co-activation
    (d5; HP-7 belt off for differential stores). (iv) the witness
    grew retract/dead-key-edge/rebirth/second-death phases + the
    @differential nbhd_out tap (EQGATE UPGRADED: answer + sorted
    published-delta identity); DS-R4-10 moved POST-FIXPOINT with the
    is_dead filter + an ALWAYS-ON no-predicate fallback (review-[C]);
    negate_never_diff_1 = the fence witness; FINDINGS.md F25;
    + the review-[A] R-MONO twin demand_neighborhood_mono_witness
    (the pre-slice witness bytes as a SECOND eqgate case — the
    monotone nested lowering keeps end-to-end coverage). Suite
    175->177. STAGE-(d) THREE-WAY: design §6 == the desired-states
    author's hand-derived 36-line golden == the blind prototype, on
    every compared surface; the census adjudicated at the dump
    (kSeedFold=7 = the two differential joins' signed seed arms,
    kJoinEmit=2 their delta forms — the DS lane under-predicted, the
    prototype was correct); red set EXACTLY the 10 pre-registered
    lines; golden churn EXACTLY 3 witness files via --bless (+ the 3
    resurrected mono-twin goldens committed directly). d7
    PERTURBATIONS L1-L10 ALL FIRED with recorded texts (amendments:
    L3's never-minted role is kProductInput; L5 perturbs
    mint-consistently past G-8; L7/L8 vehicle is the NESTED arm;
    L2 recorded in BOTH configurations — tip validator:
    V-INST-DRAIN-first, regime-split: V-INST-EMITTED "2 vs 3").
    INCIDENT RECORDED: an orchestrator git-checkout during the L5
    cycle wiped P1's uncommitted Rel.cpp edits; reconstructed from
    the binding specs, PROVEN byte-equivalent (nested .rel/.ir/.h ==
    pre-wipe artifacts); RITUAL AMENDMENT: WIP-commit the prototype
    worktree BEFORE perturbation cycles. FABLE REVIEW (workflow,
    high; 21 agents, 10 ranked findings, ZERO live miscompiles):
    [A] coverage regression FIXED (the mono twin); [B]-[E] NDEBUG/
    fence hardenings FIXED; [G] shared DeathHarness.h (3rd
    fork/waitpid copy eliminated); [H] HasTableDeltaVector helper x3
    sites; [I] the a2 probe nested (hot-path hash probe eliminated);
    [J] stale ships-INERT comments; [F] the ~90-line retract/forcer
    builder duplication DEFERRED with a NAMED OBLIGATION (D3.a.3
    touches the registry walk — dedup rides there; the forcer twin's
    pre-existing assert-only handler guard rides with it). GATES
    (final bytes, all personally executed or re-executed; error-grep
    0 x3 trees at every rebuild): SUITE PASS(177) debug; ASAN
    PASS(177) x2 sweeps + ctest 6/6 debug + 6/6 ASAN; the pre-bless
    red set matched the prediction EXACTLY (10/10, twice — worktree
    + pristine); 20/20 pinned regen [BYTE] x3 rounds; eqgate LIVE x4
    both witnesses (flat==nested==golden; the differential witness
    additionally delta-stream-identical); config-invariance
    SINGLE-HASH x3 + release==debug on both witness arms +
    demand_tc; E-62 CLEAN; Q5 progsize@128 release ABABAB MEASURED:
    A~=148.8ms vs B~=148.2ms warm medians — compile-time NEUTRAL
    (<2%). RESIDUALS OPENED: the [F] dedup obligation (D3.a.3); the
    §20(AF) §3.6 liveness obligation is DISCHARGED (L6 both
    polarities with the TRUE-bit carrier; L7 the partition belt; L10
    OWN-3). NEXT: D3.a.2 — differential input (lift FENCE (iii) +
    V-INST-SOLE, the input net-removals a2 trigger ->
    RecycleCurrent + full rescan; the a2 quiescence-argument rider
    from design §3.2 re-derives there) under the full per-slice
    ritual; then D3.a.3 multi-adornment (+ the View.cpp fold-
    predicate precondition + the [F] dedup).

(AL) D3.a.2-OPEN RE-VERIFICATION RECORD (2026-07-28, tip bff75eb6;
    the §8 SINGLE-PASS banner discharged). (0) Frozen A baselines
    RE-SNAPSHOTTED from tip into the session scratchpad BEFORE any
    code (debug + release + oracle binaries; BOTH witnesses'
    nested-arm reference dumps, SHA-256-pinned; all three trees
    rebuilt, error-grep 0); SUITE PASS(177) against the FROZEN A
    binary; ctest 6/6 debug + 6/6 ASAN. (1) §8 + §20(AH)-(AK) + the
    d3a1 contracts FLEET-RE-VERIFIED (house precedent): 2 seed-UNREAD
    opus derivation lanes (lane1 retract/death/validators/band as
    landed; lane2 the INPUT-side substrate D3.a.2 will touch) + 1
    seed-READ adversarial verifier + 1 sonnet mechanical lane + 1
    xhigh consolidator (5 agents, ~572k tokens, zero deaths). The
    carrier-golden referee EXECUTED by the orchestrator personally:
    20/20 pinned surfaces regenerate BYTE-IDENTICAL at tip; the MECH
    lane's independent regen agreed 20/20, plus census x11 (all
    kIngestLoop=/kJoinEmit=, kInstanceDeath=0), BOTH witnesses'
    nested dumps byte-identical to the frozen refs (diff census
    kInstanceDeath=1 kSubgraphInstantiate=1 kInstanceSeal=1
    kSeedFold=7 kJoinEmit=2; mono kInstanceDeath=0), eqgate
    SUITE: PASS (2 cases) 20/20 sub-verdicts, cross-knob 11/11,
    ctest 6/6 both trees, L1/L10 spot-reruns 4/4. The E-62 re-grep
    EXECUTED by the orchestrator personally: CLEAN (zero
    body_ops/output_ops readers outside lib/Rel/; the 5 sanctioned
    pinned_order hits). VERDICT: SOUND-WITH-ERRATA — ZERO false
    behavioral claims, ZERO code-deviations, both seed-unread maps
    converge with the verifier and §8/(AK); the D3.a.2 CHARTER IS
    UNCHANGED (no e1-e8 premise false). ONE inter-lane contradiction
    adjudicated at code (lane1 right, verifier wrong): the death
    gate `if` sits at Rel.cpp:1140, NOT :1139 — the slice's two-line
    comment respell at :1138-1139 pushed it down one; §8 had carried
    the pre-slice anchor forward (historical :1139 citations in
    (AH)/(AI)/§1-§7/annexes are correct at their stamps, untouched).
    FOUR ERRATA (all COSM, all d3a1-substrate.md §8, applied by the
    orchestrator per the consolidator's exact specs,
    LINE-COUNT-PRESERVING; docs-only so binaries unmoved):
      E-148 COSM §8 flags block Main.cpp range :488-497 -> :488-491
                 (:493 opens the -M handler).
      E-149a COSM §8 mint block gate anchor :1139 -> :1140 +
                 "comment :1138 respelled" -> "comment :1138-1139".
      E-149b COSM §8 THE TWO PREDICATES P-DEATH (:1139) -> (:1140)
                 (load-bearing for the e5 divergence audit).
      E-150 COSM §8 band block band-(b) :2540ff -> :2525ff.
    ADVISORY for stage (a)/(b) (consolidation record, 9 items —
    seeds, not rulings): ADV-3 is LOUD — the shared rescan mold
    (Database.cpp:2366-2404) has NO presence filter; over a
    DiffTable input it would re-materialize dead rows (a wrong-
    answer shape unless the mold gains a Present conjunct — a
    stage-(b) must-have). ADV-9: e3's "RecycleCurrent + full
    rescan" wording is loose — V-INST-FRESH guarantees current is
    EMPTY at band-(a) entry, so a live-key input-removal rebuild
    likely reduces to the a2 Touch+rescan mold with a SECOND drain
    source (the ritual-head question's feed; if Recycle is kept it
    must stay behind !TouchedFlag or a same-epoch add-rescan is
    silently wiped). Others: V-INST-SOLE's one abort string covers
    two forbiddances (split when lifting, ADV-1); NO input-side
    death analogue — input removals never mint death (ADV-2); the
    nested pub is co-derived by flat guard-web joins AND band-(b) —
    the symmetric-firing argument is unwritten, re-derive before a
    third signed pathway (ADV-4); the input kNetRemovals PRODUCER
    is free (generic both-sign mint Rel.cpp:2541ff) — the slice's
    work is the drain side (ADV-5); V-INST-EFFECT's split is
    pub-keyed with input_drains==1 hard-coded — the input branch
    keys on a THIRD predicate axis, pose the L3-style question
    before code (ADV-6); discharge N-1 explicitly at stage (a)
    (ADV-7); demand_diff_input_1.dr's comment cites stale
    Build.cpp:1344, fix when e1 flips it (ADV-8). GATES post-errata:
    docs-only edits, binaries PROVABLY unmoved (no rebuild;
    baselines stand). NEXT: D3.a.2 stage (a) — the input-side
    substrate build-out (lane2's map + the advisories seed the
    lanes), then the ritual-head a2-trigger-shape ruling BEFORE
    stage-(b) lanes; this record took the charter's §20(AL) letter,
    so the D3.a.2 landing record shifts to §20(AM)+. [EXECUTED —
    §20(AM); the landing record shifts again, to §20(AN)+.]

(AM) D3.a.2 STAGE (a) COMPLETE + THE R-A2-TRIGGER RULING (2026-07-28,
    tip bf0315a0): THE SLICE SUBSTRATE COMMITTED —
    KeyedInstances.artifacts/d3a2-substrate.md (§1 the flat oracle
    O-1..O-9 / §2 the input path as landed / §3 fences + the
    input_table site census + the abort chain / §4 the epoch
    catalogue + OB1-OB8 quiescence obligations / §5 the gap ledger
    mapped to b1-b4 / §6 the ritual-head fact base / §7 the ruling /
    XC-5..XC-10). Fleet: 3 seed-UNREAD opus lanes (flat-oracle
    trace; exhaustive input_table census; epoch-interleaving
    catalogue) + fleet1's lane2 input-substrate map as a fourth lane
    (same code bytes, house precedent) + xhigh seed-READ consolidator
    (~482k tokens, zero deaths); the ORCHESTRATOR personally
    re-verified all ten load-bearing anchor families at code
    pre-commit (all exact). HEADLINE STRUCTURAL FACTS: O-1 the e5
    divergence is EMPIRICAL — plain -demand on demand_diff_input_1
    gives a monotone demand Table beside DiffTable pt/ans/getpt
    (P-STORE true, P-DEATH false); O-3 the edge channel already
    NetBatches (OB7 discharged); O-8 the flat oracle is made ENTIRELY
    of landed machinery (no exotic op; kInstance*=0 flat); O-9 the
    two axes compose as plain symmetric differential (D3.a.2's input
    axis orthogonal to D3.a.1's demand axis — the slice serves BOTH
    diff-input x mono-demand AND diff-input x diff-demand). XC-9
    (substantive): the a2 gate selector is PUB-keyed, so the
    e5-carrier emits the R-3 gate against a MONOTONE demand member —
    soundness there is IRREVOCABILITY, not quiescence (a NEW e4
    sentence). XC-6: the in-source "Build.cpp:999" cross-refs
    (Rel.cpp ~:4507, Procedure.cpp ~:326) are STALE-BY-DRIFT — the
    live monotone append is Build.cpp:1110-1114 (H-20 comment rider
    lands with the slice). ADV-3 CONFIRMED all-lanes: the shared
    rescan mold needs a Present(s) conjunct over a DiffTable input
    (spelling RULED Present), on ALL THREE sources (incl. a1 birth —
    the E-F2 rebirth cell). THE RULING (substrate §7, adjudicated at
    code, NO owner brief — the §6 fact base proves no admissible
    option changes observable behavior): **R-A2-TRIGGER = TWO
    DRAINS, NO RECYCLE** — (1) the input kNetRemovals frontier is a
    SECOND a2 drain arm (third rescan source) into the ONE shared
    mold; combined ± drain REJECTED (no combined VecRole — new model
    surface for zero gain); V-INST-EFFECT grows input_drains==2
    under the INPUT-keyed regime (ADV-6 third axis, never folded
    into P-STORE/P-DEATH); (2) gate-set IDENTITY across the two a2
    arms is binding (behavior-neutrality holds only under it); (3)
    RecycleCurrent stays DEATH-ONLY — gated Recycle is a provable
    no-op that blinds V-INST-FRESH in that arm; the UNGATED-late
    Recycle is the one observably-divergent shape (E-E silent
    full-retract, no landed belt catches it) and is FORBIDDEN (named
    stage-(b) design fence); OQ-INPUT's ruled SEMANTICS preserved
    verbatim, its "RecycleCurrent +" mechanism wording superseded at
    band position (the R-2 precedent); (4) band order a0 -> a1 ->
    a2 -> a2'-removals-APPENDED (diff minimality; landed bytes
    untouched ahead; does NOT discharge the OB8 combined-entry
    fence-or-proof — that stays e4's). Docs only; no gates beyond
    anchor verification (no code moved; binaries stand). NEXT:
    stage (b) — design lanes under this substrate + the ruling
    (suggested split per the session charter: b1 fence lifts +
    input kNetRemovals provisioning + effect/validator regime
    splits; b2 the a2' removal arm + TouchedFlag coupling + the
    band-(b) net-retraction ride; b3 the e4 quiescence
    re-derivation + OB8 + the e5 divergence-goes-live audit; b4 the
    witness family + gate plan), then critics + adjudicator ->
    d3a2-design.md; landing record = §20(AN)+. [EXECUTED — §20(AN);
    the landing record shifts to §20(AO).]

(AN) D3.a.2 STAGES (b)/(c) COMPLETE (2026-07-28, tip b4d08307): THE
    BINDING DESIGN COMMITTED — d3a2-design.md + the four annexes
    d3a2-b{1,2,3,4}-design.md (the design binds where it amends; a
    lane's edit spec is adopted verbatim where it is silent). Fleet:
    4 xhigh design lanes + 4 FRESH adversarial critics (pipelined
    per lane) + 1 xhigh adjudicator (~1.65M tokens, zero deaths);
    21 findings CONFIRMED at code and folded (0 refuted outright, 0
    escalations — §8 OWNER-ESCALATION: NONE); the orchestrator
    personally re-verified the design's new load-bearing anchors at
    code pre-commit (Rel.cpp:4271 diff-at-case-head, :4324-4331
    totality, :1170-1173 ready_after both tables; runall.sh:361;
    the inert stub driver; Vec.h:176 NetBatch; Table.h:261-266
    monotone Present). THE MERGED SLICE = ONE commit, four
    sub-diffs: (i) DR-layer admission — FENCE (iii) diff_input arm
    lifted (cyclic + recursive-content SURVIVE, OB8-load-bearing);
    V-INST-SOLE differential half lifted + pub-alias half reworded
    + a NEW induction-owned-input belt (ADV-1); the input_diff
    THIRD axis (own spelling, never folded — d2 extended) threaded
    into InstantiateEffects with the kVecDrain{input, kNetRemoval}
    leg; V-INST-EFFECT split (drains input_diff?3:2, input_drains
    ?2:1) + the adjudicator-folded O-1 CLOSURE BELT (input_diff &&
    !diff aborts — R-3, the checked one-directional theorem, a
    drift guard not a unification); V-INST-DRAIN input arm
    regime-split (both-sign dr_ok diff / cf_ok monotone — the XC-3
    twin); region input_removal_frontier UseRef + accessor +
    ClassifyVector read arm + the fenced pre-minted ± fetch + NEW
    always-on V-INST-INPUT-COHERENCE (member presence ==
    input_diff); 2 new fork/waitpid TESTs in rel_validators_test
    (R-7, ctest stays 6/6); the H-20 stale-comment rider. (ii)
    codegen band — E2a member-presence selector (belt-checked); E2b
    the input.Present(s) conjunct on the ONE mold, ALL THREE
    sources; E2c band-(a2') as a character-for-character gate-set
    CLONE of a2 (NO RecycleCurrent; landed a0/a1/a2 bytes untouched
    ahead — R-A2-TRIGGER honored; the ~24-line clone joins the
    D3.a.3 [F] dedup candidate list). (iii) the ARGUMENT lane
    (comments only, [BYTE] everywhere): the e4 lemma
    (L-EDGE/L-MONO-irrevocability/L-DEMAND/L-COMBINED-documentary)
    discharging the Database.cpp:2494-2495 RIDER; the e5 audit
    A1-A12 ZERO EDITS (the d2 anti-fold payoff); the N-1 close; the
    pinned FIVE-WAY input-quiescence coupling. (iv) the witness 2x2
    — NEW flagship demand_diff_neighborhood_witness (diff-input x
    MONO-demand, the e5 carrier, FIRST P-STORE-and-not-P-DEATH
    program, census kInstanceDeath=0 beside kSubgraphInstantiate=1)
    + demand_diff_input_1 REPURPOSED diagnostic->golden as the
    diff x diff composition witness (E-F1/E-F2/E-F3); both with
    .batches (the oracle SEES input retractions — stronger than
    D3.a.1) + .eqgate; suite 177->178, eqgate carriers 2->4; the
    b4C-1 HIGH repaired (A4.1: the a1-Present discriminator goes
    CROSS-batch — NetBatch annihilates same-batch +/-, the
    as-designed perturbation could never fire). KEY RATIFICATIONS:
    R-2 the demand_diff_input_1 flip is MANDATORY AT CO-LAND (else
    the suite reds); R-4 NO new dump marker/token — E-71 notes owed
    ZERO (the removal leg renders from existing kVecDrain tokens;
    the divergence is census-visible); R-5 the derived-acyclic
    diff-input class is EMPTY at tip (adjudicator PROBES: CMP body
    rejected upstream by plain -demand; MERGE input mints no
    instance; admitted inputs are ingest-written kSeedFold=0) — a
    NAMED OBLIGATION binds any future body-walk/recognition
    widening to re-derive OB8(i)'s derived branch with a directed
    witness; R-8 the (d0) protocol is WHOLE-SLICE-FIRST-GREEN (no
    witness-alone baseline exists — the abort chain forbids it) +
    the input-writer census check. GATES PRE-REGISTERED (§6):
    Phase-A pre-bless red set EXACTLY 14 lines (all GOLDEN-MISSING
    class — both new cases had no goldens); golden churn EXACTLY 6
    NEW files, zero existing golden changes; [BYTE] on 176
    non-witness stdouts x4 + 20 pinned + BOTH frozen D3.a.1
    witnesses (regression anchors) + data/ 36x4; Q5 MUST RUN
    (ABABAB vs tip snapshot); ASAN x2 hard (a2' rescan over
    DiffTable dead-row terrain); config-invariance on both new
    witness arms; d7 L-TABLE CONSOLIDATED L1-L22 incl. L15 the
    FORBIDDEN ungated-late-Recycle demonstration (belt-invisible by
    design — only eqgate+HP-5 catch it, WHY the fence is
    documentary) and L11 the co-landability chain (E1a+E1b-only
    first abort = V-INST-DRAIN input arm). NEXT: stage (d) — (d0)
    whole-slice-first-green baseline in the WIP-committed prototype
    worktree, the desired-states author lane + the BLIND prototype
    converging with pristine; then Fable review; then land as ONE
    commit. Landing record = §20(AO). [EXECUTED — §20(AO).]

(AO) D3.a.2 LANDED (2026-07-29) — DIFFERENTIAL INPUT: the fence
    lifts, the a2' removal arm, the Present rescan, and the witness
    2x2, per d3a2-design.md as amended by the Fable review;
    desired-states record = d3a-desired-states.md SLICE 2; substrate
    = d3a2-substrate.md (incl. §7 the R-A2-TRIGGER ruling).
    MECHANISM AS LANDED: (i) FENCE (iii)'s diff_input arm DELETED
    (cyclic + recursive-content siblings survive); V-INST-SOLE's
    differential half lifted, pub-alias half reworded, + the NEW
    induction-owned-input belt; input_diff = the THIRD predicate
    axis (own spelling, never folded); InstantiateEffects grows the
    kVecDrain{input, kNetRemoval} leg; V-INST-EFFECT totality
    input_diff?3:2 + the O-1 closure belt (input_diff && !diff
    aborts); V-INST-DRAIN both-sign dr_ok input arm;
    region input_removal_frontier + InputRemovalFrontier() +
    ClassifyVector arm + fenced pre-minted ± fetch +
    V-INST-INPUT-COHERENCE; CheckInstanceInputArm (the D-1
    deviation: a pure tested belt per the CheckInstanceDeathFrontier
    factoring precedent — inline-path teeth joined to the D3.a.3
    obligation) + InputArmTest.cpp (4 TESTs, DeathHarness mold;
    ctest stays 6/6 binaries). (ii) band-(a2') = the edge
    net-removals drain APPENDED after a2, a gate-set clone (NO
    RecycleCurrent — the R-A2-TRIGGER fence, in-code comment);
    the ONE rescan mold gains the input.Present(s) conjunct under
    input_diff at ALL THREE sources; landed a0/a1/a2 bytes untouched.
    (iii) comments only: the e4 lemma discharged the :2494-2495
    RIDER; the five-way coupling block (D-2 home: EmitSubgraphInstance
    head); the N-1 close; H-20 respells. (iv) the witness 2x2:
    demand_diff_neighborhood_witness (the e5 carrier — the FIRST
    P-STORE-and-not-P-DEATH program; census kInstanceDeath=0 beside
    kSubgraphInstantiate=1, kSeedFold=6) + demand_diff_input_1
    REPURPOSED diagnostic->golden (diff x diff; E-F1/E-F2/E-F2b/
    E-F3; census kInstanceDeath=1, kSeedFold=8); suite 177->178;
    eqgate carriers 2->4 (16 live verdicts). STAGE-(d) THREE-WAY:
    blind prototype == blind DS-author BYTE-EQUAL on all 6 golden
    surfaces + census (the DS lane derived kSeedFold correctly this
    time); Phase-A red set EXACTLY the 14 predicted lines; churn
    EXACTLY 6 NEW goldens (one amended in place by review WIT-4),
    zero existing goldens touched. LANE INCIDENT (recorded): the P2
    lane capped mid-suite — the orchestrator personally re-executed
    Phase A, the cmp ritual, the bless, and Phase B (the standing
    placeholder-gate rule); P3 supplied P2's missing WIP-commit as
    the perturbation anchor. d7 L1-L22: all scratch rows FIRED with
    recorded texts EXCEPT two HONEST masked negatives — L15 (the
    forbidden ungated-late-Recycle produced NO red: belts silent AS
    PREDICTED but split counters >= 2 ALSO masked eqgate/HP-5 — the
    documentary fence is now demonstrably belt-AND-witness-invisible;
    E-E red-team robustness flagged) and L17 (the ClassifyVector a2'
    arm is belt-only on single-proc witnesses; teeth refuted for the
    arm, kept as mold symmetry — review WIT-2). FABLE REVIEW
    (workflow, high; 21 agents; 14 raw -> 12 verified -> 11 ranked):
    ZERO HIGH, ZERO MED, zero live miscompiles; 7 FIXED pre-commit
    (WIT-4 the E-F2b live-key retract cell — the one runtime-artifact
    fix, its own golden amended; WIT-3 the .ir `input-removals`
    only-when-present production, death-branch mold — the slice's ONE
    E-71 grammar note, orchestrator-adjudicated, amending R-4's
    zero-note tally; DOCS-3 the :1530-1537 fence anchor x4 sites;
    DOCS-2/validators-1/validators-2/DOCS-1 comment respells);
    1 DEFERRED-with-obligation (design-1 inline-validator teeth ->
    D3.a.3, joins review-[F]); 4 ACCEPT. Dump-neutrality of every
    comment fix PROVEN (20/20 pinned + both frozen witnesses
    byte-identical on FINAL bytes). GATES (final bytes, personally
    executed or re-executed; error-grep 0 x3 trees every rebuild):
    SUITE PASS(178) debug post-fix; ASAN PASS(178) x2 sweeps + ctest
    6/6 debug + 6/6 ASAN; 20/20 pinned regen [BYTE]; BOTH frozen
    witnesses' nested .h/.rel/.ir byte-identical to the frozen refs;
    config-invariance SINGLE-HASH x3 (both new witness arms flat+
    nested + demand_tc) + release==debug; data/ corpus 144/144
    old-vs-new byte-identical x4 modes; E-62 CLEAN (5 sanctioned);
    eqgate 16/16 OK; Q5 progsize@128 release ABABAB MEASURED:
    warm medians A=139ms vs B=139ms — compile-time NEUTRAL (cold
    first pair discarded; A/B headers byte-identical). RESIDUALS
    OPENED: the R-5 named obligation (body-walk/recognition widening
    re-derives OB8(i)'s derived branch with a directed witness); the
    design-1 inline-teeth obligation + the E2c ~24-line gate clone
    join the D3.a.3 [F] dedup sweep; the L15 E-E red-team robustness
    note. NEXT: D3.a.3 — multi-adornment (preconditions f1 the
    View.cpp fold-predicate re-derivation with directed witnesses
    BOTH directions + f2 the review-[F] retract/forcer dedup, now
    carrying design-1 + the E2c clone).

(AP) D3.a.3-OPEN RE-VERIFICATION RECORD (2026-07-30, tip 31eb9308; the
    §8 SINGLE-PASS banner DISCHARGED). (0) Frozen A baselines
    RE-SNAPSHOTTED from tip into the session scratchpad BEFORE any code
    (debug + release + oracle binaries; ALL FOUR eqgate witnesses'
    nested-arm reference dumps h/.rel/.ir, SHA-256-pinned — 17-line
    SHA256SUMS; all three trees rebuilt, error-grep 0 x3); SUITE
    PASS(178) against the FROZEN A binary; ctest 6/6 debug + 6/6 ASAN;
    20/20 pinned regen BYTE-IDENTICAL (orchestrator personally); E-62
    re-grep CLEAN (orchestrator personally: zero body_ops/output_ops
    readers outside lib/Rel/; the sanctioned Stratum.cpp:1073 comment +
    the RelValidators/InstanceOrderTest.cpp fixture pinned_order
    neighbors). (1) §8 + §20(AL)-(AO) + the d3a2 contracts
    FLEET-RE-VERIFIED (house precedent, workflow wf_6a638824-229): 2
    seed-UNREAD opus derivation lanes (lane1 the differential-input
    machinery as landed — fence lifts, input_diff axis, effect/validator
    regime splits, CheckInstanceInputArm, the a2' arm + Present mold, the
    .ir production; lane2 the ADORNMENT-side substrate D3.a.3 will touch
    — Demand.cpp pass structure, the name-only vs (query,BindingPattern)
    keying sweep, the builder twins, GuardAnnotationsCompatible + the
    fold-arm survivorship, N-store recognition/mint/census) + 1
    seed-READ adversarial verifier over §8 + §20(AL)-(AO) +
    d3a2-design-as-landed + 1 sonnet mechanical gate lane + 1 xhigh
    consolidator (5 agents, ~478k tokens, zero deaths). The mech lane's
    gates (all folded into the consolidation, all GREEN): 20/20 pinned
    regen, 12/12 nested witness dumps byte-identical to frozenA,
    config-invariance debug x3 same-hash + release==debug (8/8) all ==
    frozenA/SHA256SUMS, eqgate SUITE PASS(4 cases) 16/16 sub-verdicts,
    cross-knob 22/22, ctest 6/6 + 6/6, census 29 kinds
    (demand_diff_neighborhood_witness kInstanceDeath=0
    kSubgraphInstantiate=1 kSeedFold=6; demand_diff_input_1
    kInstanceDeath=1 kSeedFold=8). VERDICT: **SOUND — ZERO substantive
    false claims, ZERO errata.** No §8 code anchor FALSE; no §8/§20(AO)
    behavioral claim FALSE; no g1-g8 premise FALSE. Two off-by-one COSM
    anchors CONSIDERED-AND-DECLINED (§8 body-walk reject :667-670 vs the
    :668-672 statement; retract match :570-573 one line short of the
    :574 IsDifferential belt) — both land the reader on the right
    construct and match the doc's comment-inclusive convention; the
    verifier's raised View.cpp :584-588 COSM was itself a miscount
    (:584 IS the signature line — DISMISSED). §20(AL)'s E-148/E-149
    (P-DEATH :1140) corrected the OLDER d3a1 §8; the slice drifted
    :1140 -> :1162, and current §8 correctly cites :1162 — different
    stamps, self-consistent (no contradiction). (2) THE ONE MATERIAL
    FORWARD FINDING — a charter-completeness gap in g5, orchestrator-
    verified at code: **ADV-1 the V-INST-SOLE per-pub obstruction.**
    inst_per_store/seal_per_store/death_per_store (Rel.cpp:4404/:4438/
    :4448/:4460) are keyed on op.instance_store_id and are ALREADY
    N-safe; but inst_per_pub (:4406) is keyed on op.table_op_table and
    V-INST-SOLE LOUD-ABORTS at :4454-4458 when the count != 1 — and N
    adornments of ONE (query,arity) resolve pub by q_decl.Id()
    (name+arity, ResolveLiveRecognition :992) to the SAME model table,
    so N mints set table_op_table = the shared pub_table -> count N ->
    abort. g5's "N disjoint stores" is thus validator-INCOMPLETE as
    written (not FALSE — g5 never claimed V-INST-SOLE was N-safe): the
    per-PUB check redesign (relax to per-(pub, forcing/key-shape) OR
    merge the N publishes) is the CENTRAL D3.a.3 design question and is
    now folded into §8's g5 as a hard prerequisite. ADVISORY SEEDS for
    D3.a.3 stage (a) (facts + hazards, not rulings; full text in the
    session's consolidated.md): ADV-1 the g5 per-pub abort (LOUD);
    ADV-2 the forcing_index/first_annotation snapshots (Demand.cpp:
    983-985) MUST stay INSIDE the per-adornment loop or all adornments
    mis-key under forcing 0; ADV-3 the known_consumers stray set
    (Demand.cpp:761-791) must UNION across adornments guarding a shared
    predecessor or a per-adornment run false-rejects; ADV-4 the fold-arm
    survivorship policy — CopyDifferentialAndGroupIdsTo (:625-683) keeps
    `that` as survivor and does NOT pick by role, but
    ResolveLiveRecognition derives input ONLY from a role==kBody stamp
    (:974-983), so g1 must FORCE the kBody survivor (the LABELED RESIDUAL
    View.cpp:571-583, corpus-DORMANT, goes live with multi-guard folds);
    ADV-5 the proxy-TUPLE Equals false-abort (g1 direction (b) — the
    key-invariance argument fails once annotations migrate to propagate-
    arm TUPLEs); ADV-6 the once-per-module boundary (Step-11 census
    :1132-1163 + MarkDemandFabricated :1165 stay AFTER the last
    adornment; steps 2-10 loop); ADV-7 fabrication + handler-map + ABI
    suppression are ALREADY N-safe (adornment suffix + fresh
    DeclarationContext + per-message keying) — g4 is a VERIFY sweep, not
    a re-key, EXCEPT ADV-1; ADV-8 the [F] forcer/retract guard asymmetry
    (assert-only :393-394 vs always-on :508-512) — g2 lands one
    parameterized builder + the shared always-on guard; ADV-9 the R-5
    OB8(i) widening obligation stands (any body-walk/recognition
    widening re-derives the derived-input branch with a directed witness
    FIRST); ADV-10 confirm the Database.cpp:2343-2390 five-way coupling
    holds per-store under N regions (esp. no cross-store TouchedFlag
    aliasing). GATES: docs-only (this record + the §8 g5 amendment +
    banner discharge); binaries PROVABLY unmoved (no rebuild after the
    snapshot; baselines stand). NEXT: D3.a.3 stage (a) — the
    adornment-side substrate build-out under the g1-g8 charter (g5 now
    carrying ADV-1); the D3.a.3 landing record shifts to §20(AQ)+.

(AQ) D3.a.3 STAGE (a) COMPLETE + THE PER-PUB RULE-AT-CODE RULING
    (2026-07-30, tip 1f74b5c9): THE ADORNMENT-SIDE SUBSTRATE COMMITTED —
    KeyedInstances.artifacts/d3a3-substrate.md (§1 the axis as it stands
    + the N-safe-already inventory / §2 the first two-adornment program +
    the g6 re-disposition / §3 the fold-arm substrate g1 / §4 the pass
    loop g3 / §5 the keying sweep g4 + N-store g5 / §6 the per-pub
    ritual-head ruling / §7 the gap ledger / §8 the stage-(b)
    ritual-head questions / §9 the load-bearing anchors). Fleet: 3 xhigh
    opus derivation lanes (laneA the fold-arm reality; laneB the
    pass-loop decomposition; laneC the keying/N-store/per-pub) + 1 xhigh
    consolidator (~517k tokens, zero deaths); the ORCHESTRATOR personally
    re-verified the ten §9 anchors at code AND RE-RAN the g6/misnomer
    behavioral adjudications as scratch compiles pre-commit (E-77).
    HEADLINE STRUCTURAL FACTS: (i) the both-set guard-annotation FOLD ARM
    is DORMANT even under multi-adornment — the two guards of one forcing
    forward structurally DIFFERENT incoming views so QueryJoinImpl::Equals
    never holds (View/Join Equals proof); g1's work is DEFENSIVE, not
    load-bearing-today. (ii) g6 RE-DISPOSED at code: demand_multi_adorn_1
    does NOT flip diagnostic->golden — its `fb` half hits the
    demand-propagation (left-linear) reject Demand.cpp:717-722 (bound `To`
    over a right-linear TC never traces off a read of `path`), so the case
    STAYS diagnostic (its reject MOVES :457->:717-722); a NEW
    From-preserving two-adornment SUCCESS witness is needed (the §2.3
    non-recursive symmetric seed — both `bf` and `fb` compile in
    isolation, together hit :457; orchestrator-recompiled). (iii) the
    body-walk "Multi-adornment demand is not yet supported" message is a
    MISNOMER — a ONE-`bf`-adornment swap recursion fires it (a SIP-derived
    SIDEWAYS adornment, not a second DECLARED pattern); g3 RE-MESSAGES
    :668-672/:723-726 (condition unchanged, they STAY fences), never
    narrows. (iv) ADV-7 HOLDS IN FULL — 34 keying sites swept, all N-safe
    with NO re-key except the fixes already owned by g1/g2/g3 and the one
    per-pub obstruction. THE RULING (substrate §6, adjudicated at code, NO
    owner brief — the d2/R-A2-TRIGGER precedent: no admissible option
    changes flat-oracle answers or published deltas; the shared pub is the
    reference-counted union of the N forcings' demanded rows, band-(b)
    scans each store's OWN Touched() keys and folds into pub
    non-destructively, Database.cpp:2736/:2801): **THE PER-PUB RULING =
    RULE AT CODE, ADOPT O1** — relax V-INST-SOLE's inst_per_pub
    (Rel.cpp:4406) to key on (pub_table, forcing_index), abort unchanged
    (!=1u). Grounds: diff-minimal (one map-key change, zero emission
    touched); byte-neutral for the whole single-adornment corpus (each pub
    has one forcing -> count 1 as before); OD-15-aligned (keeps N disjoint
    stores / N instantiates / N seals); protection-preserving (a spurious
    second instantiate for ONE forcing still aborts). O2 (merged publish)
    REJECTED — answer-neutral but CONFLICTS with the ratified OD-15
    N-disjoint-stores mandate + diverges the census (N->1) + higher diff
    cost; O3 (drop the check) DOMINATED by O1. ESCALATION: NONE (the
    central D3.a.3 design question is DISCHARGED at code). g1/g3/g6 all
    also rule-at-code; the two OWNER-BRIEF triggers (a live role-divergent
    fold; any body-walk/recognition WIDENING per R-5/OB8(i)) are
    CONTINGENT on findings neither this stage nor any lane produced (the
    multi-adornment loop as scoped widens NO body shape — each declared
    adornment is an independent From-preserving SIP). Docs only; no code
    moved; binaries stand. NEXT: stage (b) — the design lanes under this
    substrate + the per-pub ruling (suggested split per §7/§8: b1 g3 the
    two-phase pass loop + the RE-MESSAGE + [BYTE] proof; b2 g1 the
    survivor-record kBody policy + the predicate justification + directed
    witnesses, co-landing the :668/:723 fence-lift; b3 g2 the
    forcer/retract builder dedup + the [F] always-on guard + design-1
    inline teeth + the E2c clone; b4 g5 the O1 per-pub relaxation +
    per-store ADV-10 confirmation + g6 the new success witness + census),
    then critics + adjudicator -> d3a3-design.md; landing record =
    §20(AR)+. [EXECUTED — §20(AR); the landing record shifts to §20(AS).]

(AR) D3.a.3 STAGES (b)/(c) COMPLETE (2026-07-30, tip b65e7668): THE
    BINDING DESIGN COMMITTED — d3a3-design.md + the four annexes
    d3a3-b{1,2,3,4}-design.md (the design binds where it amends; a lane's
    edit spec is adopted verbatim where it is silent). Fleet: 4 xhigh
    design lanes + 4 FRESH adversarial critics (pipelined per lane) + 1
    xhigh adjudicator (~1.33M tokens, zero deaths); 17 findings CONFIRMED
    at code and folded (0 refuted, 0 escalations — §8 OWNER-ESCALATION:
    NONE); the orchestrator personally re-verified the load-bearing
    anchors at code pre-commit (R-DUP's RewireConsumer :236 + the shared-
    reader comment :230-231; O1 inst_per_pub :4289 + forcing_index on the
    DROp Rel.h:713/:1117; the census recount gate :3999/:4020; the
    file-local static ResolveLiveRecognition :918; the internal-only fold
    helpers Query.h:1186/:1188). THE ONE HIGH FINDING THAT REWROTE THE
    SLICE — F1/R-DUP (b1-critique, CONFIRMED): for EVERY two-adornment
    query both adornments trace to the SAME q_read/q_consumer (one clause,
    one materialization Demand.cpp:477; full-width reader :530-552) and,
    for the g6 seed, the SAME body (consumer,read) over edge_2;
    RewireConsumer substitutes ONLY c->view==read (:236), so the first
    adornment's rewire consumes the read-uses and the second adornment's
    guard is ORPHANED -> DCE'd -> skipped at mint -> HP-5 at the witness.
    b1's naive "move Steps 5-10 verbatim under Loop 2" does NOT lower the
    witness. FOLDED as amendment R-DUP (§1.5): group minted guards by
    (consumer,read); a singleton rewires directly (today's bytes, [BYTE]
    for |plan|==1); a multi-guard group mints a MERGE UNION of the guards'
    restored outputs and rewires the consumer ONCE (the flat-arm
    realization of the reference-counted-union pub, substrate §6.2). It
    RULES AT CODE (the answer is FIXED — no owner brief) but CANNOT be
    byte-closed read-only, so stage (c) OWNS mechanism selection
    (union-rewire PREFERRED) + verification (R-9: the g6 eqgate +
    the NEW structural gate that each forcing's ResolveLiveRecognition
    yields a non-null input_table, Rel.cpp:976-982). THE MERGED SLICE =
    ONE commit, coherent sub-diffs: (g3/b1) the two-phase locate/check/
    mint pass loop + the seen_variants dedup + RE-MESSAGE :668/:723
    (never narrow) + the stray-consumer UNION + R-DUP; DELETE the :444-462
    per-name reject. (g1/b2, FIRST/BINDING, co-lands with g3) the
    PromoteSurvivorToBody kBody-survivor policy (decl in the INTERNAL
    Query.h) + the stamp-time kBody-census belt + KEEP GuardAnnotations-
    Compatible (re-derive its justification, scoping BOTH the :571-583
    residual AND the :559-562 sentence to JOIN carriers) + 4 pure
    DataFlowValidators tests. (g5/b4) O1 relax inst_per_pub to key
    (pub_table, forcing_index) (Rel.cpp:4289/:4406/:4454, +#include <map>)
    + ADV-10 per-store coupling confirmed (real band-(b) anchors
    Database.cpp:2736/:2763/:2801) + the RelValidators O1 pair. (g6/b4)
    the NEW From-preserving witness demand_multi_adorn_witness (mono
    flagship, bare -demand + -demand-instance eqgate; suite 178->179,
    eqgate 4->5; demand_multi_adorn_1 STAYS diagnostic, reject moves
    :457->:717-722). (g2/b3) the ONE parameterized forcer/retract builder
    + the [F] always-on fence + the E2c gate-clone dedup + design-1 inline
    teeth by PURE EXTRACTION (CheckInstantiateEffects/CheckInstanceInputDrain,
    the CheckInstanceInputArm precedent — REJECTING b3's gValidateDROpsTestHook
    production global per the no-env-gated-debug-scaffolding memory;
    DEFER-if-entangled). RATIFICATIONS R-1..R-9 (R-1 g1 FIRST co-lands with
    g3; R-2 g3 owns R-DUP; R-3 O1 byte-neutral standalone; R-4
    PromoteSurvivorToBody b2-owned hard dep; R-5 b3 §3.4 test DELETED
    (uncompilable static symbols + toothless dormant arm) -> teeth = b2's
    pure unit; R-6 design-1 by extraction not hook; R-7 g6 mono-only,
    diff-witness deferred; R-8 ASAN pre-registered; R-9 R-DUP stage-(c)
    mechanism+verify). GATES PRE-REGISTERED (§6): Phase-A red EXACTLY +1
    (demand_multi_adorn_witness GOLDEN-MISSING); golden churn ZERO existing
    + the NEW witness files; [BYTE] on 176 stdouts x4 + 20 pinned + the 4
    frozen nested witnesses + both frozen diff witnesses (E2c) + config-
    invariance; ctest DataFlowValidators +4 + RelValidators +O1-pair
    (+design-1 if it lands); census the new witness kSubgraphInstantiate=2
    kInstanceSeal=2 kInstanceDeath=0 (MEASURE at stage (c), the D3.a.1
    under-prediction lesson); ASAN SUITE + ctest; the d7 L-table (both g1
    fold directions, L-keying-probe the R-DUP orphan tooth = a real HP-5
    divergence, L-O1-liveness positive+negative, the L15 support-1 honesty).
    STAGE-(c) LOAD-BEARING RESIDUALS (the two items this read-only stage
    could not close): the ResolveLiveRecognition x interposed-MERGE
    interaction (Rel.cpp:933-1022/:976-982) and the MEASURED witness census.
    Docs only; no code moved; binaries stand. NEXT: stage (d) — (d0)
    whole-slice-first-green in the WIP-committed prototype worktree
    (R-DUP forces this — no witness-alone baseline), the desired-states
    author lane + the BLIND prototype converging with pristine; then Fable
    review; then land as ONE commit. Landing record = §20(AS).

(AS) D3.a.3 LANDED (2026-07-30) — MULTI-ADORNMENT: N adornments of one
    query name -> N disjoint keyed stores over ONE shared pub, per
    d3a3-design.md as amended by the Fable review; desired-states record
    = d3a-desired-states.md SLICE 3; substrate = d3a3-substrate.md (incl.
    §6 the per-pub RULE-AT-CODE O1); design = d3a3-design.md + annexes
    b1-b4. MECHANISM AS LANDED: (g3) lib/DataFlow/Demand.cpp
    ApplyDemandTransform is a TWO-PHASE per-adornment loop over
    UniqueRedeclarations() (Phase 1 locate/check + seen_variants dedup +
    the all-free-sibling REJECT; Step 4 stray-consumer union ONCE between
    the loops; Phase 2 mint per adornment with snapshots-in-loop) +
    RE-MESSAGE :668/:723 (never narrow) + the R-DUP deferred grouped-by-
    (consumer,read) rewire (SINGLETON direct = [BYTE] for |plan|==1;
    MULTI-guard mints a MERGE UNION + rewires ONCE — the flat-arm
    reference-counted-union pub); the :444-462 per-name reject DELETED.
    (g1) lib/DataFlow/View.cpp + INTERNAL Query.h PromoteSurvivorToBody
    (kBody survivor policy in the both-set fold arm; the arm is DORMANT
    but this is the belt) + the stamp-time kBody-census belt + KEEP
    GuardAnnotationsCompatible (justification scoped to JOIN carriers) +
    4 DataFlowValidators tests. (R-9b) lib/DataFlow/Link.cpp
    ProxyMergedViews PRESERVES a guard annotation on its JOIN when the
    R-DUP union's identity restore collapses a guard into a direct MERGE
    member (recognition + cut-successor detection key on the annotated
    JOIN; dormant single-adornment) — the concrete stage-(c) resolution
    of the design's UNVERIFIED R-9(b) obligation, root-caused by the
    prototype. (g5/O1) lib/Rel/Rel.{cpp,h} CheckInstanceSolePub pure
    helper re-keys V-INST-SOLE inst_per_pub on (pub_table, forcing_index)
    (std::map; +#include <map>) so N forcings share one pub; +2
    RelValidators tests (the O1 pair: double-mint-one-forcing still
    aborts; two-forcings-one-pub PASSES). (g2) lib/ControlFlow/Build/
    Build.cpp ONE parameterized BuildQueryInjectorFromRegistry(...,
    is_retract) + the always-on [F] handler fence; (E2c) lib/CodeGen/
    CPlusPlus/Database.cpp the emit_edge_drain lambda called twice
    (band-a2/a2'). (g6) the NEW witness demand_multi_adorn_witness
    (q(bound A,free B)+q(free A,bound B) over rel(A,B):edge_2(A,B); mono
    flagship; .drflags -demand, .eqgate -demand -demand-instance) — the
    FIRST program minting kSubgraphInstantiate=2 (two stores, one pub);
    eqgate carriers 4->5. demand_multi_adorn_1 STAYS diagnostic (reject
    MOVES :457->:717-722). design-1 inline-teeth DEFERRED (R-6; a
    separable Rel.cpp refactor, follow-up); the .rel census pin dropped
    (the .irgold harness pins the FLAT arm — the nested kSubgraphInstantiate=2
    census is MEASURED, the neighborhood-witness precedent). STAGE-(d)
    THREE-WAY: BLIND prototype == BLIND DS-author BYTE-EQUAL on the
    witness .dr + edge set + 7-line golden + .drflags/.eqgate + the
    MEASURED census (kSubgraphInstantiate=2 kInstanceSeal=2
    kInstanceDeath=0 kIngestFold=3 kEagerForward=3 kCommitSweep=3
    kSeedFold=0 — matches the design prediction EXACTLY, no
    under-prediction); the DS lane MEASURED the golden VALUES by running
    the bf/fb halves in isolation. FABLE REVIEW (workflow, 9 agents: 4
    dimension lanes -> adversarial verify -> adjudicator): ONE HIGH that
    BLOCKED, zero refuted. FIXED pre-commit: the all-free-SIBLING SILENT
    WRONG-ANSWER (a query name with a bound AND an all-free adornment
    compiled but the all-free cursor read the demand-guarded pub and
    under-answered — the g3 prototype's bound_indices.empty() SKIP, whose
    "trips the Step-4 stray reject" justification was empirically FALSE,
    all adornments share ONE materialization) -> the skip became a clean
    reject; the orchestrator ADDED demand_multi_adorn_allfree_1 as the
    all-4-modes diagnostic (the reject had NO test — WHY it survived to
    review; suite 179->180) + 2 COSM stale-anchor respells (Rel.cpp:976->
    :977 x4, Build.cpp:1528->:1452 x2, Build.h twin-name). The HIGH fix
    proven DUMP-NEUTRAL. GATES (final bytes, orchestrator-executed;
    error-grep 0 x3 trees every rebuild): SUITE PASS(180) debug; ASAN
    SUITE PASS(179) + the new diagnostic rejects on the ASAN binary all 4
    modes; ctest 6/6 debug + 6/6 ASAN (DataFlowValidators +4,
    RelValidators +2); 20/20 pinned regen [BYTE]; the 4 frozen D3.a.1/
    D3.a.2 witnesses' nested h/.rel/.ir byte-identical (the Link.cpp
    dormancy proof); config-invariance both witness arms debug-3x
    single-hash + release==debug; E-62 CLEAN; new witness census
    kSubgraphInstantiate=2; eqgate carriers 5 (20 verdicts); Q5
    VACUOUS-BY-EARLY-RETURN (the demand pass early-returns off-mode; the
    corpus generated bytes provably unmoved — 20/20 pinned + 4 frozen
    witnesses identical — the ADJ-S8 discipline). RESIDUALS OPENED: the
    design-1 inline-teeth follow-up (V-INST-EFFECT/V-INST-DRAIN forked
    ValidateDROps teeth); the R-5 OB8(i) widening obligation still binds
    any future body-walk widening. NEXT candidates (owner re-ranks): the
    diff-multi-adorn witness (R-7 deferred); the design-1 follow-up; the
    header-token E-71 mini-diff; the agent-substrate direction; P2-P5.

(AT) DESIGN-1 FOLLOW-UP LANDED (2026-07-30) — the deferred D3.a.2/D3.a.3
    inline V-INST-EFFECT teeth, by PURE EXTRACTION (R-6, NOT the
    gValidateDROpsTestHook production seam the b3 lane proposed — that
    global is the no-env-gated-debug-scaffolding class the owner forbids).
    The V-INST-EFFECT effect-multiset TOTALITY of a kSubgraphInstantiate
    (the O-1 closure belt + the full regime-split count: drains/
    demand-drains/input-drains/demands/leaves/rebuilds/rebuild-sign/emits/
    olds and the diff-split counters/counter-signs/crossings/appends) had
    ZERO teeth — it lived only inline in ValidateDROps, and a one-token
    weakening (e.g. counters==2u->1u under diff) moves NO program's emitted
    output so the 180-case golden suite is BLIND to it. FACTORED into the
    pure `CheckInstantiateEffects(op, diff, input_diff)` (Rel.cpp/Rel.h) —
    `diff`/`input_diff` passed as BOOLEANS (no TABLE deref; the caller
    derives them via TableIsDifferential), so the negative space is
    death-testable with FAKE table pointers (the CheckInstanceInputArm/
    InputArmTest mold). The inline site now computes the two booleans and
    CALLS it, keeping the two V-INST-SOLE clauses (pub-alias + the
    induction-owned belt, the latter needing Context) inline —
    behavior-VERBATIM. NEW tests/RelValidators/InstanceEffectsTest.cpp:
    3 positive controls (the exact R-MONO / R-DIFF / input_diff
    signatures accepted) + 5 death arms with positive/negative pairing
    (E2 diff-with-single-counter; E1 negative-rebuild-sign + wrong-drain-
    role kProductInput; D1 input_diff-missing-removals-drain; the O-1
    closure input_diff-over-monotone-pub) — each mutation isolated from a
    valid baseline so it aborts for the RIGHT reason (the L15 masked-
    negative discipline). NOTE: the V-INST-DRAIN input-arm both-signs teeth
    the b3 lane also named is ALREADY provided by the landed
    CheckInstanceInputArm pure belt (D3.a.2, death-tested by InputArmTest)
    — design-1's genuine residual was only the totality, now closed.
    GATES (byte-neutral test-hardening; no codegen change): SUITE PASS(180)
    debug; ctest 6/6 debug + 6/6 ASAN (RelValidators +8: 3 accept + 5
    death); release + ASAN builds clean (error-grep 0 x3); the 4 frozen
    D3.a.1/2 witnesses' nested h/.rel byte-identical + demand_multi_adorn_
    witness kSubgraphInstantiate=2 unchanged; 20/20 pinned regen [BYTE].
    The R-6 DEFER-IF-ENTANGLED did NOT trigger (the extraction landed clean
    beside the O1 edits, no entanglement). Docs+test+refactor only; the
    design-1 residual is CLOSED. Remaining residuals: the R-5 OB8(i)
    widening obligation; the diff-multi-adorn witness (R-7).

(AU) SESSION SEED — RECURSIVE DEMAND + THE DEMAND COST MODEL (2026-07-30,
    docs-only, tip f3a55a8f). A design conversation on the demand pipeline
    produced a whole-program SEED not previously recorded:
    KeyedInstances.artifacts/recursive-demand-seed.md (SINGLE-PASS, ONE
    session, NO fleet — the next session's fleet re-derives + verifies it
    before code). Contents: §1 the two-lowerings frame (transform in
    data-flow — VERIFIED the .df/.dot are byte-identical between -demand and
    -demand-instance — vs the instance selector in control-flow); §2 the
    keyed-instance lowering as pseudocode distilled from the real generated
    C++ (band-a1 BIRTH / a2 REBUILD / b PUBLISH; the RESCAN a SINGLE monotone
    section-walk); §3 the TWO recursive fences (Build.cpp:1463/:1468
    cyclic_demand + :1452-1461/:1471 recursive_content) bottoming out at
    NeedsInductionCycleVector (Induction.cpp:10) + the OQ-INDUCTION-UNION
    DEFER ruling & §20(AB) precondition; §4 why it is hard (flat recursive
    demand lowers to an INDUCTION region with $induction_pivots cycle
    vectors + round shells, which a keyed instance would have to host
    PER-INSTANCE — the model today has NO fixpoint, only a monotone rescan);
    §5 the PATH FORWARD AS DIFFS (r0 model the instance-scoped round in the
    Rel-IR FIRST [the precondition]; r1 RESCAN becomes a fixpoint; r2 the
    mint grows a P-RECURSIVE axis, never folded; r3 fences narrow, R-5
    obligation binds; r4 differential recursive demand = a further slice);
    §6 the ORTHOGONAL PERF DIFF (p1 indexed rescan — the landed RESCAN is a
    full section-walk O(|input|) per touched key, `for s < table.NumRows()`;
    an index on the key cols makes it O(matches); normal mode already
    index-probes the bound column, so this is the one spot that regressed to
    a scan); §7 the DEMAND COST MODEL (two tiers — push-persisted inputs
    [unavoidable] vs pull-demanded derivation [prunable]; demand pays iff
    pruned-derivation > machinery-cost; message-rooted trivial shapes NEVER
    satisfy it, so their only justification is the MECHANISM, and recursive
    demand is where the OPTIMIZATION genuinely pays). NEXT-SESSION candidates
    (owner re-ranks): recursive demand (the seed's subject — largest gap);
    the p1 indexed-rescan perf win (cheap, self-contained); the R-7
    diff-multi-adorn witness; the E-71 header-token mini-diff; the
    agent-substrate direction; P2-P5. Baselines session-local; RE-SNAPSHOT
    from tip at next session open (binaries at f3a55a8f).

(AV) COST-MODEL EPOCH OPENED + THE IDENTITY-JOIN RECOGNIZER LANDED
    (2026-07-31). The session did NOT take the recursive-demand r0 target;
    the owner re-pointed it, across several in-session steers, at a COST
    MODEL (an analytic "is this sensical / does it explode" referee for
    every "do less" feature) and specifically at the magic-sets DOUBLE JOIN
    ("just because magic sets does the double join, it wasn't obviously
    justified"). WHAT LANDED (this commit): (1) THE DOUBLE JOIN GROUNDED on
    real IR (CostModel.artifacts/grounding-double-join.md): a 1-hop bound
    query is normal=0 joins (index probe) vs -demand=2 joins; the step-8
    query-projection guard (Demand.cpp:1086-1127, kQueryProjection) is a
    PROVABLE IDENTITY join on the supported slice (the push-down guard
    already constrains the bound column to the demand set; the projection
    guard re-joins a superset of itself, σ=1), surviving to codegen
    (kJoinEmit). (2) THE GENERAL FIX (owner's idea: "a transform pass to
    recognize degenerate double joins that are identities") as the SHARED-Prov
    program: lib/DataFlow/Prov.{h,cpp} = a keyset VALUE-containment provenance
    analysis (bot-default; SELECT seeds, JOIN pivot=union, MERGE=intersect,
    NEGATE/AGG/KV/@product/free-MAP=bot) + always-on V-PROV-* validators
    (V-PROV-BOT anti-fold + V-PROV-PIVOT anti-FABRICATION, the cycle-safe
    direction — a completeness `⊇` check false-aborted on inductive joins,
    found+fixed at validation); lib/DataFlow/IdentityJoin.cpp = the recognizer
    (forward J's users to the keep view when the guard side is an EXACT
    projection subsuming the keep pivot via Prov), gate df.ident_join
    (registered in PassPolicy), MONOTONE-FENCED (can_receive/produce_deletions
    /inductive → skip; the differential regime deferred). (3) VERIFIED by
    PREDICT-THEN-VERIFY ([[predict-then-verify-ir]], owner's standing
    methodology): a committed prediction P1-P8, all matched — mono df joins
    2→1, .rel kEagerJoin 4→2 / kJoinEmit 2→1, OWN-3 census intact (annotation
    rides CopyDifferentialAndGroupIdsTo), -demand-instance recognition
    survives (kSubgraphInstantiate=1), tc/multi-adorn/normal unchanged,
    differential fence holds; SUITE PASS(180), stdout byte-identical, mono
    .irgold MODE-SPLIT pin blessed (opt/nocf=2, nodf/none=4). (4) MEASURED
    (answering "is it make-believe": CostModel.artifacts/measured-calibration-1.md):
    dropping join.7 = ~45% fewer hash ops/probe via gBenchCounters, with the
    falsifiable law ΔidxAdds=F·K (F per demanded key) + N-independence (demand
    pruning is O(K·F) not O(N), measured). (5) THE COST MODEL RESHAPED from
    SYMBOLIC (the fleet's CostModel.md N·M algebra) to a NUMERIC SIMULATOR
    validated against gBenchCounters (owner steer: "actual numbers", "how are
    you evaluating these"); the .cost surface is a SCENARIO FAMILY
    ([[cost-scenario-family]]), not one workload. FLEET WORK: the cost-model
    design fleet (8 opus agents, TwoLayer IR-home, the double-join verdict) →
    CostModel.md; a pre-commit review fleet (Fable OVER QUOTA → re-run on opus,
    result pending at commit). SEED FOR STEPS 2/3 (the reshape + bin/Cost):
    CostModel.artifacts/cost-simulator-seed.md (whole-program pseudocode +
    path-forward r0-r6 + anchors) + next-session-prompt.md. This commit is the
    recognizer only (owner: "do 1"); the cost simulator is the next session.
    NEXT: reshape CostModel.md numeric (step 2) + build bin/Cost the numeric
    simulator (step 3), per the seed + prompt.

(AW) REGIONAL-DATAFLOW-CORE EPOCH OPENED — A DESIGN-GROUNDING TURN
    (2026-08-02, docs-only, tip f0c913e0). The owner's RegionalDataFlowCore.md
    (§20(AV)'s successor candidate: request edges + typed identity replacing the
    `-demand`/keyed-instance machinery — F1 two-demand-authorities, F2 physical-
    row-equality-as-identity, F3 demand-vs-cursor two-lifetimes, F4 no-request-
    owner) was GROUNDED, DIFFED, CRITIQUED, and TEST-DESIGNED across a six-phase
    fleet session; NO production code, NO goldens touched, NO commits. All
    artifacts in docs/proposals/RegionalDataFlowCore.artifacts/ (INDEX.md is the
    map). WHAT LANDED (as design artifacts): (1) THE PSEUDOCODE re-verified +
    EXTENDED (regional-arch-pseudocode.md, nine-extractor fleet): §4b the runtime
    epoch path (entry → ingest folds → eager web → stratum phases → publication
    tail → commit/compaction, placing LowerSubgraphInstances OUTSIDE all strata at
    the epoch tail) and §4c the generated cursor lifetime were newly derived,
    exposing F3 at emission grain (the RootRequestLease vs raw `uint32_t pos` are
    STRUCTURALLY UNCONNECTED APIs; the CLAUDE.md cursor contract rules out the
    CompactDead()-renumber hazard by CONVENTION, never by type). (2) FIVE STAGE
    DIFFS authored with golden-master exit gates (A typed identity + explicit
    projections; I0 the reference relational interpreter INSERTED per the review's
    Concern 1; B the canonical planning regional program; C request edges replace
    forcing — THE cutover, deleting ApplyDemandTransform/FabricateDemand*/
    Guard-Annotation/RecognizedSubgraph/ResolveLiveRecognition/BuildSubgraphInstance-
    Ops/BuildQueryInjector*/the three flags; D deep forest + instance-qualified
    local recursion). (3) ADVERSARIALLY CRITIQUED (opus panel, ≥4 lenses/stage +
    a refuter): 59 findings survived / 23 refuted; FIVE BLOCKING across four stages
    (A: T-conf-1/T-conf-2 cyclic-graph contract soundness — InferConservativeRow-
    Contracts is a single depth-ordered pass over a genuinely-cyclic contract graph,
    hard-rejecting the recursive corpus; I0: A-corr-1 the bound-query probe-
    enumeration contract — the flagship demand gate compares EMPTY content; C:
    corr-1 the force.dr `@first` byte-identity over-claim; D: T-oracle-4 the
    §0.1-abort-vs-E3-silent-full-materialize contradiction — the SAME decision as
    the review's Concern 2). Stage B is the ONLY clean stage. (4) COVERAGE AUDITED
    (phase3-coverage-audit.md): 5 §11 ORPHAN validators (3/5/6/9/10, concentrated
    at Stage C's H-I where the purity/port-agreement/sequestered-key/permanent-
    root/effects surface Stage B deferred under-delivers — X1/X2), 2 §12.3 UNHOMED
    witnesses (Permanent root, Effects), 3 §15 caveated invariants, X3
    (V-DEMAND-SUPPORT-DERIVED named-in-C-undefined-in-A), X4 (bound-query count
    drift), ZERO §14 ordering violations (every replacement lands ≤ its deletion).
    (5) FOUR DESIRED-STATE SURFACES authored + critiqued (the .df/.contract after A,
    the new -region-out grammar after B, the .rel lifecycle ops after C, the
    generated header after C), yielding owner-facing ADJUDICATION INPUTS AI-1..AI-8
    (the in-.df-vs-separate-sink role rendering; the G1/G2/G3 grammar; the 29→36
    census re-bless; tc's .rel under Variant A/B; the differential= flag; the
    PIVOTAL owner-identity-vs-refcount request-edge schema; A2-A5; the Stage-B
    ADJ-2/ADJ-3). (6) THE TEST MATRIX designed AND EMPIRICALLY VERIFIED at tip
    (test-matrix-proposal.md): a gate×mode COVERING ARRAY (13 configs, complete
    strength-2 CA over the 8 PassPolicy gates — the 4 golden modes flip gates in
    correlated blocks, leaving S-always-on and intra-df-body mixed pairs uncovered)
    + a FEATURE-MIXING corpus (14 crossings). VERIFIED: kvindex_1's canon-off
    compile/reject split (carve-out A, the sole compilation mode-split); the
    ident-off × -demand probe SAFE (recognizer tolerates raw_seed un-folded); the
    S=0 space SAFE on witnesses. PREDICTION-FAILED (four): PF-1/PF-2 the df.dfe
    SIGABRT — `-opt-disable=df.dfe` (canon/cse ON) aborts Stratify.cpp:420 on
    deadflowelimination_1/2/4 + recursion, root-caused to an UNSTATED, UNOWNED
    pipeline postcondition (canonicalization DESTROYS the merge/io-seam structures,
    DFE's underivable-cycle collection is the janitor that DELETES the residue;
    canon-ON+dfe-OFF = demolition without the janitor; the dead cycles are
    USER-AUTHORED LEGAL programs denoting empty relations, NOT corruption — the F1
    disease class recurring at the OPTIMIZATION layer, mis-binned on the optional
    side of the Optimize.cpp:884 required-hygiene line); PF-3 the demand-body reject
    vocabulary splits THREE ways (demand-SINK / R-MAT / R-BODYWALK, `!`↔`@never`
    indistinguishable by message); PF-4 `@barrier`/`:-` IS a demand fence today
    (the SIP walk does not traverse a barrier-staged join chain). (7) THE NECESSITY
    AUDIT returned a SMALLER-NOT-LARGER verdict: NO legacy-two-authority and NO
    zero-consumer mechanism survived the lens; all 5 simplification candidates are
    deferrals/reductions folding into stage diffs (top: strip the POPULATED
    RowContract.derivation_support, keep the F4 static_assert domain types; defer
    V-OWNERSHIP-ACYCLIC to its Stage-C birth). THE OWNER-DECISION QUEUE
    (owner-adjudication-brief.md, three tiers): T1 ratify A→I0→B→C→D +
    interpreter-before-cutover + tagged-binary oracle, the 5 blocking findings, and
    Concern 2's inadmissible-extraction semantics (Variant A uniform full-
    materialization vs Variant B retain-recursion-rejects — the MASTER decision the
    §6-vs-§11 bound-query-routing conflict blocks, binding Stage C H-J, Stage D
    T-oracle-4, AI-4, and the four clean diagnostics' fate); T2 the Stage-C H-I
    validator cluster + AI-1..8 + O-A1/2/3 + E-A2 + the R-DIFF oracle case + V-PI/
    V-CW; T3 the df.dfe hygiene/optimization split + Stratify-validator promotion
    (a standalone pre-Stage-A cleanup, effort S, with a FINDINGS.md F-record + an
    audit prompt to re-ask "whose absence breaks whom" of every gate), the
    PassPolicy.h stale 4-vs-5-gate prose fix, the 9-case pre-Stage-A landing set,
    and the necessity deferrals. SIX SESSION ERRATA recorded (the demand_diff_pub_1
    /_witness name drift; the residual bound-query count beyond X4; pseudocode §6's
    stale distinct-nodes Stage-A hunk; the df-stage-a §4.2 views=19→20 census error;
    the X1/X2/X3 validator-handoff mismatch; the PassPolicy.h prose). §19 acceptance
    for the design-grounding charter MET (verified pseudocode + diffs + critiques +
    desired states + test matrix + this draft + the brief + the updated prompt).
    NEXT: owner walks owner-adjudication-brief.md; then EITHER the pre-Stage-A
    cleanup slice (df.dfe split + PassPolicy prose + the 9 matrix cases + apply the
    Phase-3 amendments to the blocked stage docs) OR — if the owner ratifies the
    fast path — directly the Stage A implementation slice (typed identity +
    Member/Distinct enum-in-identity + SCC-aware InferConservativeRowContracts +
    explicit aggregate input key + lint→contract-validation), per next-session-
    prompt.md. STANDING RULE: the blocked stage docs MUST have their Phase-3
    amendments applied before any implementation of that stage begins.

(AX) ADJUDICATION + BRANCH A + STAGE A LANDED — THE FIRST PRODUCTION-CODE
    TURN OF THE EPOCH (2026-08-02/03, working-tree only, base f0c913e0,
    NOTHING COMMITTED). All 23 brief decisions adjudicated tier-by-tier
    (owner-adjudication-record.md is the authority): fast path ratified
    with D2.6 DEFERRED (the concurrency requirement recorded as the
    deciding fact; Stage-C header authoring paused), D1.4 = drop force.dr's
    query-time forcing, D3.4 candidate-3 = FLAT-KEY ratified. BRANCH A:
    F26 landed (the df.dfe hygiene/optimization split — CollectDeadCycles
    REQUIRED beside RemoveUnusedViews, TaintDerivedFromInput factored,
    Stratify's assert promoted to always-on V-SCC-SEAM; dfe-off full-corpus
    sweep 190/190 clean, covering-array carve-out B GONE, the 4 SIGABRT
    cases now directed witnesses); the whose-absence-breaks-whom audit
    (8 gates x 190 compile-only) leaves carve-out A as the ONLY coupling;
    F23 promoted (exit-139 no longer reproduces; JOIN-taint null guard;
    pinned as product_in_scc_diff_1); PassPolicy.h 4->5 prose fixed
    (Errata-6); Errata-2 resolved (49/181 bound-query cases at HEAD);
    Errata-5 applied (V-PORT-AGREE/V-PURE-REGION named at stage-c H-I,
    lines-3/6 softening stated, X3 miscite reconciled). THE 8-CASE D3.3
    LANDING SET LANDED (suite 181->190) and caught TWO REAL BUGS on day
    one: F27 FIXED (dataflow-opt phantom re-publish — 1-arm-MERGE
    elimination detached a monotone tap from its table, eager descent
    published UNGATED; fix = dedup table per table-less monotone stream
    insert in FillDataModel; zero golden fallout) and F28 FIXED (oracle
    modeled @invertible KV merges last-writer; now folds by declared
    algebra; zero existing-golden churn), plus F29 RECORDED (idx_43 scope
    bug, bound-query-over-KV codegen, repro in the record). demand_diff_pub_1
    proved the R-DIFF pub arm LIVE (answers retract through the
    @differential tap under standing demand; nested==flat all 4 modes) —
    the stage-c E1 oracle gap is closed. STAGE A LANDED under the standing
    method (anchors fleet-re-verified — BROKEN-2: IdentifyInductions
    membership is PARTIAL, resolved by STRATUM-based cycle membership
    post-Stratify; BROKEN-3: the two-site stamping story extended to a
    ~48-mint-site audit with kMember default; amendments applied as dated
    diffs; 5-lens refute-verified panel: 18 findings -> 4 surviving, all
    applied with fresh-dump grounding): lib/DataFlow/Identity.h typed ids
    + the F4 static_assert battery (ctest IdentityTypes), ProjectionRole
    folded into Equals ONLY (the Hash fold empirically perturbs hash-derived
    Rel/CF tie-breaks for zero CSE benefit — CSE buckets by cse_color,
    Equals decides; the T-oracle-1 "Hash differs" unit is unrealizable),
    InferConservativeRowContracts (two-phase pure graph function: Phase-1
    AllFields on multi-view strata, Phase-2 acyclic flat-key transfer,
    QueryImpl::row_contracts, post-Stratify slot), V-CONTRACT-CENSUS/
    V-MEMBERKEY-REALIZED/V-AGG-INPUT-KEY always-on + V-NO-COLLAPSE
    BELT-ONLY (the amended hard-abort false-fired on valid programs —
    AllFields keys make benign drops indistinguishable; E-A2's fallback
    taken), LintAggregateProjection DELETED (agg_distinct_1 zero warnings,
    stdout untouched), -contract-out sink + .contract irgold surface with
    THREE blessed goldens (agg_distinct_1, demand_tc_witness, join_1).
    All three H-A9 witnesses proven UNCONSTRUCTIBLE with evidence (role-OFF
    full-corpus sweep = zero .df diffs; refusal load-bearing only on
    dead-cycle shapes; AllFields floor makes V-AGG-INPUT-KEY unfireable
    from surface programs) — the stage doc's own fallback branches, no
    fabricated witnesses. PREDICT-THEN-VERIFY: keys/input-keys/census
    matched the desired states EXACTLY; the role= tally diverged (the
    structural stamp does not survive proxying — sole surviving kDistinct
    = a renaming clause head feeding a JOIN) and was adjudicated WRONG-
    PREDICTION, docs reconciled, produced dumps blessed. Owner directives
    landed mid-session: DOT digraph twins (dataflow -dot-out now renders
    role=/KEY(...) + cluster_stratum_<id> per multi-view stratum; Stage-B
    -region-out gains a cluster_region DOT twin; advisory, never goldened);
    two design conversations recorded as re-brief inputs (Mobius/DD: the
    counter split = DD's trip axis quotiented to a 2-point order, the
    claim matrices = hand-rolled inclusion-exclusion; shared arrangements:
    D2.6 IS the reader-handle question, compaction needs frontiers once
    leases outlive epochs, logical-origin provenance on models = Stage-B
    planning input). Exit state: SUITE: PASS (190), ctest 7/7, zero
    existing-golden churn all session, goldens changed ONLY by reviewed
    bless of NEW surfaces. NEXT: owner commits (or splits) the tree;
    I0 (the ranked-#2 interpreter, .probes contract ratified) opens; the
    Stage-C re-brief queue = D2.6 concurrency requirement + the §6-vs-§11
    routing rule + the arrangement/provenance direction.
    LANDED 2026-08-03 (session 3): the tree committed as the owner-ratified
    3-commit split — b61797cd (F26 dfe hygiene/optimization split + V-SCC-SEAM
    + F23 pin), bd694541 (the D3.3 landing set + F27/F28 fixes + F29 record),
    1d28a74c (Stage A: typed identity + row contracts + -contract-out + DOT).

(AY) STAGE B LANDED — THE FROZEN REGIONAL LAYER (2026-08-03, session 4).
    The full (a)-(d) standing-method design pass and the implementation
    landed in ONE session. Design: regional-arch-pseudocode.md Part B
    (fleet-verified Stage-B-grain pipeline; drift ledger DB-1..DB-13; the
    two-caller Query::Build ripple fact incl. bin/Oracle);
    stage-b-diff.md AMENDMENTS (DELTA-1..6 folded; 4-lens refute-verified
    panel, 12 findings -> 9 survived at re-triaged severities — the
    F-REGION-DEAD positive-presence referee, run_irgold region wiring,
    four-mode pinning, eqgate=6 correction, the NEC-1 library-placement
    constraint); regional-dump-stage-b-desired-states.md §9 (ADJ-2/ADJ-3-
    applied G1 blocks for FOUR witnesses, the R-STORE materialized-model
    rule, session-1's join_1 contract identity FALSIFIED by fresh dumps)
    + §9.7 (implementation reconciliation). Owner RATIFIED TO RECOMMENDED:
    ESC-4 variant (iii) (main-level free build, Query::Build untouched,
    lib/Regional an acyclic peer), Minimize DEFERRED, ESC-1/ESC-2, the
    4-witness pin set, no key-invariant token at B. Implementation:
    lib/Regional (FrozenRegionalProgram::Build at the Main.cpp third slot;
    G1 -region-out + advisory -region-dot-out; freeze scaffolds
    V-FROZEN-NO-OPEN-PORT / V-OWNERSHIP-ACYCLIC); H4 Program::Build
    consumes frozen via the thin frozen.Query() seam; the ALWAYS-ON
    V-REGION-CENSUS recount (stored == DeriveRegionalCensus(query)) at the
    ValidateDROps tail; 16 .region goldens blessed once after review.
    PREDICT-VERIFY ADJUDICATION (toward derivability, the Stage-A-#4
    pattern): R-STORE NARROWED to insert-materialized relations —
    merge-materialized interiors (demand_tc's `path`) are unnameable in
    ANY mode (QueryMergeImpl carries no decl link; ConnectInsertsToSelects
    removes relation inserts) — the concrete NECESSITY WITNESS for the
    reserved logical-origin-provenance direction. Record-only wart:
    implicitly-declared zero-arity exports mint without a name spelling
    (empty rel= in -region-out only; no pinned witness). Exit gate: suite
    PASS (190) with run_refinterp + the 16 region pins + V-REGION-CENSUS
    live; ctest 7/7; zero pre-existing goldens changed. NEXT: the owner
    ranks Stage-C re-brief (D2.6 + §6-vs-§11 STOPs; DIFF-R1's 15 normative
    amendments to fold) vs DIFF-R3 declared regions vs the R-STORE lift;
    entry prompt = next-session-prompt.md, seed = stage-b-landed-seed.md.
