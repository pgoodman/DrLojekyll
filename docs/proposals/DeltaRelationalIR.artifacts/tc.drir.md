# tc.drir — hand-written target DR-IR artifact (pre-generalization)

Written 2026-07-14 at the delta-relational-IR epoch's design stage
(ledger: docs/proposals/DeltaRelationalIR.md §5-§6). The emitted CF-IR
identity targets are the pinned program.ir files in this directory
(regenerable: `drlojekyll <case>.dr -ir-out ...`, branch delta-relational-ir).

======================================================================
DR-IR ARTIFACT — transitive_closure_diff.dr (hand-written, §5 vocab)
======================================================================
Source (tests/OptDiff/cases/transitive_closure_diff.dr):
  #message add_edge(From,To) @differential          (input)
  #message found_path(From,To) @differential         (output, on tc)
  #query reachable_from(bound From, free To) : tc.    (bf cursor, idx on From)
  #query reaching_to(free From, bound To) : tc.       (fb cursor, idx on To)
  edge(F,T)      : add_edge(F,T).
  tc(F,T)        : edge(F,T).                          [linear/seed rule]
  tc(F,T)        : tc(F,X), tc(X,T).                   [nonlinear/recursive]
  found_path(F,T): tc(F,T).

Identity lowering target: artifacts/tcd/program.ir (296 lines).
All lines below refer to that file unless a datalog.h line is named.

----------------------------------------------------------------------
§A. TABLES  (from `create %table:*`, program.ir:9-34)
----------------------------------------------------------------------
Each is a DiffTable (differential; split C_nr/C_r counters, kInI watermark).
No monotone table in this module — even `edge` is @differential-fed.

T_edge   = %table:13[u64,u64]  cols (From:14,To:15)              ir:24-28
           model: `edge` relation. deriv-class of its own writes = NonRecursive.
           index: %index:16 full-key [From,To]                    ir:28
           kind: NON-RECURSIVE (not in tc SCC); lowest stratum.

T_tc     = %table:4[u64,u64]   cols (From:5,To:6)                 ir:9-16
           model: `tc` / `reachable_from`. BACKS output msg found_path/2
           and both query cursors. Compacts + reindexes on commit.
           indices:
             %index:7  full-key [From,To]      (membership Find)  ir:13
             %index:80 [From,_]  (key=From)    -> query bf; scan tc(X,·) ir:14
             %index:87 [_,To]    (key=To)      -> query fb; scan tc(·,X) ir:15
           kind: RECURSIVE (in the tc SCC).

T_join   = %table:8[u64,u64,u64] cols (X:9,From:10,To:11)         ir:17-22
           model: the nonlinear JOIN view  tc(From,X) ⋈_X tc(X,To).
           Pivot column = X (position 0). Payload = (From,To).
           index: %index:12 full-key [X,From,To]                  ir:22
           kind: RECURSIVE (in the tc SCC).

T_proj   = %table:17[u64,u64]  cols (From:18,To:19)               ir:30-34
           model: TUPLE/MERGE-arm view = projection πFrom,To(T_join),
           the recursive arm of the tc MERGE (distinct model from T_tc
           by identity — the group_ids CSE guard; do NOT structurally
           dedup it against T_tc).
           index: %index:20 full-key [From,To]                    ir:34
           kind: RECURSIVE (in the tc SCC).

SCC (recursive) = { T_tc, T_join, T_proj }.  T_edge is a lower,
non-recursive stratum. RuleClass (Stratum.cpp:291) is fixed per rule:
  edge(F,T):add_edge      -> kNonRecursive  (target T_edge ∉ SCC)
  tc(F,T):edge            -> kNonRecursive  (reads T_edge, lower stratum)
  tc(F,T):tc(F,X),tc(X,T) -> kRecursive     (target+reads share the SCC)
  found_path/reachable_from/reaching_to : projections of T_tc, no own table.

Global: `global u64 @22` = epoch/batch counter (ir:7; g22 in header).

----------------------------------------------------------------------
§B. STRATA  (topological; computed by the scheduling fixpoint,
             Stratum.cpp:1732 — order is DATA here, not emitted control)
----------------------------------------------------------------------
S0  NON-DIFFERENTIAL-SEED band (message ingest -> edge deltas):
      ACCESS/UPDATECOUNT into T_edge from the netted add/del queues.
      Lowered in ^entry:21 (ir:42-58), NOT in ^flow.
S1  NON-RECURSIVE stratum: edge -> tc seed (T_edge frontier -> T_tc),
      pre-induction body of ^flow (ir:96-124).
S2  RECURSIVE stratum (SCC {T_tc,T_join,T_proj}):
      OVERDELETE fixpoint round -> REDERIVE -> INSERT fixpoint round,
      then deferred SCC frontier filters (ir:125-290).
S3  COMMIT band: per-table commit sweeps in table order (ir:291-294).

Note the "stratum" integers do not appear in the IR; they only decide
the emission order §D reproduces. This is exactly the E-16 shared state
(branches/joins/products/drain_stratum) that R1 externalizes.

======================================================================
§C. PER-STRATUM OPERATOR LIST  (§5 vocabulary, all attrs explicit)
======================================================================
Legend for ACCESS lowering choice under identity:
  point-test = single Find + membership predicate (CHECKMEMBER/claim)
  chain walk = idx.First/Next section walk + if-compare pivot re-test
  full scan  = vector-loop over a frontier vec (no table index)
Vectors named V<n> = program.ir $...:<n>.

......................................................................
S0 — MESSAGE INGEST / EDGE SEED     (proc ^entry:21, ir:42-58)
......................................................................
[S0.1] NET_BATCH(add_edge)                                       ir:62 (header
       datalog.h:246 NetBatch): SET-net the two message vecs (dedup
       each; adds∩removes annihilate). OQ3 semantics.
       -- only present on the receive path (^receive:add_edge, ir:60-64).

[S0.2] SEED_FOLD(rule=edge:add_edge, delta_pos=0, sign=+,
                 class=NonRecursive)
       vectors: over $param:23 (add queue)
       op: UPDATECOUNT +nonrecursive {From,To} in T_edge, if-crossed
           append -> $add_queue:28                               ir:47-50
       ACCESS: none beyond the count RMW (explicit-add form
           update-count-EXPLICIT — AddExplicit, not a join fire).

[S0.3] SEED_FOLD(rule=edge:add_edge, delta_pos=0, sign=-,
                 class=NonRecursive)
       vectors: over $param:24 (del queue)
       op: UPDATECOUNT -nonrecursive {From,To} in T_edge, if-crossed
           append -> $delete_queue:32                            ir:51-54
       (S0.2/S0.3 run in a `par`, ir:46; then clear params, call ^flow.)

......................................................................
S1 — EDGE -> TC SEED (non-recursive)   (proc ^flow:198, ir:96-124)
......................................................................
[S1.0] epoch bump  @22 += 1                                      ir:96

--- claim-drain of the edge delta queues (T_edge is differential) ---
[S1.1] CLAIM_DRAIN(table=T_edge, sign=-)
       vector: sort-unique $delete_queue:32 (ir:97) then loop
       op: claim-del {From,To} in T_edge, if-claimed -> $overdelete:34
                                                                 ir:98-101
       claim-context: TryClaimDel gate (F17: proceeds iff C_nr ≤ 0).
       ACCESS(T_edge, bound-prefix={From,To}, pred=(claim-del gate))
         lowering: point-test (Find full-key + TryClaimDel).      hdr:285-286

[S1.2] CLAIM_DRAIN(table=T_edge, sign=+)
       vector: sort-unique $add_queue:28 (ir:102) then loop
       op: claim-add {From,To} in T_edge, if-claimed -> $addition:38
                                                                 ir:103-106
       claim-context: TryClaimAdd (proceeds iff total > 0).
       ACCESS(T_edge,{From,To},claim-add gate): point-test.       hdr:294-295

--- edge frontier filters (D\A, A\D) ---
[S1.3] FRONTIER_FILTER(table=T_edge, sign=-)
       vector: loop $overdelete:34
       pred: check-member NET-DELETED {From,To} in T_edge
             (kNetDeleted = kDel && !kAdd, E-14) -> $net_removals:42
                                                                 ir:107-110
       ACCESS(T_edge,{From,To},kNetDeleted): point-test.          hdr:302-303

[S1.4] FRONTIER_FILTER(table=T_edge, sign=+)
       vector: loop $addition:38
       pred: check-member NET-ADDED {From,To} in T_edge
             (kNetAdded = kAdd && !kDel && !kInI, E-12/E-14) -> $net_additions:46
                                                                 ir:111-114
       ACCESS(T_edge,{From,To},kNetAdded): point-test.            hdr:310-311

--- edge -> tc seed fold (rule tc:edge; T_edge is a LOWER stratum,
    so this is a seed-schema arm with delta over T_edge's signed frontier;
    delta_pos=0, class=NonRecursive because RuleClass(tc:edge)=kNonRec) ---
[S1.5] SEED_FOLD(rule=tc:edge, delta_pos=0, sign=-, class=NonRecursive)
       vector: sort-unique $net_removals:42 (ir:115) then loop
       op: UPDATECOUNT -nonrecursive {From,To} in T_tc, if-crossed
           -> $delete_queue:53                                   ir:116-119
       ACCESS: none beyond the fold (single-atom body; delta IS the row).

[S1.6] SEED_FOLD(rule=tc:edge, delta_pos=0, sign=+, class=NonRecursive)
       vector: sort-unique $net_additions:46 (ir:120) then loop
       op: UPDATECOUNT +nonrecursive {From,To} in T_tc, if-crossed
           -> $add_queue:57                                      ir:121-124
       ACCESS: none.
       [$delete_queue:53 / $add_queue:57 are the SCC's entry queues.]

......................................................................
S2 — RECURSIVE SCC {T_tc,T_join,T_proj}    (ir:125-290)
......................................................................
Structure = FIXPOINT_ROUND(scc=DEL) ; REDERIVE(*) ; FIXPOINT_ROUND(scc=ADD)
            ; deferred FRONTIER_FILTERs. Two `induction` regions.

===== FIXPOINT_ROUND(scc, phase=OVERDELETE) =====  ir:125-201
  induction / empty-init / fixpoint-loop testing the three claimed-del
  frontier vecs $claimed_del:59(T_tc), :60(T_join), :61(T_proj)  ir:125-127
  Loop body (ir:128-187), one round:

  --- claim drains (these ARE the OVERDELETE loop for SCC tables) ---
  [S2.D1] CLAIM_DRAIN(T_tc, sign=-)
        clear frontiers (ir:129-131); sort-unique $delete_queue:53;
        loop; claim-del {F,T} in T_tc if-claimed ->
          $overdelete:62 AND round-frontier $claimed_del:59       ir:132-138
        claim-ctx TryClaimDel (C_nr ≤ 0). ACCESS(T_tc,{F,T}): point-test.
  [S2.D2] CLAIM_DRAIN(T_join, sign=-)
        sort-unique $delete_queue:66; claim-del {X,F,T} in T_join ->
          $overdelete:67 AND $claimed_del:60                      ir:140-146
        ACCESS(T_join,{X,F,T}): point-test.
  [S2.D3] CLAIM_DRAIN(T_proj, sign=-)
        sort-unique $delete_queue:72; claim-del {F,T} in T_proj ->
          $overdelete:73 AND $claimed_del:61                      ir:148-154

  --- FIXPOINT_FIRE of the recursive join, OVERDELETE column ---
  The single JoinEmission for tc(F,X)⋈tc(X,T) fires from the SAME
  claimed-del frontier of T_tc ($claimed_del:59) along BOTH section
  walks (the shared kJoinPivots vec — one join view, two feeding
  chains). delta_pos names which body atom is the delta:

  [S2.D4] FIXPOINT_FIRE(join=tc⋈tc, delta_pos=0 [tc(F,X) is delta],
                        sign=-, class=Recursive)
        vector: loop $claimed_del:59  ({From:78,X:79})
        ACCESS(T_tc, bound-prefix={X=@X:79}, section walk via idx_80[From,_]):
          scan-index select {X,To} where {X:79} in %index:80     ir:157
          if-compare {X:82}={X:79}  (pivot re-test on scan cursor id)
          -> chain walk.
        same-SCC j>p read: check-member ALIVE-AT-CLAIM {X,To} in T_tc ir:160
          (fixpoint-schema OVERDELETE, same j>i = kAliveAtClaim, §4/§5.1)
          -> point-test (on the scan cursor id).
        fold: UPDATECOUNT -recursive {X:79,From:78,To:83} in T_join,
          if-crossed -> $delete_queue:66                         ir:162-164
  [S2.D5] FIXPOINT_FIRE(join=tc⋈tc, delta_pos=1 [tc(X,T) is delta],
                        sign=-, class=Recursive)
        vector: loop $claimed_del:59  ({X:85,To:86})
        ACCESS(T_tc, bound-prefix={X=@X:85}, section walk via idx_87[_,To]):
          scan-index select {From,X} where {X:85} in %index:87   ir:166
          if-compare {X:90}={X:85}  -> chain walk.
        same-SCC j<p read: check-member SURVIVES-SO-FAR {From,X} in T_tc ir:169
          (fixpoint OVERDELETE, same j<i = kSurvivesSoFar, §4/§5.1)
          -> point-test.
        fold: UPDATECOUNT -recursive {X:85,From:89,To:86} in T_join ->
          $delete_queue:66                                       ir:171-173

  --- projection folds (T_join -> T_proj -> T_tc), recursive column ---
  [S2.D6] SEED_FOLD(rule=proj:join, delta_pos=0, sign=-, class=Recursive)
        loop $claimed_del:60 ({X,From,To}); UPDATECOUNT -recursive
        {From:93,To:94} in T_proj, if-crossed -> $delete_queue:72 ir:174-177
        ACCESS: none (pure projection drop of X).
  [S2.D7] SEED_FOLD(rule=tc:proj [the MERGE arm], delta_pos=0, sign=-,
                    class=Recursive)
        loop $claimed_del:61 ({From,To}); UPDATECOUNT -recursive
        {From:96,To:97} in T_tc, if-crossed -> $delete_queue:53   ir:178-181
        ACCESS: none (MERGE arm copy T_proj -> T_tc).

  --- RETIRE band (clear kDelNow; swap round vectors) ---
  [S2.D8] RETIRE(T_tc, sign=-)   loop $claimed_del:59 retire-del  ir:182-183
  [S2.D9] RETIRE(T_join, sign=-) loop $claimed_del:60 retire-del  ir:184-185
  [S2.D10] RETIRE(T_proj, sign=-) loop $claimed_del:61 retire-del ir:186-187
        (retire ACCESS in header is a Find+RetireDel: point-test, hdr:435-437)

===== REDERIVE  (induction `output` of the DEL round) =====  ir:188-201
  A counter read, not a search (§5.2). Loop each table's OWN $overdelete:
  [S2.R1] REDERIVE(T_tc)
        loop $overdelete:62; check-member RECURSIVELY-SUPPORTED {F,T}
        in T_tc (C_r > 0), if-member -> $add_queue:57            ir:190-193
        ACCESS(T_tc,{F,T},kRecursivelySupported): point-test.
  [S2.R2] REDERIVE(T_join)
        loop $overdelete:67; recursively-supported {X,F,T} in T_join
        -> $add_queue:111                                        ir:194-197
  [S2.R3] REDERIVE(T_proj)
        loop $overdelete:73; recursively-supported {F,T} in T_proj
        -> $add_queue:116                                        ir:198-201

===== FIXPOINT_ROUND(scc, phase=INSERT) =====  ir:202-264
  Second `induction`; fixpoint-loop testing $claimed_add:121(T_tc),
  :122(T_join), :123(T_proj).                                    ir:202-204

  --- claim drains (ARE the INSERT loop) ---
  [S2.A1] CLAIM_DRAIN(T_tc, sign=+)
        clear (ir:206-208); sort-unique $add_queue:57; claim-add {F,T}
        in T_tc if-claimed -> $addition:124 AND $claimed_add:121  ir:209-215
        claim-ctx TryClaimAdd (total > 0). point-test.
  [S2.A2] CLAIM_DRAIN(T_join, sign=+)
        sort-unique $add_queue:111; claim-add {X,F,T} -> $addition:128
        AND $claimed_add:122                                     ir:217-223
  [S2.A3] CLAIM_DRAIN(T_proj, sign=+)
        sort-unique $add_queue:116; claim-add {F,T} -> $addition:133
        AND $claimed_add:123                                     ir:225-231

  --- FIXPOINT_FIRE join, INSERT column (shared kJoinPivots vec) ---
  [S2.A4] FIXPOINT_FIRE(join=tc⋈tc, delta_pos=0, sign=+, class=Recursive)
        loop $claimed_add:121 ({From:138,X:139})
        ACCESS(T_tc,{X=@X:139}, idx_80[From,_] section walk):
          scan-index {X,To} where {X:139} in %index:80; if-compare  ir:234-235
        same-SCC j>p read: check-member IN-NEW-SANS-FRONTIER {X,To}
          in T_tc (fixpoint INSERT same j>i = kInNewSansFrontier, §4) ir:237
          -> point-test.
        fold: UPDATECOUNT +recursive {X:139,From:138,To:142} in T_join ->
          $add_queue:111                                         ir:239-241
  [S2.A5] FIXPOINT_FIRE(join=tc⋈tc, delta_pos=1, sign=+, class=Recursive)
        loop $claimed_add:121 ({X:144,To:145})
        ACCESS(T_tc,{X=@X:144}, idx_87[_,To] section walk):
          scan-index {From,X} where {X:144} in %index:87; if-compare ir:243-244
        same-SCC j<p read: check-member IN-NEW-WITH-FRONTIER {From,X}
          in T_tc (fixpoint INSERT same j<i = kInNewWithFrontier, §4) ir:246
          -> point-test.
        fold: UPDATECOUNT +recursive {X:144,From:147,To:145} in T_join ->
          $add_queue:111                                         ir:248-250

  --- projection folds (INSERT column) ---
  [S2.A6] SEED_FOLD(rule=proj:join, delta_pos=0, sign=+, class=Recursive)
        loop $claimed_add:122; UPDATECOUNT +recursive {From:151,To:152}
        in T_proj -> $add_queue:116                              ir:251-254
  [S2.A7] SEED_FOLD(rule=tc:proj, delta_pos=0, sign=+, class=Recursive)
        loop $claimed_add:123; UPDATECOUNT +recursive {From:154,To:155}
        in T_tc -> $add_queue:57                                 ir:255-258

  --- RETIRE band (add) ---
  [S2.A8]  RETIRE(T_tc, +)   loop $claimed_add:121 retire-add     ir:259-260
  [S2.A9]  RETIRE(T_join, +) loop $claimed_add:122 retire-add     ir:261-262
  [S2.A10] RETIRE(T_proj, +) loop $claimed_add:123 retire-add     ir:263-264

===== DEFERRED SCC FRONTIER FILTERS =====  ir:265-290
  (induction `output` of the ADD round; E-17: SCC tables' D\A and A\D
   filters run AFTER the add loop quiesces — deferral is correctness-
   load-bearing: a re-added row's kAdd must be final before the del
   filter reads kNetDeleted.)
  [S2.F1] FRONTIER_FILTER(T_tc, -)   loop $overdelete:62;
        check-member NET-DELETED {F,T} -> $net_removals:166       ir:267-270
  [S2.F2] FRONTIER_FILTER(T_join, -) loop $overdelete:67;
        net-deleted {X,F,T} -> $net_removals:170                  ir:271-274
  [S2.F3] FRONTIER_FILTER(T_proj, -) loop $overdelete:73;
        net-deleted {F,T} -> $net_removals:175                    ir:275-278
  [S2.F4] FRONTIER_FILTER(T_tc, +)   loop $addition:124;
        check-member NET-ADDED {F,T} -> $net_additions:179        ir:279-282
  [S2.F5] FRONTIER_FILTER(T_join, +) loop $addition:128;
        net-added {X,F,T} -> $net_additions:183                   ir:283-286
  [S2.F6] FRONTIER_FILTER(T_proj, +) loop $addition:133;
        net-added {F,T} -> $net_additions:188                     ir:287-290
  ACCESS for all six: point-test (Find + kNetDeleted/kNetAdded).
  NOTE: these six $net_* vectors are DEAD SINKS in this module — no
  higher stratum consumes T_tc/T_join/T_proj frontiers (tc is the top
  SCC; found_path publishes from the commit sweep, not a frontier).
  They are emitted unconditionally by the schema anyway (see gap G8).

......................................................................
S3 — COMMIT band     (ir:291-294)
......................................................................
[S3.1] COMMIT_SWEEP(T_tc) publishing found_path/2                ir:291
       publishes was!=now (kInI vs total>0); then DebugValidateCounts;
       then CompactDead + reindex idx_80/idx_87 (hdr:646-660).
       This is the ONLY publishing sweep (backs the @differential msg).
[S3.2] COMMIT_SWEEP(T_join)   no publish                         ir:292
[S3.3] COMMIT_SWEEP(T_proj)   -- wait: table order is 4,8,13,17 --
       Actual emitted order (ir:291-294): T_tc(4), T_join(8),
       T_edge(13), T_proj(17). Commit sweeps are DOWNSTREAM of
       BuildStratumPhases, per differential table in TABLE-ID order.
[S3.2'] COMMIT_SWEEP(T_join=%table:8)                            ir:292
[S3.3'] COMMIT_SWEEP(T_edge=%table:13)                           ir:293
[S3.4']  COMMIT_SWEEP(T_proj=%table:17)                          ir:294


## Lowering trace (DR op -> emitted region, the R2 identity contract)

Each DR op -> exact program.ir region the R2 generic lowering must
reproduce byte-identically (identity target). Format: DR-op => ir:lines.

--- proc scaffolding (not per-op; the CONSTRUCTION/emission frame) ---
module const/global decls                 => ir:1-7
create T_tc / T_join / T_edge / T_proj    => ir:9-16 / 17-22 / 24-28 / 30-34
init proc + empty-vec call                => ir:36-40  (hdr:212-216)
entry proc frame + par + tail call        => ir:42-58  (hdr:219-242)
receive(add_edge) NET_BATCH + call        => ir:60-64  (hdr:245-249)
flow proc frame + all vector-define        => ir:66-95  (hdr:252-281)
epoch bump @22+=1                          => ir:96

--- S0 (entry proc ^entry:21) ---
S0.1 NET_BATCH                             => ir:62         (hdr:246)
S0.2 SEED_FOLD(edge:add_edge,+,NonRec)     => ir:47-50      (hdr:222-229)
S0.3 SEED_FOLD(edge:add_edge,-,NonRec)     => ir:51-54      (hdr:230-237)

--- S1 (flow, pre-induction) ---
S1.1 CLAIM_DRAIN(T_edge,-)                 => ir:97-101     (hdr:282-290)
S1.2 CLAIM_DRAIN(T_edge,+)                 => ir:102-106    (hdr:291-299)
S1.3 FRONTIER_FILTER(T_edge,-)             => ir:107-110    (hdr:300-307)
S1.4 FRONTIER_FILTER(T_edge,+)             => ir:111-114    (hdr:308-315)
S1.5 SEED_FOLD(tc:edge,-,NonRec)           => ir:115-119    (hdr:316-328)
S1.6 SEED_FOLD(tc:edge,+,NonRec)           => ir:120-124    (hdr:329-341)

--- S2 OVERDELETE round (induction #1, ir:125-201) ---
FIXPOINT_ROUND(DEL) region shell (induction/empty-init/fixpoint-loop
  testing $claimed_del:59,60,61 + per-round clears)
                                           => ir:125-131    (hdr:342-345)
S2.D1 CLAIM_DRAIN(T_tc,-)                  => ir:132-139    (hdr:346-356)
S2.D2 CLAIM_DRAIN(T_join,-)                => ir:140-147    (hdr:357-367)
S2.D3 CLAIM_DRAIN(T_proj,-)                => ir:148-155    (hdr:368-378)
S2.D4 FIXPOINT_FIRE(join,dpos0,-)          => ir:156-164    (hdr:379-395)
S2.D5 FIXPOINT_FIRE(join,dpos1,-)          => ir:165-173    (hdr:396-412)
S2.D6 SEED_FOLD(proj:join,-,Rec)           => ir:174-177    (hdr:413-420)
S2.D7 SEED_FOLD(tc:proj,-,Rec)             => ir:178-181    (hdr:421-432)
S2.D8 RETIRE(T_tc,-)                       => ir:182-183    (hdr:433-440)
S2.D9 RETIRE(T_join,-)                     => ir:184-185    (hdr:441-448)
S2.D10 RETIRE(T_proj,-)                    => ir:186-187    (hdr:449-456)

--- S2 REDERIVE (induction #1 `output`, ir:188-201) ---
REDERIVE region shell (output/seq)         => ir:188-189
S2.R1 REDERIVE(T_tc)                       => ir:190-193    (hdr:458-465)
S2.R2 REDERIVE(T_join)                     => ir:194-197    (hdr:466-473)
S2.R3 REDERIVE(T_proj)                     => ir:198-201    (hdr:474-481)

--- S2 INSERT round (induction #2, ir:202-264) ---
FIXPOINT_ROUND(ADD) region shell           => ir:202-208    (hdr:482-485)
S2.A1 CLAIM_DRAIN(T_tc,+)                   => ir:209-216    (hdr:486-496)
S2.A2 CLAIM_DRAIN(T_join,+)                => ir:217-224    (hdr:497-507)
S2.A3 CLAIM_DRAIN(T_proj,+)                => ir:225-232    (hdr:508-518)
S2.A4 FIXPOINT_FIRE(join,dpos0,+)          => ir:233-241    (hdr:519-535)
S2.A5 FIXPOINT_FIRE(join,dpos1,+)          => ir:242-250    (hdr:536-552)
S2.A6 SEED_FOLD(proj:join,+,Rec)           => ir:251-254    (hdr:553-560)
S2.A7 SEED_FOLD(tc:proj,+,Rec)             => ir:255-258    (hdr:561-572)
S2.A8 RETIRE(T_tc,+)                        => ir:259-260    (hdr:573-580)
S2.A9 RETIRE(T_join,+)                      => ir:261-262    (hdr:581-588)
S2.A10 RETIRE(T_proj,+)                    => ir:263-264    (hdr:589-596)

--- S2 deferred frontier filters (induction #2 `output`, ir:265-290) ---
FRONTIER_FILTER region shell (output/seq)  => ir:265-266
S2.F1 FRONTIER_FILTER(T_tc,-)              => ir:267-270    (hdr:598-605)
S2.F2 FRONTIER_FILTER(T_join,-)            => ir:271-274    (hdr:606-613)
S2.F3 FRONTIER_FILTER(T_proj,-)            => ir:275-278    (hdr:614-621)
S2.F4 FRONTIER_FILTER(T_tc,+)              => ir:279-282    (hdr:622-629)
S2.F5 FRONTIER_FILTER(T_join,+)            => ir:283-286    (hdr:630-637)
S2.F6 FRONTIER_FILTER(T_proj,+)            => ir:287-290    (hdr:638-645)

--- S3 commit band ---
S3.1 COMMIT_SWEEP(T_tc, publish found_path/2)
                                           => ir:291         (hdr:646-660)
S3.2' COMMIT_SWEEP(T_join)                 => ir:292         (hdr:661-665)
S3.3' COMMIT_SWEEP(T_edge)                 => ir:293         (hdr:666-670)
S3.4' COMMIT_SWEEP(T_proj)                 => ir:294         (hdr:671-675)
return-true                                => ir:295

Query cursors (bf/fb) are NOT flow-proc regions — they lower from the
#query decls to the cursor structs (hdr:150-195), reading T_tc via
idx_80/idx_87 with Present(id) filtering. No DR op in ^flow emits them;
they are a separate lowering family keyed on the query view + index.

VECTOR SORT-UNIQUE placement (byte-identity-critical, see gap G6):
  vector-unique appears at: ir:97,102,115,120,132,140,148,209,217,225.
  Rule reproduced: sort-unique is emitted on a frontier vec IMMEDIATELY
  before the vector-loop that drains it, for delta-side / claim-drain
  vecs only (NOT on the same-SCC fire output vecs :66/:111 inside a
  round — those are sort-uniqued at the TOP of the NEXT round's drain,
  ir:140/217). $net_removals:42 / $net_additions:46 are sort-uniqued
  (ir:115,120) before the seed fold because seeds must be multiset-once.


## Self-flagged vocabulary gaps (consolidated into ledger §6 vocabulary v2)

- G1 SAME-SCC JOIN SEED SUPPRESSION IS INVISIBLE IN THE VOCABULARY. The rule tc(F,T):tc(F,X),tc(X,T) has NO seed-schema arm in S1 (no SEED_FOLD firing edge/lower-frontier into T_join at round 0) because all sides share the SCC (Stratum.cpp:1541-57 / §2 ln148-149: 'all-sides-same-SCC joins keep their JoinEmission but suppress seeds; round-0 carried by the claim-round fire'). The §5 vocab has SEED_FOLD and FIXPOINT_FIRE as separate ops but no attribute marking 'this JoinEmission is seed-suppressed'. An R1 constructor emitting SEED_FOLD for every rule would wrongly emit an edge->T_join seed. Need a JoinEmission-level flag (seed_suppressed: all sides in one SCC) that FIXPOINT_FIRE carries and SEED_FOLD omission derives from.
- G2 THE SHARED kJoinPivots VEC IS NOT MODELED. Both FIXPOINT_FIRE arms per phase (S2.D4/D5, S2.A4/A5) feed ONE join view T_join from ONE claimed frontier ($claimed_del:59 / $claimed_add:121) via TWO section walks (idx_80 and idx_87), and both append to ONE shared output queue ($delete_queue:66 / $add_queue:111). §5's FIXPOINT_FIRE(join,delta_pos,sign) is per (join,delta_pos) — it does not express that the two delta_pos arms of one join SHARE the input frontier vec and the output vec, nor that they are dedup'd by join view (Stratum.cpp:1541). The vocab needs the join, not the arm, to own the vecs; delta_pos becomes a section index under one FIXPOINT_FIRE(join).
- G3 SECTION WALKS (the two scan directions of a self-join) HAVE NO FIRST-CLASS FORM. Each FIXPOINT_FIRE arm is a scan-index + if-compare pivot re-test (ir:157-160, 166-169, 234-237, 243-246) — a 'chain walk' ACCESS. But which INDEX (idx_80 vs idx_87) and which pivot-equality column is a function of (which body atom is delta, which is the pivot column of the join). The ACCESS(table,bound-prefix,pred) op names bound-prefix={X} and lowering=chain walk, but does NOT encode the index choice nor the if-compare re-test column. For a self-join both scans read the SAME table T_tc under DIFFERENT indices; the vocab must carry (index, pivot-col) per section, or the lowering is ambiguous between idx_80 and idx_87.
- G4 THE INDUCTION / FIXPOINT-ROUND REGION SHELL IS UNDER-SPECIFIED. §5 has FIXPOINT_ROUND(scc) as a flat op, but the IR shows it is a structured REGION: induction / empty-init / fixpoint-loop testing <the per-table claimed-frontier vecs> / body / output. The DEL round and ADD round are TWO SEPARATE induction regions (ir:125 and ir:202), and REDERIVE lives in the DEL round's `output` while the deferred FRONTIER_FILTERs live in the ADD round's `output`. The vocab does not say (a) that FIXPOINT_ROUND wraps CLAIM_DRAIN+FIXPOINT_FIRE+SEED_FOLD+RETIRE as its loop body, (b) which vecs are the loop's fixpoint-test set (the claimed-* vecs, one per SCC table), nor (c) that REDERIVE/FRONTIER_FILTER are hosted in the two rounds' `output` clauses. The round is a container, not a leaf op.
- G5 RETIRE BANDS HAVE NO ORDERING/GROUPING CONTRACT. RETIRE(table,sign) exists, but the IR emits all three RETIREs as a BAND at the tail of each round body (ir:182-187, 259-264), AFTER all fires/folds of that round, one per SCC table in table order. §5 does not state that RETIRE is deferred to the round tail (it must be — retiring kDelNow before the same-round fires read AliveAtClaim/SurvivesSoFar would corrupt the same-round exactly-once counting). The band's position relative to FIXPOINT_FIRE and its per-table iteration order need to be a construction invariant, not left to op scheduling.
- G6 VECTOR SORT-UNIQUE PLACEMENT IS NOT AN ATTRIBUTE OF ANY OP. vector-unique is byte-present at 10 sites (ir:97,102,115,120,132,140,148,209,217,225) and byte-ABSENT everywhere else. Whether a drain is preceded by sort-unique depends on the vec's role (frontier/claim-drain = yes; per-round fire-output consumed next round = sort-uniqued at next round's drain, not at production). §5's ops carry `vectors` but no per-vector 'sort-unique before drain' bit. R2 identity lowering will mis-place vector-unique (extra or missing) unless each CLAIM_DRAIN/SEED_FOLD input vec declares whether it is sort-uniqued at its drain site. This is the single highest byte-divergence risk.
- G7 THE PROJECTION / MERGE-ARM FOLDS (T_join->T_proj->T_tc) ARE MODELED AS SEED_FOLD BUT ARE NOT SEEDS. S2.D6/D7/A6/A7 are UPDATECOUNT folds copying a claimed frontier of one SCC table into another SCC table (join->proj projection dropping X; proj->tc MERGE arm). They are class=Recursive and live INSIDE the fixpoint round, driven by a claimed-* frontier, not by a lower-stratum signed frontier. Calling them SEED_FOLD(rule,delta_pos,sign,class) overloads SEED_FOLD with two distinct meanings (lower-stratum seed vs same-SCC intra-round projection fold). The vocab needs a distinct PROJECT_FOLD / MERGE_FOLD op (or a 'driver=claimed-frontier, intra-scc' attribute) so lowering knows the input vec is a claimed-* round vec, not a net_* frontier.
- G8 DEAD FRONTIER FILTERS ARE EMITTED UNCONDITIONALLY WITH NO LIVENESS ATTRIBUTE. The six S2.F* frontier filters (ir:265-290) write $net_removals:166/170/175 and $net_additions:179/183/188, which NOTHING downstream reads (tc is the top SCC; found_path publishes from the commit sweep). §5's FRONTIER_FILTER(table,sign) has no notion of a consumer; the code emits them because the schema is uniform. For byte-identity R2 must emit these dead ops, but the vocabulary cannot express 'this frontier has no consumer' — so a future optimization pass has no hook, and a naive constructor cannot tell required frontiers (S1.3/S1.4, feeding S1.5/S1.6) from dead ones (S2.F*).
- G9 COMMIT_SWEEP ORDER IS TABLE-ID, NOT STRATUM/SCC ORDER, AND THE PUBLISH TARGET IS AN ATTRIBUTE. The four COMMIT_SWEEPs emit in table-id order 4,8,13,17 (ir:291-294) = T_tc,T_join,T_edge,T_proj — which is NEITHER stratum order (edge=13 is lowest but sweeps third) NOR SCC order. §2 ln167-168 confirms sweeps are emitted downstream of BuildStratumPhases in table order. COMMIT_SWEEP(table) needs an explicit publish-view attribute (only T_tc carries `publishing found_path/2`, ir:291) AND the construction must fix table-id ordering independent of strata. The vocab as written implies stratum-ordered sweeps.
- G10 CLAIM_DRAIN DUAL-APPEND (overdelete vec AND round-frontier vec) IS NOT EXPRESSED. Inside the SCC rounds each claim-drain appends the claimed row to TWO vecs: the persistent $overdelete:62/$addition:124 (survives the round, feeds REDERIVE/frontier filters) AND the per-round $claimed_del:59/$claimed_add:121 (cleared each round, drives this round's fires). ir:136-138, 213-215. The S1 edge claim-drains (S1.1/S1.2) append to only ONE vec (no round frontier, non-recursive). §5's CLAIM_DRAIN(table,sign) has no attribute distinguishing single-target (non-recursive/pre-induction) from dual-target (in-round) drains. Lowering cannot pick byte-correctly without it.
- G11 THE `if-crossed`/`if-claimed`/`if-member`/`if-compare` GUARD NESTING IS THE OP'S SHAPE, NOT NOISE. Every UPDATECOUNT wraps its append in `if-crossed`; every claim in `if-claimed`; every check-member in `if-member`; every scan arm in `if-compare {pivot}={bound} / if-true`. These are load-bearing control structure the R2 lowering must reproduce verbatim, but §5's leaf ops (SEED_FOLD, FIXPOINT_FIRE, CLAIM_DRAIN, FRONTIER_FILTER) are named for their SEMANTIC effect and do not enumerate the guard scaffold. A generic lowering needs each op to expand to a fixed (guard-chain + body-append) template; the vocabulary should pin those templates or they drift.

## Notes

Method: read program.ir end-to-end and datalog.h flow proc end-to-end; cross-referenced every region against the ledger §2 emission order (ir:156-168), §4 predicate matrix, §5 delta schemas (with E-13/E-14 supersession), and CLAUDE.md differential invariants. All ir: line refs verified against the 296-line program.ir; all hdr: refs against the 679-line datalog.h.

Key topology confirmed from code, not assumed:
- The tc SCC is {T_tc=%table:4, T_join=%table:8, T_proj=%table:17}; T_edge=%table:13 is a lower non-recursive stratum (RuleClass kNonRecursive for both edge:add_edge and tc:edge, since the read table edge is lower). Confirmed by: T_edge uses update-count-EXPLICIT / +-nonrecursive (ir:48,52,117,122) and is NOT in either induction's fixpoint-test set (ir:127,204); T_tc/T_join/T_proj all use +/-recursive folds inside the induction rounds.
- The nonlinear self-join tc(F,X)⋈tc(X,T) pivots on X (T_join col X:9 = pivot). The two FIXPOINT_FIRE section walks read T_tc under idx_80[From,_] (finding tc(X,·) i.e. tc(X,To), delta=tc(F,X)) and idx_87[_,To] (finding tc(·,X) i.e. tc(From,X), delta=tc(X,To)). The if-compare re-tests the pivot on the scan cursor id (D1 row-binding scope stack, §2 ln174-176) — the value-keyed re-Find is gone (CLAUDE.md data-structures epoch).
- Fixpoint-schema membership predicates match §4 exactly: OVERDELETE reads kAliveAtClaim (same j>p, ir:160) / kSurvivesSoFar (same j<p, ir:169); INSERT reads kInNewSansFrontier (j>p, ir:237) / kInNewWithFrontier (j<p, ir:246). This is E-13/E-14-consistent and the F17/F18/F22 shape.

Confidence: high on the op inventory and lowering trace (direct line-by-line correspondence, no join-suppressed seed appears in the IR, confirming G1). The vocabulary_gaps are the real deliverable — G1 (seed suppression), G2 (shared kJoinPivots vec), G6 (sort-unique placement) are the three that will most directly break a naive R1->R2 identity lowering; G4 (round-as-container) and G7 (projection folds mis-typed as SEED_FOLD) are structural mis-fits in the §5 first cut that the R1 critique pass should resolve before construction.

This module exercises NO negate/crossover and NO @product, so CROSSOVER / PRODUCT_ARM ops from §5 are untested here — their gaps are out of scope for this artifact. It DOES stress: nonlinear same-SCC self-join (G1/G2/G3), multi-table SCC with projection+merge-arm folds (G7), deferred SCC frontier filters (E-17 / G8), and the full DEL/REDERIVE/ADD three-phase round structure (G4).
