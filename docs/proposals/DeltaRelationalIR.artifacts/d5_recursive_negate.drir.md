# d5_recursive_negate.drir — hand-written target DR-IR artifact (pre-generalization)

Written 2026-07-14 at the delta-relational-IR epoch's design stage
(ledger: docs/proposals/DeltaRelationalIR.md §5-§6). The emitted CF-IR
identity targets are the pinned program.ir files in this directory
(regenerable: `drlojekyll <case>.dr -ir-out ...`, branch delta-relational-ir).

====================================================================
DR-IR ARTIFACT — d5_recursive_negate.dr
  reach(X) : src(X).
  reach(Y) : reach(X), edge(X,Y), !dead(Y).   ; negate INSIDE the SCC
  reach_out(X) : reach(X).
Identity lowering target: artifacts/d5rn/program.ir (409 lines) + datalog.h flow_219.
Vocabulary: DeltaRelationalIR.md §5 first cut. All attributes explicit.
====================================================================

-------------------------------------------------------------------
0. TABLE / MODEL DECLARATIONS  (program.ir:9-45; identities from the dot graph)
-------------------------------------------------------------------
Seven data-model TABLEs. FLAVOR read from datalog.h:202-209
(DiffTable<> = differential, Table<> = monotone), NOT assumed:

  T4   reach            cols[X]     DIFFERENTIAL  idx6[X]
       (== reach_out query relation; header names it reach_out_4;
        dot: TABLE 4 UNION "reach" EQ-SET 12 + MATERIALIZE reach_out)
  T7   edge             cols[X,Y]   MONOTONE      idx10[X,Y], idx70[X,_]
       (I/O edge; datalog.h:203 Table<Row7>; Seal() at commit)
  T11  dead             cols[Y]     DIFFERENTIAL  idx13[Y]   (@differential)
       (the NEGATED table; DiffTable<Row11>)
  T14  reach⋈edge join  cols[X,Y]   DIFFERENTIAL  idx17[X,Y]
       (dot: TABLE 14 JOIN EQ-SET 11; the pivot-join pair view)
  T18  negate result    cols[Y]     DIFFERENTIAL  idx20[Y]
       (dot: TABLE 18 AND-NOT EQ-SET 13; reach's inductive Y head; merges into T4)
  T21  src              cols[X]     MONOTONE      idx23[X]
       (I/O src; datalog.h:209 Table<Row21>; Seal() at commit)
  T24  join→Y project   cols[Y]     DIFFERENTIAL  idx26[Y]
       (dot: TABLE 24 TUPLE EQ-SET 7; the NEGATE's POSITIVE predecessor)

MONOTONE = {T7 edge, T21 src}.  DIFFERENTIAL = {T4, T11, T14, T18, T24}.
Confirmed ir:401-407 / datalog.h:784-808: monotone => Seal();
differential => Commit + DebugValidateCounts + CompactDead.

RECURSIVE SCC (the induction group, dot strata 6): { T4 reach, T14 join,
T18 negate-head, T24 project }.  RuleClass(edge into any of these,
target-and-read-share-SCC) = kRecursive for every fold on the cycle.
T11 dead is LOWER-stratum (dot stratum 2/3) and monotone-final w.r.t. the
SCC when the negate reads it: it is a lower position of the schemas, sign
dualized.  T7 edge / T21 src are monotone lower strata (seeds only).

-------------------------------------------------------------------
1. INGEST  (the entry procedure ^entry:27, ir:55-83; not part of flow but
   the frontier producer the DR-IR SEED band ranges over)
-------------------------------------------------------------------
Each explicit message batch nets SET-semantics, then folds nonrecursive:
  INGEST_FOLD(T21 src,  +, kNonRecursive, delta=$param:29)  -> A\D:$na32   ; reach(X):src(X) seed source
  INGEST_FOLD(T7  edge, +, kNonRecursive, delta=$param:33)  -> A\D:$na37
  INGEST_FOLD(T11 dead, +, kNonRecursive, explicit, delta=$param:38) -> addQ:$aq42
  INGEST_FOLD(T11 dead, -, kNonRecursive, explicit, delta=$param:39) -> delQ:$dq45
(explicit +/- => update-count-explicit, ir:71/75; @differential channel,
 net-batch at ir:103. src/edge use plain update-count, ir:63/67.)

===================================================================
2. FLOW BODY  ^flow:219  (ir:107-408) — the DR-IR proper
===================================================================

-------------------------------------------------------------------
2a. LOWER-STRATUM (dead) CLAIM DRAIN + FRONTIER BUILD  (ir:146-164)
    dead is differential and non-recursive => its OVERDELETE/INSERT is a
    plain claim drain with no fixpoint (single pass).
-------------------------------------------------------------------
  epoch-bump g28                                            ; ir:146

  CLAIM_DRAIN(T11 dead, sign=-)                             ; ir:147-151
      drain delQ:$dq45 ; gate TryClaimDel (C_nr<=0, F17) ; -> $overdelete:47
  CLAIM_DRAIN(T11 dead, sign=+)                             ; ir:152-156
      drain addQ:$aq42 ; gate TryClaimAdd (total>0, F17) ; -> $addition:50

  FRONTIER_FILTER(T11 dead, sign=-)                         ; ir:157-160
      loop $overdelete:47 ; ACCESS(T11, key=Y, pred=kNetDeleted) ; -> D\A:$nr53
  FRONTIER_FILTER(T11 dead, sign=+)                         ; ir:161-164
      loop $addition:50 ; ACCESS(T11, key=Y, pred=kNetAdded) ; -> A\D:$na56

  NOTE: dead is NON-recursive, so its two frontiers are finalized HERE,
  BEFORE the SCC runs. They are the exact deltas the crossover consumes.
  (No REDERIVE for dead: no recursive support to restore.)

-------------------------------------------------------------------
2b. SCC SEED BAND  (in_fixpoint=false)  (ir:165-206)
    All folds here are the seed schema; delta ranges over a lower/finalized
    frontier; same-SCC non-delta positions read InI; lower positions read
    InNew (j<i) or InI (j>i).
-------------------------------------------------------------------

  ; --- pivot vector assembly for the join seed (ir:165-174) ---
  ; reach's X pivots = reach-deltas (both signs) UNION edge-adds' X.
  PIVOT_ASSEMBLE($pivots:46 from D\A(T4)=$nr59, A\D(T4)=$na62,
                 A\D(T7 edge)=$na37 projected to X)          ; ir:165-174
      ; NB $nr59/$na62 are reach's OWN frontiers, produced by the SCC's
      ; add-loop output (2e); on the FIRST seed pass they are empty, so the
      ; seed join fires only for edge-adds. This vector is re-fed by the
      ; deferred filters — the seed/frontier feedback the ledger flags.

  ; --- SEED_FOLD for reach(Y):reach(X),edge(X,Y) into the JOIN table T14 ---
  SEED_FOLD(rule=reach⋈edge, delta_pos=pivot X, class=kRecursive) ; ir:175-186
    join-tables over $pivots:46 (ir:176):
      ACCESS(T4 reach, bound-prefix={X}, pred=<row>) via idx6  ; ir:177 select @X:71
      ACCESS(T7 edge,  bound-prefix={X}, pred=<row>) via idx70 ; ir:178 select @X,@Y:73
    ARM +  (label "added: all-in-new, some-net-added"):        ; ir:179-182
        reads reach.InNew && edge.InNew && (reach.NetAdded || edge.NetAdded)
        FOLD(T14, +, kRecursive, {X,Y}) if-crossed -> addQ(T14):$aq74
    ARM -  (label "removed: all-in-i, some-net-deleted"):       ; ir:183-186
        reads reach.InI && edge.InI && (reach.NetDeleted || edge.NetDeleted)
        FOLD(T14, -, kRecursive, {X,Y}) if-crossed -> delQ(T14):$dq75
    ; This is one JoinEmission shared by both signs (§2 dedup-by-join-view).
    ; edge is MONOTONE => it has NetAdded (A\D) but its NetDeleted arm can
    ; still appear because reach (the other side) is differential; the
    ; guard is the OR over the two sides (datalog.h:379/387).

  ; ================= CROSSOVER (the load-bearing pair) ==================
  ; negate = !dead(Y); negate result table = T18; positive predecessor = T24;
  ; negated table = T11 dead (DIFFERENTIAL => BOTH arms live).
  ; Fold class = RuleClass(T18, {pred T24, negated T11}) = kRecursive
  ; (T18 is in the SCC).  Emitted in the SEED band, seed-before-drain.
  ; pred-row read = kInNew on the POSITIVE predecessor T24, BOTH arms (§4).

  CROSSOVER(negate=!dead, sign=-)                            ; ir:188-194
      ; "negated view GAINED rows" arm: dead's A\D is a MINUS delta on reach.
      loop A\D(T11 dead)=$na56                                ; ir:189
      ACCESS(T24 project, key=Y, pred=kInNew) if-member       ; ir:190-191
      FOLD(T18 negate-head, -, kRecursive, {Y}) if-crossed -> delQ(T18):$dq78
                                                              ; ir:192-194
  CROSSOVER(negate=!dead, sign=+)                            ; ir:195-201
      ; "negated view LOST rows" arm: dead's D\A is a PLUS delta on reach.
      loop D\A(T11 dead)=$nr53                                ; ir:196
      ACCESS(T24 project, key=Y, pred=kInNew) if-member       ; ir:197-198
      FOLD(T18 negate-head, +, kRecursive, {Y}) if-crossed -> addQ(T18):$aq81
                                                              ; ir:199-201
  ; NB the crossover does NOT read dead here — the frontiers $na56/$nr53
  ; ARE the delta over dead (built in 2a). The negate's forward gate
  ; (kInNew on dead) appears ONLY in the fixpoint (2c/2d), never in seed
  ; band for this program (see vocabulary_gaps / notes: E-13 seed-context
  ; kInI is unexercised here because the whole positive path is on-cycle).

  ; --- SEED_FOLD for reach(X):src(X) : nonrecursive base into reach T4 ---
  SEED_FOLD(rule=reach:-src, delta_pos=src X, class=kNonRecursive) ; ir:202-206
      loop A\D(T21 src)=$na32                                  ; ir:203
      FOLD(T4 reach, +, kNonRecursive, {X}) if-crossed -> addQ(T4):$aq84
      ; src is monotone => no minus arm (adds-only, E-17).

-------------------------------------------------------------------
2c. FIXPOINT_ROUND(SCC) — OVERDELETE fixpoint  (ir:207-295)
    induction / empty-init / fixpoint-loop testing the four claim frontiers.
    One round = { CLAIM_DRAIN×4 ; FIXPOINT_FIRE(delta over each claim) ;
    RETIRE×4 }.  Loops while any claimed frontier nonempty (ir:209).
-------------------------------------------------------------------
  FIXPOINT_ROUND(scc, phase=OVERDELETE):                     ; ir:207-277
    per round:
      CLAIM_DRAIN(T4 reach, -)  drain delQ:$dq90 gate TryClaimDel -> $od91, claim $cd86 ; ir:215-222
      CLAIM_DRAIN(T14 join, -)  drain delQ:$dq75 -> $od94, claim $cd87                  ; ir:223-230
      CLAIM_DRAIN(T18 head, -)  drain delQ:$dq78 -> $od98, claim $cd88                  ; ir:231-238
      CLAIM_DRAIN(T24 proj, -)  drain delQ:$dq101 -> $od102, claim $cd89                ; ir:239-246

      ; ---- FIXPOINT_FIRE, OVERDELETE column, claim-relative matrix ----
      FIXPOINT_FIRE(join reach⋈edge, delta=claim(T4 reach)=$cd86, sign=-)  ; ir:247-255
          scan-index ACCESS(T7 edge, key=X in idx70) ; if-compare X ; ir:248-249
          same-SCC other side edge... but edge monotone: gate is
          ACCESS(T7 edge... actually reads) — matrix per §4:
             lower/other join side  -> here the delta is reach(X), the other
             positive side is edge (monotone, InNew, ir:251 check-member in-new T7)
          FOLD(T14, -, kRecursive, {X,Y}) if-crossed -> delQ(T14):$dq75    ; ir:253-255
      FIXPOINT_FIRE(project T14->Y, delta=claim(T14 join)=$cd87, sign=-)   ; ir:256-259
          FOLD(T24, -, kRecursive, {Y}) if-crossed -> delQ(T24):$dq101
      FIXPOINT_FIRE(union T18->reach, delta=claim(T18 head)=$cd88, sign=-) ; ir:260-263
          FOLD(T4 reach, -, kRecursive, {Y}) if-crossed -> delQ(T4):$dq90
      FIXPOINT_FIRE(negate !dead, delta=claim(T24 proj)=$cd89, sign=-)     ; ir:264-269
          ; the negate FORWARD gate in REFIRE context = kInNew (E-13):
          ACCESS(T11 dead, key=Y, pred=kInNew) if-ABSENT      ; ir:265-266
          FOLD(T18 negate-head, -, kRecursive, {Y}) if-crossed -> delQ(T18):$dq78
                                                              ; ir:267-269
      RETIRE(T4 reach, -)  over $cd86 -> retire-del           ; ir:270-271
      RETIRE(T14 join, -)  over $cd87                         ; ir:272-273
      RETIRE(T18 head, -)  over $cd88                         ; ir:274-275
      RETIRE(T24 proj, -)  over $cd89                         ; ir:276-277

  ; ---- REDERIVE (the counter-read replacement for the checker web) ----
  ; runs in the OVERDELETE induction's `output` region (ir:278-295), after
  ; the del-fixpoint quiesces. Gate = kRecursivelySupported (C_r>0).
  REDERIVE(T4 reach):  loop $od91 ; ACCESS(T4, pred=kRecursivelySupported)
      if-member -> addQ(T4):$aq84                             ; ir:280-283
  REDERIVE(T14 join):  loop $od94 ; pred=kRecursivelySupported -> addQ(T14):$aq74 ; ir:284-287
  REDERIVE(T18 head):  loop $od98 ; pred=kRecursivelySupported -> addQ(T18):$aq81 ; ir:288-291
  REDERIVE(T24 proj):  loop $od102 ; pred=kRecursivelySupported -> addQ(T24):$aq133; ir:292-295
  ; survivors re-enter INSERT with kDel&kAdd both set => net no publish,
  ; consequences replayed.

-------------------------------------------------------------------
2d. FIXPOINT_ROUND(SCC) — INSERT fixpoint  (ir:296-366)
    second induction; same skeleton, sign=+; claim = TryClaimAdd.
-------------------------------------------------------------------
  FIXPOINT_ROUND(scc, phase=INSERT):                         ; ir:296-366
    per round:
      CLAIM_DRAIN(T4 reach, +)  drain addQ:$aq84 gate TryClaimAdd -> $add141, claim $ca137 ; ir:304-311
      CLAIM_DRAIN(T14 join, +)  drain addQ:$aq74 -> $add144, claim $ca138                  ; ir:312-319
      CLAIM_DRAIN(T18 head, +)  drain addQ:$aq81 -> $add148, claim $ca139                  ; ir:320-327
      CLAIM_DRAIN(T24 proj, +)  drain addQ:$aq133 -> $add151, claim $ca140                 ; ir:328-335

      ; ---- FIXPOINT_FIRE, INSERT column ----
      FIXPOINT_FIRE(join reach⋈edge, delta=claim(T4 reach)=$ca137, sign=+) ; ir:336-344
          scan-index ACCESS(T7 edge, key=X in idx70); if-compare X ; ir:337-338
          ACCESS(T7 edge, key, pred=kInNew) if-member         ; ir:340-341
          FOLD(T14, +, kRecursive, {X,Y}) if-crossed -> addQ(T14):$aq74
      FIXPOINT_FIRE(project T14->Y, delta=claim(T14)=$ca138, sign=+)       ; ir:345-348
          FOLD(T24, +, kRecursive, {Y}) if-crossed -> addQ(T24):$aq133
      FIXPOINT_FIRE(union T18->reach, delta=claim(T18)=$ca139, sign=+)     ; ir:349-352
          FOLD(T4 reach, +, kRecursive, {Y}) if-crossed -> addQ(T4):$aq84
      FIXPOINT_FIRE(negate !dead, delta=claim(T24 proj)=$ca140, sign=+)    ; ir:353-358
          ; negate forward gate, REFIRE context = kInNew, if-ABSENT (E-13):
          ACCESS(T11 dead, key=Y, pred=kInNew) if-ABSENT      ; ir:354-355
          FOLD(T18 negate-head, +, kRecursive, {Y}) if-crossed -> addQ(T18):$aq81
                                                              ; ir:356-358
      RETIRE(T4 reach, +)  over $ca137 -> retire-add          ; ir:359-360
      RETIRE(T14 join, +)  over $ca138                        ; ir:361-362
      RETIRE(T18 head, +)  over $ca139                        ; ir:363-364
      RETIRE(T24 proj, +)  over $ca140                        ; ir:365-366

-------------------------------------------------------------------
2e. DEFERRED FRONTIER FILTERS  (ir:367-400)  — the §3 E-17 ordering
    Emitted in the INSERT induction's `output` region, AFTER the add loop
    quiesces. For SCC tables BOTH signs are deferred to here (a re-added
    row's kAdd must be final before the del filter reads kNetDeleted).
    Ordered: all four D\A filters, THEN all four A\D filters.
-------------------------------------------------------------------
  FRONTIER_FILTER(T4 reach, -)  loop $od91 ; ACCESS(pred=kNetDeleted) -> D\A:$nr59  ; ir:369-372
  FRONTIER_FILTER(T14 join, -)  loop $od94 ; pred=kNetDeleted -> D\A:$nr177          ; ir:373-376
  FRONTIER_FILTER(T18 head, -)  loop $od98 ; pred=kNetDeleted -> D\A:$nr181          ; ir:377-380
  FRONTIER_FILTER(T24 proj, -)  loop $od102; pred=kNetDeleted -> D\A:$nr184          ; ir:381-384
  FRONTIER_FILTER(T4 reach, +)  loop $add141 ; ACCESS(pred=kNetAdded) -> A\D:$na62   ; ir:385-388
  FRONTIER_FILTER(T14 join, +)  loop $add144 ; pred=kNetAdded -> A\D:$na189          ; ir:389-392
  FRONTIER_FILTER(T18 head, +)  loop $add148 ; pred=kNetAdded -> A\D:$na193          ; ir:393-396
  FRONTIER_FILTER(T24 proj, +)  loop $add151 ; pred=kNetAdded -> A\D:$na196          ; ir:397-400
  ; $nr59/$na62 (reach's frontiers) feed BACK into the seed pivot assembly
  ; (2b, ir:165-170) — the single-pass flow relies on these being built by
  ; the add-loop output. reach_out has no downstream stratum so T14/T18/T24
  ; frontiers ($nr177.. / $na189..) are dead-ends here (built but unread) —
  ; the emitter builds them uniformly per SCC table.

-------------------------------------------------------------------
2f. COMMIT SWEEP  (ir:401-407)
-------------------------------------------------------------------
  COMMIT_SWEEP(T4 reach)   -> Commit(sink) + DebugValidateCounts + CompactDead ; ir:401
  COMMIT_SWEEP(T7 edge)    -> Seal()  (MONOTONE)                                ; ir:402 / datalog.h:789
  COMMIT_SWEEP(T11 dead)   -> Commit + DebugValidateCounts + CompactDead        ; ir:403
  COMMIT_SWEEP(T14 join)   -> Commit + ...                                      ; ir:404
  COMMIT_SWEEP(T18 head)   -> Commit + ...                                      ; ir:405
  COMMIT_SWEEP(T21 src)    -> Seal()  (MONOTONE)                                ; ir:406 / datalog.h:805
  COMMIT_SWEEP(T24 proj)   -> Commit + ...                                      ; ir:407
  ; sink is a no-op here (reach_out is a QUERY relation, not a @differential
  ; transmit view; datalog.h:784 Commit([](const Row4&,bool){})). Present()
  ; filters the reach_out_f cursor (datalog.h:185).
  return-true                                                                    ; ir:408

## Lowering trace (DR op -> emitted region, the R2 identity contract)

Per DR-IR op -> program.ir region (the identity target). Line refs are
program.ir unless prefixed datalog.h.

INGEST (entry proc, ir:55-83 — precedes flow):
  INGEST_FOLD(T21 src,+,kNonRecursive)      -> ir:62-65   (update-count +nonrecursive T21 -> $na32)
  INGEST_FOLD(T7 edge,+,kNonRecursive)      -> ir:66-69   (-> $na37)
  INGEST_FOLD(T11 dead,+,explicit)          -> ir:70-73   (update-count-explicit + -> $aq42)
  INGEST_FOLD(T11 dead,-,explicit)          -> ir:74-77   (update-count-explicit - -> $dq45)
  (call ^flow:219 ir:82; net-batch dead at ir:103 in ^receive:dead)

2a LOWER dead drain/frontier:
  epoch-bump                                 -> ir:146
  CLAIM_DRAIN(T11,-)                         -> ir:147-151
  CLAIM_DRAIN(T11,+)                         -> ir:152-156
  FRONTIER_FILTER(T11,-) kNetDeleted->D\A    -> ir:157-160
  FRONTIER_FILTER(T11,+) kNetAdded->A\D      -> ir:161-164

2b SEED band:
  PIVOT_ASSEMBLE reach pivots                -> ir:165-174
  SEED_FOLD(reach⋈edge) ARM+/ARM-            -> ir:175-186
     ACCESS reach idx6                       -> ir:177   (datalog.h:372-374)
     ACCESS edge idx70                       -> ir:178   (datalog.h:375-378)
     ARM+ FOLD T14 +kRecursive               -> ir:179-182 (datalog.h:379-386)
     ARM- FOLD T14 -kRecursive               -> ir:183-186 (datalog.h:387-394)
  CROSSOVER(!dead,-) over A\D(dead)          -> ir:188-194 (datalog.h:399-412)
     ACCESS T24 kInNew if-member             -> ir:190
     FOLD T18 -kRecursive                    -> ir:192-194
  CROSSOVER(!dead,+) over D\A(dead)          -> ir:195-201 (datalog.h:413-426)
     ACCESS T24 kInNew if-member             -> ir:197
     FOLD T18 +kRecursive                    -> ir:199-201
  SEED_FOLD(reach:-src) +kNonRecursive       -> ir:202-206 (datalog.h:427-435)

2c OVERDELETE FIXPOINT_ROUND (induction, ir:207-295):
  CLAIM_DRAIN(T4,-)                          -> ir:215-222 (datalog.h:441-451)
  CLAIM_DRAIN(T14,-)                         -> ir:223-230
  CLAIM_DRAIN(T18,-)                         -> ir:231-238
  CLAIM_DRAIN(T24,-)                         -> ir:239-246
  FIXPOINT_FIRE(reach⋈edge,-) claim T4       -> ir:247-255 (datalog.h:485-499; scan idx70, edge kInNew gate)
  FIXPOINT_FIRE(T14->T24,-)   claim T14      -> ir:256-259
  FIXPOINT_FIRE(T18->T4,-)    claim T18      -> ir:260-263
  FIXPOINT_FIRE(!dead,-)      claim T24      -> ir:264-269 (datalog.h:516-528; ACCESS dead kInNew if-absent)
  RETIRE(T4/T14/T18/T24,-)                   -> ir:270-277
  REDERIVE(T4)  kRecursivelySupported        -> ir:280-283 (datalog.h:563-570)
  REDERIVE(T14)                              -> ir:284-287
  REDERIVE(T18)                              -> ir:288-291
  REDERIVE(T24)                              -> ir:292-295

2d INSERT FIXPOINT_ROUND (induction, ir:296-366):
  CLAIM_DRAIN(T4,+)                          -> ir:304-311 (datalog.h:600-610)
  CLAIM_DRAIN(T14,+)                         -> ir:312-319
  CLAIM_DRAIN(T18,+)                         -> ir:320-327
  CLAIM_DRAIN(T24,+)                         -> ir:328-335
  FIXPOINT_FIRE(reach⋈edge,+) claim T4       -> ir:336-344 (datalog.h; scan idx70, edge kInNew)
  FIXPOINT_FIRE(T14->T24,+)   claim T14      -> ir:345-348
  FIXPOINT_FIRE(T18->T4,+)    claim T18      -> ir:349-352
  FIXPOINT_FIRE(!dead,+)      claim T24      -> ir:353-358 (datalog.h:661-679; ACCESS dead kInNew if-absent)
  RETIRE(T4/T14/T18/T24,+)                   -> ir:359-366

2e DEFERRED FRONTIER FILTERS (INSERT induction output, ir:367-400):
  FRONTIER_FILTER(T4,-)  kNetDeleted->D\A    -> ir:369-372 (datalog.h:720-727)
  FRONTIER_FILTER(T14,-)                     -> ir:373-376
  FRONTIER_FILTER(T18,-)                     -> ir:377-380
  FRONTIER_FILTER(T24,-)                     -> ir:381-384
  FRONTIER_FILTER(T4,+)  kNetAdded->A\D      -> ir:385-388 (datalog.h:752-759)
  FRONTIER_FILTER(T14,+)                     -> ir:389-392
  FRONTIER_FILTER(T18,+)                     -> ir:393-396
  FRONTIER_FILTER(T24,+)                     -> ir:397-400

2f COMMIT:
  COMMIT_SWEEP(T4/T11/T14/T18/T24) diff      -> ir:401,403,404,405,407 (datalog.h:784/790/794/798/806: Commit+Validate+Compact)
  COMMIT_SWEEP(T7/T21) monotone Seal         -> ir:402,406 (datalog.h:789/805: Seal)
  return-true                                -> ir:408

STRUCTURAL NOTES ON THE LOWERING:
- One DR CLAIM_DRAIN lowers to {SortAndUnique + vector-loop + claim gate +
  append-to-overdelete + append-to-claim-accumulator} (ir shows the two
  appends explicitly, e.g. ir:219-221). The claim accumulator ($cd86..) is
  the fixpoint-loop test vector (ir:209).
- One DR FIXPOINT_FIRE that crosses a monotone side (edge) still lowers to a
  scan-index over idx70 + if-compare + kInNew CHECKMEMBER (edge is monotone
  so its InNew is trivially true, but the gate is emitted uniformly:
  datalog.h:340 / ir:251).
- The DR REDERIVE and the DR FRONTIER_FILTER(-) both loop the SAME
  $overdelete vectors ($od91/$od94/$od98/$od102); REDERIVE reads
  kRecursivelySupported and lands in addQ (2c output), the FRONTIER_FILTER
  reads kNetDeleted and lands in D\A (2e output). Same source vector, two
  different membership predicates, two different induction `output` regions
  (OVERDELETE output vs INSERT output) — an ordering the DR-IR must preserve.
- CROSSOVER lowers with NO explicit read of the negated table: the negated
  table's delta is already materialized as its frontier vectors ($na56/$nr53),
  built by 2a. The only CHECKMEMBER inside a CROSSOVER arm is the POSITIVE
  predecessor gate (T24 kInNew).

## Self-flagged vocabulary gaps (consolidated into ledger §6 vocabulary v2)

- FRONTIER-FEEDBACK / single-pass ordering is invisible in the §5 vocabulary. The seed pivot-assembly (2b, ir:165-174) reads reach's OWN frontiers $nr59/$na62, which are PRODUCED by FRONTIER_FILTER in the INSERT induction's output (2e, ir:385-388). This intra-flow producer/consumer edge across the OVERDELETE/INSERT/output phase boundary is the correctness-load-bearing §3-E17 ordering, but §5's flat operator list (SEED_FOLD, FRONTIER_FILTER) carries no scheduling/dependency attribute to express 'this SEED_FOLD's delta vector is the output of that FRONTIER_FILTER'. The vocabulary needs an explicit vector-dataflow / phase-placement attribute, not just per-op typing.
- CLAIM_DRAIN is one §5 op but lowers to TWO distinct emitted shapes: (a) the non-recursive single-pass drain for lower differential tables (dead, 2a ir:147-156: plain vector-loop, no fixpoint, feeds FRONTIER_FILTER immediately) vs (b) the SCC in-fixpoint claim drain that ALSO writes the fixpoint-loop test accumulator ($cd86.., ir:219-221) and is wrapped by FIXPOINT_ROUND. §5 has no attribute distinguishing 'drain that seeds a fixpoint round' from 'terminal drain'; both are CLAIM_DRAIN(table,sign). The RETIRE and REDERIVE ops only make sense inside case (b), implying a coupling §5 leaves implicit.
- The negate FORWARD-GATE read context is under-specified by §5's CROSSOVER(negate,sign). In this program the forward gate (kInNew on dead, if-absent) appears ONLY in FIXPOINT_FIRE refire (2c ir:265, 2d ir:354), NOT in the CROSSOVER op and NOT in the seed band. The E-13 'seed context kInI both signs' row of the §4 matrix is UNEXERCISED here (the entire positive path is on-cycle, so the negate is only ever refired). §5's CROSSOVER carries a sign but no read-context attribute, and the forward gate is actually a property of FIXPOINT_FIRE / SEED_FOLD when a negate sits on the delta's downstream path — the crossover and the forward gate are TWO different lowerings of the same source NEGATE, and §5 names only one (CROSSOVER). The vocabulary needs the forward gate as an attribute on ACCESS (pred=kInNew/kInI, polarity=absent) attached to whatever fires it, decoupled from CROSSOVER.
- FRONTIER_FILTER(table,sign) does not capture the DEFERRAL contract. For non-recursive dead (2a) the filter runs immediately after the drain; for SCC tables (2e) BOTH-sign filters are deferred into the add-loop output AFTER the whole INSERT fixpoint quiesces. Same op, two placements, and the placement is correctness-load-bearing (§3 E-17). §5 has no phase/deferral attribute.
- PIVOT_ASSEMBLE (the union of a join's delta sources into one pivot vector before SEED_FOLD, ir:165-174: three vector-unique + copy loops into $pivots:46) has NO §5 operator. It is neither ACCESS nor SEED_FOLD — it is delta-vector plumbing that precedes the join seed. §5 folds it implicitly into SEED_FOLD's 'delta_pos' but the emitted IR shows it as a distinct multi-source concat+unique step.
- The double role of one TABLE backing two relations (T4 = reach AND reach_out, header 'reach_out_4') is not expressible: §5's ACCESS(table,...) assumes table<->relation, but T4's UNION carries both the recursive 'reach' merge and the MATERIALIZE reach_out (dot EQ-SET 12). The MATERIALIZE / query-relation aliasing has no §5 marker, and it affects COMMIT_SWEEP's sink (no-op here because reach_out is a query, not a @differential transmit view) — a distinction §5's COMMIT_SWEEP(table) does not carry.
- COMMIT_SWEEP(table) conflates two lowerings selected by table FLAVOR: monotone => Seal() (ir:402/406), differential => Commit+DebugValidateCounts+CompactDead (ir:401 etc). §5 gives one op; the monotone-vs-differential split (and the sink-publishes-vs-no-op sub-choice for query vs transmit views) is an attribute §5 omits.
- edge is MONOTONE yet participates in a differential JOIN arm that has a MINUS side (SEED_FOLD ARM-, ir:183-186, guard 'reach.NetDeleted || edge.NetDeleted'). §5's SEED_FOLD(rule,delta_pos,sign,class) has no way to say 'this side is monotone (adds-only, no standing NetDeleted of its own) but the join still emits a minus arm because the OTHER side is differential'. The monotone/differential per-SIDE typing inside one arm is lost.

## Notes

Table identities were derived from the DOT dataflow graph (drlojekyll -dot-out), not assumed: T7 edge and T21 src are the only two monotone tables (I/O message relations with no deletion path); confirmed independently by datalog.h:203/209 (Table<> not DiffTable<>) and by the Seal() calls at ir:402/406 (datalog.h:789/805). All five of {T4 reach, T11 dead, T14 join, T18 negate-head, T24 project} are differential (DiffTable<>, Commit+CompactDead).

The CROSSOVER is the fixture's whole point and it checks out against §4/E-13/E-14: dead is @differential so BOTH arms live (- over dead's kNetAdditions A\D=$na56 at ir:189; + over dead's kNetRemovals D\A=$nr53 at ir:196); both arms gate the POSITIVE predecessor T24 with kInNew (ir:190/197), fold class kRecursive into the negate's own table T18, seed-before-drain. This matches the ledger §4 CROSSOVER row exactly.

E-13 subtlety worth flagging to the orchestrator: the ledger says seed-context negate forward gate = kInI both signs, refire = kInNew. In THIS program the forward gate is refire-ONLY (ir:265 del-loop, ir:354 add-loop; both kInNew + if-absent) because the entire positive derivation path (reach->join->project->negate) is on-cycle — the negate is never fired in the seed band, only inside the two fixpoints. So the seed-context-kInI half of E-13 is NOT exercised by d5_recursive_negate; a fixture with a negate whose positive predecessor is a LOWER (off-cycle) stratum would be needed to exercise it. The crossover (which IS in the seed band) reads the positive predecessor with kInNew, not kInI — it is a distinct mechanism from the forward gate.

The ten-predicate matrix is fully witnessed by this one program: kNetDeleted (ir:158,369-384), kNetAdded (ir:162,385-400), kInNew (ir:190,197,251,265,340,354), kInI (ir:387 via NetDeleted-guard reads reach.InI/edge.InI in the seed - arm at datalog.h:387), kRecursivelySupported (ir:281-293), plus the claim gates C_nr<=0 / total>0 (TryClaimDel/Add, ir:149,154,217 etc). Present() only appears in the query cursor (datalog.h:185), not the flow. kSurvivesSoFar/kAliveAtClaim/kInNewWithFrontier/kInNewSansFrontier (the §4 same-SCC join j<p / j>p reads) are NOT distinctly emitted here because the join's same-SCC side that would need them is edge (monotone) — the fixpoint join fire scans idx70 and reads plain kInNew (ir:251/340), so the four frontier-flavored predicates are collapsed to kInNew. That is a real coverage gap in this fixture: a two-differential-side same-SCC join is needed to exercise SurvivesSoFar/AliveAtClaim/InNewWithFrontier/InNewSansFrontier.

Read end-to-end: program.ir (all 409 lines) and datalog.h flow_219 (lines 285-808). All line refs cited are program.ir except where prefixed datalog.h.
