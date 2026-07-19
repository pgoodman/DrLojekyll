# T2/T3/P1 — the BINDING spec, formulated against the fleet-verified record

Checkpoint step 2, KeyedInstances epoch. v2 (2026-07-18): v1 was
adversarially critiqued by the desired-states fleet (4 writers + 4
per-artifact critics + an xhigh spec critic, 9 agents ~1.03M tokens;
session scratchpad desired-states/critique-*.md); every amendment
below carries its finding tag. Written at tip b577735e against the
ckpt re-derivation fleet's consolidated record (fleet-ckpt/
consolidated.md; ledger §4 errata E-55..E-60). SUPERSEDES
ir-dump-formats.md §2 (memory-sourced) and its §2.5 placement (E-59);
refines epoch-diffs.md §T2/§T3.

STATUS: awaiting owner ratification of decisions (a)-(e) + the v2
sub-decisions (§5). The desired-output-state artifacts
(t2-desired-*.md) get one revision pass against the ratified spec,
then commit; implementation = "make the compiler print exactly this",
gated by diff.

--------------------------------------------------------------------
## 1. T2a — `-df-out <PATH>`: the DataFlow BB-with-arguments dump

### 1.1 Wiring (Main.cpp)

  + `static OutputStream *gDFStream` beside gDOTStream (Main.cpp:54-56);
    arg-parse arm on the -dot-out mold (:303-316); help-text line
    (:180-186).
  + DRAIN POINT: beside the -dot-out drain (Main.cpp:106-109), AFTER
    Program::Build — TableId() is populated (the :103-105
    abstraction-break note). Streams the Query through a thin
    `QueryDF` tag-struct operator<< so the DOT operator is untouched.
  + Emitter in lib/DataFlow/Format.cpp; decl in
    include/drlojekyll/DataFlow/Format.h.

### 1.2 Block identity — decision (a), RECOMMENDATION: det_seq

  - No finalized view-id space exists (no FinalizeViewIDs pass, no
    integer view-id field); UniqueId() is the impl POINTER (Node.h:31)
    — what -dot-out names nodes with — REJECTED (E-57).
  - det_seq (Query.h:472, DeterministicOrder()): pointer-free, TOTAL,
    dense 0..N-1 over LIVE views in ForEachView (per-kind DefList)
    order at last stamp. DENSITY MECHANISM (spec-critic 1.1, stated
    precisely): ForEachView SKIPS dead views (Query.h:1176-1214), so
    the stamp is dense over the live set; it stays dense at the drain
    because every pass between the last stamp (IdentifyInductions,
    Build.cpp:2597) and the drain is VIEW-NEUTRAL — FinalizeDepths
    (2602), FinalizeColumnIDs (2603), TrackDifferentialUpdates (2604),
    TrackConstAfterInit (2608), BuildEquivalenceSets (2626), Stratify
    (2627) — all independently verified view-neutral, and
    Program::Build creates/kills no Query view. A future view-killing
    pass in that window would gap the sequence SILENTLY, so:
  - THE EMITTER ASSERTS DENSITY, not just stamped-ness: max
    DeterministicOrder() over emitted blocks == block count - 1
    (fprintf+abort, survives NDEBUG — the always-on validator idiom).
  - Block id rendered `^<kind>.<det_seq>`, blocks ascending. The
    kind sequence is ForEachView's DefList order (Query.h:1178-1207):
    selects, tuples, kv_indices, joins, maps, aggregates, merges,
    compares, negations, inserts (read the code; the emitter shares
    the traversal, never re-derives it).
  - ALTERNATIVE (fresh FinalizeViewIDs-style renumber): rejected as
    redundant — a renumber sweep at the same traversal produces the
    same numbering (claim scoped to a sweep placed at the SAME point;
    v1's broader "by construction" phrasing dropped per spec-critic
    1.2). Mint it only if a future pass must re-stamp det_seq after
    Build.

### 1.3 The form (ir-dump-formats.md §1, refined + v2 rulings)

  - HEADER: the single line `dataflow` — NO clause reconstruction, no
    module-name dependency (tc-critic F8, demand-critic F7: clause
    text is not reliably recoverable post-optimize and a multi-line
    prose header is not emitter output).
  - COLUMN TOKEN: `<var-or-cN>:<type>` where var = the finalized
    column's variable name when present, else `c<id>` (finalized
    FinalizeColumnIDs id). NEVER `_MissingVar` (render c<id> instead);
    NO `@cN` suffix (demand-critic F4 — the v1-era writer addition is
    rejected; cN is a fallback, not an always-suffix).
  - BLOCK PARAMS (tc-critic F1/F2/root-cause MED — THE rename rule):
    a block's parameter list is the view's OWN finalized columns
    (names+types as finalized), NEVER the caller's names. Renames
    appear ONLY in `=>` arg maps as `dst=src` where dst is the USER's
    column token and src is the PRODUCER's column token.
  - `=>` LINES: one per user column-edge, ordered (user det_seq,
    port); `dst=src` maps; nothing else on the line.
  - CYCLE MARKER — v1's `; back-edge` (user det_seq <= def det_seq)
    is WITHDRAWN as UNSOUND (demand-critic F0: over-fires on non-cycle
    lower-id targets, and misses genuinely-cyclic det_seq-forward
    edges; symrec-critic F-C: inconsistently applicable). v2 rule,
    decision (a2) options:
      (i) RECOMMENDED: `; cycle` marked iff def is REACHABLE from
          user (the edge closes a cycle) — exact, deterministic, one
          memoized reachability pass in the emitter.
      (ii) fallback: no marker at all (minimal; cycles readable from
          the graph).
    Under (i) the tc inductive back-edges are marked and nothing
    else is.
  - JOIN GRAMMAR (symrec-critic F-B, demand-critic F1/F1b — pinned):
    inputs labeled `.in<K>` by joined_views position (deterministic
    UseList order); the block renders, in OUTPUT-COLUMN-POSITION
    order (the NthInputPivotSet/NthInputMergedColumn accessor order —
    Query.cpp:822-844, key-lookup not map iteration):
      `pivot <col-token> <- .in<J>.<col>, .in<K>.<col>[, ...]` per
        pivot set, then
      `out <col-token> <- .in<J>.<col>` per merged column.
    The header arity = ALL output columns including projected pivots
    (demand-critic F1: pivots are live outputs when consumed). No
    lhs/rhs role labels (not graph-carried).
  - INSERT: header `insert ^insert.<id> (<input col tokens>) into
    %table:<TableId()>` — input tokens are the PRODUCER's columns in
    input-position order (an INSERT owns no finalized output
    columns); NO ATTRIBUTES table= line (redundant — demand-critic
    F3); terminal, no `=>`.
  - CALLERS: `; callers: ^a, ^b` ONLY on MERGE blocks (the fan-in
    join point), ascending det_seq — MANDATED sort (tc-critic LOW:
    an unsorted caller walk is a determinism hole).
  - ATTRIBUTES line (only fields that apply): `table=%table:<id>`
    (TableId — printed on every table-backed view incl. shared-model
    pairs, faithful to the model), `class=<differential|monotone|
    table-less>`, `stratum=<Stratum()>`, and for induction members
    `set=<merge_set_id> depth=<InductionDepth>` (the DOT's SET/DEPTH
    surface, ratified in — tc-writer F7).
  - CLASS SEMANTICS PINNED (tc-writer F5, symrec-critic F-D):
    differential = backing table present AND deletion-capable
    (CanReceiveDeletions-derived); monotone = table present, not
    deletion-capable; table-less = no table. RECURSION DOES NOT IMPLY
    DIFFERENTIAL — a fully monotone recursive program (insert-only
    tc) is class=monotone throughout.
  - PRODUCER TAG — v1's conditional producer= is WITHDRAWN
    (spec-critic 4.1 HIGH): `producer` is #ifndef NDEBUG-only
    (Query.h:531-535; every write NDEBUG-guarded), which would make
    .df the ONLY config-VARIANT golden surface (.deltarel/.ir/.h are
    all config-invariant) — a release-preset suite run would
    spuriously fail every producer-tagged golden. v2: the DEFAULT
    dump NEVER prints producer; a `-df-out-verbose` variant (never
    blessed, never a golden) may add debug annotations later. The
    D1-review need (seeing demand-pass mint sites) is served by the
    fabricated names (demand__*) and structure, which ARE
    config-invariant.

### 1.4 Predictions (pre-registered, unchanged from v1)

Zero golden churn; suite stays 169; Q5 neutral; symrec_tie_1's
-df-out byte-stable across runs (tripwire extends at T3).

--------------------------------------------------------------------
## 2. T2b — `-deltarel-out <PATH>`: the DeltaRel dump

### 2.0 T2b.0 — THE HARDENING PRE-DIFF (new in v2; deltarel-critic
### C-1, CONFIRMED by the orchestrator at the source)

`op_table_id` in the band-key comparator returns
`reinterpret_cast<uintptr_t>(t)` (DeltaRel.cpp:3387-3394), used at
key_less level 4 (:3461). So `pinned_order`'s within-band op order —
notably the 8 commit sweeps sharing (lead=2, stratum=max+1, band=9)
on average_weight, and the seals at band 10 — is POINTER-ordered:
exactly the anti-pattern (F) purged from DataFlow. TODAY this is
emission-invisible (grep-verified: pinned_order's only consumers are
the validators, DeltaRel.cpp:3804-3981; the :3437 comment
"VALIDATOR-ORDERING ONLY — emission never reads this key" is
accurate; the corpus 8-run byte-stability is consistent). But (a) the
DUMP walks pinned_order, so the pointer order becomes emitted bytes,
and (b) dep-edge orientation for same-(lead,stratum,band) pairs is
key_less-decided (:3474ff), so validator VERDICTS are in principle
allocation-dependent — a latent flaky-abort class (F20's sibling).
T2b.0: harden op_table_id to the TABLE's deterministic id (the same
DataTable::Id() the .ir prints) BEFORE the dump lands. PREDICTIONS:
zero emission change (nothing emission-side reads the key — the
grep), validators stay green corpus-wide (any consistent total order
is a valid linearization), full-suite byte-identity. Any churn = a
consumer the grep missed — STOP and re-derive.

### 2.1 Wiring — decision (b') mechanism

RECOMMENDED (i): a lib/DeltaRel-owned sink
`SetDeltaRelDumpStream(OutputStream *)` (default nullptr; a null
sink is a guarded pure no-op — spec-critic 2.4), set from Main.cpp's
arg loop (gIRStream mold — CLI-wired, no env scaffolding). Rejected
(ii): publishing dr_flow on Program (new public surface for a
compiler-internal IR).

### 2.2 Emit point

Immediately after the context.dr_flow stash (Stratum.cpp:2166) —
past LinearizeAndValidateDRFlow (:2097) and the V-PRED-XCHECK/Site-5
block, reading `*context.dr_flow` (NOT the moved-from local
`dr_flow` — spec-critic 2.2; :2166 is a std::move). No return
executes between :2049 and :2166 (spec-critic verified), so the dump
always fires on a fully-validated graph.

### 2.3 The format (reconciled inventory + v2 rulings)

  deltarel                                   ; header, no module name
  vec $<role>.<idx> <shape> uniq=<contract> def=[...] use=[...]
     ...all vecs, vecs-vector (mint) order; role = VecRole spelling;
     shape from ElementShape (+ element types); debug_table
     cross-ref as %table:<id> when present...
  join.<idx> / branch.<idx> sections         ; first-class (v2 —
     deltarel-critic F-2): flow.joins / flow.branches in their own
     vector order, pivot/section fields rendered
  op.<idx> <DROpKind> ctx=<Ctx> sign=<+|-|·> stratum=<k>
    reads: <Pred spellings>
    effects: {<EffKind + live tagged-union fields per effect>}
    spine: <PlanNode chain: PlanKind + fields>
    args: <table (%table:<id>)/vec ($role.idx)/view refs>
     ...ops in pinned_order (post-T2b.0: fully id-deterministic)...
  rounds: ...DRRound body_ops/output_ops rendered under a
     `; substrate (unread by lowering)` banner...
  deps: one line per DRDep, flow.dep_edges VECTOR ORDER, EXHAUSTIVE —
     no elision (deltarel-critic C-3: a byte-golden cannot elide);
     loop_carried rendered here (not on vec lines)
  census: <per-kind counts re-derived via the count_kind mold,
     DeltaRel.cpp:2854-2879; keys stay out (E-28)>

  Bindings carried from v1 (all confirmed): op/vec ids = dense vector
  indices (no id fields exist); Pred spellings (DeltaRel.h:94-105,
  ten) not the build-layer order; EffKind spellings (:73-84, ten);
  kStateEmit/kStateOld stay under effects: (they are EffKinds, not
  Preds — deltarel-writer F-5 confirmed); stratum via keyed lookup of
  the *_stratum maps, never iterated; no stored band key — banding is
  visible as pinned_order grouping; `.` sigil separator for dump ids
  (deliberate divergence from the .ir's `:` — different id spaces,
  confirmed as intended); %table:<id> cross-references the .ir.

### 2.4 Predictions

T2b.0: zero emission change, suite byte-identical, validators green.
T2b dump: zero golden churn; suite 169; Q5 neutral. The dump is
deterministic BY CONSTRUCTION only post-T2b.0 — landing order is
T2b.0 → T2b. Carrier note (spec-critic 2.3): the (F)-gate value
needs graph-RICH carriers; average_weight (GROUP_UPDATE) qualifies.

--------------------------------------------------------------------
## 3. T3 — IR-golden sidecars (the permanent (F) gate)

### 3.1 Harness placement (E-58/E-59)

The compare arm lives in runall.sh's `--one` worker ONLY — a new
helper parallel to run_oracle(), guarded by sidecar existence,
invoked `run_irgold || st=1` beside runall.sh:248. diffrun.sh stays
the pure 4-mode primitive. The helper reuses the worker's OWN
flags_of(mode) (runall.sh:122-140), NOT diffrun.sh's copy
(spec-critic 3.1).

### 3.2 Sidecar + golden format — decision (c)

  - Sidecar `cases/<name>.irgold`: one file, one surface per line,
    `<surface> <mode>` with surface in {df, deltarel, ir, h} and mode
    in {opt, nodf, nocf, none}; bash-3.2 while-read loop.
  - Goldens `goldens/<name>.<surface>.<mode>.golden`; STRICT
    byte-compare (cmp -s); permcheck.py stays stdout-only.
  - PINNED PATHS (spec-critic 3.3 — both halves): the helper writes
    produced surfaces to `$WORKROOT/$NAME/irgold/<surface>.<mode>.out`
    (nested layout, matching run_oracle); the --bless mirror block
    reads EXACTLY that path into
    `goldens/<name>.<surface>.<mode>.golden`. No flat-layout
    exception (the kvindex_1 precedent is a wart, not a pattern).
  - VERDICT TOKENS (spec-critic 3.2): three failure modes, all
    grep-visible at runall.sh:284 — compile failure prints
    `IRGOLD-FAIL`, missing golden prints `IRGOLD-MISSING`, mismatch
    prints `IRGOLD-DIVERGE` (each contains a matched substring:
    FAIL/MISSING/DIVERGE).
  - The helper compiles once per pinned mode with .drflags appended
    (demand_tc_witness's goldens are demand-ON by construction),
    emitting all pinned surfaces from that one compile.
  - Blessing: --bless-only, never auto-on-red.

### 3.3 First carriers

demand_tc_witness: `h opt` + `ir opt` + `df opt` + `deltarel opt` —
THE restored permanent (F) gate. symrec_tie_1: `ir opt` + `df opt`.
Then (separate reviewed commits): fixpoint_stress_1, reconverge_1,
average_weight (`deltarel opt`). H-GOLDEN COST NOTE (spec-critic
3.4): an `h` golden pins datalog.h byte-for-byte — EVERY future
codegen change re-blesses it; keep `h` to the single demand witness
by policy.

### 3.4 Predictions

Suite count 169 (sidecars ride existing cases); goldens/ grows by
the blessed surfaces; zero churn on the 158 stdout goldens; Q5
neutral. The scripted 8-run -ir-out sweep retires only after the
demand_tc_witness + symrec_tie_1 sidecars are blessed and green.

--------------------------------------------------------------------
## 4. P1 — pass harness slice (decision (d): the slot)

Unchanged from v1 (spec-critic: CONFIRMED-SOUND): after T3, before
the D1 design fleet; reseeds next-epoch if tight. Facts: two toggles
(gOptimizeDataFlow/gOptimizeControlFlow, Main.cpp:46-52, set
:333-346) at exactly two Build call sites (:60-61, :66-67).
PassPolicy replaces the bools; legacy flags = exact aliases; default
config byte-identical. Sequencing note (spec-critic 6.2): the §1.3
producer ruling lands before any .df golden is blessed, so the P1
byte-identity baseline is config-clean.

--------------------------------------------------------------------
## 5. The owner decisions this spec brings

  (a)  -df-out block id: det_seq with the DENSITY assert
       (RECOMMENDED, §1.2); fresh renumber redundant; UniqueId
       rejected (E-57).
  (a2) NEW (v2): the cycle marker — (i) reachability-exact `; cycle`
       (RECOMMENDED) vs (ii) no marker. v1's det_seq-comparison rule
       withdrawn as unsound.
  (a3) NEW (v2): producer= dropped from the default dump (config-
       invariance of the golden class); -df-out-verbose reserved,
       never blessed. RECOMMENDED as stated.
  (b)  Dump positions: -df-out post-Program beside -dot-out;
       -deltarel-out at post-stash validate-exit reading
       *context.dr_flow; NO pre-optimize -df-out variant this epoch.
  (b2) NEW (v2): T2b.0 — harden the band-key op_table_id pointer
       tie-break to the table id BEFORE the dump lands (emission-
       neutral by the consumer grep; full-suite byte-identity gate).
       RECOMMENDED; landing order T1✓ → T2a ∥ (T2b.0 → T2b) → T3.
  (c)  Sidecar format: `.irgold` surface+mode lines;
       `goldens/<name>.<surface>.<mode>.golden`; strict byte-compare;
       pinned nested workdir/bless paths; IRGOLD-* verdict tokens;
       --bless-only.
  (d)  P1 slot: after T3, before D1 design; reseed-if-tight.
  (e)  PICK-A D1 witness: demand_neighborhood_witness.dr (drafted,
       compiles demand-ON + flag-off at b577735e); enters the suite
       at D2 with .batches/oracle goldens (witness-deltarel-target §3
       option-2 batches).
