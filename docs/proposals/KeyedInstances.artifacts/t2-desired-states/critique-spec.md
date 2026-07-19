# SPEC CRITIC — adversarial critique of t2-dump-spec.md (the T2/T3/P1 BINDING spec)

Reviewer pass at tip b577735e on branch keyed-instances. Target:
`docs/proposals/KeyedInstances.artifacts/t2-dump-spec.md`. Every anchor
below was RE-READ this session (file:line cited inline). Findings carry
severity; the file ends with a per-decision (a)-(e) adjudication.

Convention: CONFIRMED = I re-read the code and the spec's claim holds;
DEFECT = spec claim is wrong or unsupported; GAP = spec claim is true but
its stated justification is incomplete/misleading in a way that could
misdirect the implementer.

---

## ANGLE (1) — the -df-out block-id recommendation (det_seq) — §1.2 / decision (a)

### Finding 1.1 [GAP, medium] — the "dense over exactly the live views at the drain point" claim is TRUE, but the spec's justification is incomplete and one clause is materially misleading.

The load-bearing §1.2 claim is:

> "Last stamp: IdentifyInductions head (Build.cpp:2597) ... FinalizeColumnIDs
> (:2603) and Stratify (:2627) mint no views. So at the drain point det_seq
> is stamped, dense, and total over every live view."

I verified the CONCLUSION is correct, but by a chain the spec does NOT state,
and the spec's enumeration is wrong-by-omission:

1. The stamp loop (Induction.cpp:143-144) writes `v->det_seq = next_det_seq++`
   inside `ForEachView`, which SKIPS dead views (Query.h:1176-1214, every arm
   guarded `if (!view->is_dead)`). So det_seq is dense 0..N-1 over the LIVE set
   *at stamp time* — never over dead views. This is the actual mechanism that
   makes density hold; the spec never states that ForEachView filters dead
   views, which is the entire crux of "dense over exactly the live views."

2. The spec names only `FinalizeColumnIDs` (2603) and `Stratify` (2627) as
   view-neutral. The REAL post-IdentifyInductions pipeline (Build.cpp:2597-2627)
   is: IdentifyInductions(2597) → FinalizeDepths(2602) → FinalizeColumnIDs(2603)
   → TrackDifferentialUpdates(log,true)(2604) → TrackConstAfterInit(2608) →
   BuildEquivalenceSets(2626) → Stratify(2627). The spec SILENTLY OMITS
   FinalizeDepths, TrackDifferentialUpdates, TrackConstAfterInit, and
   BuildEquivalenceSets. I independently verified each of these creates no
   view and marks no view dead:
     - TrackDifferentialUpdates (Differential.cpp:41ff): grep for
       `.Create(` / `is_dead =` / `RemoveUnused` in Differential.cpp = EMPTY.
     - TrackConstAfterInit (Constant.cpp:94): const method, no mutation.
     - BuildEquivalenceSets (Build.cpp:2297): only `new EquivalenceSet(...)`
       (a data-model side object), attaches `view->equivalence_set`; no view
       create, no is_dead write.
     - FinalizeDepths (Link.cpp per consolidated §1.A): resets/recomputes
       depths only.
     - Stratify (Stratify.cpp): grep `.Create(`/`is_dead`/`RemoveUnused` = EMPTY.
   So the conclusion HOLDS — but the spec asserts it from a two-pass survey
   that misses four passes. An implementer trusting the spec's enumeration
   would not know to re-audit those four, and a FUTURE pass inserted at
   2604-2626 that marks a view dead (very plausible — differential tracking
   or equivalence-set merging are exactly the kind of passes that could kill
   a redundant view) would SILENTLY break density with no assert to catch it.

   RECOMMENDATION: the spec must (i) state the true mechanism — ForEachView
   skips dead views, so det_seq is dense over the live set at stamp AND the
   live set is frozen from IdentifyInductions to drain because NO pass in
   2598-2627 creates or kills a view; (ii) enumerate ALL six passes, not two;
   (iii) the dump emitter's assert (§1.2 "ASSERTS stamped") should be
   STRENGTHENED to also assert DENSITY (max det_seq == live-view-count - 1),
   so a future view-killing pass trips a loud abort instead of producing a
   golden with a silent gap in the id sequence.

### Finding 1.2 [DEFECT, low→medium] — the "same numbering by construction" claim for the ALTERNATIVE is not quite right, and the comparison it draws is the wrong axis.

§1.2 says the FinalizeViewIDs-style alternative "produces the SAME numbering
as det_seq's last stamp by construction (same traversal)." That is only true
if the hypothetical renumber pass runs at a point where the live-view set is
identical to the live-view set at IdentifyInductions' final stamp AND uses the
identical ForEachView traversal. The spec's own drain point is POST-Program::Build
(§1.1, Main.cpp:106-109) — but det_seq is last written during Query::Build
(IdentifyInductions, 2597). A renumber pass "at Build tail" (the spec's phrasing,
§1.2) is ambiguous between Query-build tail and Program-build; and Program::Build
can mutate view liveness/TableId assignment. The spec has not shown the two id
spaces coincide at the ACTUAL drain point — it asserts "same traversal" without
pinning where the alternative pass would run relative to the drain. This does not
change the recommendation (det_seq is still the right pick) but the stated
equivalence is unproven as written. Minor because the alternative is rejected
anyway.

### Finding 1.3 [CONFIRMED — no defect] — det_seq is stable through the POST-Program drain.

The spec drains -df-out post-Program::Build (§1.1). I verified this does not
disturb det_seq: (i) det_seq is written at EXACTLY two sites, both in DataFlow
(Induction.cpp:142/144, Optimize.cpp:285/287 — grep `det_seq =` over lib/
include/ = those 4 lines only); (ii) ControlFlow/Program::Build writes no
det_seq, marks no Query view dead, creates no Query view (grep over
lib/ControlFlow for `is_dead = true` / `*.Create(` on query DefLists / det_seq
= EMPTY). So the live-view set and det_seq are frozen from end-of-Query::Build
through the drain. The det_seq recommendation is SOUND on this axis.

---

## ANGLE (2) — the -deltarel-out emit point (Stratum.cpp:2166) + SetDeltaRelDumpStream — §2.1/§2.2 / decision (b)

### Finding 2.1 [CONFIRMED — no defect] — dr_flow is built ONCE per program, not per stratum. No rebuild hazard.

The spec's §2.1 premise (dr_flow lives as context.dr_flow, built in
BuildStratumPhases) is correct AND the once-per-program property holds:
`BuildStratumPhases` is called from exactly ONE site (Procedure.cpp:739),
which sits OUTSIDE the `for (auto io : query.IOs())` loop (that loop closes at
Procedure.cpp:703), inside `BuildEntryProcedure` (Procedure.cpp:625) — itself
built once per program. So dr_flow is constructed once, validated once, stashed
once (Stratum.cpp:2166). A SetDeltaRelDumpStream sink planted at 2166 therefore
fires exactly once per compile. The spec's "does BuildStratumPhases run once"
concern is resolved: YES, once.

### Finding 2.2 [GAP, low] — the dump at 2166 must read *context.dr_flow, NOT the moved-from local.

Stratum.cpp:2166 is `context.dr_flow = std::make_shared<DRFlowGraph>(std::move(dr_flow));`
— a std::move of the local `dr_flow`. Line 2167 rebinds
`const DRFlowGraph &flow = *context.dr_flow`. A dump planted "immediately after
the stash" MUST read `flow` / `*context.dr_flow`; reading the moved-from local
`dr_flow` is a use-after-move. The spec never states which handle. Trivial for a
careful implementer, but pin it: "the dump reads `*context.dr_flow`."

### Finding 2.3 [DEFECT, low→medium] — the (F)-gate claim requires a graph-RICH carrier; a near-empty deltarel dump exposes nothing.

BuildStratumPhases ALWAYS runs and ALWAYS stashes (the 2160-2166 comment; I
confirmed NO `return` executes between 2049 and 2166 — the only match in that
range is the comment). So a dump at 2166 always fires with a valid graph. But a
purely-monotone program yields a minimal graph; the spec's §2.4 claim "the dump
exposes any linearization nondeterminism byte-for-byte" only holds for a
graph-RICH carrier. The chosen deltarel carrier `average_weight` (§3.3) DOES
exercise a GROUP_UPDATE, so the claim holds for it — but the spec should state
the carrier-must-be-graph-rich requirement so future deltarel sidecars aren't
added on trivial cases that gate nothing.

### Finding 2.4 [GAP, low] — SetDeltaRelDumpStream must default nullptr with a pure no-op guard for the flag-off byte-identity claim to hold.

§2.4 claims "Flag-off suite untouched (runall.sh never passes the flag)." That
holds ONLY if the sink defaults to nullptr and the 2166 call is a plain
`if (stream) EmitDeltaRel(*stream, *context.dr_flow);` — no allocation, no
side effect when unset. The spec should pin this (a global read deep in a build
pass is the shape the "no debug scaffolding" house rule polices; matching the
gIRStream precedent means: Main.cpp-owned, nullptr default, guarded read).

---

## ANGLE (3) — the .irgold sidecar + harness — §3.1/§3.2 / decision (c)

### Finding 3.1 [GAP, low] — "compose exactly as diffrun.sh does" cites the wrong precedent; reuse the worker's own flags_of.

§3.2 says the helper composes drflags+mode "exactly as diffrun.sh does." The
--one worker has its OWN `flags_of(mode)` (runall.sh:122-140) doing mode→mflags
then `mflags="$mflags $(cat cases/$NAME.drflags)"` — a copy of diffrun.sh's
logic. The T3 helper lives IN the worker (§3.1) and should CALL that in-file
`flags_of`, not re-derive from diffrun.sh. Composition is otherwise exactly
right (drflags appended for every mode → demand_tc_witness goldens demand-ON by
construction).

### Finding 3.2 [DEFECT, medium] — verdict-grep trap only PARTIALLY covered: the COMPILE-FAILURE path is omitted.

§3.2 pins DIVERGE/MISSING but the helper has THREE failure modes, not two:
(a) byte-mismatch → DIVERGE; (b) golden absent → MISSING; (c) the compiler run
itself fails (crash/timeout/flag-error) → neither. The existing helpers cover
(c) with `DR-FAIL`/`ORACLE-FAIL`/`CXX-FAIL` (all contain `FAIL`, the summary
grep at runall.sh:284 matches `FAIL|DIVERGE|EXPECT-ERROR|MISSING`). If the T3
helper's compile-fail verdict lacks a grep substring, a compiler crash under
-df-out SILENTLY PASSES the suite. The spec must require the compile-fail path
to emit a `FAIL`-containing token. Sharpest T3 trap; undercovered.

### Finding 3.3 [DEFECT, medium] — the --bless mirror block has no specified source path; helper-write and bless-read can silently disagree.

§3.2: "a mirror block in the --bless loop copies the produced surfaces." The
existing bless loop (runall.sh:71-92) reads FIXED nested paths
(`$d$name.opt/stdout`, `$d$name.oracle/stdout`, `$d$name.monotone/stdout`). For
the mirror to work, run_irgold must WRITE each produced surface to a
deterministic path the bless block re-derives (e.g.
`$WORKROOT/$NAME/$NAME.<surface>.<mode>.out`). The spec pins NEITHER path.
Mismatch → bless finds nothing → golden never updates → next run DIVERGEs. Same
latent shape as the kvindex_1 flat-vs-nested exception (runall.sh:94-100). Pin
both paths before implementation.

### Finding 3.4 [GAP, medium] — the .h golden's bless-surface amplification cost is undersold by §3.4's "zero churn."

§3.4's "zero churn on the 158 stdout goldens" is true (new flags, no emission
path touched). BUT the `h opt` golden pins generated `datalog.h` byte-for-byte;
the header shifts under ANY codegen change (it even emits literal `#ifndef
NDEBUG` blocks, Database.cpp:2368/2400, plus the whole hidden-friend surface).
So every future codegen refactor forces a re-bless of this `.h` golden. That is
the intended (F)-gate cost, but the spec never states it adds a
high-sensitivity golden whose maintenance lands on all future codegen work.
Recommend: state the tradeoff explicitly; keep `h` to the SINGLE demand witness
(§3.3 already leans this way) — do not let `h opt` spread across the corpus.

### Finding 3.5 [CONFIRMED — no defect] — naming/cmp/bless/bash-3.2 all match precedent.

Dot-separated `<name>.<surface>.<mode>.golden` avoids underscore/case-name
collision; `cmp -s` is the existing byte-compare (runall.sh:172); --bless is
the sole write path checked first (67-103); no `declare -A` anywhere. A
`while read surface mode` loop over `.irgold` is bash-3.2-safe. permcheck.py
stays a stdout-token referee (never touches IR goldens) — correct.

---

## ANGLE (4) — the §1.3 producer-tag decision (config-VARIANT dump text)

### Finding 4.1 [DEFECT, HIGH] — the producer tag makes -df-out the ONLY config-VARIANT golden surface; every sibling IR-golden surface is config-INVARIANT. Avoidable inconsistency in the byte-compare golden class.

`producer` EXISTS ONLY under `#ifndef NDEBUG` (Query.h:531-535: the field at 534
is inside the `#ifndef NDEBUG ... #endif` bracket) and EVERY write is
NDEBUG-guarded (Induction.cpp:431; Demand.cpp:159/204/843/864/900/931/946/992;
Compare.cpp:160/202/483/503/616/617; Merge.cpp:305/551/756 — all inside
`#ifndef NDEBUG`). So a -df-out dump printing `producer=` emits text PRESENT in
a debug-built compiler and ABSENT in a release-built compiler.

Every sibling surface is config-INVARIANT (I checked):
  - .deltarel: DRVec.debug_table/debug_kind are plain always-present fields
    (DeltaRel.h:251-253 — "DEBUG annotation" names semantics, not storage; no
    #ifndef); NDEBUG in DeltaRel.h hits only always-on validator comments.
  - .ir: grep NDEBUG in ControlFlow/Format.cpp = EMPTY.
  - .h: CodeGen emits the STRING "#ifndef NDEBUG\n" into datalog.h
    unconditionally (Database.cpp:2368/2400) — same header text regardless of
    the compiler's OWN build config.

So §1.3's resolution ("producer IS golden-visible; the suite runs the debug
compiler, goldens stable") accepts a UNIQUELY config-coupled golden for `.df`
while every peer is config-clean. Consequences:
  (1) A release-preset suite run (`DR=build/release/bin/drlojekyll runall.sh`
      — a documented, valid invocation; CLAUDE.md lists both presets) would
      spuriously FAIL every `.df` golden carrying producer, with no assert
      explaining why. The harness pins no build config.
  (2) It breaks the "IR-golden class is a strict byte-compare surface"
      uniformity — one member silently means "strict byte-compare, DEBUG only."
  (3) demand_tc_witness — the flagship (F)-gate carrier — has the DENSEST
      producer tags (DEMAND-GUARD/DEMAND-SEED/DEMAND-RESTORE/... from Demand.cpp),
      so the config-coupling bites hardest exactly there.

RECOMMENDATION: make the df GOLDEN config-invariant. Preference order:
  (A) DROP producer from default -df-out; expose it only under `-df-out-verbose`
      (never blessed). D1 review — the only stated producer consumer
      (ir-dump-formats §1.2) — runs a debug compiler + verbose flag; the GOLDEN
      never sees producer; the whole IR-golden class becomes config-invariant.
      STRONGLY PREFERRED.
  (B) Keep producer in the golden but pin the golden build config: the harness
      must require the debug preset for any `df` sidecar and the spec must
      declare `.df` goldens debug-only-by-contract. Strictly worse (couples the
      golden to a preset with nothing enforcing it).
The spec currently picks (B)-WITHOUT-enforcement — the worst cell. Amend to (A).

---

## ANGLE (5) — the pre-registered predictions (zero churn / suite 169 / Q5 neutral)

### Finding 5.1 [GAP → cross-refs 4.1/3.4] — predictions hold for stdout, but two paths could violate them.

- "Zero golden churn / suite stays 169 / Q5 neutral" for the DUMP FLAGS
  themselves: SOUND. The flags are opt-in drain sites; runall.sh never passes
  them (confirmed: diffrun.sh/runall.sh have no -df-out/-deltarel-out string);
  no emission path is touched; the 4 modes are unchanged. Suite stays 169
  because sidecars ride existing cases (no new .dr).
- VIOLATION PATH 1 (config): if the suite is ever run against the release
  preset, the producer-tagged `.df` goldens fail — NOT zero churn, and a RED
  suite. See Finding 4.1. Amending to 4.1(A) closes this.
- VIOLATION PATH 2 (bless-path mismatch): if the run_irgold write path and the
  --bless read path disagree (Finding 3.3), the first bless produces goldens
  that the next run cannot reproduce → DIVERGE → suite RED. Pinning both paths
  closes this.
- VIOLATION PATH 3 (grep trap): a compiler crash under -df-out with a
  non-grep-matching verdict (Finding 3.2) makes the suite report PASS while a
  surface is broken — a FALSE "zero churn / 169." Requiring a FAIL token closes
  this.
- Q5 (perf) neutral: SOUND — the dumps are off the hot path (opt-in drains,
  post-build), and the SetDeltaRelDumpStream guard is a nullptr check when off.

### Finding 5.2 [CONFIRMED] — suite count 169, not 168.

The spec correctly predicts 169 (§1.4/§2.4/§3.4). CLAUDE.md:51 still says 168
(E-60 doc lag); the live checkout has 169 .dr (consolidated §1.C). The spec is
right; the stale doc is elsewhere.

---

## ANGLE (6) — P1-slot reasoning — §4 / decision (d)

### Finding 6.1 [CONFIRMED — sound] — the P1 slot reasoning and the threading facts are correct.

The two toggles are gOptimizeDataFlow/gOptimizeControlFlow, consumed at exactly
two Build call sites (Query::Build, Program::Build) — consistent with the
consolidated §4.d record (Main.cpp:46-52/60-61/66-67, set at 333-346). A
PassPolicy replacing the bools at those two sites with legacy-flag aliases and a
byte-identical default is a well-scoped, byte-identity-gated refactor. The slot
("after T3 lands + blessed, before the D1 design fleet; reseeds next-epoch if
tight") is sound: P1 is pure infra with a byte-identity gate, D1's design work
does not depend on it, and the non-blocking framing (§0.6.1) is respected.

### Finding 6.2 [GAP, low] — P1 should land AFTER the angle-4 producer fix, or its byte-identity gate could bless a config-coupled df golden into the P1 baseline.

Minor sequencing note: if T3 blesses producer-tagged `.df` goldens (pre-fix)
and P1's "default config byte-identical" gate then re-runs the suite, P1's gate
inherits the config-coupling. Landing the angle-4(A) fix BEFORE any `.df` golden
is blessed keeps the P1 baseline config-clean. Not a P1 defect; a sequencing
consequence of Finding 4.1.

---

## PER-DECISION ADJUDICATION (a)-(e)

(a) -df-out block id = det_seq.
    ADJUDICATION: RATIFY-AS-RECOMMENDED (det_seq), with a REQUIRED amendment to
    the JUSTIFICATION and the emitter assert (Finding 1.1): the spec must state
    the real mechanism (ForEachView skips dead views; the live-view set is
    frozen across ALL SIX passes 2597-2627, not the two the spec names) and the
    dump emitter must assert DENSITY (max det_seq == live-count-1), not merely
    "stamped," so a future view-killing pass aborts loudly instead of blessing a
    gapped golden. Raw UniqueId correctly REJECTED. The "same numbering by
    construction" claim for the rejected renumber alternative is unproven as
    written (Finding 1.2) — drop or qualify it.

(b) Dump positions: -df-out post-Program drain; -deltarel-out at Stratum.cpp:2166.
    ADJUDICATION: RATIFY-AS-RECOMMENDED. Both positions are sound (Findings 1.3,
    2.1, 2.3). AMEND with two implementer pins: the deltarel dump reads
    `*context.dr_flow` not the moved-from local (2.2); the sink defaults nullptr
    with a guarded pure-no-op read for the flag-off byte-identity claim (2.4).
    Add the carrier-must-be-graph-rich note (2.3) for the (F)-gate claim.

(c) Sidecar format: one .irgold per case, surface+mode lines, strict cmp,
    --bless-only.
    ADJUDICATION: RATIFY-WITH-AMENDMENTS. The format/naming/bless-only/bash-3.2
    design is sound (3.5). THREE amendments are load-bearing before code:
    (i) require the helper's COMPILE-FAILURE verdict to contain a `FAIL`
    grep-substring (3.2 — a suite-blinding trap otherwise); (ii) pin BOTH the
    run_irgold write path and the --bless read path to one convention (3.3);
    (iii) reuse the worker's in-file flags_of, not diffrun.sh (3.1). State the
    .h-golden bless-amplification cost and keep `h` to the single demand witness
    (3.4).

(d) P1 slot: after T3, before the D1 fleet; reseeds if tight.
    ADJUDICATION: RATIFY-AS-RECOMMENDED (6.1). Minor sequencing note: land the
    angle-4(A) producer fix before any `.df` golden is blessed so P1's
    byte-identity baseline stays config-clean (6.2).

(e) PICK-A D1 witness: demand_neighborhood_witness.dr.
    ADJUDICATION: OUT OF SCOPE for this critique (a D1 design-witness selection,
    not a T2/T3/P1 spec decision; no T2/T3/P1 code claim rests on it). No
    objection on the facts available (it compiles demand-ON and flag-off at
    b577735e per §5). Defer substantive review to the D1 fleet.

--- BEYOND THE FIVE MANDATED ANGLES ---

The single HIGH-severity finding is 4.1 (producer config-variance). Everything
else is medium/low: three harness pins (3.2/3.3 compile-fail token + bless-path
convention are the load-bearing ones) and a cluster of justification/completeness
GAPs where the spec's CONCLUSIONS are correct but its stated reasoning is
incomplete (1.1/1.2/2.2/2.4/3.1/3.4). No finding overturns a core recommendation
(det_seq, 2166 emit point, .irgold class, P1 slot all stand). The spec is
SOUND-WITH-AMENDMENTS.
