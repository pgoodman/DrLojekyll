# Demand-seeds / keyed-instances epoch — design ledger

Status: OPEN. Opened 2026-07-17 on branch `demand-seeds` off main
ca569dd8 (the subgraphs/demand epoch merged; its §15 deviations 1-4
RATIFIED at close; the §16.5 close-session amendment on main). This
file is the epoch's working ledger, in the SubgraphsDemand.md mold:
the PerfRoadmap §16 seed's re-verification record (errata continue at
E-35), the diff plan as amended, the hand-written target artifacts
index, and — as they land — the per-diff records. The landing record
goes to PerfRoadmap §17 at close. Charter: PerfRoadmap §16.0 — close
the two holed design records TOGETHER (p3-demand-argument.md's seed
mechanism + p2b-instance-target.md's store/witness holes); the path as
diffs D0-D4 per §16.3 (owner re-ranks at the design checkpoint).

## 0. Epoch-start baseline (2026-07-17, branch tip == main ca569dd8)

- debug + release builds green (incremental, ~2s each; the epoch-close
  binaries carry over — working tree clean at ca569dd8).
- ctest 3/3 (MiniDisassembler 0.28s, PointsTo 59.84s, Runtime 0.37s).
- FULL SUITE: PASS (165 cases), zero churn — the epoch-start baseline.
- Q5 spot @128 (3-rep): debug/opt 977-990ms (mean 984ms, +3.6-5.8% vs
  the run-10 930-950ms — within the 10% line). release/opt: first spot
  146-157ms under fleet load (loadavg 3.1-5.3, discarded); RE-SPOT
  after the fleet at loadavg ~1.6: 141.7/143.0/149.1ms (mean 144.6) —
  still +11-15% over the run-10 high edge, but the box is an active
  desktop this session (Chrome/audio/session load) and NO compiler
  source changed at ca569dd8 (clean tree; the release binary is an
  incremental relink of the epoch-close objects), so this is a
  MEASUREMENT-ENVIRONMENT shift, not a code regression. DISPOSITION
  (COST-honest): the raw numbers stand as recorded; per-diff Q5
  comparisons this epoch are SAME-SESSION INTERLEAVED A/B (ambient
  load cancels), and the 10%-stop-line applies to those deltas, not
  to cross-session absolutes. An idle-machine absolute re-spot is
  optional residue for the next quiet window.
- Witness artifacts at session scratchpad witness/ (12 files): force
  (174-line datalog.h) / transitive_closure / config_agg_1 /
  average_weight (945) in opt; force + config_agg_1 also nocf — the
  D0 fleet's emitted-artifact inputs.

## 1. Seed re-verification record (§16.2/§16.3/§16.5 vs HEAD; 2026-07-17)

Fleet: 5 opus derivation lanes (forcing surface / adornment+bound-query
/ StateCell config ABI / ingest hole+census / runtime substrate), each
re-deriving pseudocode FROM CODE with the seed UNREAD, chased by 5
adversarial opus verifiers checking every §16.0/§16.1/§16.2/§16.3/
§16.4/§16.5 claim against BOTH the lane report and the code, plus an
xhigh consolidator/completeness critic (11 agents, ~759k tokens;
91 claims checked across lanes, coverage gaps G1-G4 closed by the
consolidator's own reads). Lane + verify reports: session scratchpad
fleet-d0/*.md; consolidated record fleet-d0/consolidated.md. THE
PRECEDENT HELD AN EIGHTH TIME — one REAL-DEFECT (E-35). No lane report
contained a mechanism error; the one cross-lane disagreement (tc
adornment count) and the 38-vs-39 recount were resolved by the
consolidator's direct reads.

### Seed errata (E-35.. continuing the house numbering)

- E-35 (REAL-DEFECT, the D3 anchor): §16.1/§16.5(E) say the
  InstanceStore sealed side rides "the nested DiffTable's OWN kInI
  watermark (Table.h Seal/sealed)" — but Seal()/sealed exist ONLY on
  the MONOTONE Table (Table.h:225-300; Seal :277-278, sealed :282).
  A DiffTable's batch-start snapshot is the PER-ROW kInI FLAG
  (Table.h:37), read by DiffTable::InI (:357-360), advanced by
  DiffTable::Commit (:544-546) — never a Seal-advanced id-order
  watermark ("watermark" is monotone-only vocabulary). The design
  INTENT is correct and load-bearing (Table.h:584-585 verbatim:
  "kInI/kExplicit travel with the row; no watermark exists on a
  differential table, so nothing else remaps" — exactly why a
  store-held row-id breaks under CompactDead), but the parenthetical
  sends an implementer hunting for a primitive that does not exist.
  D3's sealed side must be specified in terms of the per-row kInI
  flag (or an explicit new mechanism), not a Seal watermark.
- E-36 (STALE-ANCHOR): the forcing-lower anchor "Build.cpp:246-290"
  truncates BuildQueryForceProcedureImpl (real span :246-368; the
  injector body VECTORAPPEND→CALL messsage_handler→RETURN at
  :344-366 sits PAST :290) and omits BuildQueryEntryPointImpl
  (:384-412), the separate function carrying forcer_proc.
- E-37 (STALE-ANCHOR, sharpened obstacle): the "Parse.cpp:1026
  one-forcer-per-clause assert" is at :1025 and enforces AT-MOST-one
  (zero forcers → nullopt at :1022-1023), not "exactly one"; and the
  per-adornment scheme trips THREE gates, not one: the clause-level
  assert (:1025), the query-level ParsedQuery::ForcingMessage
  assert(!pred) (:1146, at-most-one ACROSS clauses), and the DataFlow
  single-binding-pattern gate (lib/DataFlow/Build.cpp:1911, a forced
  query must have exactly ONE unique redeclaration).
- E-38 (NUANCE): §16.5(C)'s StateCell.h:158-166 range covers
  Invertible::Emit only; SealFrom is at :168-171. Mechanism
  (accept-and-ignore config pack) correct for both.
- E-39 (NUANCE, D2-load-bearing): the config-@recompute seal
  enumeration omits a THIRD config-free caller — FindOrAddGroup's
  group-birth sealed.Add(Algebra::SealFrom(w)) (StateCell.h:344),
  which arity-mismatches a config-dependent ReduceLive at COMPILE
  time; and fork (i) is not a call-site tweak — the current emission
  is one opaque statecell_<id>.Seal() (Database.cpp:2297), so (i)
  means REPLACING STATE_SEAL emission with a generated per-touched-
  group loop that reproduces the store's touched/touched_flag/
  sealed_occupied bookkeeping (StateCell.h:423-426).
- E-40 (NUANCE): the bound-query corpus count is 38/165 (23.0%), not
  "37-38" (the extra grep hit is map_3.dr's all-free query NAMED
  dup_bound — a false positive); PerfRoadmap:2478's "37-38/164" was
  never re-swept after config_agg_1 grew the corpus.
- E-41 (NUANCE): "THREE adornments of tc in transitive_closure.dr"
  conflates the three #query decls (bf/fb/f, all distinct relations
  projecting from #local tc) with tc adornments; literal left-to-right
  magic-set propagation over tc(From,To):-tc(From,X),tc(X,To) yields
  FOUR distinct tc adornments {bf, fb, ff, bb}. The load-bearing point
  (multi-adornment split mandatory) holds under either count.
- E-42 (NUANCE): "the ONLY remaining hand-coded emission surface"
  over-claims — the table-less monotone receive hand-mints a
  VECTORLOOP shim in ExtendEagerProcedure (Procedure.cpp:93-105) via
  no DR-IR op; the descent is the PRINCIPAL surface, not the only one.
- E-43 (NUANCE): §16.1's "(a)-(f)" P3-obligation range is not
  self-contained in the seed span — (a)-(d) live only in
  p3-demand-argument.md §2; the seed carries just E-33's (e)/(f).
- E-44 (NUANCE): the forced-query entry point signature (db, log,
  functors, bound...) is confirmed (force.datalog.h:118-119), but Log
  is threaded UNUSED on the inject path — inject_20 takes only
  allocator+functors. Relevant if D1 assumed injection publishes
  through the log; it does not.
- E-45 (NUANCE, D1-load-bearing): "the existing forcer machinery then
  applies UNCHANGED in shape" glosses the wiring dependency — the
  injector's CALL target context.messsage_handler[message]
  (Build.cpp:356) is populated ONLY by BuildIOProcedure
  (Procedure.cpp:399) from a real QueryIO with a non-empty receive
  (:385-391), built at Build.cpp:1234 before query entry points
  (:1245). A synthesized message with no parsed #message + DataFlow
  receive has NO handler entry and NO forced_view flow (DataFlow
  Build.cpp:1669) — D1 must fabricate the Parse/DataFlow objects or
  bypass the handler lookup (the seed's own §16.5(B) hedge, now made
  concrete).

### Verified facts the epoch stands on (fleet-confirmed)

Full 19-fact table in fleet-d0/consolidated.md §3. The load-bearing
subset:

- The forcing-message mechanism IS a full .dr surface (force.dr's
  @first on a #message inside a #query clause; ParsedClause::
  ForcingMessage Parse.cpp:1021-1028, ParsedQuery:: :1142-1155). D1 =
  synthesize what force.dr makes the user write, per adornment.
- BuildQueryForceProcedureImpl (Build.cpp:246-368) builds the
  kQueryMessageInjector proc: VECTORAPPEND(bound→add_vec) → CALL
  context.messsage_handler[message] → RETURN true; the bound argument
  becomes an ordinary message batch (witness force.ir ^inject:20 →
  ^receive:trigger_generate_next_id/1).
- A bound #query with no forcer is a pure read-time probe
  (BuildQueryEntryPointImpl :384-412; GetOrCreateIndex :405-409;
  witness transitive_closure.datalog.h) — the P3 judge's CRITICAL
  re-confirmed: no demand flows back into computation today.
- DiffTable has no Seal/sealed; frozen-I is the per-row kInI flag,
  Commit-advanced, compaction-safe by riding the row (E-35).
- The config gap is exactly (config × @recompute); fence at
  Build.cpp:1123. @invertible config rides Fold (Combine/Uncombine
  leading config, StateCell.h:149-156); @recompute config must ride
  Emit/ReduceLive (:239-247); the three config-free compile-failure
  sites are Emit(gid) (Database.cpp:2139), the bulk-Seal SealFrom
  (StateCell.h:420 via Database.cpp:2297), and group-birth SealFrom
  (StateCell.h:344). The ReduceLive DECL is already config-aware
  (Database.cpp:1145/1187) — the gap is purely call sites.
- The ingest hole contract: LowerIngestFold (Stratum.cpp:1909) returns
  the empty-bodied UPDATECOUNT (:1985); INGEST-CURSOR-SHAPE downcast
  (Procedure.cpp:86-92); descent fills (Procedure.cpp:107); Site 5
  6-tuple multiset x-check (Stratum.cpp:2113-2158, survives NDEBUG).
- The P0 census template: recount from the Query's OWN accessors
  (DR.cpp:2819-2851), no mint-order keys, V-AGG-PAIR bijection
  (DR.cpp:3002-3012) — E-27/E-28 house law.
- Commit-tail order (witness average_weight.datalog.h:877-944):
  DiffTable.Commit → DebugValidateCounts → CompactDead(+index
  rebuild) → StateCell.Seal (after its feeder) → monotone Table.Seal
  LAST.
- Vec<T> hard-asserts trivially-copyable+destructible (Vec.h:28-29):
  a per-instance nested DiffTable must be a POD handle into a
  store-owned pool (the @recompute membership idiom, StateCell.h:
  207-212); Allocator is POD-by-value, no globals (Allocator.h:13-35).
- E-32 stands re-confirmed: demand copies need a REAL structural
  input edge (the ⊥c-pivot shape), never group_ids; source-less
  cycles are dead-flow-collected (View.cpp:996-1000).
- Bound-query corpus: 38/165 (23.0%); an unconditional demand
  transform rewrites ~23% of goldens — mode-gating is mandatory.

## 2. The diff plan as amended

(PENDING — after D0: D1 seed-mechanism design + judge; D2 config_agg_2
Seal-config fork; D3 InstanceStore redesign + demand-wired witness;
D4 the mode-gated live transform, contingent on D1's judge. Owner
decisions (a)-(d) taken before any emission-changing code.)

## 3. Artifact index

(PENDING)
