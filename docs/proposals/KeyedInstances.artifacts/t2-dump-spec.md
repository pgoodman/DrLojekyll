# T2/T3/P1 — the BINDING spec, formulated against the fleet-verified record

Checkpoint step 2, KeyedInstances epoch. v3 (2026-07-18, at tip
63c8443c): folds the §5 re-verification fleet's nine amendments
(ledger §6, errata E-61..E-66; committed record
s5-fleet-consolidated.md) — the E-61 kind-order transposition fix,
the seen-bitset density witness, the E-64 null-guard, the E-65
layout correction, the retained-sweep ruling, and the A1-A8
landing-gate riders. v2 (2026-07-18): v1 was
adversarially critiqued by the desired-states fleet (4 writers + 4
per-artifact critics + an xhigh spec critic, 9 agents ~1.03M tokens;
session scratchpad desired-states/critique-*.md); every amendment
below carries its finding tag. Written at tip b577735e against the
ckpt re-derivation fleet's consolidated record (fleet-ckpt/
consolidated.md; ledger §4 errata E-55..E-60). SUPERSEDES
ir-dump-formats.md §2 (memory-sourced) and its §2.5 placement (E-59);
refines epoch-diffs.md §T2/§T3.

STATUS: RATIFIED 2026-07-18 (owner, at the §6 brief): ALL EIGHT
decisions (a), (a2), (a3), (b), (b2), (c), (d), (e) ratified as
recommended/v3-amended — det_seq ids + bijection witness +
reachability-exact `; cycle` + no producer=; post-Program /
post-stash drain points + the lib-owned pre-guarded sink; T2b.0
before T2b with the E-64 null-guard as hard precondition; the
E-65-corrected `$NAME.irgold/` sidecar machinery with the retained
sweep; P1 after T3; PICK-A witness. The desired-output-state
artifacts (t2-desired-*.md) get one revision pass against THIS
ratified spec, then commit; implementation = "make the compiler
print exactly this", gated by diff; Fable review before each
emission commit.

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
  - THE EMITTER ASSERTS BIJECTION, not just stamped-ness (v3, §6
    fleet — density-critic S4/S5: `max == count-1` is necessary but
    NOT sufficient — a duplicate-with-gap stamp {0,1,2,2,4} passes
    it; the accessor's stamped-ness assert compiles OUT under NDEBUG
    (Query.cpp:431), so this check is the ONLY always-on authority in
    release; and the literal `count-1` underflows to ~0u at N==0):
    two-pass seen-bitset — pass 1 counts N over the traversal; pass 2
    checks every RAW stamp s < N (subsumes the ~0u unstamped
    sentinel) and no-duplicate via the bitset. A
    bijection-onto-{0..N-1} witness, release-safe, N==0-safe
    (fprintf+abort, survives NDEBUG — the always-on validator idiom).
    Sum/XOR fingerprints REJECTED (cancellation: {0,1,1,3,5} fools a
    sum).
  - Block id rendered `^<kind>.<det_seq>`, blocks ascending. The
    kind sequence is ForEachView's DefList order (Query.h:1176-1214):
    selects, tuples, kv_indices, joins, maps, aggregates, merges,
    NEGATIONS, COMPARES, inserts (E-61: v2 transposed
    compares/negations — the code pushes negations BEFORE compares in
    both overloads, Query.h:1196-1204/:1248-1258; the density witness
    is ORDER-INDEPENDENT and would NOT catch a golden hand-blessed
    from the transposed prose. Read the code; the emitter SHARES the
    traversal, never re-derives it — and any .df golden is diffed
    against a code-derived traversal, never prose).
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
  - v3.1 SESSION-PINNED EMITTER RULINGS (from the artifact-revision
    critique round — grammar points the byte-golden artifacts cannot
    leave open):
      (p1) UNIFORM BLOCK HEADER: `<kind> ^<kind>.<det_seq> (<own
           finalized column tokens>)` for ALL kinds INCLUDING select
           and join — no `() -> (...)` arrow form, no `[pivot ...]`
           header tag (redundant with the body pivot line); the
           leading keyword always equals the id kind (`select`, not
           `recv`); a select's message provenance renders in the
           trailing `;` comment.
      (p2) IDENTITY EDGES: in `=>` maps, an identity mapping renders
           the BARE token; `dst=src` appears ONLY when the names
           differ. Applies to producer-side edges into joins too.
      (p3) PRODUCER-SIDE JOIN EDGES: a producer block DOES emit its
           `=> ^join.<id> .in<K> (...)` line (tail-call
           completeness), rendering the producer's OWN tokens (bare
           where identity per p2); the join block owns the role
           mapping. The .in<K> assignment is joined_views UseList
           position — artifacts carry it as PREDICTED until the
           first bless code-reads it.
      (p4) `reads:` renders Pred spellings ONLY; a frozen-InI read
           is the kInIReadFrozen EffKind and renders under
           `effects:`, never on `reads:` (deltarel surface).
  - v3.2 SESSION-PINNED EMITTER RULINGS (2026-07-19, the round-2
    grammar unification — E-70: the four committed byte-exact
    contracts each rendered a DIFFERENT grammar (identity maps,
    .in<K> arg form/order, ATTRIBUTES keyword, prose comments,
    comment columns, join-body form, INSERT typing/attributes)
    because the v3.1 pins were applied only to the artifact whose
    critique minted them; one emitter cannot match four grammars, so
    the T2a byte-gate was unsatisfiable as committed. These pins
    close every divergence axis; all §1 blocks were re-rendered
    under them in the same commit, graph facts untouched):
      (p3-order) a producer-side `.in<K>` line's entries are the
           producer's OWN tokens in the JOIN's output-column-
           position order RESTRICTED to refs into `.in<K>` (the
           same port order the join body renders) — never the
           producer's own column order, never dst=src role maps
           (the join block owns the role mapping).
      (p5) the attributes line ALWAYS carries the leading
           `ATTRIBUTES` keyword, on every block including INSERT.
      (p6) TRAILING-COMMENT COLUMN: every emitter comment
           (provenance, `; cycle`, `; callers:`) starts at byte 52
           — content padded with spaces through byte 51; content
           ≥ 51 bytes gets exactly one space before the `;`.
      (p7) SELECT provenance spelling: `; recv #message
           <name>/<arity>` for message receives; `; relation
           <name>/<arity>` for relation selects.
      (p8) NO PROSE: a .df byte-block contains ONLY emitter-
           derivable comments — p7 provenance, `; cycle`,
           `; callers:`. Hand annotations live in derivation prose,
           never in the block. INSERT lines carry no comment.
      (p9) JOIN body: 2-space body indent; `pivot <col-token> <-
           .in<J>.<name>[, ...]` then `out <col-token> <-
           .in<J>.<name>` lines; lhs col-tokens TYPED (the §1.3
           column-token definition applies everywhere, including
           INSERT input tokens), ref names bare and untyped,
           single-space separators, NO alignment padding; closing
           `}` at column 0.
  - v3.3 SESSION-PINNED RULINGS (2026-07-19, the round-3 T2b grammar
    adjudication — 1 opus lane + 1 adversarial verifier, verdict
    SOUND; ledger §11):
      (p10) THE DELTAREL BYTE-BLOCK CARRIES ZERO COMMENTS (the p8
           analogue): no `;;` banners, no `;` annotations — the
           section leads are the bare keywords (vec/branch./join./
           op./rounds:/deps:/census:). Unlike .df, no derivable
           comment class is defined for this surface.
      (p11) SECTION LAYOUT: single blank line between sections,
           none within a section.
      (p12) NO-SOURCE FIELDS NEVER RENDER: the join section-walk
           subline (%index ids are ControlFlow-only), the
           `receive=<recv ...>` wrapper, functor-name glosses in
           spine/args (PlanNode has no functor field), and the
           `branch=<..>` composite (redundant with src=/tgt=) are
           all STRUCK; commit sweeps render publish_target
           uniformly (a real stored bool); INGEST_FOLD spine = `—`;
           census = the 15 DROpKind counts in ENUM order, one line.
           A field with no stored DR-IR source is dropped or the
           field is ADDED to the model — never guessed at emission.
      Notes: sign glyph `·` and spine `—` are the corpus's first
      multibyte golden bytes — byte-verify both at first bless
      (G-2). `mutable(new_weight_i32)` is the live .ir spelling of
      the KV summary type (G-1). Config-invariance pre-audit of
      every rendered field: CLEAN (zero NDEBUG-gated members).
  - v3.4 SESSION-PINNED RULINGS (2026-07-19, the T2b first-emission
    adjudication — ledger §15):
      (p13) MECHANICAL WHITESPACE on the deltarel surface:
           single-space field separators, no column padding,
           effects on ONE line. (The p9 no-alignment precedent;
           the artifacts' hand alignment was unpinned and
           internally inconsistent.)
      (p14) DEPS = sorted by (from, to, kind, scope, carried) AND
           exact-duplicate rows DEDUPED (the flag-enrollment walk
           mints one row per access pair; the section renders the
           dependence RELATION — dedup elides no edge).
      Rulings: agg= renders the DERIVABLE functor name (kOver: the
      aggregate functor; kKv: the merge functor) — no relation-name
      model field; vec def=[] is the faithful render where the
      model registers no defs (overdelete/addition/net/join-pivot
      classes — def enrollment is a recorded model-fidelity
      improvement, surfaced via re-bless when added); spine `—`
      uniform on all join-pivot seeds. CONFIG-INVARIANCE PROVEN
      DIRECTLY at first emission: debug and release binaries
      produce byte-identical dumps.
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
-df-out byte-stable across runs (tripwire extends at T3). v3 rider
(predictions-critic A1): before any .df golden is blessed, EYEBALL a
real `%table:<id>` in the output — the bijection witness guards view
NUMBERING only, not table-id population (the post-Program drain is
what populates TableId; a mis-drained emitter would print empty
table ids deterministically).

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
emission-invisible (grep-verified, restated precisely per E-62:
pinned_order's consumers are the VALIDATORS (:3817-3981) plus the
NEVER-READ body_ops/output_ops substrate-fill loop (:3804-3815 —
DRRound substrate, "populated, never read", DeltaRel.h:592, zero
readers repo-wide); none emission-reaching. pinned_order itself is a
Kahn topological linearization whose ready-set tie-break is key_less
(:3702-3739), not a flat stable_sort. The corpus 8-run byte-stability
is consistent). But (a) the
DUMP walks pinned_order, so the pointer order becomes emitted bytes,
and (b) dep-edge orientation for same-(lead,stratum,band) pairs is
key_less-decided (:3474ff), so validator VERDICTS are in principle
allocation-dependent — a latent flaky-abort class (F20's sibling).
T2b.0: harden op_table_id to the TABLE's deterministic id (the same
DataTable::Id() the .ir prints — `t->id`, in scope via ControlFlow's
Program.h) BEFORE the dump lands. v3 MANDATORY AMENDMENTS (§6 fleet,
E-64 + t2b0-critic A/B/C + predictions-critic A3):
  - NULL GUARD (E-64, the diff CRASHES without it): `t` is the first
    non-null of six DROp table fields, else `op.fire_table`, which is
    itself NULL for kNegateGate(eager)/kSeedFold/kChainFold/
    kPivotAssemble ops — today reinterpret_cast maps null→0
    harmlessly; a bare `t->id` null-derefs (exit 139) on nearly every
    corpus case. The hardened form is `return t ? uintptr_t(t->id) +
    1u : 0u;` — AS-LANDED STRENGTHENING (the T2b.0 Fable review's
    CONFIRMED finding): the v3-recommended `t ? t->id : 0u` relied on
    the "real ids ≥ first_id+3" invariant, which is FALSE under
    unsigned wraparound — the unvalidated `-first-id` flag
    (Main.cpp:329, strtoul into unsigned) at ≥ UINT_MAX-2 wraps
    next_id so the first table mints id 0, silently colliding with
    the null sentinel (no validator catches it — key_less stays a
    total order). The +1 shift into the 64-bit key space (table ids
    are 32-bit) makes the sentinel disjoint BY CONSTRUCTION and
    preserves real-table order.
  - PRECISE RATIONALE (replaces v2's "any consistent total order is a
    valid linearization", which wrongly implies inertness): the
    tie-break feeds pinned_order POSITION and same-band dep-edge
    ORIENTATION; emission reads neither, and every pos-comparing
    validator (V-RETIRE-AFTER, V-LOOP, V-READY) pairs ops that differ
    in a key field ABOVE table_id (lead/stratum/band) or reads the
    stratum maps — so validator verdicts cannot flip on the level-4
    key.
  - TRIPWIRE (E-62 rider): the emission-neutrality argument holds
    ONLY while body_ops/output_ops stay reader-less — RE-GREP their
    readers at implementation time, and again whenever an R2+
    lowering lands; a reader voids the argument.
  - STALE COMMENT (predictions-critic A3): the ~:3971-3973 comment
    "the band key IS the emission driver's walk order" is
    aspirational — emission walks construction order (Stratum.cpp
    :1071-1073 disclaims the band key). Correct it in the same diff.
PREDICTIONS:
zero emission change (nothing emission-side reads the key — the
grep), validators stay green corpus-wide, full-suite byte-identity
(the suite gate is SUFFICIENT for verdict flips: validators are
fprintf+abort, so a flipped verdict = compile failure = visible red).
Any churn = a consumer the grep missed — STOP and re-derive.

### 2.1 Wiring — decision (b') mechanism

RECOMMENDED (i): a lib/DeltaRel-owned sink
`SetDeltaRelDumpStream(OutputStream *)` (default nullptr; a null
sink is a guarded pure no-op — spec-critic 2.4; v3 sharpening,
predictions-critic A5: the guard is a PRE-guard — `if (stream) {
format... }` — never format-then-discard, because the hook fires on
EVERY Program::Build (169 cases × 4 modes per suite run) and
off-path formatting work is invisible to the bench yet taxes every
compile), set from Main.cpp's
arg loop (gIRStream mold — CLI-wired, no env scaffolding). Rejected
(ii): publishing dr_flow on Program (new public surface for a
compiler-internal IR).

### 2.2 Emit point

Immediately after the context.dr_flow stash (Stratum.cpp:2166) —
past LinearizeAndValidateDRFlow (:2097) and the V-PRED-XCHECK/Site-5
block, reading via the `flow` ref bound at :2167 (`const DRFlowGraph
&flow = *context.dr_flow`), NEVER the moved-from local `dr_flow`
(spec-critic 2.2; :2166 is a std::move — dumping the local is
UB/empty). v3 (§6 fleet loud finding 6): drain in the :2167-:2199
window BEFORE the no-phase-work early return (:2200-2203), or
unconditionally, so no-phase programs still emit a (small) dump. No
return
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
  deps: one line per DRDep, CANONICALLY SORTED by (from, to, kind)
     BY THE EMITTER (v3.3/F-9, round-3 CONFIRMED at code: dep_edges
     is appended from TWO std::unordered_map traversals — by_vec +
     by_flag — so its vector order is hash-order, NOT byte-stable;
     T2b.0 did not fix this second hole; an unsorted deps golden is
     unblessable), EXHAUSTIVE — no elision (deltarel-critic C-3);
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

  v3 additions (§6 fleet):
  - VALIDATOR TOKENS: any validator name the dump or a census renders
    MUST be harvested from the emitted `ValidatorFail(...)` STRING
    LITERAL, never a comment title or CLAUDE.md — live drift exists:
    the final monotonicity validator emits `V-OLD-EQUIV(order)`
    (DeltaRel.cpp:3982) under a comment block titled
    V-ORDER-CONSISTENT.
  - CONFIG-INVARIANCE AUDIT (predictions-critic A4): before ANY
    deltarel golden is blessed, audit every §2.3-rendered field for
    `#ifndef NDEBUG`-gated members — the exact trap the (a3) producer
    ruling fixed for .df has NOT yet been run on this surface; a
    debug-blessed golden must not fail under the release preset.

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
flags_of(mode) (runall.sh:122-134 — E-66 cite fix; called UNQUOTED,
`$DR $CASE $(flags_of $mode) ...`, the deliberate word-split idiom
the existing callers use), NOT diffrun.sh's copy
(spec-critic 3.1). v3 GUARD (irgold-critic D-C / loud finding 10):
`.irgold` sidecars are permitted ONLY on all-4-modes-clean golden
cases — the always-diagnostic set and mode-split kvindex_1 exit-1
inline and would yield spurious IRGOLD-FAIL (both first carriers
qualify).

### 3.2 Sidecar + golden format — decision (c)

  - Sidecar `cases/<name>.irgold`: one file, one surface per line,
    `<surface> <mode>` with surface in {df, deltarel, ir, h} and mode
    in {opt, nodf, nocf, none}; bash-3.2 while-read loop.
  - Goldens `goldens/<name>.<surface>.<mode>.golden`; STRICT
    byte-compare (cmp -s); permcheck.py stays stdout-only.
  - PINNED PATHS (spec-critic 3.3 — both halves; v3 layout per E-65):
    the helper writes produced surfaces to
    `$WORKROOT/$NAME/$NAME.irgold/<surface>.<mode>.out` — the
    `$NAME.<kind>` SIBLING-dir shape run_oracle actually uses
    (runall.sh:190,210 write `$NAME.oracle`/`$NAME.monotone`; v2's
    bare `irgold/` subdir was a THIRD layout matching nothing —
    E-65). The separate dir stays mandatory (never clobber diffrun's
    `$NAME.<mode>` stdout dirs). The --bless mirror block reads
    EXACTLY that path into `goldens/<name>.<surface>.<mode>.golden`,
    guarded `[ -d "$d$name.irgold" ]`. No flat-layout
    exception (the kvindex_1 precedent is a wart, not a pattern).
  - ATOMICITY + DISCIPLINE (predictions-critic A6/A7): a sidecar and
    its goldens land in the SAME commit (a sidecar without goldens =
    instant IRGOLD-MISSING red); IR goldens are STRICTER than
    permcheck'd stdout — never re-bless to green on a red diff
    without the reviewed-truth ritual.
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
average_weight (`deltarel opt` — blessable ONLY after T2b.0, else
the golden bakes pointer order, the (F) anti-pattern). H-GOLDEN COST
NOTE (spec-critic
3.4): an `h` golden pins datalog.h byte-for-byte — EVERY future
codegen change re-blesses it; keep `h` to the single demand witness
by policy.

### 3.4 Predictions

Suite count 169 (sidecars ride existing cases); goldens/ grows by
the blessed surfaces; zero churn on the 158 stdout goldens; Q5
neutral. SWEEP RETIREMENT, rewritten per irgold-critic D-B (v3): the
per-run golden byte-compare and the N-run allocation sweep measure
ORTHOGONAL classes — the golden pins the canonical VALUE (catches
deterministic regressions the sweep is value-blind to); the sweep
proves ABSENCE of run-to-run flakiness (a one-sample-per-run compare
turns residual 1/N flakiness into an unattributable intermittent
red). So after the demand_tc_witness + symrec_tie_1 sidecars are
blessed and green, the sweep retires as the ROUTINE per-commit step
ONLY — it is RETAINED as the acceptance gate for any
determinism-substrate-touching change.

--------------------------------------------------------------------
## 4. P1 — pass harness slice (decision (d): the slot)

Unchanged from v1 (spec-critic: CONFIRMED-SOUND): after T3, before
the D1 design fleet; reseeds next-epoch if tight. Facts: two toggles
(gOptimizeDataFlow/gOptimizeControlFlow, Main.cpp:46-52, set
:333-346) at exactly two Build call sites (:60-61, :66-67).
PassPolicy replaces the bools; legacy flags = exact aliases. v3
widening (predictions-critic A8): the byte-identity prediction
covers ALL FOUR suite modes, not just the default config — the
golden gate exercises all four (df,cf) bool pairs; pin PassPolicy's
default to (df=true, cf=true) at both Build sites and map the four
modes to today's exact flag combinations, or a mode silently churns.
Sequencing note (spec-critic 6.2): the §1.3
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
       v3: the E-64 null-guard (`t ? t->id : 0u`) is a HARD
       precondition — the diff crashes corpus-wide without it.
  (c)  Sidecar format: `.irgold` surface+mode lines;
       `goldens/<name>.<surface>.<mode>.golden`; strict byte-compare;
       pinned `$NAME.irgold/` sibling-dir workdir + bless paths
       (E-65-corrected); IRGOLD-* verdict tokens; --bless-only;
       all-4-modes-clean cases only; the N-run sweep RETAINED as the
       substrate-change acceptance gate (retires as the routine
       per-commit step only).
  (d)  P1 slot: after T3, before D1 design; reseed-if-tight.
  (e)  PICK-A D1 witness: demand_neighborhood_witness.dr (drafted,
       compiles demand-ON + flag-off at b577735e); enters the suite
       at D2 with .batches/oracle goldens (witness-deltarel-target §3
       option-2 batches).
