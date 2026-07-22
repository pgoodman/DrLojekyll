======================================================================
COMMITTED AT THE R1 LANDING (2026-07-22; the Rel epoch's first
step-kind migration, per OD-12's sequencing). The BINDING adjudicated
implementation contract: 3 build-out lanes + xhigh designer + 3
adversarial critics (the correctness critic recovered from disk after
a structured-output death; its F2/F3/F4 adjudicated personally by the
orchestrator at the code as ADJ-S12/S13/S14) + xhigh adjudicator;
amendments ADJ-S1..S14 applied IN PLACE. POST-LANDING NOTE: the Fable
review deleted the A.6(c) chain-breaker arm as tautologically dead
(finding [1] — the cut discipline is not view-kind-checkable; the
standing controls are the A/B net, the eqgate, and the ADJ-S10 count
read) and single-sourced the INSERT-mint message extraction ([3]).
======================================================================

# R1 design — the monotone eager web's TUPLE-forward + terminal-INSERT arms as DR-IR ops

Ritual stage (b), xhigh. This is R1 formulated **as a diff on the build-out
pseudocode** (`r1-buildout-arms.md` / `r1-buildout-model.md` /
`r1-buildout-contracts.md`), reconciled against the code at tip **6d695aec**
wherever a build-out claim was load-bearing (every anchor below re-read; the two
places code refines the build-out are called out inline as CODE-CONFIRMED).

R1 = model the monotone eager web's **TUPLE-forward** dispatch arm
(`Build.cpp:1150`, `BuildEagerTupleRegion`) and **terminal-INSERT** dispatch arm
(`Build.cpp:1154`, `BuildEagerInsertRegion`) as two DR-IR op kinds, lowered AT
the original walk position (the `kIngestFold` hole-contract trick generalized).
PURE MODELING + LOWERING-IN-PLACE: **zero emission change**, full-corpus 4-mode
byte-identity vs frozen 6d695aec is the hard gate, and the ONLY golden that
changes is `demand_tc_witness.deltarel.opt.golden`.

The five load-bearing facts the design rests on, all CODE-CONFIRMED at tip:

- **F-ORDER.** The entire eager walk (const-entry `BuildEagerRegion` at
  `Procedure.cpp:813`, per-IO `ExtendEagerProcedure` at `:820`, and
  `CompleteProcedure` work-list induction/join emission at `:856`) runs and
  COMPLETES before `BuildStratumPhases` at `Procedure.cpp:862`. `BuildDRInventory`
  (`Stratum.cpp:2081`), the linearizer (`:2097`), and `DumpDeltaRelIfEnabled`
  (`:2173`) all run INSIDE `BuildStratumPhases`, i.e. strictly AFTER the walk.
  **The flow graph does not exist at walk time** (Build.h:211 states this as the
  reason `emitted_ingest_folds` is a Context list). No eager TUPLE/INSERT
  dispatch happens inside `LowerDRFlow`/`LowerDRRounds` — those call `BuildJoin`
  (differential), never `BuildEagerRegion` (grep-confirmed: Stratum.cpp has no
  `BuildEager*` call). ⇒ a Context list the walk fills is FULLY POPULATED when
  `BuildDRInventory` reads it.
- **F-DUMPGUARD.** `DumpDeltaRelIfEnabled` (`Format.cpp:932`) PRE-guards on
  `gDeltaRelDumpStream != nullptr`: with no `-deltarel-out`, `EmitDRFlow` is never
  called — **zero render cost off-flag**. But `BuildDRInventory` runs
  unconditionally every compile (it is the sole stratum authority), so the MINT
  cost is paid every compile (Q5).
- **F-CENSUSABORT.** `Format.cpp:914`: `if (census_total != flow.ops.size())
  abort()`. The instant an op of a kind absent from `kAllKinds` is minted, every
  `-deltarel` dump aborts. "Census extended day one" is FORCED, not hygiene.
- **F-SWITCHTOTAL.** `DROpKindName` (`Format.cpp:96`) is `-Wswitch`-total with no
  `default` → a new kind without a `case` is a COMPILE error.
- **F-EFFECTDRIVEN.** Dep edges are derived by iterating every op's EFFECTS
  (`DeltaRel.cpp:4054`, switch on `EffKind`, not `op.kind`). An op with NO effects
  produces NO vec/flag access ⇒ NO dep edge ⇒ is invisible to V-LINEAR /
  V-BAND-HAZARD / V-READY / V-LOOP (all dep-edge-driven). This is the lever the
  whole zero-perturbation design pulls (Q1).

---

## (A) THE MINT

### A.1 Op kinds — TWO, appended at the enum tail

Decision: **two** new `DROpKind`s, not one.

- `kEagerForward` — the TUPLE-forward dispatch arm (`BuildEagerTupleRegion`).
- `kEagerInsert` — the terminal-INSERT dispatch arm (`BuildEagerInsertRegion`).

Appended after `kInstanceSeal` (`DeltaRel.h:162`), positions **18** and **19**
(uint8_t-backed, positional; nothing serializes the numeric value — HP-17 /
build-out Q7 re-confirmed: the only "all kinds" assumptions are the
`census_total` guard and `-Wswitch` totality, both edited). Append-at-tail keeps
every existing census column and every existing enum value byte-identical (the
keyed-instance 15/16/17 precedent).

Why two, not one (resolving build-out Q2/model Q3):
1. The arms have genuinely different shapes and different census meaning. The
   TUPLE arm emits NO local fold (pure forward into `BuildEagerInsertionRegions`
   on the tuple's successors, `Tuple.cpp:14`); the INSERT arm emits a local fold
   (`Insert.cpp:26`) THEN a three-way sink discriminant (relation-descent /
   publish-now / publish-vec / commit-published). Two columns make the dump both
   proportional AND diagnostic — you can read forwards-vs-terminals directly,
   which is exactly the §19(H) observability win.
2. `rel-arch-pseudocode.md §3` enumerates "TUPLE forward" and "INSERT terminal"
   as DISTINCT step kinds of the R1..Rk strangler-fig; modeling them as two kinds
   keeps the op taxonomy 1:1 with the step-kind taxonomy.
3. The build-out itself proposes exactly these two spellings
   (`kEagerForward`/`kEagerInsert`, model §2.4).

Explicitly OUT of R1 (deferred to later Rk, task NON-GOALS + `rel-arch-pseudocode
§3`): the SELECT rebind arm (`Build.cpp:1133`), which ALSO funnels into
`BuildEagerInsertionRegions` but is a distinct monotone arm (E-95's "eight
monotone arms"). The terminal-INSERT relation branch (C, `Insert.cpp:75`) recurses
through `BuildEagerInsertionRegions` → the SELECT arm → descends again; **R1 models
only the INSERT DISPATCH itself** (its local fold + sink discriminant), NOT the
downstream SELECT rebind or the recursive descent. Clean boundary: `kEagerInsert`
marks the dispatch; everything below the dispatch stays hand-coded and
unmodeled until its own Rk.

### A.2 Effects — NONE. The ops are pure position markers (resolving Q1)

Decision: **`kEagerForward` and `kEagerInsert` carry ZERO `DREffect`s.**

This is the central design choice, and it is what makes R1 a *trivial-shape*
first slice that earns the machinery with the smallest possible blast radius
(rel-epoch-open-brief §5: "earning the census/dump/gate machinery on a trivial
shape"). The argument, against F-EFFECTDRIVEN:

- Effect-free ⇒ the eager ops generate NO vec/flag accesses in the
  `DeltaRel.cpp:4054` collection loop ⇒ NO new RAW/WAR/WAW dep edges ⇒ the
  `deps:` section is **byte-unchanged** (empty in the witness), and V-LINEAR /
  V-BAND-HAZARD / V-READY / V-LOOP never see the eager ops at all. The
  perturbation surface collapses to exactly: `kAllKinds`, `DROpKindName`,
  `key_of`, the render switch, and the census columns. Nothing that can trip a
  cross-op hazard.
- The terminal INSERT's genuine table-counter bump is ALREADY MODELED where it
  is load-bearing — for a monotone table it is the same `%table` the
  `kIngestFold` seeds. Putting a `kCounter` effect on `kEagerInsert` would
  **double-represent** that write and manufacture a spurious WAW against the
  ingest fold over the same table (Q5 in the build-out model). That is pure
  downside for a first slice: it forces lead-0 gymnastics AND a hazard-direction
  proof, for a modeling nicety R-final delivers properly.
- The dump is STILL proportional: one op BLOCK per eager dispatch appears in the
  ops section; the census columns grow with the eager web. The "effects: / spine:"
  lines are simply absent on these blocks. Honest effect/regime/spine modeling —
  "differentialness an op attribute", "eager-per-row vs frontier-batch a lowering
  choice", the access-plan/WCOJ consumer — is EXPLICITLY R-final work
  (`rel-arch-pseudocode §3` R-final), not R1.

Consequence for `EmissionDerivClass` (build-out Q4): MOOT for R1. The
recursive-vs-nonrecursive class is a property of the `UPDATECOUNT` the descent
still emits by hand inside `InTryInsert`/`Insert.cpp:26` — invisible to an
effect-free marker op. R1 never captures a klass, so the walk-position-dependent
capture-timing problem does not arise. (When R-final adds the counter effect, it
inherits the `kIngestFold` klass-on-effect discipline; not now.)

### A.3 Payload — view identity + minimal render/routing state

Following the `kIngestFold` field-shape precedent (`DeltaRel.h` INGEST_FOLD
block). New DROp fields (one small block appended after the instance block):

```
// ---- EAGER_FORWARD / EAGER_INSERT data (R1) --------------------------------
std::optional<QueryView> eager_view;         // the dispatched TUPLE/INSERT view
EagerSink eager_sink{EagerSink::kNone};      // kEagerInsert only (§A.3)
std::optional<ParsedMessage> eager_message;  // kEagerInsert stream sinks only
```

with a new small enum:

```
enum class EagerSink : uint8_t { kNone, kRelation, kPublishNow, kPublishVec,
                                 kCommitPublished };
```

- The **target table** rides the EXISTING `table_op_table` field (nullable — a
  table-less TUPLE carries `table_op_table == nullptr`). This is the
  `kSubgraphInstantiate`/HP-3 precedent (build-out Q4): reusing `table_op_table`
  means `op_table_id` (`DeltaRel.cpp:4193`) picks the eager op's table up with
  **no new ternary arm**, giving a deterministic table-keyed sort for free.
- `table_op_sign` stays **0** (signless markers). `op_sign` (`DeltaRel.cpp:4202`)
  returns 0 ⇒ header renders `sign=·`.
- `eager_view` drives render (`view.UniqueId()`, the args line) and the structural
  well-formedness recount (A.6). `pred_view` and `last_table` are NOT stored —
  they are lower-time cursors (build-out §4.3), consumed only by the in-place
  emission, never by the model.
- `eager_sink` + `eager_message` distinguish the four `kEagerInsert` terminal
  shapes so two structurally-different terminals never render identically
  (build-out §3 warning). `kEagerForward` leaves both at their defaults.

Single-authority constructors (the `Make*` mold, pure functions, no ids):

```
DROp MakeEagerForwardOp(QueryView tuple_view, TABLE *table):
    DROp op(kEagerForward); op.ctx = kEager;
    op.eager_view = tuple_view; op.table_op_table = table;  // table may be null
    return op                                    // NO effects, NO ids

DROp MakeEagerInsertOp(QueryView insert_view, TABLE *table,
                       EagerSink sink, optional<ParsedMessage> msg):
    DROp op(kEagerInsert); op.ctx = kEager;
    op.eager_view = insert_view; op.table_op_table = table;
    op.eager_sink = sink; op.eager_message = msg;
    return op                                    // NO effects, NO ids
```

### A.4 MINT SITE — walk-time construct-and-lower, enrolled from the walk record

**Decision: walk-time mint-and-lower (the `kIngestFold` mold), with the
inventory enrollment reading the walk's recorded dispatch stream.** Argued below
against the alternative (pre-walk independent inventory) and against id-stream
identity + the (F) law.

The pipeline (F-ORDER) FORCES the walk-time half: at the dispatch arm the flow
graph does not exist, so — exactly as `LowerIngestFold` does — the op is
constructed fresh from the single-authority ctor and lowered in place. The op fed
to the lowering is NEVER looked up from `flow.ops` (there is none yet).

The one deliberate deviation from the `kIngestFold` mold is the **enrollment
source**. `kIngestFold` enrolls into `flow.ops` by an INDEPENDENT re-derivation
(`BuildDRInventory`'s `query.IOs()×Receives()×polarity` walk, `DeltaRel.cpp:2252`)
and cross-checks that against the walk record via Site-5. R1 CANNOT cheaply
re-derive the eager-web dispatch set independently: the eager web's membership IS
a reachability walk of monotone successors with cut-stops (differential
consumers, agg/kv, recog-guards) that can reach the same TUPLE from >1 edge — an
independent enumerator is as complex as the descent itself (build-out Q2, the
"make-or-break"). So for R1 the **walk is the reachability authority**, and
`BuildDRInventory` enrolls FROM the walk's recorded stream:

```
# --- walk side (the two dispatch arms in Build.cpp, wrapped) --------------
# ARM 1 (Build.cpp:1150), was: BuildEagerTupleRegion(impl, pred_view, tuple, ...)
} else if (view.IsTuple()) {
    op = MakeEagerForwardOp(view, model_table_of(view))     # single authority
    LowerRelStep_Forward(impl, context, op, pred_view, QueryTuple::From(view),
                         context, parent, last_table)        # §B — emits in place

# ARM 2 (Build.cpp:1154), was: BuildEagerInsertRegion(impl, pred_view, insert, ...)
} else if (view.IsInsert()) {
    insert = QueryInsert::From(view)
    op = MakeEagerInsertOp(view, table_of(insert), classify_sink(insert, context),
                           message_of(insert))               # single authority
    LowerRelStep_Insert(impl, context, op, pred_view, insert, context,
                        parent, last_table)                  # §B — emits in place

# each LowerRelStep_* records the dispatch into context.emitted_eager_ops
# (view id, kind, table ptr, sink) at emission time — the (F)-clean walk order.

# --- inventory side (BuildDRInventory, a new EAGER_WEB block) --------------
for rec in context.emitted_eager_ops:            # walk order, deterministic
    if rec.kind == kEagerForward:
        flow.ops.push_back(MakeEagerForwardOp(rec.view, rec.table))
    else:
        flow.ops.push_back(MakeEagerInsertOp(rec.view, rec.table, rec.sink,
                                             rec.message))
    # NO vec def-edge (effect-free — no vec to define)
```

Note the inventory re-invokes the SAME single-authority ctor from the recorded
view (it does NOT stash a pre-built `DROp` in the Context and splice it) — so the
ctor stays the sole op-builder (§12.6 single-authority discipline preserved for
PAYLOAD), and only the SET/COUNT comes from the walk record.

> **[ADJ-S2] UPHELD (crit-gates F1 HIGH + crit-model F2 MEDIUM) — PIN: the
> EAGER_WEB block is appended STRICTLY AFTER all existing `flow.ops` enrollment.
> Load-bearing, was only implicit.** The additive-diff / single-golden claim
> (E.1 criterion #1, D.2) rests on the two ingest folds KEEPING construction
> indices `oi = 0, 1` so their `op.0 kIngestFold` / `op.1 kIngestFold` headers
> stay byte-identical. CODE-CONFIRMED: `BuildDRInventory`'s INGEST_FOLD loop is
> the LAST family that pushes to `flow.ops` (`DeltaRel.cpp:2251-2291`); the
> trailing FIXPOINT_ROUND block (`:2293+`) mints into `flow.rounds`, not
> `flow.ops`. For `demand_tc_witness` (a monotone program, every other census
> kind 0) the two ingest folds ARE the only `flow.ops` entries, so they hold
> indices 0/1 **only if** the EAGER_WEB enrollment appends at the TAIL. If an
> implementer splices the block before/among the existing families (a natural
> reading of "beside the ingest enrollment"), the ingest folds shift to k+1/k+2,
> every `op.N kIngestFold` line re-labels, and BOTH E.1 criteria fail wholesale
> (the bless becomes a non-additive rewrite). **BINDING PIN:** the EAGER_WEB
> `flow.ops.push_back` loop is placed AFTER the INGEST_FOLD loop (and after every
> other existing enrollment family) in `BuildDRInventory`, so no pre-existing
> op's construction index shifts. Add to E.1 criterion #1 as a stated
> precondition: assert `op.0`/`op.1` still spell `kIngestFold` in NEW (a
> construction-index-shift tripwire, orthogonal to render position — an eager
> block rendered *between* op.0 and op.1 by the A.5/[ADJ-S1] key is fine; only a
> LABEL shift is the hazard). Note the render-order interleaving from [ADJ-S1] is
> independent of, and does not violate, this construction-order pin.

**Justification against id-stream identity.** `LowerRelStep_Forward` /
`LowerRelStep_Insert` emit AT the original call site by *calling the existing
region-builder body* (see §B) — the exact same `impl->next_id++` mints, in the
exact same order, as the frozen build. This is STRONGER than `LowerIngestFold`
(which relocated emission into `Stratum.cpp`): R1 keeps the emission function
bodies where they are and only interposes the op construction + record around the
call. The one `impl->next_id` any TUPLE arm can trigger — the lazy first creation
of a `(table, kind)` delta vector in `TableDeltaVector` (build-out §2.4, Q3) — is
UNMOVED, because the delta-vector creation is still performed by the unchanged
descent body at the identical walk moment. The op construction + `push_back` mint
no `impl->next_id` at all.

**Justification against the (F) law.** The walk records into
`context.emitted_eager_ops` in syntax-directed DFS order (deterministic given the
query — walk order / mint order only, never pointer order). `BuildDRInventory`
iterates that vector in order. No `std::unordered_map`, no pointer-ordered
container, touches the eager-op enrollment. `key_of` (A.5) orders by
`(table->id, ctor)` — deterministic `id`, never pointer.

**Why not pre-walk independent inventory.** Two reasons: (i) it cannot exist
cheaply (Q2); a hand-rolled reachability enumerator is exactly the F17/F18-class
bug surface R1 is trying to *remove*, and a divergence from the true walk in a
legal program would FALSELY ABORT (availability bug) for zero correctness gain
over the byte-identity A/B net; (ii) the §19(H) goal is a dump that is a
PROPORTIONAL IMAGE of the actual eager web — which is precisely the walk's
recorded stream, not a re-derived approximation. The direction flips at R-final,
when `LowerRelStep` is DRIVEN from the modeled ops and the walk becomes the
derived side; until then, walk-authoritative enrollment is the honest
strangler-fig intermediate.

### A.5 Census day one

Forced by F-CENSUSABORT + F-SWITCHTOTAL. R1 adds, in one diff:
- `DROpKind::kEagerForward`, `kEagerInsert` at the enum tail (`DeltaRel.h:162`).
- Two `case`s in `DROpKindName` (`Format.cpp:115`) → `"kEagerForward"` /
  `"kEagerInsert"`.
- Two entries at the tail of `kAllKinds[]` (`Format.cpp:905`), in enum order —
  so `census_total` covers them and the dump-abort guard stays green, and the two
  new columns render at the RIGHT end with every existing column byte-identical.

> **[ADJ-S11] UPHELD (crit-gates F7 LOW) — bump the census comment in the same
> diff.** CODE-CONFIRMED: `Format.cpp:886` reads `// ---- census (18 DROpKind
> counts, enum order, one line; grammar R-10) ----` and `kAllKinds[]` currently
> holds 18 entries. Appending `kEagerForward, kEagerInsert` makes it 20. The T2b
> dump-law / p10 zero-drift discipline expects the comment count to track
> `kAllKinds` length. **Bump `18` → `20` in the same R1 diff** (no gate fails;
> pure hygiene, but the count-in-comment must not drift).

### A.6 Enrollment + the EXACT exclusion from the band/linearizer path

R1 generalizes the `kIngestFold` three-part separation (build-out model §2.3):

**(a) They DO enter `pinned_order` and are validated — with a lead-0 off-lattice
`key_of` branch.** `LinearizeAndValidateDRFlow` Kahn-sorts over ALL ops and
V-LINEAR asserts `pinned_order.size() == flow.ops.size()` (`DeltaRel.cpp:4644`),
so the eager ops are in `pinned_order` and render. R1 adds a `key_of` branch
(`DeltaRel.cpp:4238`), mirroring the `kIngestFold` lead-0 arm at `:4248`:

```
if (op.kind == DROpKind::kEagerForward || op.kind == DROpKind::kEagerInsert) {
    // VALIDATOR-ORDERING ONLY — emission never reads this key; the lowering is
    // the walk's own DFS order. Lead-0 off-lattice (eager/monotone push path,
    // pre-phase) alongside the ingest folds; sign 0 (signless markers).
    return Key{0u, 0u, 0u, op_table_id(op), 0, oi};
}
```

Lead-0 is semantically correct: the eager forwards/inserts ARE the pre-phase
monotone push path, the same band as the ingest seeds that feed them. `op_table_id`
resolves via `table_op_table` (A.3) with NO new ternary arm. The final `ctor`
(construction index `oi`) is the deterministic tie-break within a table — and
since the ops carry no dep edges, the Kahn sort places them by key alone, so the
band-key order check (V-ORDER-CONSISTENT, `DeltaRel.cpp:4790`) holds trivially
(order == key order, non-decreasing).

Every validator/dispatch site that touches `op.kind`, with its R1 disposition
(from build-out model §1.4, each re-checked against code):

| Site (`DeltaRel.cpp` unless noted) | disposition for the eager kinds |
|---|---|
| `key_of` (`:4238`) | **EDIT** — add the lead-0 branch above (REQUIRED: default arm at `:4272` is the lead-1 phase key, which would wrongly claim a phase stratum). |
| `op_table_id` (`:4193`) | no edit — resolves via `table_op_table` (present). |
| `op_sign` (`:4202`) | no edit — `table_op_sign==0` ⇒ returns 0 (signless). |
| `DROpStratum` (`:3865` switch) | no edit — `default: return 0u`; renders `stratum` if the dedicated case prints it (C decides it does NOT). SAFE. |
| `op_band` (`:4153`) | no edit — `default: return 0u`. Off-lattice ops never consult band via the lead-0 key. |
| `op_pivot_hint` / `scope_pair` / `op_round` / `op_output_round` | no edit — all default gracefully (not a round/pivot op). |
| `ValidateDROps` per-op (`:2863`) | no edit — `default: break`, no family validator runs (effect-free ⇒ nothing to validate structurally beyond A.6(c)). |
| instance-effect / V-AGG / V-RETIRE-AFTER validators | no edit — keyed on other kinds; ignore. |
| DeriveDRStrata `*_stratum` fill (`:2573`) | no edit — no stratum-map entry (off-lattice, fine). |
| effect-collection loop (`:4054`) | no edit — switches on EffKind; eager ops have NONE ⇒ contribute zero accesses ⇒ zero dep edges. **This is the exclusion that keeps V-LINEAR/V-BAND-HAZARD/V-READY/V-LOOP inert.** |
| V-READY off-lattice set (`:4762`) | **NO edit needed** — the set only matters for ops that appear as `d.from`/`d.to` of a RAW edge; effect-free eager ops appear in NO dep edge, so V-READY never evaluates them. (Contrast build-out model §1.4, which lists this as a conditional edit "if the new op carries effects" — it does not.) |
| the `expect()` census-recount function (`DeltaRel.cpp:3294`ff) | **EDIT** — append the A.6(c) structural loop AFTER the expect() lines. [ADJ-S12 — ADJUDICATED UPHELD (recovered crit-correctness F2 MED, orchestrator-verified at Stratum.cpp:2077 + DeltaRel.cpp:3294-3313): "V-OLD-EQUIV (:3303)" was a RETIRED-validator anchor; the live site is the expect(kind, want) COUNT mold, which cannot host a countless structural recount as an expect() line — A.6(c) lands as a NEW per-op loop appended to that function, with NO exp_eager count oracle (the F5/ADJ-S10 bless-time count read is the compensating control).] |
| `DROpKindName` (`Format.cpp:96`) | **EDIT** (F-SWITCHTOTAL). |
| `kAllKinds` (`Format.cpp:895`) | **EDIT** (F-CENSUSABORT). |
| Format render switch (`Format.cpp:654`) | **EDIT** — dedicated cases (C); without them the generic `default` at `:823` renders no `args:` and under-specifies. |

**(b) The emission path NEVER iterates them.** `LowerDRFlow`/`LowerDRRounds`/
`LowerCommitSweeps` walk FAMILY-SPECIFIC containers (`flow.branches`,
`flow.joins`, `flow.Crossovers()`, `flow.GroupUpdates()`, `flow.ops` filtered to
`kClaimDrain`, …). There is NO family loop for `kEagerForward`/`kEagerInsert`, so
the band lowering is structurally incapable of double-emitting them — exactly the
`kIngestFold` guarantee. Their sole emission is the walk-time `LowerRelStep`.

**(c) Independent recount — STRUCTURAL well-formedness, day one; exact-count
oracle STAGED.** Since enrollment reads the walk record (A.4), an EXACT
independent re-count would require the reachability oracle R1 declines to build.
The day-one addition (a NEW per-op structural loop appended after the
`expect()` lines at `DeltaRel.cpp:3294`ff — ADJ-S12; "V-OLD-EQUIV" was a
stale anchor) is therefore a structural INVARIANT over the
enrolled eager ops that is genuinely independent of the walk (it checks against
Query view-kinds, not against the walk's count):

```
for op in flow.ops where op.kind in {kEagerForward, kEagerInsert}:
    assert op.eager_view.has_value()
    if op.kind == kEagerForward: assert op.eager_view->IsTuple()
    else:                        assert op.eager_view->IsInsert()
    # cut-exclusion: an eager op is never minted for a chain-breaker view
    assert not (op.eager_view->IsAggregate() or op.eager_view->IsKVIndex())
    # table routing well-formed: table_op_table matches the view's model table
    #   (null iff the TUPLE/relation is table-less)
    assert op.table_op_table == model_table_or_null(*op.eager_view)
```

This catches payload corruption and mis-kinded enrollment per compile. The EXACT
per-dispatch COUNT is pinned by the first post-R1 `-deltarel` census (OQ1) under
the binding full-corpus byte-identity A/B gate — the real miscompile net for R1's
zero-emission-change claim (the emission, hence the recorded dispatch stream, is
proven unchanged corpus-wide).

> **RESIDUAL for the critique / desired-states stages (R1-DESIGN-Q1):** whether
> to strengthen A.6(c) to an EXACT independent recount (reusing the already-
> replicated DR-side cut predicates `AnyCutSuccessorDR` / `CollectSectionTargetsDR`
> as a reachability oracle) + a non-vacuous Site-5-analog, or accept the
> structural recount + byte-identity net for R1. REC: accept for R1 (lowest risk;
> the exact oracle is real bug surface for zero correctness gain over A/B), and
> let the direction-flip at R-final supply the strong check when flow DRIVES
> emission. Flagged LOUD.

No Site-5-analog (`V-EAGER-XCHECK`) is added at R1: under walk-authoritative
enrollment it would compare the walk record to itself (vacuous). This is the
honest cost of Opt-3 and is called out so a later reviewer does not expect it.

> **[ADJ-S10] UPHELD (crit-gates F6 LOW) — the census COUNTS `<F>`/`<I>` have NO
> independent verifier; the bless ritual MUST require a structural count read.**
> R1 adds no `expect(kEagerForward,…)`/`expect(kEagerInsert,…)` recount (A.6(c) is
> structural well-formedness only; Opt-3 declines the reachability oracle;
> Site-5-analog is vacuous). CODE-CONFIRMED the existing `ValidateDROps`
> `expect()` mold (`DeltaRel.cpp:3294-3312`) has no grand-total, so the non-dump
> path stays green without an eager entry — but that also means the full-corpus
> byte-identity A/B (D.1) nets that EMISSION didn't move, NOT that the census is
> CORRECT: there is no prior-correct baseline for a brand-new op kind (the count
> is BORN at R1). So `<F>`/`<I>` are *pinned by inspection at bless* (OQ1), and
> that inspection is the ONLY correctness check on the counts. **AMEND the E.1
> bless ritual:** require a STRUCTURAL read of `<F>`/`<I>` against C.4's
> per-dispatch reasoning (count the TUPLE / INSERT dispatches the walk actually
> takes) before `--bless` — not merely "the diff is additive." Otherwise an
> off-by-N enrollment (a TUPLE reached from 2 edges counted once; a cut-successor
> leaking in) blesses green. This is a bless-gate discipline addition, not new
> code — and it subsumes the existing R1-DESIGN-Q1 residual's "accept structural
> recount for R1" recommendation.

---

## (B) THE LOWERING — the strangler-fig cut

The cut is: the dispatch arm becomes **mint-op-then-`LowerRelStep(op)`**, where
`LowerRelStep`'s body IS the existing emission — reached by CALLING the existing
`BuildEagerTupleRegion` / `BuildEagerInsertRegion`, not by moving their bodies.

**Decision: CALL, do not move.** `LowerIngestFold` *moved* the hand-coded fold
into `Stratum.cpp` because that emission had to relocate to the flow-driven
lowering site. R1 has the opposite constraint — it must PROVE zero emission
change — so the least-risk refactor keeps `Tuple.cpp` / `Insert.cpp` byte-for-byte
and interposes only a thin wrapper. `LowerRelStep_Forward` / `LowerRelStep_Insert`
are two new free functions (in `Build.cpp`, beside the dispatcher) whose entire
body is: (1) record the dispatch into `context.emitted_eager_ops`, (2) call the
untouched region builder. Because the region builder is called with the identical
`(impl, pred_view, view, context, parent, last_table)` arguments at the identical
walk moment, every `impl->next_id++` inside it (and inside the descent it drives)
fires in the identical order → id-stream identity is mechanical, not argued.

### B.1 The exact mechanical refactor (diff on `Build.cpp:1150-1157`)

Before (build-out arms §0):
```
} else if (view.IsTuple()) {
    BuildEagerTupleRegion(impl, pred_view, QueryTuple::From(view), context,
                          parent, last_table);
} else if (view.IsInsert()) {
    const auto insert = QueryInsert::From(view);
    BuildEagerInsertRegion(impl, pred_view, insert, context, parent,
                           last_table);
```

After:
```
} else if (view.IsTuple()) {
    const DROp op = MakeEagerForwardOp(view, ModelTableOrNull(impl, view));
    LowerRelStep_Forward(impl, context, op, pred_view, QueryTuple::From(view),
                         parent, last_table);
} else if (view.IsInsert()) {
    const auto insert = QueryInsert::From(view);
    const DROp op = MakeEagerInsertOp(view, ModelTableOrNull(impl, view),
                                      ClassifyEagerSink(context, insert),
                                      MessageOfInsertOrNull(insert));
    LowerRelStep_Insert(impl, context, op, pred_view, insert, parent,
                        last_table);
```

with the two thin wrappers (new, in `Build.cpp`):
```
void LowerRelStep_Forward(impl, context, const DROp &op, pred_view, tuple,
                          parent, last_table):
    RecordEagerDispatch(context, op)              # push {kind,view,table,sink,msg}
    BuildEagerTupleRegion(impl, pred_view, tuple, context, parent, last_table)
                                                  # UNTOUCHED body, in place

void LowerRelStep_Insert(impl, context, const DROp &op, pred_view, insert,
                         parent, last_table):
    RecordEagerDispatch(context, op)
    BuildEagerInsertRegion(impl, pred_view, insert, context, parent, last_table)
                                                  # UNTOUCHED body, in place
```

Three trivial helpers (all pure, id-free, deterministic):
- `ModelTableOrNull(impl, view)` = a **`.find()`-guarded** lookup on
  `impl->view_to_model` returning null when absent, then
  `->FindAs<DataModel>()->table`. [ADJ-S14 — ADJUDICATED UPHELD (recovered
  crit-correctness F4 LOW, orchestrator-verified at Tuple.cpp:10-16): the
  TUPLE arm is a bare forward with NO baseline model lookup, so this is a
  NEW deref on that arm — `operator[]` would default-insert a null NODE and
  `->FindAs` would SIGSEGV on any view absent from the map. `.find()`
  honors the OrNull contract; the model pass is total over views today, so
  absent = null table, never a crash. FindAs path-compression is (F)-safe
  (no id-minting reads union-find pointer order at CF-build time).]
  Shared by ctor + recount so both see the same table.
- `ClassifyEagerSink(context, insert)` = a PURE restatement of `Insert.cpp`'s sink
  branch, reading only `insert.IsStream()`/`IsRelation()`,
  `context.commit_published_view.count(message)` (→ `kCommitPublished`),
  a **`.find()`** probe of `context.publish_vecs` (entry present AND
  non-null → `kPublishVec`), else `kPublishNow`; `insert.IsRelation()` →
  `kRelation`. [ADJ-S13 — ADJUDICATED UPHELD (recovered crit-correctness
  F3 LOW→MED, orchestrator-verified at Insert.cpp:48 + Procedure.cpp:336-362
  + Build.h:121): the originally-drafted `publish_vecs[message]` operator[]
  DEFAULT-INSERTS on an unordered_map whose LATER iteration mints
  impl->next_id per non-null entry — neutrality held only because baseline
  does the identical operator[] at the same dispatch (Insert.cpp:48). The
  `.find()` form removes the load-bearing-but-fragile invariant entirely:
  the classifier now NEVER mutates the map, so hoisting it can never
  reorder bucket history. BINDING: the classifier must never use
  operator[] on publish_vecs.] **This is a read-only
  classification — it mints nothing and changes no emission.** It exists so the
  op payload records which terminal shape fired (render + the A.6(c) recount),
  NOT to re-drive emission (the untouched `BuildEagerInsertRegion` still owns the
  actual branch). The duplication is a KNOWN small S2-flavored replica, retired at
  R-final when the one path both classifies and emits.

> **[ADJ-S4] UPHELD (crit-model F4 LOW) — acknowledge LOUD: `ClassifyEagerSink`
> ships with NO cross-check against the sink `BuildEagerInsertRegion` actually
> emits.** Because the classification is render-only (it feeds `eager_sink` for
> the dump, never re-drives emission), a mis-classification (e.g. tagging a
> `publish-vec` terminal `publish-now`) does NOT perturb emission — so the
> full-corpus byte-identity A/B net, R1's SOLE miscompile referee, CANNOT catch
> it, and the wrong `sink=` byte would be blessed silently into the golden. This
> is the same replica-divergence hazard (S2 / F17-F18 flavor) R1 elsewhere works
> to remove; R1 accepts it because (i) both witnesses publish zero messages ⇒
> every emitted `kEagerInsert` is `sink=relation` (the trivial
> `insert.IsRelation()` branch — the stream discriminant is never exercised in
> corpus), and (ii) the honest cross-check (a `V-EAGER-XCHECK` Site-5-analog)
> arrives with the direction-flip at R-final when the one path both classifies
> and emits. RECORDED as a labeled residual, not a gate.
- `MessageOfInsertOrNull(insert)` = `insert.IsStream() ?
  ParsedMessage::From(QueryIO::From(insert.Stream()).Declaration()) : nullopt`.

### B.2 `RecordEagerDispatch` — the Context list

A new Context field beside `emitted_ingest_folds` / `emitted_instance_ops`
(Build.h ~:225):
```
struct EmittedEagerOp {
    uint8_t kind;                    // kEagerForward / kEagerInsert cast
    std::optional<QueryView> view;   // eager_view (re-invokes the ctor at inventory)
    const void *table;              // table_op_table (for the ctor re-build)
    uint8_t sink;                   // EagerSink cast (kNone for forwards)
    std::optional<ParsedMessage> message;  // kEagerInsert stream sinks
};
std::vector<EmittedEagerOp> emitted_eager_ops;
```
`RecordEagerDispatch` push_backs one entry per dispatch, in walk order — enough
to re-invoke the single-authority ctor at inventory time. (A `view_id`/`UniqueId`
is available off the view for the cross-check should R1-DESIGN-Q1 later
strengthen it; not needed for the enrollment itself.)

### B.3 What is NOT touched (the id-stream proof obligation, mechanically)

- `Tuple.cpp` and `Insert.cpp` bodies: **byte-unchanged**.
- `BuildEagerInsertionRegions` / `InTryInsert` / `TableDeltaVector` /
  `AppendViewTupleToVector`: **byte-unchanged** — every `impl->next_id++` (the
  lazy delta-vector creation, the PUBLISH id, VECTORLOOP/VAR ids) fires in the
  same order because the descent is entered from the same wrappers at the same
  moment.
- `MapVariablesInEagerRegion` (`Build.cpp:1087`): stays a pre-arm CF step, mints
  no id, unmodeled (build-out §1.2). The op does not carry `col_id_to_var`.
- The SELECT / JOIN / MERGE / MAP / CMP / NEGATE / AGG arms: **untouched** — R1
  interposes only on the TUPLE and INSERT arms of the `Build.cpp:1089-1165`
  switch.
- `ExtendEagerProcedure`'s E-42 table-less VECTORLOOP shim (`Procedure.cpp:995`):
  **untouched** (non-goal F).

---

## (C) GRAMMAR (E-71, adjudicated PRE-CODE)

New productions for the two op blocks, conforming to t2b-grammar §2.4 (the
`op.<idx> <DROpKind> sign=<glyph> ctx=<Ctx> …` header) + §2.5 (sublines) and the
T2b dump law (render from STORED fields; loud `fprintf+abort` on an unrenderable
spelling; never a silent fallback). All spellings are k-STRIPPED lowercase-hyphen
per the ClaimForm/Deferral/SweepFlavor house convention (t2b-grammar §2.4.c/d).

### C.1 Header + spelling tables

Common header (both kinds): `op.<oi> <Kind> sign=· ctx=eager stratum=0`.
- `sign=·` — `SignGlyph(op.table_op_sign)` with `table_op_sign==0` →
  `"\xC2\xB7"` (the existing signless glyph, corpus-proven multibyte-clean;
  t2b-grammar FLAG S-1's `·` pin).
- `ctx=eager` — `CtxName(op.ctx)`, `op.ctx==Ctx::kEager`.
- `stratum=0` — `DROpStratum(flow, op)`; the `default: return 0u` arm
  (`DeltaRel.cpp:3946`) already covers both new kinds (NO edit) — the nearest-peer
  (`kIngestFold`) rendering, which is also `stratum=0`. (Chosen over a `band=` form
  because the eager ops are pre-phase lead-0, like ingest folds, not trailing
  commit-band ops.)

New spelling table (T2b loud-abort idiom, `-Wswitch`-total, no default):
```
static const char *EagerSinkName(EagerSink s) {
  switch (s) {
    case EagerSink::kRelation:        return "relation";
    case EagerSink::kPublishNow:      return "publish-now";
    case EagerSink::kPublishVec:      return "publish-vec";
    case EagerSink::kCommitPublished: return "commit-published";
    case EagerSink::kNone: break;   // never on a kEagerInsert — loud-abort
  }
  fprintf(stderr, "DELTAREL-DUMP: unhandled EagerSink in a spelling table\n");
  abort();
}
```
`DROpKindName` (F-SWITCHTOTAL) gains `case kEagerForward: return "kEagerForward";`
and `case kEagerInsert: return "kEagerInsert";`.

### C.2 The two render cases (`Format.cpp:654` switch)

```
case DROpKind::kEagerForward: {
  os << " sign=" << SignGlyph(op.table_op_sign)   // "·"
     << " ctx=" << CtxName(op.ctx)                // "eager"
     << " stratum=" << DROpStratum(flow, op)      // 0
     << "\n";
  os << "    args:";
  if (op.table_op_table) os << " table=" << tid(op.table_op_table);
  else                   os << " table=\xE2\x80\x94";   // em-dash: table-less TUPLE
  os << "\n";
  break;
}

case DROpKind::kEagerInsert: {
  os << " sign=" << SignGlyph(op.table_op_sign)
     << " ctx=" << CtxName(op.ctx)
     << " stratum=" << DROpStratum(flow, op)
     << " sink=" << EagerSinkName(op.eager_sink)
     << "\n";
  os << "    args:";
  if (op.table_op_table) os << " table=" << tid(op.table_op_table);
  else                   os << " table=\xE2\x80\x94";
  if (op.eager_message.has_value())
    os << " message=" << std::string(op.eager_message->NameAsString())
       << "/" << op.eager_message->Arity();
  os << "\n";
  break;
}
```

Grammar decisions, argued for the (F) law + T2b:
- **NO `reads:` / `effects:` / `spine:` sublines.** The eager ops are effect-free
  markers (A.2); `emit_reads`/`emit_effects`/`emit_spine` would each render an
  empty/`—` line for no information. Omitting them (as `kEagerInsert`/`kEagerForward`
  carry no `DREffect` and no `arms`) keeps the block minimal and unambiguous. This
  is the deliberate divergence from the `kIngestFold` case, which DOES call all
  three (because it has effects).
- **Op identity is the `op.<oi>` header prefix (`Format.cpp:652`), NOT a rendered
  view id.** The args render only the DETERMINISTIC target `table=%table:N`
  (`tid(op.table_op_table)` → `table->id`, the id the `.ir` prints) plus, for a
  stream terminal, the message `name/arity` (the `kIngestFold` `args:` precedent).
  `QueryView::UniqueId()` is NOT rendered — it is not a proven-stable
  cross-build byte and would risk the (F) law; the construction index `oi`
  already disambiguates every op, so no view id is needed for uniqueness. Two
  same-table forwards render identical ARGS but distinct `op.N` headers — no
  collision, fully byte-stable.
- **`table=—` for the table-less TUPLE** uses the em-dash (`\xE2\x80\x94`),
  corpus-proven multibyte-clean (it is the existing `spine: —` byte). Deterministic.

> **[ADJ-S3] UPHELD (crit-model F3 LOW) — replace `table=—` with the
> OMIT-when-null idiom; `table=—` is a novel production.** CODE-CONFIRMED: every
> existing emitter OMITS a null table ref rather than rendering a sentinel —
> GROUP_UPDATE guards `if (input_table) os << " input=" << tid(input_table)`
> (`Format.cpp:679,687`); SubgraphInstantiate/ClaimDrain/Sweep only ever render
> non-null `tid(...)`. `t2b-grammar.md` pins table refs SOLELY as `%table:N`.
> `table=—` invents a second null-rendering convention (the em-dash-as-none
> marker belongs to `spine:`, not to a table ref) where the codebase has exactly
> one (omit). This is the E-71 PRE-CODE adjudication, so it RULES: **render
> `table=` only when `op.table_op_table != nullptr`, matching the `input=`
> precedent** — the `args:` line for a table-less TUPLE forward emits the `args:`
> header with the `table=` token omitted (an eager insert always has a target, so
> its args are never empty). This removes the novel production AND any dependence
> on em-dash byte handling. Amend both C.2 cases: drop the `else os << " table=…"`
> arm; keep only `if (op.table_op_table) os << " table=" << tid(...)`.
- **`sink=` is `kEagerInsert`-only**; `kEagerForward` never has a sink. `kNone`
  reaching `EagerSinkName` is a loud-abort (T2b law).

### C.3 The census line change (18 → 20 kinds)

`kAllKinds[]` (`Format.cpp:905`) appends `kEagerForward, kEagerInsert` at the tail
(enum order). Every existing column is byte-identical; two columns append at the
right. `census_total` now includes them (F-CENSUSABORT stays green). The witness
census line becomes (the sole edited existing byte is the tail append):
```
census: kCrossover=0 kProductArm=0 kSeedFold=0 kFixpointFire=0 kChainFold=0 kClaimDrain=0 kRetire=0 kRederive=0 kFrontierFilter=0 kCommitSweep=0 kNegateGate=0 kPivotAssemble=0 kIngestFold=2 kGroupUpdate=0 kStateSeal=0 kSubgraphInstantiate=0 kInstanceDeath=0 kInstanceSeal=0 kEagerForward=<F> kEagerInsert=<I>
```
`<F>`/`<I>` are the actual per-dispatch counts, PINNED AT BLESS from the first
post-R1 census (OQ1). Structural prediction (build-out §5.4, upper-ish, NOT
verified): `<F> ≈ 11`, `<I> ≈ 3–4` for `demand_tc_witness`.

### C.4 Predicted `demand_tc_witness.deltarel.opt.golden` — BYTE-DRAFT (schematic)

The STABLE parts are exact; the eager-op block count/order/table-ids are the
OQ1-pinned-at-bless residue (marked ⟨…⟩). Render order is `pinned_order` = all
lead-0 ops sorted by `(table_id, ctor)`, so the two ingest folds (ctor 0/1, lowest
ctor) sort FIRST within their table, and eager ops interleave by target table id.
`op.N` labels are construction indices and are therefore NON-MONOTONIC in render
order (already true of every differential dump — model §3).

> **[ADJ-S1] UPHELD (crit-model F1) — the render-order law above is WRONG; the
> schematic below is INVERTED. CODE-CONFIRMED at tip.** `key_less`
> (`DeltaRel.cpp:4275-4282`) compares `(lead, stratum, band, table_id, sign,
> ctor)` — **`sign` BEFORE `ctor`**. The eager ops carry `sign 0` (A.3); the
> witness's ingest folds key on `op.ingest_sign == +1` (golden renders `op.0
> kIngestFold sign=+`). And `op_table_id` (`:4200`) maps a **null** table to
> sentinel `0`, a real table to `id+1`. Two consequences the C.4 draft gets
> backwards:
> 1. Within a shared `table_id`, a same-table eager op (sign 0) sorts **BEFORE**
>    its ingest fold (sign +1) — `ctor` is never consulted at a shared table.
> 2. Every **table-less** TUPLE forward (`op_table_id == 0`, TC plumbing is full
>    of them) and every eager op targeting a lower-id table (`.ir` shows tables
>    `4,8,12,15,19,23`) sorts STRICTLY BEFORE `op.0 kIngestFold` (`op_table_id`
>    20) — so `op.0` lands roughly **mid-section**, not first.
>
> The corrected law: render order = `pinned_order` sorted by
> **`(op_table_id, sign, ctor)`** within lead-0 — the **sentinel-0 table-less
> block leads**, then each real table's block (`id+1`) with its **sign-0 eager
> ops preceding its sign-+1 ingest fold**. The order is still fully
> DETERMINISTIC and STABLE (total pointer-free key; Kahn emits edgeless ops in
> exact key order; V-ORDER-CONSISTENT `:4804` holds trivially) — only the
> STATED law and the ⟨…⟩ interleaving in the C.4 schematic were wrong. The
> schematic's op-block CONTENT (headers, args, census) is unchanged; treat its
> op-ORDERING as superseded by this law (regenerate at bless from the first run).
> The E.1 bless gate is UNAFFECTED — its three criteria check line CONTENT and
> the `>`-line CLASS, not position, and a same-table eager block rendered before
> `op.0` is still a purely additive `>` insertion (op blocks are atomic,
> blank-separated). The defect is a wrong E-71 deliverable (the lane's own job is
> to state the order law correctly), not a gate breaker.

```
deltarel

op.0 kIngestFold sign=+ ctx=eager stratum=0
    effects: {kCounter(%table:19, +, NonRecursive)}
    spine: —
    args: table=%table:19 message=edge_2/2
⟨eager ops whose target table_id == 19, in ctor order — e.g.:⟩
⟨op.K kEagerForward sign=· ctx=eager stratum=0⟩
⟨    args: table=%table:19⟩
op.1 kIngestFold sign=+ ctx=eager stratum=0
    effects: {kCounter(%table:23, +, NonRecursive)}
    spine: —
    args: table=%table:23 message=demand__reachable_from_bf/1
⟨eager ops for table_id == 23, then higher table ids, each a block of:⟩
⟨op.M kEagerForward sign=· ctx=eager stratum=0⟩
⟨    args: table=%table:NN⟩
⟨op.P kEagerInsert  sign=· ctx=eager stratum=0 sink=relation⟩
⟨    args: table=%table:NN⟩
⟨… (the ~11 forwards + ~3–4 relation inserts for the reachable computation;
   both witnesses PUBLISH ZERO messages (build-out §5.3), so EVERY kEagerInsert
   here is sink=relation — NO publish-now / publish-vec / commit-published block,
   and NO new PUBLISH id in the .ir/.h — a helpful byte-identity invariant)⟩

rounds:

deps:

census: kCrossover=0 kProductArm=0 kSeedFold=0 kFixpointFire=0 kChainFold=0 kClaimDrain=0 kRetire=0 kRederive=0 kFrontierFilter=0 kCommitSweep=0 kNegateGate=0 kPivotAssemble=0 kIngestFold=2 kGroupUpdate=0 kStateSeal=0 kSubgraphInstantiate=0 kInstanceDeath=0 kInstanceSeal=0 kEagerForward=<F> kEagerInsert=<I>
```

`rounds:` and `deps:` stay EMPTY (no recursive SCC; effect-free ops ⇒ no dep
edges — A.2/F-EFFECTDRIVEN). This is the byte-certain prediction; the ⟨…⟩ blocks
and `<F>`/`<I>` are pinned when the first post-R1 dump is run through the C.4 /
(E) direct-diff referee.

---

## (D) PRE-REGISTERED PREDICTIONS

Baseline = **frozen 6d695aec** binary (the E-90 frozen 169×4 A/B baseline; never
a live-count recompute). All predictions are pre-registered before code lands.

### D.1 Full-corpus byte-identity — all 4 modes (exit + `.h` + `.cpp` + `.ir`)

**Prediction: byte-identical, corpus-wide, all 4 optimization modes.** The
mechanism: R1 changes NO emission (§B — the region builders are called unchanged
at the unchanged walk moment; the op construction/record/enrollment mint no
`impl->next_id` and touch no region). The 676-row corpus A/B and `data/` A/B stay
byte-identical; suite ends `SUITE: PASS` (173).

**The 5 non-`.deltarel` IR goldens stay byte-unchanged** (the live referees that
must NOT move):
- `symrec_tie_1.ir.opt.golden`, `symrec_tie_1.df.opt.golden`
- `demand_tc_witness.ir.opt.golden`, `demand_tc_witness.h.opt.golden`,
  `demand_tc_witness.df.opt.golden`

These are the ControlFlow IR / generated header / dataflow — all downstream of
emission, which is unchanged. `symrec_tie_1` DOES mint eager ops into its own
`flow.ops` (it has a monotone eager web too), but it carries NO `.deltarel`
golden, so that growth is invisible to any pinned surface; its `.ir`/`.df`
byte-identity is the gate that its emission did not move.

> **[ADJ-S9] UPHELD (crit-gates F5 MEDIUM) — pin a SECOND eager web: add `deltarel
> opt` to `symrec_tie_1.irgold` and bless it in the SAME R1 bless.** After R1 the
> eager census + block render is byte-compared against a golden for EXACTLY ONE
> program (`demand_tc_witness`) — the §19(H) observability win is
> regression-gated on a single web. CODE-CONFIRMED coverage elsewhere is a hole:
> `symrec_tie_1`'s `run_irgold` all-sinks compile ALREADY passes `-deltarel-out`
> (`runall.sh:271-278`) and renders its own (structurally-different, multi-TUPLE)
> eager web — but its sidecar pins only `ir opt` / `df opt`, so those bytes are
> DISCARDED (no comparison); `demand_neighborhood_witness` (flat + nested eqgate)
> exercises its web only through stdout (no deltarel golden, OD-10); the 3 fences
> reject before minting. A regression that mis-renders/mis-counts eager ops on a
> structurally-different web would pass the whole suite. The render already runs,
> so pinning it costs ONE extra golden file and widens the pinned surface to two
> distinct eager webs — directly serving §19(H)'s "proportional image of the
> eager web." **RULING: add the `deltarel opt` sidecar line to
> `symrec_tie_1.irgold` and bless `symrec_tie_1.deltarel.opt.golden` in the same
> R1 bless.** (This makes symrec a SECOND intentionally-churned golden — update
> D.2's "only one golden churns" to "the two eager-web carriers' deltarel goldens
> churn; all other pinned surfaces byte-identical," and the E.1/E.7 zero-stray
> check accordingly.) If DECLINED, the design must state LOUD that eager
> observability is single-program-gated and symrec's web is rendered-but-uncompared.

**The 4 post-baseline cases stay byte-identical too:**
- `demand_neighborhood_witness` — covered by (i) its stdout golden byte-identity
  in the diffrun suite and (ii) the **live eqgate** (`run_eqgate`, `--one`), which
  re-compiles the `-demand-instance` nested arm in all four modes and byte-compares
  each against the flat golden. R1 does not touch demand/instance lowering, so both
  hold. It carries NO `.deltarel` golden (OD-10 defer), so the eager ops it now
  mints produce NO golden churn (its dump is never blessed).
- `demand_cyclic_1`, `demand_diff_input_1`, `demand_recursive_content_1` — the
  three `-demand-instance` / `-demand` fences. They REJECT (clean diagnostics) at
  the Program::Build nested pre-pass / the plain-`-demand` body-walk, which R1 does
  not touch; their expected-diagnostic status in `runall.sh` is unchanged. Covered
  by the standard suite pass.

### D.2 The ONLY golden churn — `demand_tc_witness.deltarel.opt.golden`

Re-blessed wholesale (OD-10-expected). Content = C.4's byte-draft: the two ingest
folds unchanged, plus ⟨N⟩ eager-op blocks, plus the two appended census columns.
`rounds:`/`deps:` stay empty. No existing census column changes value or position.

### D.3 Op-count predictions for the two carriers

Structural upper-ish (build-out §5.4, NOT verified; the ground truth is the first
post-R1 census, pinned at bless — OQ1):

| carrier | `kEagerForward` | `kEagerInsert` | `kIngestFold` (unchanged) |
|---|---|---|---|
| `symrec_tie_1` | ~7 (one per TUPLE view) | ~2–3 (all sink=relation) | 1 |
| `demand_tc_witness` | ~11 | ~3–4 (all sink=relation) | 2 |

Both witnesses PUBLISH ZERO messages (build-out §5.3) ⇒ every `kEagerInsert` is
`sink=relation`; NO `publish-now`/`publish-vec`/`commit-published` block, and NO
new PUBLISH id in `.ir`/`.h` (the byte-identity invariant D.1 rests on).
Per-dispatch grain means a TUPLE reached from >1 eager edge counts >1; the exact
integers are the OQ1 residue.

> **[ADJ-S5] UPHELD (crit-model F5 LOW) — FLAG LOUD: the `publish-now` /
> `publish-vec` / `commit-published` sink spellings (C.1) and the stream-terminal
> `message=<name>/<arity>` render (C.2) are CORPUS-UNWITNESSED at R1.** Since both
> pinned carriers publish zero messages, every emitted `kEagerInsert` is
> `sink=relation`, so those four `EagerSinkName` productions plus the stream
> `message=` arm ship structurally-predicted-only, byte-unverified against any
> corpus golden (the same unwitnessed-spelling gap class as pre-existing
> loud-abort arms). NOT a blocker. RECOMMEND a future publishing-demanded-insert
> corpus case (a demanded body whose terminal INSERT publishes a message) to pin
> those bytes; until then they are a labeled prediction. A later reviewer must NOT
> assume the four publish-* sink bytes are corpus-pinned.

### D.4 Q5 (mint cost) prediction — argued bound

- **Is the dump emitted when no `-deltarel-out`?** NO (F-DUMPGUARD:
  `DumpDeltaRelIfEnabled` PRE-guards on `gDeltaRelDumpStream != nullptr`). Render
  cost off-flag = zero. The suite/bench compile lines do NOT pass `-deltarel-out`,
  so the timed path pays only the MINT cost.
- **Mint cost per compile.** Two additions, both O(eager-web-size): (i) at the
  walk, one `RecordEagerDispatch` `push_back` per TUPLE/INSERT dispatch (a
  struct-copy into a `std::vector`, amortized O(1)); (ii) at `BuildDRInventory`,
  one ctor call + `flow.ops.push_back` per recorded dispatch (no ids, no
  effects, no vec def-edge). On `progsize@128` (a 128-rule chain), the eager web
  is O(rules) ⇒ O(128) extra `push_back`s + O(128) op constructions per compile,
  against a walk that already visits every rule's views. Predicted overhead:
  **sub-1%, inside ABABAB noise** (the D1.b/D2.b Q5 runs sat at ±1.4% noise for
  larger changes). `flow.ops` grows by the eager-web size, so the linearizer's
  Kahn sort over `n` ops grows — but with NO dep edges on the eager ops the sort
  is dominated by the existing differential ops on differential programs, and on a
  monotone chain like `progsize@128` the sort is O(n log n) on n≈web-size, still
  tiny. **The ABABAB gate (same-session interleaved) is the decider; prediction:
  0.0–1% median, noise.**

> **[ADJ-S8] UPHELD (crit-gates F4 MEDIUM) — the Q5 bound counts only the MINT;
> the ABABAB progsize@128 run is the GATE, not the pre-registered number
> (E-77: RUN the referee).** D.4 bounds only the per-dispatch `push_back` + ctor.
> It omits that on every MONOTONE compile after R1, `flow.ops` grows from ~2 ops
> (just the ingest folds) to O(rules), and every `flow.ops`-iterating pass now
> scans the enlarged vector: `LinearizeAndValidateDRFlow` (Kahn + per-op
> `key_of`), the V-OLD-EQUIV recount (`count_kind` scans `flow.ops` per
> `expect`), `ValidateDROps` per-op, the `DeriveDRStrata` `*_stratum` fill.
> ADJUDICATED at the code: none is O(n²) in `flow.ops` from the eager ops — the
> dep-edge derivation iterates per-SHARED-VEC (`DeltaRel.cpp:4300+`), and
> effect-free eager ops touch no vec, so they add zero edges and zero pairwise
> work; `key_of`/`count_kind`/the strata fill are each O(n); the Kahn sort is
> O(V+E). So the analytic sub-1% is plausible — BUT "plausible" is the E-77
> anti-pattern. **AMEND D.4:** replace the pre-registered "0.0–1% median" with a
> MEASURED verdict — run `bench/runbench.sh <root> progsize 128` ABABAB
> (same-session interleaved; `progsize@128` is a monotone chain, the worst case)
> and report the actual median. The number is the GATE; the prediction is only
> the going-in expectation.

---

## (E) GATES + THE BLESS PLAN

### E.1 The direct-diff referee for the deltarel golden (E-77: it gets RUN)

The re-bless is NOT eyeballed. Concrete referee, RUN before `--bless`:
```
DR=build/debug/bin/drlojekyll
$DR tests/OptDiff/cases/demand_tc_witness.dr -demand -deltarel-out NEW.deltarel …
diff tests/OptDiff/goldens/demand_tc_witness.deltarel.opt.golden NEW.deltarel
```
Acceptance (all three must hold, the HP-14 three-point mold, D1.b §19(L)
precedent):
1. **Existing lines untouched.** Every line present in the OLD golden that is NOT
   a new eager-op block appears byte-identical in NEW — the two `kIngestFold`
   blocks (op.0/op.1 payloads), `rounds:`, `deps:` (still empty). The ONLY
   census-line delta is the tail append ` kEagerForward=<F> kEagerInsert=<I>`;
   NO existing census column changes value.
2. **Every added line is an eager-op block or the census tail.** The diff's `>`
   lines are exclusively `op.N kEagerForward …`/`op.N kEagerInsert …` headers,
   their `    args:` lines, and the census tail — nothing else.
3. **`deps:`/`rounds:` empty, census-sum green.** The dump does not abort
   (F-CENSUSABORT: `census_total == flow.ops.size()`), so `<F>+<I>+2 ==
   flow.ops.size()` holds by construction.
Then `runall.sh --bless <workroot> demand_tc_witness`; `git` must show EXACTLY
ONE golden file changed (the `.deltarel`), the other 5 IR goldens byte-identical
(P-D1b.2-style zero-stray-churn check).

> **[ADJ-S7] UPHELD (crit-gates F3 MEDIUM) — bless provenance: referee the ACTUAL
> bless-source file, not a separate compile.** CODE-CONFIRMED: `runall.sh
> --bless` copies the deltarel golden from
> `$WORKROOT/demand_tc_witness/demand_tc_witness.irgold/deltarel.opt.out` (bless
> loop `runall.sh:107-118`), a file produced by `run_irgold`'s ALL-SINKS-IN-ONE
> compile (`-df-out -deltarel-out -ir-out -cpp-out` together + the case's
> `.drflags` = `-demand`, `run_irgold` lines 271-283). E.1's referee instead
> hand-compiles a SEPARATE `NEW.deltarel` with only `-demand -deltarel-out` — the
> refereed bytes are not PROVABLY the blessed bytes (they *should* match
> regardless of co-present sinks, but a bless ritual must not assume "should").
> **AMEND E.1:** the referee diffs the actual bless-source file from a full suite
> run — `diff goldens/demand_tc_witness.deltarel.opt.golden
> $WORKROOT/demand_tc_witness/demand_tc_witness.irgold/deltarel.opt.out` — so
> `--bless` then copies the exact file that was refereed. Two supporting pins:
> (a) run-then-bless-from-the-SAME-workroot ordering is required (a stale/empty
> workroot trips the `[ ! -f "$isrc" ]` FATAL at `runall.sh:111-113`, the safety
> net — document the ordering anyway); (b) `--bless demand_tc_witness` re-copies
> ALL FOUR pinned surfaces (h/ir/df/deltarel, bless loop iterates every sidecar
> line) — the "exactly one file changed" claim holds ONLY by content-identity of
> h/ir/df, so make it a POST-condition to verify: `git status` after bless shows
> only `.deltarel.opt.golden`; if h/ir/df appear, R1's zero-emission-change claim
> is ALREADY violated (a useful extra tripwire — >1 file ⇒ emission moved).

### E.2 Config-invariance / determinism on the dump surface

- **Determinism ((F) law).** Run the referee twice; the two `-deltarel-out`
  outputs must be byte-identical (the eager-op enrollment iterates the
  walk-recorded `std::vector` in order — no `unordered_map`, no pointer order).
- **Knob-independence.** The eager web exists in every mode; verify the eager op
  COUNT does not vary across the 4 optimization modes for a spot case (compile
  `demand_tc_witness` under `-disable-dataflow-opt` / `-disable-controlflow-opt`
  with `-deltarel-out` and confirm the census `kEagerForward`/`kEagerInsert`
  columns are stable — the dump is pinned at `opt` only, but a mode-varying eager
  census would signal an accidental knob coupling). Expected: stable (the walk is
  post-optimize, syntax-directed over the same monotone views).

> **[ADJ-S6] UPHELD (crit-gates F2 HIGH) — the task's "3-run + debug==release,
> which surfaces" is not scheduled for the new dump surface. ADD to (E):**
> 1. **debug==release on the deltarel dump.** Render `demand_tc_witness` AND
>    `symrec_tie_1` to `-deltarel-out` under BOTH presets (`debug`, `release`) and
>    `cmp -s` the two. The (F)-law audit (E.6) *argues* no pointer-ordered
>    iteration leaks into the dump; the standing G5 check *verifies* it
>    empirically — a release-only reordering (an `unordered_map` traversal the
>    audit missed, a build-flag-dependent `table->id` assignment) surfaces ONLY
>    here, on exactly the new path (`BuildDRInventory` reading the walk-recorded
>    `std::vector` and minting into `flow.ops`). Expected: byte-identical.
> 2. **Suite ×3.** D.1 pre-registers a single `SUITE: PASS (173)`; the standing
>    ritual runs it ×3 to catch nondeterministic flakes — relevant precisely
>    because R1 enlarges `flow.ops` (a new ordering-sensitive Kahn input) on every
>    compile. Pre-register ×3, and pre-register that `demand_tc_witness deltarel
>    opt IRGOLD-DIVERGE` is the ONLY expected red across all three PRE-bless runs
>    (green post-bless).

### E.3 E-62 tripwire re-grep

At this DeltaRel diff, re-grep for new `pinned_order` / `body_ops` / `output_ops`
readers (the E-62 discipline). R1 adds NO new `pinned_order` consumer — `key_of`
is EDITED (a new branch in the existing consumer), not a new reader; the render
walk already reads `pinned_order`. The sole sanctioned external hit stays the
`Stratum.cpp:1073` comment. Expected: CLEAN (no new reader).

### E.4 ASAN both surfaces

Per the standing gate (§19(F)/(J)): `build/asan` ctest + the OptDiff suite under
`DR=asan` + the env-CXX-wrapper second surface. R1 adds a `std::vector` field +
`std::optional<QueryView>`/`std::optional<ParsedMessage>` records — ASAN watches
for use-after-free of the Query handles (they outlive `BuildStratumPhases`, same
lifetime as `ingest_receive`; no new hazard). Expected: zero reports, SUITE PASS
173 both surfaces.

### E.5 ctest / DeltaRelValidators fixture impact

`tests/DeltaRelValidators` (ctest target 4, the `CheckInstanceOrder` death test)
is UNAFFECTED: it exercises `V-INST-ORDER` on the keyed-instance kinds, which R1
does not touch. The A.6(c) structural recount is inline in `V-OLD-EQUIV`
(compile-covered corpus-wide, not a new fixture). Expected: ctest 5/5 debug +
DeltaRelValidators green under ASAN, no new fixture.

### E.6 The (F)-law audit of every new loop

Every loop R1 adds, audited for walk/mint-order-only (no pointer order):
- `RecordEagerDispatch` — appends in walk (DFS) order. ✓
- `BuildDRInventory` EAGER_WEB block — iterates `context.emitted_eager_ops` (a
  `std::vector`) in index order. ✓
- `key_of` eager branch — orders by `(op_table_id via table->id, ctor=oi)`,
  deterministic ids only. ✓
- The A.6(c) recount loop — iterates `flow.ops` by index; asserts only, no order
  dependence. ✓
- The C.2 render cases — read stored fields (`table_op_table->id` via `tid`,
  `eager_message` name/arity, `eager_sink` spelling); no pointer-ordered
  iteration. ✓
No new `std::unordered_map`/`std::set`-over-pointers iteration is introduced.

---

## (F) NON-GOALS (explicit)

- **No other dispatch arms.** CMP / MAP / JOIN / MERGE / SELECT / NEGATE all wait
  for their own Rk. In particular the SELECT rebind arm (`Build.cpp:1133`), though
  it shares `BuildEagerInsertionRegions` with the TUPLE arm, is NOT modeled — the
  terminal-INSERT relation branch's recursive descent into SELECT readers stays
  hand-coded. The JOIN slice (a later Rk) carries the owner-raised, NOT-RULED
  pivot-equality-belt fold candidate (rel-epoch-open-brief §5) — R1 does NOT touch
  the join probe or its TUPLECMP belt.
- **The E-42 VECTORLOOP shim untouched** (`Procedure.cpp:995`, the table-less
  monotone receive). It is seam artifact S4, retired only at R-final.
- **No seam-artifact retirement.** The ingest-fold hole contract (S1), the
  replicated cut-successor predicates + §7d cross-check (S2), and V-INGEST-XCHECK
  Site 5 (S3) all STAND unchanged. R1 adds observability alongside them; the seam
  deletion is R-final (§19(H) acceptance).
- **V-INGEST-XCHECK untouched.** Its enrolled loop filters to `kIngestFold`
  (`Stratum.cpp:2124`) — R1's new kinds are skipped, Site 5 is inert to them
  (build-out §4). No `V-EAGER-XCHECK` sibling is added at R1 (it would be vacuous
  under walk-authoritative enrollment — A.6(c)).
- **No access-plan / WCOJ work.** The eager ops carry no `spine`/`arms` at R1
  (effect-free markers); the access-plan consumer opens only when honest
  effect-bearing eager ops land at R-final.
- **HP-17 untouched.** The keyed-instance kinds 15/16/17 and their inert
  death-op machinery are unaffected by appending 18/19 (positional, unserialized).
