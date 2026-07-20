======================================================================
COMMITTED AT THE §19 CHECKPOINT (2026-07-20, tip 1aaca896). This is the
judge round's PINNED CONTRACT, verbatim from the session scratchpad
(judge-d1/d1-pinned.md). It is the TOP of the D1 precedence stack: where
it contradicts d1-design-consolidated.md or d1-desired-states.md, THIS
FILE WINS. Implementation starts ONLY after the owner rules on §4's
OD-1..OD-10. Sibling references resolve as in d1-design-consolidated.md's
banner.
*** RATIFIED 2026-07-20 (KeyedInstances.md §19(I) is the binding ruling
*** record): ALL owner decisions ruled per the recommendations, with the
*** §19(H) Rel-direction deltas — notably OD-4 = MECHANISM-NATURAL
*** provisioning (this file's §4 REC of demand-only is SUPERSEDED; the
*** nested census becomes kCommitSweep=2 per d1-desired-states §C-2) and
*** the new OD-11 = §9-Rel epoch BEFORE D3.a. IMPLEMENTATION UNBLOCKED.
======================================================================

# D1 PINNED DESIGN CONTRACT — keyed-instances nested lowering, stage D1/D2

Consolidated from judge-correctness / judge-precedent / judge-process,
adjudicated at code. Repo tip **1aaca896** (keyed-instances; design-only —
NO D1/D2 code exists; implementation starts ONLY after owner ratification of
§4's owner list). Package under judgment:
fleet-d1/d1-design-consolidated.md (§0/§A/§B/§C/§D) +
fleet-ds/ds-consolidated.md (§A/§B/§C). All line refs re-verified at tip by
this consolidator; the one executable claim (permcheck) was RE-EXECUTED, not
re-read.

**VERDICT: GO-WITH-AMENDMENTS — unanimous (3/3 judges), upheld.** The
architecture (per-forcing recognition, three-op family, regime split,
band-key + V-INST-ORDER ordering, InstanceStore transpose, Alt-A dump
partition, staged R-MONO→R-DIFF) survives all three lenses with no
re-architecture. Every judge amendment survived my re-verification and is
folded below as a HARD PIN (§2) or an owner decision (§4). Three findings
are OVERRULED in whole or part (§5) — including two upstream critics who
rubber-stamped a referee that provably rejects the change it was pinned to
referee (P-1, executed twice: by the process judge and by me,
`permcheck.py ... exit=1`).

The design consolidated record + the ds dump blocks become BINDING as
amended here. Where this contract contradicts d1-design-consolidated.md or
ds-consolidated.md, THIS CONTRACT WINS (specifically: §A.7/decision-9's
permcheck clause, §A.3.4's seal wiring, §A.2.1's "compile-time-absent",
§A.2.3's "always" drain framing, §B's "OD-6" labels, and §B-D3.a's churn
line are each superseded below).

======================================================================
§1. CONSOLIDATED VERDICT — what was judged and what held
======================================================================

- correctness (silent-miscompile lens): GO-WITH-AMENDMENTS, A1–A8. All four
  wrong-answer-without-abort paths are REAL (re-verified: the §7d loop
  filters `op.kind != kIngestFold` at DeltaRel.cpp:3043-3044 so instantiate
  drains are unchecked; op_table_id's ?: chain has NO pub_table branch,
  :3400-3406; the demand body-walk rejects MAP/NEGATE/AGG at
  Demand.cpp:598-607 so V-ALPHA's functor blind spot is unreachable
  in-slice but decoupled; V-INST-EMITTED's design text enrolls only
  instantiate/death, seal excluded). All closable by pins; no redesign.
- precedent (house-law lens): GO-WITH-AMENDMENTS, A1–A9. The one mandate
  contradiction is real and verified: KeyedInstances.md:1103-1104/:1834-1835
  literally mandates minus-before-plus "PINNED by an explicit DR-IR edge";
  the design substitutes band-key + V-INST-ORDER because the explicit edge
  is provably circular (emit_waw key-forces its own orientation,
  DeltaRel.cpp:3540-3556). The substitution is sound and STRONGER, but the
  owner must ratify the wording change (§4 OD-2). §13's CANDIDATE-A test
  (KeyedInstances.md:1398-1403 "PLUMBING absence stated as a feature gap")
  genuinely cuts against fence (ii) — owner call (§4 OD-3).
- process (gates/predictions lens): GO-WITH-AMENDMENTS, A-1..A-5. The
  BLOCKER is proven by execution: permcheck.py treats the census line as a
  structural boundary (boundary_re `^\S.*:\s*$|^\S.*:\s`, permcheck.py:62;
  its only permutable tokens are `[+-](...)` deltas) and FAILS the D1.b
  three-counter append — "segment 3: boundary line differs", exit=1,
  re-executed by this consolidator on the judge's old/new pair. Decision #9's
  named referee cannot referee the change. Corrected in HP-14.

======================================================================
§2. HARD PINS (implementation-binding; each with its evidence)
======================================================================

Ordering note: HP-1..HP-8 are correctness pins, HP-9..HP-13 precedent pins,
HP-14..HP-18 process pins. Sources given as (judge:finding).

--- THE VALIDATOR / SILENT-MISCOMPILE PINS ---

HP-1 (corr:F-C1 + prec:F-14; resolves ds C-3) — **The seal's lowering
  carrier is kInstanceSeal ITSELF (ds C-3 option iii): LowerSubgraphInstance's
  dispatch lowers the seal (the pointer swap emitted at the commit-sweep-tail
  position), NOT via EmitCommitSweep's exits. AND V-INST-EMITTED enrolls all
  THREE kinds — kSubgraphInstantiate, kInstanceDeath, kInstanceSeal — so a
  minted-but-unlowered seal ABORTS.** Design §A.3.4's three-exit
  emit_inst_seal wiring (store-F-4) is SUPERSEDED; whether a
  SealInstanceStoreId region attribute survives is the implementer's, but
  the carrier MUST be unconditional on any pub-table sweep existing.
  Evidence: emit_seal is gated on SealStateCellId and called only at the
  three EmitCommitSweep exits (Database.cpp:2326-2330, :2378/:2418/:2445 —
  verified); under R-MONO the monotone pub table mints no sweep of its own
  (queue/sweep mint gates on TableIsDifferential, DeltaRel.cpp:879-880);
  V-INST-EMITTED as designed (§A.5c verbatim) excludes the seal — census
  kInstanceSeal=1 + V-INST-PAIR green while the swap is silently absent =
  stale frozen on every second epoch, the exact F17/F18 class. Both judges
  independently converge on option (iii). (Severity re-graded in §5.2 —
  the pin stands at full strength.)

HP-2 (corr:F-C9) — **V-INST-DRAIN: extend the §7d cross-check (or add a
  sibling loop) so every kSubgraphInstantiate op's
  kVecDrain(demand_table, kNetAddition) is asserted against a provisioned
  `table_delta_vecs[demand_table]` kNetAdditions entry — always-on
  ValidatorFail. §A.2.3's "always" drain is REFRAMED as a compile-asserted
  precondition, never an unconditional effect.** Evidence: the §7d loop
  covers ONLY ingest folds (`if (op.kind != DROpKind::kIngestFold ...)
  continue;`, DeltaRel.cpp:3043-3044, verified); ResolveVecIdx returns ~0u
  gracefully for an un-minted (table, role) (:3110-3127) and add_vec_access
  skips it — so an un-provisioned OD-7 births ZERO keys with no abort and
  an empty pub table as the "answer." The gate would catch it once, on one
  witness; the validator catches it on every compile.

HP-3 (corr:F-C2 + prec:A1 sub-pins) — **The minus-before-plus realization:
  BOTH instance ops set `table_op_table = pub_table` (reusing op_table_id's
  EXISTING first ?: arm — no new pub_table field threaded), with
  death.table_op_sign=-1 / instantiate.table_op_sign=+1. V-INST-ORDER
  groups death/instantiate by their SHARED `instance_store_id` (never
  table_id, never forcing index), is ALWAYS-ON (survives NDEBUG), and D1.b
  ships a negative-space test: a deliberately mis-minted plus-before-minus
  must trip it.** Evidence: op_table_id = `table_op_table ? ... :
  product_table : agg_table : negate_table : ingest_table : fire_table`
  (DeltaRel.cpp:3400-3406 — NO pub branch today; "gains op.pub_table in its
  ?: chain" was ambiguous); key_less reaches the sign compare only on equal
  table_id (:3470-3477, "− before +" verified). If the two ops key on
  different fields the tie-break NEVER fires and V-INST-ORDER is the sole
  guarantor — hence pinning both halves.

HP-4 (corr:F-C3) — **V-ALPHA arm B is pub_row-COLUMN-total: every published
  column (ik: AND row: positions) has a modeled BindingSource, and no row:
  column's source transitively traces to an instance-key value. PLUS a
  recognizer refusal: if the demanded body contains ANY MAP/NEGATE/AGG
  view, RecognizedSubgraph construction aborts until V-ALPHA is extended.**
  Evidence: the in-slice safety lives in a DIFFERENT pass — the demand
  body-walk admits only TUPLE/JOIN/SELECT-from-IO and rejects everything
  else (Demand.cpp:584-607, verified: NEGATE/AGG reject at :601-604, the
  else-reject at :606-607). A D3+ body relaxation would reopen the
  α-through-functor-output path with V-ALPHA (context-col-total as
  designed) never firing. The refusal couples the guard to the guarantee.

HP-5 (corr:F-C4) — **The recognizer-correctness claim is DOWNGRADED:
  "closure-shape correctness of the FLAT answer rests on the oracle;
  demand-SCOPING correctness rests on the blessed .stdout + the .ir
  structural gate ONLY." AND the D2.c witness must expose
  over-materialization: the neighborhood driver asserts the answer is
  EXACTLY neighborhood(Start) — the witness graph must contain edges
  OUTSIDE the demanded neighborhood so an over-materialized nested arm
  diverges from flat.** Evidence: the oracle computes the FULL relation
  (Oracle never runs demand); a full-closure oracle categorically cannot
  referee partial materialization. demand_tc_witness (full closure) cannot
  expose it; the bf-keyed neighborhood witness can.

HP-6 (corr:F-C6; folds into ds C-12) — **The D1.b grammar p-rule for the
  instance op MUST pin the band-(b) partition semantics: kInstanceEmit is
  the (F,T)-GATED publish (fires only on current∖frozen) and kInstanceOld
  is the (T,F)-GATED retract (provably EMPTY under !DIFF).** Evidence: the
  §B.4 nested block's bytes (two reads + one +counter) are satisfiable by a
  publish-ALL-current lowering that double-publishes on every rebuild epoch
  — the dump alone does not force the partition. The p-rule (C-12) is
  where the discipline becomes contract.

HP-7 (corr:F-C5) — **Pin the R-MONO monotonicity invariant: input monotone
  ⇒ instance content monotone-growing ⇒ frozen ⊆ current, so the (T,F)
  drop set is PROVABLY EMPTY under R-MONO. Add V-INST-FRESH's dual: a
  debug-severity `frozen ⊆ current` assert at seal under R-MONO — a (T,F)
  that "cannot exist" trips a LOUD assert instead of attempting a retract
  on a monotone pub table.**

HP-8 (corr:F-C7/F-C11; ratifies ds C-4) — **kInstanceDemand enrolls
  frozen/NO-hazard (the kInIReadFrozen precedent: `case
  EffKind::kInIReadFrozen: break; // frozen: no hazard`,
  DeltaRel.cpp:3302-3303 — verified), pinned as the explicit D1.b
  linearizer switch case AND as a V-INST-EFFECT per-kind expectation (a
  hazard-bearing kInstanceDemand aborts). AND: D3.a CANNOT land without
  the 3-way V-INST-PAIR arm implemented + its negative-space test (an
  instantiate+seal with no death under R-DIFF must abort).**

--- THE HOUSE-LAW PINS ---

HP-9 (prec:A2) — **(F)-law walk-order pin: BOTH the ApplyDemandTransform
  stamp-site loop AND every emission/dump-time consumption are driven by
  the DefList/det_seq walk order — NEVER by guard_annotations vector order,
  NEVER by unordered_map iteration. D1.a and D2.b checklists gain the
  G6-class re-grep: no std::sort over / no range-for over
  guard_annotations or guard_annotation_of at any emission-or-dump site.**
  Evidence: UniqueId() is a reinterpret_cast pointer (Node.h:31-33);
  §A.1.2's prose leaves the vector trivially iterable and the stamp loop's
  own order unverified — this closes the one door left ajar.

HP-10 (prec:A3) — **The three new DROpKinds MUST enter Format.cpp's
  kAllKinds enum-order array at D1.b, pre-registered as the belt behind
  the census-line churn.** Evidence: kAllKinds renders every kind
  unconditionally and `census_total != flow.ops.size()` is an always-on
  fprintf+abort (Format.cpp:779-796, verified) — a missed entry aborts on
  ANY flow containing an instance op.

HP-11 (prec:A4; sequences ds C-8) — **kInstanceEmit and kInstanceOld must
  clear the E-50 seam bar AT D1.b — a DISTINCT hazard target or a census
  distinction (the bar kInstanceDemand clears) — or COLLAPSE to the landed
  kStateEmit/kStateOld (DeltaRel.h:82-84, verified live). This resolves
  BEFORE the spelling rows land, so the dump grammar is never minted
  around members that then collapse. ds C-8's arg-shape pins are
  sequenced AFTER this membership call. HP-6's (F,T)/(T,F) pin applies
  under either naming.** kInstanceRebuild (store-current write hazard) and
  kInstanceDemand (census-load-bearing) are JUSTIFIED as-is;
  kInstanceSealSwap is re-examined under HP-1's self-lowered carrier.

HP-12 (prec:A5) — **D2.b must ENUMERATE every α-consumer site on the
  neighborhood witness and show each is kInstanceKeySlot-tagged and reached
  by V-ALPHA arm A or B — a "no uncovered α-consumer" acceptance line in
  P-D2b.3, checked against the -deltarel binding-source tags.** Evidence:
  §A.4's default-all-kRowSlot short-circuit means a MISSED tag is the
  validator's own silent no-op path — §12 DIRECTIVE 2's exact target.

HP-13 (prec:A7; ratifies ds C-1) — **Alt-A is RATIFIED (nested .df ==
  flat .df byte-for-byte; FillDataModel knob-blind; every "%table:15
  vanishes" claim scoped to .ir/.h; one .df golden serves both eqgate
  arms). Precedent verified: force_agg_tables GetOrCreates the
  chain-breaker's tables unconditionally (Build.cpp:167-179). THE BOUND
  DUTY: the IR-observability directive is discharged ONLY by (a) the
  DRInstance block, kInstance* spellings, instance p-rule, and ik:/row:
  binding-source tags landing as REAL Format.cpp rows at D1.b with the
  loud-abort fallback (T2b law), and (b) a D2.b end-to-end review of the
  REAL nested .deltarel against this contract BEFORE any nested golden is
  blessed (G5 config-invariance on all four nested surfaces).**

--- THE PROCESS / GATES PINS ---

HP-14 (proc:A-1, BLOCKER — EXECUTED-CONFIRMED) — **Strike
  "permcheck-refereed" for the D1.b census-line bless EVERYWHERE (§A.7,
  P-D1b.2, decision #9, ds §B.3). The bless is a DIRECT diff bless with a
  three-point referee: (i) `diff` shows exactly one changed line; (ii) the
  delta is exactly the three enum-tail counters in kAllKinds order
  (Format.cpp:779-789, enum-order, no zero-suppression); (iii) the
  census-completeness abort (Format.cpp:796) is green. permcheck REMAINS
  the referee for genuine published-delta STDOUT permutations — its actual
  domain — and is NOT retired.** Evidence: executed twice independently —
  permcheck's boundary_re (permcheck.py:62) classifies the census line as
  a structural boundary that must be byte-identical; the +3-counter target
  fails with "segment 3: boundary line differs", exit=1. The bless itself
  stands; only its claimed justification method was false.

HP-15 (proc:A-2) — **D3.a's churn is re-registered as a surface×mode
  MATRIX, pre-registered loudly: (i) demand_neighborhood_witness .stdout +
  .oracle.stdout + .monotone.stdout ALL re-bless (the DEATH batch adds
  output — GT-2: the monotone→differential flip flows through tables
  8/15/4); (ii) IF the witness .irgold landed at D2.c, ALL its listed
  df/deltarel/ir/h surfaces re-bless on the flip; (iii) demand_tc_witness
  byte-untouched (plain -demand). "its four golden modes re-bless
  together" is struck as under-registered.** (The .irgold timing itself is
  owner fork OD-10.)

HP-16 (proc:A-3) — **D2.a's checklist gains a one-shot ON-build gate:
  configure/build with `-DDRLOJEKYLL_BENCH_COUNTERS` and confirm (a) the
  corpus COMPILES — proves InstanceStore.h uses only enumerated X-macro
  field names (HYDE_RT_BENCH_COUNTER_FIELDS, BenchCounters.h:24-52;
  on-build expansion `gBenchCounters.field += 1u` :66-71, verified) — and
  (b) SUITE: PASS holds. A genuinely-new counter name is a pre-registered
  BenchCounters.h X-macro edit, never a silent assumption.**

HP-17 (proc:A-4) — **§A.2.1's "compile-time-absent under R-MONO" is
  corrected to "mint-predicate-false (runtime-gated on
  TableIsDifferential); the enum member, its stratum/key_of/effect cases,
  and V-INST-ORDER + V-INST-PAIR's R-DIFF arm compile and ship inert at
  D1.b/D2.b." §D gains the LOUD residual: the DR-IR kInstanceDeath op and
  V-INST-ORDER have NO executing coverage until D3.a (the D2.a DrTest
  covers only the runtime store's Recycle/death half). Optional cheap
  de-risk (adopted as a D1.b checklist line): assert V-INST-ORDER runs
  vacuously-green (0 death ops) corpus-wide.** This is legitimate
  semantic-predicate staging, NOT the env-gated-scaffolding anti-pattern.

HP-18 (proc:A-5) — **The stale "OD-6" labels at design §B (D2.b
  P-D2b.1 and the D2.c file list) are bound to DECISION #8 (fence
  witnesses land with D2.c, suite 169→170→173 — arithmetic verified: 169
  cases at tip).**

--- CARRIED NOTES (not pins) ---
N-1 (prec:A9): revisit InstanceStore's dropped working_count when R-DIFF
  lands — a mid-batch dip below zero could make NumRows()-as-occupancy a
  lie. D3 concern; recorded so it is not lost.
N-2: witness-deltarel-target.md §2.6 still needs its SUPERSEDED-IN-PART
  banner when this record is adopted (design §D residual, carried).
N-3 (positive adjudications, recorded so nobody re-derives them):
  census independence honesty (corr F-C8), regime-discriminant stability
  (corr F-C10), E-62/G5/G6 gate placement (proc P-8), suite arithmetic +
  alternation site (proc P-6), D2.b single-diff justification (proc P-7),
  (F)-multiset census legality (prec F-1), PassPolicy orthogonality =
  exact P1 df.demand precedent (prec F-5, PassPolicy.cpp:38-43 verified),
  X-DS-1/X-DS-2 code-correct (prec F-16/F-17).

======================================================================
§3. CORRECTED DIFF SEQUENCE + PREDICTION SET
======================================================================

The §B sequence D1.a → D1.b → D2.a → D2.b → D2.c → D3.a STANDS unchanged
in structure (proc P-7 examined and endorsed the D2.b bundle and the
D1.b/D2.b seam). Amendments per diff:

D1.a — unchanged, PLUS: HP-9's re-grep line in the checklist (no
  sort/range-for over guard_annotations/guard_annotation_of); the stamp
  loop's DefList/det_seq order is an explicit review item.
  Predictions P-D1a.1..4 unchanged.

D1.b — PLUS: HP-10 (kAllKinds entries pre-registered); HP-11 (emit/old
  justify-or-collapse BEFORE spelling rows; C-6/C-7/C-8/C-12 pins land
  after membership resolves); HP-6 (the p-rule carries the (F,T)/(T,F)
  partition); HP-8 (kInstanceDemand no-hazard switch case + V-INST-EFFECT
  expectation); HP-3 (V-INST-ORDER negative-space test); HP-17 (vacuous-
  green corpus line). PREDICTION P-D1b.2 AMENDED per HP-14: the census
  single-line bless is diff+census-abort refereed — every
  "permcheck-refereed" clause struck.

D2.a — PLUS: HP-16 (the ON-build bench-counters compile+suite gate).
  P-D2a.1's "off-build byte-neutral" stands (verified: off-build macros
  expand to `((void)0)`, BenchCounters.h:74-77).

D2.b — PLUS: HP-1 (seal lowered from the instance dispatch; V-INST-EMITTED
  enrolls all three kinds; Database.cpp three-exit emit_inst_seal wiring
  dropped); HP-2 (V-INST-DRAIN); HP-12 (the α-consumer enumeration —
  P-D2b.3 gains the "no uncovered α-consumer" acceptance line and a
  "V-INST-DRAIN green" line); HP-13(b) (the end-to-end real-dump review
  before any nested bless). Fence (ii) code is CONTINGENT on OD-3; fences
  (i)/(iii) proceed regardless. OD-4 (provisioning blast radius) must be
  decided before the nested .deltarel census is blessable.

D2.c — PLUS: HP-5 (the witness graph contains out-of-neighborhood edges;
  the driver asserts the answer is exactly neighborhood(Start) — new
  acceptance line beside P-D2c.2). HP-18's label fix. Fence witnesses per
  decision #8 (recommended: land here, →173).

D3.a — PLUS: HP-15 (the full surface×mode re-bless matrix, pre-registered);
  HP-8's landing gate (3-way V-INST-PAIR + negative test); HP-17's
  residual retires here (death op + V-INST-ORDER get executing coverage).

The ds dump blocks: §B.1/§B.2/§B.3 stand as pinned (B.2 contingent only on
OD-veto of C-1, now ratified per HP-13). §B.4 (nested .deltarel) stands as
the certifying contract for op set / effect multisets / line order / census
/ two dep edges, with THREE amendments: its census shape is contingent on
OD-4 (kCommitSweep=1 pinned only under demand-only provisioning); its
kInstanceEmit/kInstanceOld spellings are contingent on HP-11's membership
call; its p-rules gain HP-6's partition pin. It remains UNBLESSABLE
pre-emission (illustrative ids), per its own banner.

======================================================================
§4. THE FINAL OWNER-DECISION LIST (merged §C 1-10 + ds C-1..C-12 +
    judge-minted; DEDUPED)
======================================================================

Format: statement / fleet recommendation / dissent / BLOCKS.

--- REQUIRES OWNER RATIFICATION (8 items — the brief) ---

OD-1 (=§C-1) RETRACTION STAGING. D2 ships R-MONO-a (birth/rebuild only;
  death op mint-predicate-false); D3.a ships R-DIFF via new opt-in
  `-demand-retract` (+@differential fabrication + unask ABI, carried in
  the witness .drflags). REC: staged, as consolidated. DISSENT: eqgate
  lane — ship differential THIS epoch (cost: witness flips fully
  differential now; D2 scope grows). BLOCKS: D3.a only; D2 proceeds on
  the recommendation.

OD-2 (=§C-5 + prec:A1) §18(B)(4) MECHANISM SUBSTITUTION. The mandate's
  literal text ("minus-before-plus ... PINNED by an explicit DR-IR edge",
  KeyedInstances.md:1103-1104/:1834-1835) is REPLACED by same-table_id
  band key (table_op_sign ∓1, per HP-3) + always-on V-INST-ORDER, because
  the mandated explicit edge is provably CIRCULAR (emit_waw key-forces
  edge direction, DeltaRel.cpp:3540-3556; V-LINEAR checks against the same
  key that oriented it — an explicit edge can never catch a wrong key).
  REC: ratify — the substitution fulfils the intent and is strictly
  stronger. NO dissent (all three judges + both consolidators). BLOCKS:
  D1.b (the op-family mint keys and V-INST-ORDER land there).

OD-3 (=§C-7 + prec:A6) FENCE (ii) SCOPE — the §13 judgment call. Is
  mid-stream monotone-input-add under standing demand a CORRECTNESS fence
  (keep: hazard structural, not differential — the design's reading) or a
  FRONTIER-PROVISIONING GAP that OD-7 should plumb (drop the fence — §13
  CANDIDATE-A: "PLUMBING absence stated as a feature gap",
  KeyedInstances.md:1398-1403)? The design's own mitigation ("the witness
  driver uses the demand-flap rebuild so a legal program never trips it")
  is an admission the straight path is blocked. REC: none imposed — this
  is the exact judgment call §13 was written to force; the precedent judge
  reads §13 as making "keep + flap driver" the LESS likely answer.
  BLOCKS: fence-(ii) code in D2.b + witness demand_midstream_edge_1 in
  D2.c (and the 173 count). Fences (i)/(iii) + everything else proceed.

OD-4 (=ds C-2) OD-7 PROVISIONING BLAST RADIUS. Demand-only provisioning
  (the pinned §B.4 shape: census kCommitSweep=1) vs mechanism-natural
  (the landed cut-successor test also provisions the EDGE table %table:11:
  census kCommitSweep=2, +1 WAW edge, dead per-edge-add frontier work at
  runtime). Demand-only requires splitting stop-vs-provision COHERENTLY on
  both the walk and DR sides (the §7d role/walk cross-check aborts on
  divergence). REC: demand-only (matches the pinned nested block and does
  no dead runtime work), priced as the larger compiler diff. DISSENT:
  none recorded; mechanism-natural noted as the smaller-diff option.
  BLOCKS: D2.b's provisioning site + blessing the nested .deltarel census.

OD-5 (=ds C-3, RESOLVED-BY-CONSOLIDATION — veto point) SEAL CARRIER.
  Pinned to option (iii) per HP-1 (self-lowered from kInstanceSeal;
  V-INST-EMITTED enrolls the seal; §A.3.4's three-exit wiring
  superseded). Both the correctness and precedent judges independently
  recommend (iii); correctness classifies leaving C-3 [DECIDE] as its one
  BLOCKER. REC: ratify the consolidation. BLOCKS: D2.b codegen wiring.

OD-6 (=§C-4) THE OP FAMILY. Three DROpKinds with publish-on-instantiate +
  the instance EffKind family — CONDITIONED on HP-11 (kInstanceEmit/
  kInstanceOld justify-or-collapse vs kStateEmit/kStateOld at D1.b, before
  spelling rows). REC: confirm the three-op family; leave EffKind
  membership to the HP-11 gate. BLOCKS: D1.b enum landing.

OD-8 (=§C-8) FENCE WITNESSES LAND WITH D2.c (suite →173; each fence's
  red witness registered in the runall.sh:296 alternation + CLAUDE.md).
  REC: yes — a fence with no red witness is untested. (Fence-(ii)'s
  witness contingent on OD-3.) BLOCKS: D2.c case count only.

OD-10 (=§C-10, AMENDED by proc:A-2) WITNESS .irgold TIMING. (a) D2.c
  follow-on (design's original) + a pre-registered FULL surface re-bless
  at D3.a (the monotone→differential flip churns every surface, GT-2); or
  (b) DEFER to post-D3.a so the irgold is blessed ONCE against the final
  differential shape. REC: (b) — cheap risk-retire (process judge).
  DISSENT: precedent judge ratified (a) as-written (adjudicated §5.3).
  BLOCKS: nothing either way.

--- RATIFIED BY THE JUDGE ROUND (veto-only; owner may override, no action
    needed to proceed) ---

OD-R1 (=§C-2) Recognition unit = per-DemandForcing (X-9);
  RecognizedSubgraphs 1:1 with the landed BindingPattern-keyed registry
  (DemandForcings() verified at Query.h:995); is_instance_key marks
  recursive-subgoal (D3) boundaries only.
OD-R2 (=§C-3) Knob spellings `-demand-instance` / `-demand-retract`, both
  OFF the PassPolicy registry — the exact P1 df.demand precedent
  (PassPolicy.cpp:38-43 verified). Names remain the owner's to bikeshed.
OD-R3 (=§C-6) OD-7 net-additions provisioning on the demand table —
  required for the birth arm; GROUP_UPDATE cut-successor precedent.
  (Scope fork = OD-4; the provisioning itself is ratified.)
OD-R4 (=§C-9, method corrected by HP-14) The D1.b census single-line
  bless on demand_tc_witness.deltarel.opt.golden — ratified, refereed by
  diff + the Format.cpp:796 abort, NOT permcheck.
OD-R5 (=ds C-1) Alt-A nested .df == flat .df — ratified per HP-13 with
  the bound observability duty.
OD-R6 (=ds C-4) kInstanceDemand frozen/no-hazard — ratified per HP-8.
OD-R7 (=ds C-5) NO first-class DRVec for the OD-7 frontier (LOWER-time
  TableDeltaVector, effect-only rendering) — ratified; the DRVec option
  stays a priced non-default.
OD-R8 (=ds C-11) No .df annotation rendering in D1 (recognition-metadata
  partition: nothing on .df, DRInstance on .deltarel) — ratified.
OD-R9 (=ds C-9/C-10) Spine Lowering `section-walk` as illustrative
  default; instance-op mint position — both ⟨PIN⟩-at-first-emission.

--- IMPLEMENTER PINS AT D1.b (not owner-level; sequenced by HP-11) ---

OD-I1 (=ds C-6) `instances:` section grammar (placement, keyword,
  DRInstance field order) — real Format.cpp rows + spec p-rule p15+,
  loud-abort fallback (HP-13 duty).
OD-I2 (=ds C-7) `i#<n>` vs `store=I#<n>` — lean collapse-to-one (the
  GROUP_UPDATE sc#N one-id precedent); implementer's call at enrollment.
OD-I3 (=ds C-8) kInstance* EffKind arg shapes + backing DREffect fields —
  AFTER HP-11's membership call.
OD-I4 (=ds C-12) The instantiate second header line (ik:/row: tags) —
  p-rule INCLUDING HP-6's (F,T)/(T,F) partition pin.

======================================================================
§5. OVERRULED / REJECTED FINDINGS (with reasons)
======================================================================

5.1 OVERRULED LOUDLY (the house rubber-stamp precedent): the two upstream
  critics who "verified" decision #9's permcheck clause —
  critique-cross:212-215 and critique-ds-deltarel:293 ("the mechanical
  permcheck bless the artifact pins ✓") — NEITHER EXECUTED the tool. The
  process judge ran it; I re-ran it on the judge's old/new census pair:
  `NOT A PURE PERMUTATION -- FAIL ... segment 3: boundary line differs`,
  exit=1. The d1 consolidator's §A.7/decision-9 permcheck claim falls with
  them (corrected in HP-14). This is the round's cautionary record: a
  referee named in a binding pin must be RUN against the exact change it
  is pinned to referee.

5.2 OVERRULED-IN-PART: judge-correctness F-C1's "dead code — no sweep
  exists" leg. Under X-DS-2 the demand table's monotone commit sweep is
  OD-7-FORCED to exist under R-MONO (the ds consolidator's own verified
  chain), so carrier option (i) was live, not dead — the BLOCKER's
  no-carrier-at-all scenario is overstated. What SURVIVES at full
  strength: the seal sits outside V-INST-EMITTED's enrollment (design
  §A.5c verbatim), C-3 was deferred [DECIDE], and a mis-wired
  SealInstanceStoreId on a nonexistent pub-table sweep is a silent
  stale-frozen path. Both proposed pins ADOPTED (HP-1); only the severity
  narrative is corrected for the record.

5.3 OVERRULED-IN-PART: judge-precedent's blanket "decision #10 ratifiable
  now" (its RATIFIABLE list, "#10 nested pins as D2.c follow-on"). It did
  not price the D3.a monotone→differential flip axis (GT-2) that the
  process judge's P-2 surfaced: an irgold blessed at D2.c re-blesses ALL
  its surfaces one epoch later. Adjudication: #10 becomes owner fork
  OD-10 with REC = defer (process), precedent's ratification recorded as
  the dissent.

5.4 REJECTED OUTRIGHT: none. Every judge amendment survived
  re-verification at code; no judge finding was found factually wrong
  beyond the two severity/scope corrections above. (Checked specifically:
  op_table_id chain :3400-3406 ✓; key_less :3470-3477 ✓; §7d ingest-only
  filter :3043-3044 ✓; AnyCutSuccessorDR :130-137 ✓; Demand.cpp body
  rejects :584-607 ✓; §18(B)(4) wording :1103/:1834 ✓; §13 CANDIDATE-A
  :1398-1403 ✓; emit_seal exits :2378/:2418/:2445 ✓; kState* EffKinds
  DeltaRel.h:82-84 ✓; kInIReadFrozen no-hazard :3302-3303 ✓;
  BenchCounters X-macro + on/off expansion ✓; force_agg_tables :167-179 ✓;
  169 cases ✓; permcheck boundary_re :62 + EXECUTED ✓.)

======================================================================
§6. LEDGER ERRATA (must reach FINDINGS/KeyedInstances ledger)
======================================================================

E-a (erratum, E-73 candidate): the permcheck mis-citation — a binding
  design decision (#9) named a mechanical referee that categorically
  rejects the exact change it was pinned to referee (census line =
  structural boundary under boundary_re). Two critique rounds
  rubber-stamped it; caught only when executed. Corrected per HP-14.
E-b: design §A.3.4's three-exit emit_inst_seal wiring is SUPERSEDED by the
  C-3/OD-5 resolution (seal self-lowered from the instance dispatch;
  V-INST-EMITTED enrolls the seal).
E-c: §18(B)(4)'s "explicit DR-IR edge" mechanism is pending owner
  substitution (OD-2) — the mandated edge is provably circular under
  emit_waw's key-forcing; record the ratification outcome in the ledger.
E-d (carried): witness-deltarel-target.md §2.6 needs its
  SUPERSEDED-IN-PART banner on adoption of this record.

======================================================================
§7. VERDICT (restated)
======================================================================

GO-WITH-AMENDMENTS, unanimous, upheld by this consolidation. With HP-1..
HP-18 folded, the design carries no known silent-miscompile path in the
D1/D2 (R-MONO) scope, satisfies the (F) law by the multiset-census
precedent, is mold-faithful with code-verified reasons for every
deviation, and every gate/prediction is either verified or corrected.
Implementation starts ONLY after the owner rules on OD-1..OD-10 (§4);
D1.a/D1.b are unblocked by everything except OD-2 (D1.b) and OD-6
(D1.b); D2.b additionally needs OD-3 (fence ii only), OD-4, OD-5.
