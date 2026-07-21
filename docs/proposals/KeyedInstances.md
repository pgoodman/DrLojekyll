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

PIN-3 (owner, from the review — blocks only negate-carrying bless):
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
loops, NEGATE gates, terminal inserts/vector appends) plus the E-42
VECTORLOOP shim (ExtendEagerProcedure, Procedure.cpp — today minted
from NO DR-IR op).

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
          → LowerSubgraphInstance (Stratum.cpp, GroupUpdates mold): band-a1
            drain demand frontier → FindOrAddInstance → V-INST-FRESH →
            full-rescan → TryAdd (BIRTH-ONLY per RAT-6 — no a2 rebuild) →
            band-b (F,T)→+pub_row publish (the (T,F) drop scan EMPTY under
            R-MONO); seal self-lowered
          → codegen: SUBGRAPHINSTANCE region against the real InstanceStore
            (monotone=true; the flat guard web + %table:15 EXCISED — no Row15
            in the header)
        The eqgate (run_eqgate, runall.sh --one) is the STANDING referee:
        flat stdout == nested stdout (all four modes) == blessed .stdout
        golden (ANSWER identity, never generated bytes); the oracle closes
        the flat arm; DEATH is oracle-blind forever.

(C) LOUD RESIDUALS carried forward (harvested from §19(K)-(O)).
    - EDGE-AFTER-DEMAND = a LABELED FEATURE GAP (RAT-6/OD-3). a1-only birth-
      only silently drops a monotone edge added AFTER its demand is asked;
      the mechanism-natural edge frontier is provisioned-UNDRAINED (the OD-4
      accepted pricing). The rebuild plumbing (input-triggered rebuild via
      input frontiers) LANDS WITH/AFTER Rel, where it is the natural
      generalization; the witness flips diagnostic→golden then (aggregate_1
      precedent). Named in CLAUDE.md + a Build.cpp comment.
    - HP-17 the death-op / V-INST-ORDER EXECUTING-COVERAGE residual. kInstance
      Death is minted-OFF at R-MONO and V-INST-ORDER's corpus line is
      vacuous-green (0 instance ops); no test EXECUTES an ordered instance-
      op pair through the emitter. Per OD-11 this EXTENDS PAST Rel — D3.a
      (R-DIFF) retires it. Mitigations standing: the D2.a DrTest death half +
      the RAT-3 permanent CheckInstanceOrder negative. CARRIED LOUD.
    - OWN-3 (d2b-design §OWN-3 / ADJ-C4): the View.cpp:585-587 record-
      comparing incompatible-fold diagnostic must be PROMOTED always-on — a
      HARD D3 PRECONDITION before recursive demanded content is admitted.
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