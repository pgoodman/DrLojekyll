======================================================================
COMMITTED AT THE §19 CHECKPOINT (2026-07-20, tip 1aaca896). This is the
desired-states fleet's binding consolidated record (the four adjudicated
dump blocks for demand_neighborhood_witness), verbatim from the session
scratchpad (fleet-ds/ds-consolidated.md). AMENDED BY d1-pinned.md §3's
closing paragraph (census shape contingent on OD-4; kInstanceEmit/Old
spellings contingent on HP-11; p-rules gain HP-6's partition pin); its
§C residuals C-1..C-12 are dispositioned in d1-pinned.md §4. The real
dumps it byte-pins live in d1-ground-truth-nbhd.md.
======================================================================

# DESIRED-STATES — CONSOLIDATED RECORD (fleet-ds, binding)

Consolidator: XHIGH, 2026-07-19. Repo /Users/pag/Code/DrLojekyll, branch
keyed-instances, frozen tip 1aaca896 (NO D1/D2 code exists — every NESTED
block is DESIGN-ONLY, hand-derived against the binding
fleet-d1/d1-design-consolidated.md §A).

Inputs: ds-df-nbhd.md + ds-deltarel-nbhd.md (the two desired-state artifacts),
critique-ds-df.md, critique-ds-deltarel.md, grammar-conformance.md (E-71 lane),
GROUND-TRUTH.md GT-1..GT-6 + the real dumps run1/nbhd.{df,deltarel,ir},
probes/da.deltarel (differential/OD-7 donor), t2-dump-spec.md v3.4 +
t2b-grammar.md, the committed goldens, the pinned
t2-desired-deltarel-average_weight.md contract. EVERY critique + grammar
finding was re-adjudicated at code by this consolidator; verification cites
below are mine, at tip.

VERDICT: **READY-FOR-JUDGE** — with the amended blocks in §B and the twelve
numbered residuals in §C. Two consolidator-minted mechanism findings
(X-DS-1/X-DS-2, §A.0) CORRECT the nested `.deltarel` block beyond what any
lane or critic proposed: the OD-7 frontier is effect-only (NO DRVec line —
the GROUP_UPDATE precedent never mints one), and OD-7 provisioning FORCES a
`kCommitSweep flavor=monotone` on the demand table (census `kCommitSweep=1`,
which every prior draft rendered `=0`). The nested `.df` is pinned
byte-identical to flat (Alternative A, ratified here with the
force_agg_tables precedent).

======================================================================
§A. ADJUDICATION LEDGER — every finding, accepted/rejected AT CODE
======================================================================

Notation: ACCEPT = folded into §B/§C; ACCEPT-AMENDED = folded with change;
SUPERSEDED = overtaken by a consolidator finding; REJECT = not folded.

----------------------------------------------------------------------
§A.0 CONSOLIDATOR-MINTED FINDINGS (mechanism-traced; these amend the
     nested `.deltarel` block beyond all lane/critic proposals)
----------------------------------------------------------------------

**X-DS-1 [HIGH] — the OD-7 frontier mints NO DRVec; the vec line is DROPPED
and so is the `op.demand-ingest -> op.instantiate` by_vec RAW edge.**
VERIFIED at code + the pinned GROUP_UPDATE contract (the design's own OD-7
precedent, §A.6.3 "the GROUP_UPDATE cut-successor precedent"):
- The pinned t2-desired-deltarel-average_weight.md vec section (lines
  179-218) mints vec packs ONLY for the differential tables (4, 8, 12, 17,
  23, 28, 32); the GROUP_UPDATE's MONOTONE input `%table:36` — whose ingest
  fold op.52 carries `kVecAppend(%table:36, kNetAddition)` and whose
  GROUP_UPDATE op.4 carries `kVecDrain(%table:36, kNetRemoval/kNetAddition)`
  — has NO vec line at all. Same in the live donor: da.deltarel op.14's
  `kVecAppend(%table:7, kNetAddition)` has no `%table:7` vec line (da vec
  section lines 3-15 cover only %table:4/%table:11).
- Mechanism: `MakeMonotoneIngestFold` pushes the append effect when
  `ingest_role != kEmpty` (DeltaRel.cpp:832-840); the inventory explicitly
  declines the mint+def ("NO def-edge: the monotone kNetAddition vec is not
  minted by the differential-only MintTableVec loop; the append parks via
  TableDeltaVector at LOWER time", DeltaRel.cpp:1855-1858).
- The hazard path is GRACEFUL, not asserting: `ResolveVecIdx` returns ~0u
  for an un-minted (table, role) (DeltaRel.cpp:3110-3127) and
  `add_vec_access` skips ~0u (DeltaRel.cpp:3257-3261) — so kVecDrain/
  kVecAppend on a never-minted monotone frontier enroll NO by_vec hazard.
  Confirmed live: the avg_weight contract deps have NO op.52 -> op.4 edge
  (op.52's only edge is `op.52 -> op.51 WAW`, to the monotone sweep).
- Effect rendering is independent of the mint (grammar EF-2, t2b:489-499:
  effects render table+role from the DREffect's own fields, never a vec
  index) — so the append/drain effects render fine with no vec line.
CONSEQUENCE: the deltarel author's `vec $net-addition.0 ... def=[op.1]
use=[op.2]` line and its `op.1 -> op.2 RAW epoch` dep are BOTH removed from
the pinned block. deltarel-critique F1's evidence is upheld but its
amendment ("mark def owner unpinned, mint is a precondition") is SUPERSEDED:
the precedent-faithful OD-7 needs NO DRVec mint at all — the runtime vector
is the LOWER-time `TableDeltaVector` (Build.cpp:743-757), invisible to the
DR-IR vec inventory. Ordering of ingest-before-instantiate is carried by
the stratum lift + lead-0 ingest key (DeltaRel.cpp:3455-3457), not by a dep
edge — exactly as GROUP_UPDATE orders op.52 before op.4 with no edge.

**X-DS-2 [HIGH] — OD-7 provisioning FORCES a `kCommitSweep flavor=monotone`
op on the demand table: the nested census is `kCommitSweep=1`, NOT 0, and
deps gain a WAW edge from the demand ingest fold to the sweep.**
VERIFIED at code, the whole chain:
- The eager walk's cut-successor test (Build.cpp:958-961: deletion-capable /
  aggregate / KV successor) both STOPS the descent and drives "the boundary
  append below [which] provisions this monotone input table's net-additions
  frontier" (Build.cpp:948-956 comment) via `TableDeltaVector` →
  `context.table_delta_vecs[table]` (Build.cpp:743-745).
- The DR commit-sweep mint loop: "A monotone boundary table with a delta-vec
  entry gets a Seal sweep" — `kCommitSweep`, `sweep_flavor = kMonotone`, one
  `kFlagWrite(table)` effect (DeltaRel.cpp:1673-1688).
- `MonotoneIngestRoleDR` returns kNetAddition exactly when a member view has
  a cut successor (DeltaRel.cpp:147-160 + AnyCutSuccessorDR :130-137, the
  replica of Build.cpp:958); §7d cross-checks role vs the walk map every
  compile — role and provisioning CANNOT diverge.
- Live proof: avg_weight's monotone input %table:36 gets
  `op.51 kCommitSweep sign=· ctx=seed band=9 flavor=monotone
  publish_target=false` with `effects: {kFlagWrite(%table:36)}` and the
  by_flag WAW edge `op.52 -> op.51 WAW epoch` (ingest kCounter write vs
  sweep kFlagWrite write, same table, direction key-forced ingest-first).
  da.deltarel mirrors it: `op.14 -> op.12 WAW epoch`.
CONSEQUENCE: under D2.b, OD-7 = extending the cut test to the recognized
subgraph boundary; the demand table %table:8 gains a table_delta_vecs entry
⇒ the monotone sweep MINTS ⇒ nested census `kCommitSweep=1`, ops.size()=5,
deps gain `op.<demand-ingest> -> op.<sweep> WAW epoch`. Every prior draft
(author, both critics, grammar lane) rendered `kCommitSweep=0` — all
corrected in §B.4. TWO spillover questions go to the judge as residuals:
the provisioning blast radius onto the EDGE table (§C-2) and the seal's
commit-sweep carrier for the monotone pub table (§C-3).

**X-DS-3 [MED] — nested op LABELS shift: commit sweeps mint BEFORE ingest
folds in BuildDRInventory, so the flat labels op.0/op.1 do NOT carry over.**
VERIFIED: the mint loops run in file order — commit sweeps at
DeltaRel.cpp:1660-1700, ingest folds at :1840-1858; da confirms (sweeps
op.12/13 < ingest op.14/15/16). Nested illustrative labels: op.0 = the
%table:8 monotone sweep, op.1 = add_edge ingest, op.2 = demand ingest
(flat message order preserved relative), op.3 = kSubgraphInstantiate,
op.4 = kInstanceSeal (instance-op mint position itself unpinned, §C-10).
The deltarel author's P4 ("the 2 ingest folds are byte-STABLE flat→nested")
is FALSIFIED at the label/args bytes; what survives is the RELATIVE order
and (for the edge fold) the effect set. LINE order from the band key
(DeltaRel.cpp:3445-3477: lead-0 ingest folds tie-broken by table_id —
demand %table:8 before edge %table:11, which is ALSO the true explanation
of the flat op.1-before-op.0 order; lead-1 instantiate; lead-2 sweeps band
9, seals after): demand-ingest, edge-ingest, instantiate, sweep, seal.

**X-DS-4 [MED] — the nested `.df` byte-identity claim SURVIVES X-DS-2.**
The sweep/frontier changes touch no `.df` token: `table=`/`class=` come from
TableId()/CanReceiveDeletions() (Format.cpp:1162-1185), and a monotone
Seal sweep changes neither (the table stays monotone). Alternative A's
"nested .df == flat .df" pin is unaffected.

**X-DS-5 [MED] — the SP-1 spine correction.** The live emitter renders
`kAccess(<%table:N>, <LoweringName>)` where LoweringName is HYPHENATED
lowercase (`point-test`/`section-walk`/`full-scan`/`seek`,
Format.cpp:257-263, render at :480-484); PlanNode carries a `pred` field
but the emitter NEVER renders it (Format.cpp:475-491). The author's
`kAccess(%table:11, Present)` is grammar-nonconformant (grammar SP-1 pin,
t2b:519-531: render from stored fields, no gloss). AMENDED to
`kAccess(%table:11, section-walk)` (the rescan is keyed on the instance-key
column ⇒ a keyed section walk; the Lowering choice itself is ⟨PIN⟩, §C-9).
This also DISSOLVES the "double-render" worry (grammar F-7): the `reads:`
line renders the kFlagRead effect's Pred; the spine renders the PlanNode's
Lowering — two surfaces, two DIFFERENT stored fields, no duplication.

----------------------------------------------------------------------
§A.1 critique-ds-df (verdict SOUND-WITH-AMENDMENTS — UPHELD)
----------------------------------------------------------------------
- F-1 [MED] ACCEPT. VERIFIED: `grep -c 'class=table-less' run1/nbhd.df` = 5
  (select.0, select.1, tuple.5, join.6, join.7). The prose "four" is a
  miscount; the enumeration of five was already correct. Folded (prose fix
  only; no byte changes).
- F-2 [MED] ACCEPT. VERIFIED: the phrase "read-only membership oracles"
  appears NOWHERE in d1-design-consolidated.md; §A.3.1 scopes the
  disappearance to the guarded copy (%table:15/idx_38) only, and P-D2b.2
  ratifies "idx_57 + edge's idx_32 unchanged". The df artifact's Alt-B
  blast-radius speculation (tuple.2/tuple.4 "possibly" losing tables) is
  design-contradicted. FOLDED: P-2 tightens to a SINGLE-LINE prediction —
  under Alt-B only tuple.3's ATTRIBUTES line changes (`table=%table:15
  class=monotone` → `class=table-less`); %table:8 and %table:11 are pinned
  present either way.
- F-3 [LOW] ACCEPT. VERIFIED at tip: `force_agg_tables`
  (Build.cpp:167-179) GetOrCreates the agg/KV view AND its predecessors
  unconditionally, even though those views are eager-descent chain-breakers
  — the ONE landed chain-breaker leaves FillDataModel knob-blind. FOLDED
  into the Alt-A rationale (§B.2): Alt-A is the precedent-consistent
  reading; Alt-B would be the first chain-breaker to teach FillDataModel a
  knob.

----------------------------------------------------------------------
§A.2 critique-ds-deltarel (verdict SOUND-WITH-AMENDMENTS — UPHELD,
     two findings partially superseded by §A.0)
----------------------------------------------------------------------
- F1 [HIGH] ACCEPT-EVIDENCE, AMENDMENT SUPERSEDED by X-DS-1. The critique's
  three evidence legs all re-verified (conditional append
  DeltaRel.cpp:832-840; the no-mint/no-def comment :1855-1858; da's
  `def=[]` on every net-addition vec). But its amendment (keep the vec
  line, mark the mint a "hard precondition" and the def owner unpinned)
  still assumed a DRVec must exist for the drain to work — FALSE:
  ResolveVecIdx/add_vec_access are graceful (X-DS-1), and the pinned
  GROUP_UPDATE contract drains a never-minted monotone frontier. The vec
  LINE IS DROPPED from the pinned block; a first-class DRVec mint is a
  NOT-RECOMMENDED judge option (§C-5).
- F2 [HIGH] ACCEPT-AMENDED. The by_flag mechanism re-verified (kCounter
  enrolls write, kFlagRead enrolls read, DeltaRel.cpp:3293-3301; by_flag
  cross-product → emit_rw_hazard :3665-3684; writer-first-in-key → RAW
  :3597-3605). The edge ingest fold (kCounter write on %table:11) vs the
  instantiate's kFlagRead(Present, %table:11) IS a confirmed RAW — promoted
  into the pinned block as `op.1 -> op.3 RAW epoch` (X-DS-3 labels). The
  critic's OTHER retained edge (the by_vec `op.1 -> op.2 RAW`) is REMOVED
  per X-DS-1, and a NEW edge (`op.2 -> op.0 WAW`, demand-ingest → monotone
  sweep) is ADDED per X-DS-2. Net deps = 2 lines, but a different 2 than
  the critic proposed. The critic's conditionality rider stands: the RAW
  edge and the `reads:` line are the same kFlagRead effect seen from two
  surfaces — they stand or fall together (design §A.2.3 mandates the
  effect; pinned WITH it).
- F3 [MED] ACCEPT. The op.3 effect ORDER's authority is design §A.2.3's own
  push sequence (drain, demand-read, rebuild, emit, old, then the !DIFF
  counter); avg_weight is only the band-(a)-then-(b) shape mold (it has a
  two-sign interleave and no demand-read analog). Citation corrected; the
  rendered order was already right — unchanged bytes.
- F4 [MED] ACCEPT. `stratum=2` corrected to `stratum=1`: the DR lift places
  the instantiate one above its two stratum-0 ingest inputs (the
  GROUP_UPDATE one-above precedent: avg op.4 stratum=1 over op.52 stratum
  0). Do NOT read the edge table's DF stratum (2) as a DR stratum — the two
  id spaces are unrelated. Still ⟨PIN⟩-illustrative.
- F5 [LOW] ACCEPT (endorse render-both `i#0` + `store=I#0`; collapse only
  if the implementer confirms one id space; §C-7).
- F6 [LOW] ACCEPT (positive: census append-order = enum-declaration order,
  verified at Format.cpp:779-792 — kAllKinds is the literal enum-order
  array, no zero-suppression, completeness abort at :796).
- F7 [LOW] ACCEPT (positive: flat op.1-before-op.0 is real; consolidator
  adds the true mechanism — the lead-0 ingest key tie-breaks on table_id,
  %table:8 < %table:11 — so the demand_tc_witness "opposite order" is just
  different table ids, not a different rule).

----------------------------------------------------------------------
§A.3 grammar-conformance (E-71 lane) — dispositions
----------------------------------------------------------------------
- F-1 [HIGH] ACCEPT — THE cross-artifact pin. RESOLVED: **Alternative A is
  pinned for both artifacts** (nested `.df` == flat `.df` byte-for-byte;
  FillDataModel untouched; every "%table:15 VANISHES" statement is scoped
  to the GENERATED CODE / `.ir` surface, never the DataFlow model). Grounds:
  the design's own "excision at emission" (§A.5), GT-5, and the
  force_agg_tables precedent (§A.1 F-3, verified). The deltarel artifact's
  NESTED intro prose is amended to carry the scoping. Goes to the judge as
  §C-1 for ratification (it is the one owner-visible representation choice).
- F-2 [HIGH] ACCEPT — the five kInstance* EffKind arg shapes are
  author-invented (modeled on kState*); each needs a spelling row + backing
  DREffect field confirmation at D1.b (the EF-1 class). Residual §C-8.
- F-3 [HIGH] ACCEPT — the `instances:`/`DRInstance` section is the largest
  new-grammar invention (no spec section, no exemplar, §A.7 names it with
  zero bytes). Kept in the block (banner-marked); residual §C-6.
- F-4 [MED] ACCEPT — instance header tail (`i#0 store=I#0`) + second line
  (`pub_row=[ik:Start,row:Node] nested=<Node>`, §A.7-example-verbatim) are
  design-sourced but un-pinned. Residuals §C-7/§C-12.
- F-5 [MED] SUPERSEDED by §A.2-F2/X-DS-1/X-DS-2: the deps block is now
  TWO edges, neither of which is the author's original single edge.
- F-6 [MED] ACCEPT — `instances:` placement (after vecs — here first, as
  there is no vec section — before ops) is a choice; folded into §C-6.
- F-7 [MED] ACCEPT-AMENDED — resolved by X-DS-5: `reads:` renders the
  kFlagRead Pred; the spine renders the Lowering; no double-render of the
  Pred remains in the amended block.
- F-8 [MED] ACCEPT-AMENDED — the role stays kNetAddition (R-N1 confirmed,
  no new VecRole/sigil), but the "faithful da clone" vec-line half is
  overturned by X-DS-1 (no vec line at all).
- F-9 [LOW] ACCEPT (positive: recognition metadata partition — NONE on
  `.df`, DRInstance on `.deltarel` — ratified; §C-11).
- F-10 [LOW] ACCEPT (positive: the POST-D1b census churn is pin-exact;
  unchanged in §B.3).
- F-11 [LOW] ACCEPT — same fold as X-DS-5 (spine arg-2 = Lowering, not
  Pred).
- F-12 [LOW] ACCEPT (stratum illustrative; now =1 per §A.2-F4).
- F-13 [LOW] ACCEPT-AMENDED — the ingest RELATIVE order (demand-then-edge
  by line) survives nested; the LABELS do not (X-DS-3).
- F-14 [LOW] ACCEPT (rendering the full block twice is house-style fine).

----------------------------------------------------------------------
§A.4 The authors' own residuals — dispositions
----------------------------------------------------------------------
- df R-1 (Alt-A vs Alt-B): RESOLVED → Alternative A pinned (§A.3 F-1);
  judge ratifies (§C-1).
- df R-2 (.df annotation rendering): RESOLVED → NO `.df` annotation
  rendering in D1 (§A.7 is deltarel-only); judge ratifies (§C-11).
- deltarel R-N1 (frontier role spelling): CONFIRMED — reuse kNetAddition;
  NO new VecRole/sigil; and per X-DS-1 no vec line either.
- deltarel R-N2 (reads: vs spine): RESOLVED by X-DS-5 — render BOTH, from
  their two distinct stored fields (kFlagRead effect → `reads:
  Present(%table:11)`; PlanTree → `kAccess(%table:11, section-walk)`).
- deltarel R-N3 (instances: grammar): STANDS → §C-6.
- deltarel R-N4 (by_flag edge): RESOLVED CONFIRMED (§A.2-F2) — in the
  pinned block, relabeled op.1 -> op.3.
- deltarel R-N5 (i#/I# dual id): STANDS → §C-7.
- deltarel R-N6 (kInstance* arg shapes): STANDS → §C-8.

======================================================================
§B. THE AMENDED FINAL DUMP BLOCKS (all four; every accepted amendment
    folded; ILLUSTRATIVE-id banners per the symrec precedent)
======================================================================

----------------------------------------------------------------------
§B.1 FLAT `.df` (`-demand`) — REAL, byte-pinned, ZERO prediction
----------------------------------------------------------------------
The exact bytes of run1/nbhd.df (43 lines, 3-run deterministic,
debug==release; machine-diffed EQUAL by the critic and re-checked here).
UNCHANGED by adjudication.

```
dataflow

select ^select.0 (From:u64, To:u64)                ; recv #message add_edge/2
  ATTRIBUTES class=table-less stratum=0
  => ^tuple.2 (From, To)

select ^select.1 (c3:u64)                          ; recv #message demand__neighborhood_bf/1
  ATTRIBUTES class=table-less stratum=1
  => ^tuple.4 (c8=c3)

tuple ^tuple.2 (From:u64, To:u64)
  ATTRIBUTES table=%table:11 class=monotone stratum=2
  => ^join.6 .in1(From, To)

tuple ^tuple.3 (Start:u64, Node:u64)
  ATTRIBUTES table=%table:15 class=monotone stratum=5
  => ^join.7 .in1(Start, Node)

tuple ^tuple.4 (c8:u64)
  ATTRIBUTES table=%table:8 class=monotone stratum=3
  => ^join.6 .in0(c8)
  => ^join.7 .in0(c8)

tuple ^tuple.5 (Start:u64, Node:u64)
  ATTRIBUTES class=table-less stratum=7
  => ^insert.8 (Start, Node)

join ^join.6 (From:u64, To:u64) {
  pivot From:u64 <- .in0.c8, .in1.From
  out To:u64 <- .in1.To
}
  ATTRIBUTES class=table-less stratum=4
  => ^tuple.3 (Start=From, Node=To)

join ^join.7 (Start:u64, Node:u64) {
  pivot Start:u64 <- .in0.c8, .in1.Start
  out Node:u64 <- .in1.Node
}
  ATTRIBUTES class=table-less stratum=6
  => ^tuple.5 (Start, Node)

insert ^insert.8 (Start:u64, Node:u64) into %table:4
  ATTRIBUTES class=monotone stratum=8
```

Prose corrections folded (no byte impact): FIVE table-less views
(select.0, select.1, tuple.5, join.6, join.7 — §A.1 F-1); the flag-off
baseline block (off/nbhd.df, 3 blocks) stands as rendered by the author.

----------------------------------------------------------------------
§B.2 NESTED `.df` (`-demand -demand-instance`, STAGE-1 R-MONO-a) —
     DESIGN-ONLY; **PINNED: BYTE-IDENTICAL TO §B.1** (Alternative A)
----------------------------------------------------------------------
BANNER: no nested compile exists; the claim is that the §B.1 bytes ARE the
nested dump. PINNED HERE (judge ratifies as §C-1):

- The knob is an EMISSION-level lowering selector (design §A.5); it runs no
  DataFlow rewrite, so the 9 views / det_seq 0..8 / all column tokens
  (incl. c3/c8) / all `=>` edges are knob-invariant.
- `-df-out` drains post-Program::Build; `table=` comes from TableId(), set
  only at Data.cpp:252 by FillDataModel — which is knob-blind
  (Build.cpp:34-178; insert→%table:4 at :59-62, the three join
  predecessors at :100-102).
- PRECEDENT (folded per §A.1 F-3, verified): the landed aggregate/KV
  chain-breaker still forces its tables in FillDataModel
  (force_agg_tables, Build.cpp:167-179) despite stopping the eager
  descent — emission-only excision leaving TableId() intact is how the ONE
  landed chain-breaker already behaves. Alt-B would be the first
  chain-breaker to teach FillDataModel a knob.
- X-DS-4: the OD-7 sweep/frontier machinery (X-DS-2) changes no `.df`
  token (%table:8 stays monotone; TableIds untouched).
- CONSEQUENCE: `tuple.3` KEEPS `table=%table:15 class=monotone` in the
  nested `.df`; the storage/index win (`create %table:15` + `%index:38`
  gone, `^flow:58` → instance machinery) is `.ir`/`.h`-visible ONLY
  (P-D2b.2 lives there, correctly). The `.df` is a lowering-INVARIANT —
  the structural half of the equivalence gate — and ONE golden serves both
  arms.
- FALLBACK (pre-registered, tightened per §A.1 F-2): if the judge instead
  adopts Alt-B (knob-aware FillDataModel), EXACTLY ONE line differs —
  tuple.3's ATTRIBUTES line becomes `class=table-less` (loses `table=`,
  class flips per Format.cpp:1169). %table:8 (demand, GT-1) and %table:11
  (edge, idx_32 ratified unchanged by P-D2b.2) are NOT in question under
  either alternative.
- NO `.df` annotation rendering in D1 (§A.7 is deltarel-only; D1.a side
  tables are inert to the `.df`) — ratify as §C-11.

Bytes: identical to §B.1 (not repeated; the block above is the contract
for BOTH arms under the pinned Alternative A).

----------------------------------------------------------------------
§B.3 FLAT `.deltarel` — BOTH STAGES
----------------------------------------------------------------------
STAGE 0 (today, REAL — byte-pinned from run1/nbhd.deltarel; the `—` is
U+2014; single trailing newline after the census line). UNCHANGED:

```
deltarel

op.1 kIngestFold sign=+ ctx=eager stratum=0
    effects: {kCounter(%table:8, +, NonRecursive)}
    spine: —
    args: table=%table:8 message=demand__neighborhood_bf/1
op.0 kIngestFold sign=+ ctx=eager stratum=0
    effects: {kCounter(%table:11, +, NonRecursive)}
    spine: —
    args: table=%table:11 message=add_edge/2

rounds:

deps:

census: kCrossover=0 kProductArm=0 kSeedFold=0 kFixpointFire=0 kChainFold=0 kClaimDrain=0 kRetire=0 kRederive=0 kFrontierFilter=0 kCommitSweep=0 kNegateGate=0 kPivotAssemble=0 kIngestFold=2 kGroupUpdate=0 kStateSeal=0
```

STAGE 1 (FLAT-POST-D1b, DESIGN — the mint stays gated off; the ONLY byte
change on every existing deltarel carrier is the census line: three `=0`
counters appended in enum-declaration order after kStateSeal, per the
kAllKinds array discipline verified at Format.cpp:779-792). UNCHANGED from
the author's block (verified pin-exact by both the critic and the grammar
lane, §A.3 F-10):

```
deltarel

op.1 kIngestFold sign=+ ctx=eager stratum=0
    effects: {kCounter(%table:8, +, NonRecursive)}
    spine: —
    args: table=%table:8 message=demand__neighborhood_bf/1
op.0 kIngestFold sign=+ ctx=eager stratum=0
    effects: {kCounter(%table:11, +, NonRecursive)}
    spine: —
    args: table=%table:11 message=add_edge/2

rounds:

deps:

census: kCrossover=0 kProductArm=0 kSeedFold=0 kFixpointFire=0 kChainFold=0 kClaimDrain=0 kRetire=0 kRederive=0 kFrontierFilter=0 kCommitSweep=0 kNegateGate=0 kPivotAssemble=0 kIngestFold=2 kGroupUpdate=0 kStateSeal=0 kSubgraphInstantiate=0 kInstanceDeath=0 kInstanceSeal=0
```

The demand_tc_witness.deltarel.opt.golden:16 re-bless target (same
three-counter append to its own line-16 bytes) stands as the author
rendered it — the pre-registered mechanical single-line bless, permcheck-
refereed (design §C decision 9).

----------------------------------------------------------------------
§B.4 NESTED `.deltarel` (`-demand -demand-instance`, STAGE-1 R-MONO-a,
     post-D2.b) — DESIGN-ONLY, HEAVILY AMENDED (X-DS-1/2/3/5 + F2/F4)
----------------------------------------------------------------------
BANNER: ALL ids ILLUSTRATIVE — ⟨PIN⟩ AT FIRST EMISSION (the symrec
precedent). What this block CERTIFIES: the op SET (2 ingest folds + 1
monotone commit sweep + 1 instantiate + 1 seal, ZERO death ops), the
effect MULTISETS (regime-split per design §A.2.3, !DIFF arm), the LINE
ORDER (band-key derivation, X-DS-3), the census counts, and the two dep
edges. The `instances:` section, the kInstance* arg shapes, and the
instance header tail are NEW GRAMMAR pending D1.b spelling rows
(§C-6/7/8/12).

AMENDMENTS FOLDED vs the author's draft: NO vec line (X-DS-1); the
monotone commit sweep op + census kCommitSweep=1 (X-DS-2); labels
re-derived from mint order (X-DS-3: sweep=op.0, ingest=op.1/op.2,
instance ops op.3/op.4); deps = {edge-ingest→instantiate RAW (confirmed
by_flag, §A.2-F2), demand-ingest→sweep WAW (X-DS-2)} — the author's
by_vec RAW is gone; spine arg-2 = the Lowering, not the Pred (X-DS-5);
instantiate stratum=1 (§A.2-F4).

```
deltarel

instances:
  DRInstance i#0 forcing=neighborhood/bf key=%table:8 pub=%table:4 input=%table:11 store=I#0 key_cols=[Start] row_cols=[Node]

op.2 kIngestFold sign=+ ctx=eager stratum=0
    effects: {kCounter(%table:8, +, NonRecursive), kVecAppend(%table:8, kNetAddition)}
    spine: —
    args: table=%table:8 message=demand__neighborhood_bf/1
op.1 kIngestFold sign=+ ctx=eager stratum=0
    effects: {kCounter(%table:11, +, NonRecursive)}
    spine: —
    args: table=%table:11 message=add_edge/2
op.3 kSubgraphInstantiate sign=+ ctx=seed stratum=1 i#0 store=I#0
    demand=%table:8 pub=%table:4 input=%table:11 pub_row=[ik:Start,row:Node] nested=<Node>
    reads: Present(%table:11)
    effects: {kVecDrain(%table:8, kNetAddition), kInstanceDemand(%table:8), kInstanceRebuild(%table:4, +), kInstanceEmit(%table:4), kInstanceOld(%table:4), kCounter(%table:4, +, NonRecursive)}
    spine: kAccess(%table:11, section-walk) -> kFold(%table:4, +, NonRecursive)
    args: demand=%table:8 pub=%table:4 input=%table:11 store=I#0
op.0 kCommitSweep sign=· ctx=seed band=9 flavor=monotone publish_target=false
    effects: {kFlagWrite(%table:8)}
    args: table=%table:8
op.4 kInstanceSeal sign=· ctx=seed band=11 i#0 store=I#0
    effects: {kInstanceSealSwap(%table:4, sign=0)}
    args: pub=%table:4 store=I#0

rounds:

deps:
  op.1 -> op.3 RAW epoch
  op.2 -> op.0 WAW epoch

census: kCrossover=0 kProductArm=0 kSeedFold=0 kFixpointFire=0 kChainFold=0 kClaimDrain=0 kRetire=0 kRederive=0 kFrontierFilter=0 kCommitSweep=1 kNegateGate=0 kPivotAssemble=0 kIngestFold=2 kGroupUpdate=0 kStateSeal=0 kSubgraphInstantiate=1 kInstanceDeath=0 kInstanceSeal=1
```

Line-by-line derivation deltas (everything else inherits the author's
derivations, which the critic verified):
- **op labels (⟨PIN⟩)**: mint order = commit-sweep loop (:1660) before the
  ingest-fold loop (:1840) → sweep=op.0; add_edge/demand keep flat message
  order relative (op.1/op.2); instance ops appended after (op.3/op.4 —
  mint position unpinned, §C-10). da confirms sweeps-before-ingest
  (op.12/13 < op.14/15/16).
- **LINE order** (band key, DeltaRel.cpp:3445-3477): lead-0 ingest folds
  first, table_id tie-break (%table:8 < %table:11 ⇒ demand fold first —
  the same rule that produced the flat op.1-before-op.0); lead-1
  instantiate (stratum 1); lead-2 sweep (band 9); kInstanceSeal trails
  (design band 11 > kStateSeal's 10; needs its own key_of case at D1.b).
- **op.2 (demand ingest)** gains exactly `kVecAppend(%table:8,
  kNetAddition)` after the counter — the MakeMonotoneIngestFold shape
  (:832-840) once MonotoneIngestRoleDR sees the recognized boundary as a
  cut successor; byte-identical in form to avg op.52 / da op.14.
- **op.1 (edge ingest)** is effect-UNCHANGED under the pinned demand-only
  provisioning (§C-2 carries the mechanism-natural alternative in which it
  too gains an append).
- **op.3 effects** = design §A.2.3 push order, always-block + !DIFF block
  (the order authority per §A.2-F3); the kFlagRead(Present, %table:11)
  renders on `reads:` and is filtered from `effects:` (grammar §2.5.a).
- **op.0 (sweep)** = the DeltaRel.cpp:1679-1687 monotone mint verbatim
  (kFlagWrite only; header form = avg op.51 byte-shape).
- **deps** = the two code-forced by_flag edges (§A.2-F2, X-DS-2), sorted
  ascending (from, to, kind) per p14. NO by_vec edge exists (X-DS-1); NO
  edge touches the seal (kInstanceSealSwap's hazard enrollment pending —
  kStateSeal mints none, the mold holds).
- **census_total** = 5 = ops.size() — the Format.cpp:796 completeness
  abort stays satisfied.

======================================================================
§C. RESIDUALS / OPEN PINS FOR THE JUDGE ROUND (numbered)
======================================================================

C-1 [RATIFY] **Alternative A** — nested `.df` == flat `.df` byte-for-byte;
  FillDataModel stays knob-blind; every "%table:15 vanishes" claim scoped
  to `.ir`/`.h`; one `.df` golden serves both eqgate arms. (Precedent:
  force_agg_tables, Build.cpp:167-179. Fallback Alt-B = exactly one
  changed line, tuple.3 only.)

C-2 [DECIDE] **OD-7 provisioning blast radius**: the block pins DEMAND-ONLY
  provisioning (design text: "the monotone demand table"), but the LANDED
  mechanism (cut-successor test drives stop-AND-provision at the same
  Build.cpp:948-961 site, quantified over member views by
  MonotoneIngestRoleDR :147-160) would naturally ALSO provision the EDGE
  table %table:11 (tuple.2's successor join.6 is inside the recognized
  boundary). If mechanism-natural is adopted: op.1 gains
  `kVecAppend(%table:11, kNetAddition)`, a second monotone sweep mints,
  census kCommitSweep=2, +1 WAW edge, and the runtime does dead frontier
  work per edge-add. Demand-only requires splitting stop-vs-provision (or
  a demand-keyed role arm) COHERENTLY on both the walk and DR sides (the
  §7d role/walk cross-check aborts on divergence).

C-3 [DECIDE] **The seal's commit-sweep carrier under R-MONO**: design
  §A.3.4 hooks emit_inst_seal at the EmitCommitSweep exits, but the
  monotone pub table %table:4 has NO delta-vec entry ⇒ NO commit sweep of
  its own. Options: (i) ride the demand table's monotone sweep (exists per
  X-DS-2); (ii) mint a pub-table Seal sweep (census kCommitSweep=2); (iii)
  lower the seal directly from kInstanceSeal in LowerSubgraphInstance's
  dispatch, not via EmitCommitSweep. The DR-IR op.4 exists regardless; this
  is its LOWERING carrier.

C-4 [PIN] **kInstanceDemand hazard enrollment**: recommended
  frozen/no-hazard (the kInIReadFrozen precedent, DeltaRel.cpp:3303-3304
  "frozen: no hazard") — else op.2→op.3 RAW and op.3→op.0 WAR edges
  appear. The D1.b linearizer switch case must pin this explicitly.

C-5 [DECIDE, not recommended] **First-class DRVec for the OD-7 frontier**:
  the block follows the precedent (NO DRVec; LOWER-time TableDeltaVector;
  effect-only rendering). If the judge wants the frontier modeled as a
  DRVec (visibility/validator appetite), the vec line + def/use +
  op.2→op.3 by_vec RAW return, and MintTableVec needs a monotone arm —
  a REAL divergence from the landed shape, priced accordingly.

C-6 [PIN at D1.b] **`instances:` section grammar**: placement (first
  section, before ops — the author's choice), lead keyword, DRInstance
  field order (forcing, key, pub, input, store, key_cols, row_cols). The
  largest new-grammar invention; needs its Format.cpp spelling row +
  spec p-rule (p15+).

C-7 [PIN at D1.b] **`i#<n>` vs `store=I#<n>`**: render both (1:1:1 on the
  single-adornment slice) or collapse to one id space. GROUP_UPDATE
  carries ONE `sc#N`.

C-8 [PIN at D1.b] **The five kInstance* EffKind arg shapes + backing
  DREffect fields** (the EF-1 class: kState* reuse
  counter_table/value_table; the kInstance* peers inherit the question).
  Rendered here as `<Kind>(%table:N[, sign])` — implementer confirms at
  the BuildSubgraphInstanceOps enrollment site.

C-9 [PIN at first emission] **The instantiate spine's Lowering**:
  `section-walk` rendered (keyed rescan on the instance-key column);
  full-scan possible if the rescan is emitted unkeyed. Interacts with
  V-ALPHA arm A only at ACCESS/GATE sites against the store, not here.

C-10 [PIN at first emission] **BuildSubgraphInstanceOps mint position** —
  determines op.3/op.4 labels; pure ⟨PIN⟩, no structural impact.

C-11 [RATIFY] **No `.df` annotation rendering in D1** — the recognition
  metadata partition: NOTHING on `.df`, the DRInstance descriptor on
  `.deltarel` (§A.7's scope). Defer any `.df`-visible recognition surface
  to a later epoch.

C-12 [PIN at D1.b] **The instantiate second header line**
  (`demand=/pub=/input=/pub_row=[ik:..,row:..]/nested=<..>`): §A.7-example-
  sourced but grammar-less; needs the exact p-rule (field order, bracket
  and tag spellings — `ik:`/`row:` are the §A.4 BindingSource renders).

Cross-cutting (standing, not judge-blocking): the FLAT blocks are
byte-blessable NOW; the NESTED deltarel block is UNBLESSABLE pre-emission
(illustrative ids + C-2/C-3 census forks); the nested `.df` is blessable
the moment C-1 is ratified (it is the flat golden). G5 config-invariance
(3-run + debug==release) must be re-audited on the nested dumps before any
bless — trivially inherited for the `.df` under C-1.

======================================================================
§D. VERDICT
======================================================================

**READY-FOR-JUDGE.** Grounds: (i) both FLAT surfaces are byte-pinned to
real dumps and machine-verified by two independent critics — zero
findings; (ii) the POST-D1b census churn is pin-exact and matches the
design's pre-registered single-line bless; (iii) the nested `.df` question
reduces to ONE ratification (C-1) with a code-verified precedent and a
one-line falsifiable fallback; (iv) the nested `.deltarel` block is now
MECHANISM-TRACED end-to-end — every effect, edge, and census count derived
from landed code paths (MakeMonotoneIngestFold, the commit-sweep mint,
by_flag hazards, the band key) or from binding design clauses, with the
two prior HIGH byte-errors (vec line, deps) corrected and the two
consolidator discoveries (no-DRVec precedent; the forced monotone sweep)
folded; (v) every remaining unknown is a NUMBERED, decision-shaped
residual (C-1..C-12) rather than a silent guess. NEEDS-REWORK would be
warranted only if a residual were structural (op set / regime split /
census shape unknown) — none is: C-2 and C-3 are the only census-affecting
forks and both are enumerated with exact byte deltas.
