# tc artifact in v3 form (typed vecs, effect sets, plan trees, dependence edges)

======================================================================
tc.drir — V3 FORM (vocabulary-v3 object model)
transitive_closure_diff.dr — FlowGraph over ^flow:198 (+ ^entry:21 ingest band)
Identity target: DeltaRelationalIR.artifacts/tcd/program.ir (296 lines),
  cited as ir:N below. All facts derived from that file + Table.h:357-433
  + the v3 spec §1-§6. This module exercises NO negate/crossover/product.
======================================================================

======================================================================
§I. VALUE NODES — TABLES  (DRTableImpl; §7.1)
======================================================================
All four are DiffTable (differential=true): split C_nr/C_r, kInI watermark.
No monotone table (even edge is @differential-fed). member_views held BY
IDENTITY (V-MEMBER-ID).

T_edge  = %table:13[u64,u64] (From:14,To:15)                     ir:24-28
  differential=true; kind=NON-RECURSIVE (lowest stratum, not in tc SCC).
  indices: idx16 full-key[From,To]                               ir:28
  member_views: {edge:add_edge} (1 identity-distinct feeder).
T_tc    = %table:4[u64,u64] (From:5,To:6)                        ir:9-16
  differential=true; kind=RECURSIVE (tc SCC). Backs found_path/2 + both
  query cursors. indices: idx7 full-key[From,To]; idx80[From,_];
  idx87[_,To]                                                    ir:13-15
  member_views: {tc:edge, tc:proj MERGE-arm} (2 identity-distinct).
T_join  = %table:8[u64,u64,u64] (X:9,From:10,To:11)              ir:17-22
  differential=true; kind=RECURSIVE. Pivot col = X (pos 0).
  indices: idx12 full-key[X,From,To]                             ir:22
  member_views: {tc(From,X)⋈_X tc(X,To)} (1 join view).
T_proj  = %table:17[u64,u64] (From:18,To:19)                     ir:30-34
  differential=true; kind=RECURSIVE. πFrom,To(T_join); recursive MERGE arm.
  Distinct model from T_tc BY IDENTITY (group_ids CSE guard — NEVER dedup).
  indices: idx20 full-key[From,To]                               ir:34

SCC (recursive) = { T_tc, T_join, T_proj }.  T_edge is a lower stratum.
RuleClass (§6.1, Stratum.cpp:291):
  edge:add_edge  -> kNonRecursive  (target T_edge ∉ SCC; explicit form)
  tc:edge        -> kNonRecursive  (reads T_edge, lower)
  tc:tc,tc       -> kRecursive     (join, target+reads share SCC)
  proj:join      -> kRecursive     (T_join,T_proj share SCC)
  tc:proj        -> kRecursive     (MERGE arm, both in SCC)

======================================================================
§II. VALUE NODES — VECTORS  (DRVecImpl; §1 typed IR values)
======================================================================
Format: Vec#N  shape/role/uniq  | def=<op> (append/clear effect) |
  use=[<ops with drain effect>] | debug_origin=(table,VectorKind).
def/use are the ONLY identity; (table,kind) is DEMOTED to debug (F-1/F-9).
use=[] = ZERO uses (a dead def; G8 hook).

--- ^entry:21 ingest band ---
V23  param  / param / multiset            ir:42
  def=<proc-param ^entry> | use=[S0.2 INGEST_FOLD(+)] | dbg=(add_edge msg,param)
V24  param  / param / multiset            ir:42
  def=<proc-param ^entry> | use=[S0.3 INGEST_FOLD(-)] | dbg=(del_edge msg,param)
V28  values / add-queue / sort-unique-at-drain   ir:43,50
  def=S0.2 INGEST_FOLD(+, T_edge) (append ir:50) |
  use=[S1.2 CLAIM_DRAIN(T_edge,+) drain ir:103] | dbg=(T_edge,add_queue)
  NB uniq: multiset at production (ir:50), sort-unique-at-drain edge to S1.2
  (vector-unique ir:102). Same physical vec; attribute lives on def→use edge.
V32  values / delete-queue / sort-unique-at-drain  ir:44,54
  def=S0.3 INGEST_FOLD(-, T_edge) (append ir:54) |
  use=[S1.1 CLAIM_DRAIN(T_edge,-) drain ir:98] | dbg=(T_edge,delete_queue)
  uniq: multiset out of ingest; sort-unique-at-drain into S1.1 (ir:97).

--- ^flow:198 lower stratum (T_edge) ---
V34  ids / overdelete-set / multiset       ir:67, appended ir:101
  def=S1.1 CLAIM_DRAIN(T_edge,-) (append ir:101) |
  use=[S1.3 FRONTIER_FILTER(T_edge,-) drain ir:107] | dbg=(T_edge,overdelete)
  NB SINGLE append (non-recursive drain, no round-frontier) — G10 single form.
V38  ids / addition-set / multiset          ir:68, appended ir:106
  def=S1.2 CLAIM_DRAIN(T_edge,+) (append ir:106) |
  use=[S1.4 FRONTIER_FILTER(T_edge,+) drain ir:111] | dbg=(T_edge,addition)
V42  values / net-removal / sort-unique-at-drain   ir:69, appended ir:110
  def=S1.3 FRONTIER_FILTER(T_edge,-) (append ir:110) |
  use=[S1.5 SEED_FOLD(tc:edge,-) drain ir:116] | dbg=(T_edge,net_removals)
  uniq: sort-unique-at-drain (vector-unique ir:115 before the seed drain).
V46  values / net-addition / sort-unique-at-drain   ir:70, appended ir:114
  def=S1.4 FRONTIER_FILTER(T_edge,+) (append ir:114) |
  use=[S1.6 SEED_FOLD(tc:edge,+) drain ir:121] | dbg=(T_edge,net_additions)
  uniq: sort-unique-at-drain (vector-unique ir:120).

--- SCC entry queues (T_tc), produced by S1.5/S1.6 AND fed back intra-SCC ---
V53  values / delete-queue / sort-unique-at-drain   ir:71
  def=[S1.5 SEED_FOLD(tc:edge,-) append ir:119; S2.D7 CHAIN_FOLD(tc:proj,-)
       append ir:181]  (TWO def edges — seed + intra-round MERGE fold) |
  use=[S2.D1 CLAIM_DRAIN(T_tc,-) drain ir:133] | dbg=(T_tc,delete_queue)
  uniq: sort-unique-at-drain (vector-unique ir:132 at S2.D1).
V57  values / add-queue / sort-unique-at-drain   ir:72
  def=[S1.6 SEED_FOLD(tc:edge,+) append ir:124; S2.R1 REDERIVE(T_tc) append
       ir:193; S2.A7 CHAIN_FOLD(tc:proj,+) append ir:258]  (THREE def edges) |
  use=[S2.A1 CLAIM_DRAIN(T_tc,+) drain ir:210] | dbg=(T_tc,add_queue)
  uniq: sort-unique-at-drain (vector-unique ir:209 at S2.A1).

--- OVERDELETE round claim-frontiers (per-SCC-table; the fixpoint-test set) ---
V59  ids / claimed-del-frontier / multiset   ir:73, cleared ir:129, appended ir:138
  def=S2.D1 CLAIM_DRAIN(T_tc,-) (clear ir:129 + append ir:138; dual-append) |
  use=[S2.D4 FIXPOINT_FIRE(dpos0,-) drain ir:156; S2.D5 FIXPOINT_FIRE(dpos1,-)
       drain ir:165; S2.D8 RETIRE(T_tc,-) drain ir:182] | dbg=(T_tc,claimed_del)
V60  ids / claimed-del-frontier / multiset   ir:74, cleared ir:130, appended ir:146
  def=S2.D2 CLAIM_DRAIN(T_join,-) (clear+append; dual) |
  use=[S2.D6 CHAIN_FOLD(proj:join,-) drain ir:174; S2.D9 RETIRE drain ir:184] |
  dbg=(T_join,claimed_del)
V61  ids / claimed-del-frontier / multiset   ir:75, cleared ir:131, appended ir:154
  def=S2.D3 CLAIM_DRAIN(T_proj,-) (clear+append; dual) |
  use=[S2.D7 CHAIN_FOLD(tc:proj,-) drain ir:178; S2.D10 RETIRE drain ir:186] |
  dbg=(T_proj,claimed_del)

--- OVERDELETE persistent overdelete-sets (survive round; REDERIVE+filters) ---
V62  ids / overdelete-set / multiset   ir:76, appended ir:137
  def=S2.D1 CLAIM_DRAIN(T_tc,-) (append ir:137; the OTHER dual-append) |
  use=[S2.R1 REDERIVE(T_tc) drain ir:190; S2.F1 FRONTIER_FILTER(T_tc,-) drain
       ir:267] | dbg=(T_tc,overdelete)
V67  ids / overdelete-set / multiset   ir:78, appended ir:145
  def=S2.D2 CLAIM_DRAIN(T_join,-) (append ir:145) |
  use=[S2.R2 REDERIVE(T_join) drain ir:194; S2.F2 FRONTIER_FILTER(T_join,-)
       drain ir:271] | dbg=(T_join,overdelete)
V73  ids / overdelete-set / multiset   ir:80, appended ir:153
  def=S2.D3 CLAIM_DRAIN(T_proj,-) (append ir:153) |
  use=[S2.R3 REDERIVE(T_proj) drain ir:198; S2.F3 FRONTIER_FILTER(T_proj,-)
       drain ir:275] | dbg=(T_proj,overdelete)

--- OVERDELETE round fire-output queues ---
V66  id+cols / delete-queue / sort-unique-at-drain   ir:77
  def=[S2.D4 FIXPOINT_FIRE(dpos0,-) append ir:164; S2.D5 FIXPOINT_FIRE(dpos1,-)
       append ir:173]  (TWO def edges — shared output queue, G2) |
  use=[S2.D2 CLAIM_DRAIN(T_join,-) drain ir:141] (NEXT round) |
  dbg=(T_join,delete_queue)
  uniq: multiset out of fire; sort-unique-at-drain into S2.D2 (ir:140). Same
  physical vec is multiset at production and sort-unique into the next drain.
V72  values / delete-queue / sort-unique-at-drain   ir:79
  def=S2.D6 CHAIN_FOLD(proj:join,-) (append ir:177) |
  use=[S2.D3 CLAIM_DRAIN(T_proj,-) drain ir:149] | dbg=(T_proj,delete_queue)
  uniq: sort-unique-at-drain (ir:148).
  (V53 above also carries S2.D7's append — the tc delQ back-edge.)

--- REDERIVE outputs into INSERT add-queues ---
V111 id+cols / add-queue / sort-unique-at-drain   ir:81
  def=[S2.R2 REDERIVE(T_join) append ir:197; S2.A4 FIXPOINT_FIRE(dpos0,+)
       append ir:241; S2.A5 FIXPOINT_FIRE(dpos1,+) append ir:250] (THREE defs)|
  use=[S2.A2 CLAIM_DRAIN(T_join,+) drain ir:218] | dbg=(T_join,add_queue)
  uniq: sort-unique-at-drain (ir:217).
V116 values / add-queue / sort-unique-at-drain   ir:82
  def=[S2.R3 REDERIVE(T_proj) append ir:201; S2.A6 CHAIN_FOLD(proj:join,+)
       append ir:254] (TWO defs) |
  use=[S2.A3 CLAIM_DRAIN(T_proj,+) drain ir:226] | dbg=(T_proj,add_queue)
  uniq: sort-unique-at-drain (ir:225).
  (V57 above carries S2.R1's tc addQ append — REDERIVE(T_tc) back-edge.)

--- INSERT round claim-frontiers (fixpoint-test set) ---
V121 ids / claimed-add-frontier / multiset  ir:83, cleared ir:206, appended ir:215
  def=S2.A1 CLAIM_DRAIN(T_tc,+) (clear+append; dual) |
  use=[S2.A4 FIXPOINT_FIRE(dpos0,+) drain ir:233; S2.A5 FIXPOINT_FIRE(dpos1,+)
       drain ir:242; S2.A8 RETIRE(T_tc,+) drain ir:259] | dbg=(T_tc,claimed_add)
V122 ids / claimed-add-frontier / multiset  ir:84, cleared ir:207, appended ir:223
  def=S2.A2 CLAIM_DRAIN(T_join,+) (clear+append; dual) |
  use=[S2.A6 CHAIN_FOLD(proj:join,+) drain ir:251; S2.A9 RETIRE drain ir:261] |
  dbg=(T_join,claimed_add)
V123 ids / claimed-add-frontier / multiset  ir:85, cleared ir:208, appended ir:231
  def=S2.A3 CLAIM_DRAIN(T_proj,+) (clear+append; dual) |
  use=[S2.A7 CHAIN_FOLD(tc:proj,+) drain ir:255; S2.A10 RETIRE drain ir:263] |
  dbg=(T_proj,claimed_add)

--- INSERT persistent addition-sets (survive; deferred + filters) ---
V124 ids / addition-set / multiset   ir:86, appended ir:214
  def=S2.A1 CLAIM_DRAIN(T_tc,+) (append ir:214) |
  use=[S2.F4 FRONTIER_FILTER(T_tc,+) drain ir:279] | dbg=(T_tc,addition)
V128 ids / addition-set / multiset   ir:87, appended ir:222
  def=S2.A2 CLAIM_DRAIN(T_join,+) (append ir:222) |
  use=[S2.F5 FRONTIER_FILTER(T_join,+) drain ir:283] | dbg=(T_join,addition)
V133 ids / addition-set / multiset   ir:88, appended ir:230
  def=S2.A3 CLAIM_DRAIN(T_proj,+) (append ir:230) |
  use=[S2.F6 FRONTIER_FILTER(T_proj,+) drain ir:287] | dbg=(T_proj,addition)

--- DEFERRED frontier outputs — ALL DEAD (zero uses; G8 hook) ---
V166 values / net-removal / multiset  ir:89  def=S2.F1(append ir:270) | use=[] DEAD
V170 id+cols/ net-removal / multiset  ir:90  def=S2.F2(append ir:274) | use=[] DEAD
V175 values / net-removal / multiset  ir:91  def=S2.F3(append ir:278) | use=[] DEAD
V179 values / net-addition/ multiset  ir:92  def=S2.F4(append ir:282) | use=[] DEAD
V183 id+cols/ net-addition/ multiset  ir:93  def=S2.F5(append ir:286) | use=[] DEAD
V188 values / net-addition/ multiset  ir:94  def=S2.F6(append ir:290) | use=[] DEAD
  (T_tc is the top SCC; found_path publishes from the COMMIT sweep, not a
   frontier. No higher stratum drains these — every use list is empty.)

======================================================================
§III. OP NODES WITH INSTANTIATED EFFECT SETS  (§2)
======================================================================
Notation: counter±(T,K)=UPDATECOUNT RMW; drain/append/clear(Vn) on Vecs;
read(T,Pred)=flags:read bundle; write(T,flags); kInI reads = FROZEN.
Preds expand per Table.h (§2): NetDeleted=kDel&&!kAdd (Table.h:405);
NetAdded=kAdd&&!kDel&&!kInI (Table.h:429); AliveAtClaim=kInI&&(!kDel||kDelNow)
(Table.h:376); SurvivesSoFar=kInI&&!kDel (Table.h:370);
InNewSansFrontier=(kInI&&!kDel)||(kAdd&&!kAddNow) (Table.h:386);
InNewWithFrontier=InNew=(kInI&&!kDel)||kAdd (Table.h:363,382);
RecursivelySupported=C_r>0 (Table.h:399).

--- ^entry:21 ingest band ---
S0.2 INGEST_FOLD(T_edge, +, kNonRecursive, explicit)             ir:47-50
  Effects: vector:drain(V23); counter+(T_edge,NonRecursive)[explicit form
  update-count-explicit]; flags:read(T_edge,kInI)[crossing]; vector:append(V28).
  body: no ACCESS (single-atom explicit fold; delta IS the row).
S0.3 INGEST_FOLD(T_edge, -, kNonRecursive, explicit)             ir:51-54
  Effects: vector:drain(V24); counter-(T_edge,NonRecursive)[explicit];
  flags:read(kInI); vector:append(V32).
  (S0.2/S0.3 run par ir:46; then clear V23/V24, tail-call ^flow.)

--- ^flow:198 : S1 lower stratum (T_edge non-recursive) ---
S1.1 CLAIM_DRAIN(T_edge, -, form=single-pass)                    ir:98-101
  Effects: vector:drain(V32); flags:read(T_edge,C_nr) + flags:write(T_edge,
  kDel|kDelNow) [gate TryClaimDel: proceeds iff C_nr<=0 — MANDATORY DATA,F17];
  vector:append(V34). SINGLE append (form=single-pass → no round-frontier; G10).
S1.2 CLAIM_DRAIN(T_edge, +, form=single-pass)                    ir:103-106
  Effects: vector:drain(V28); flags:read(Total)+flags:write(kAdd|kAddNow)
  [gate TryClaimAdd: Total>0]; vector:append(V38). single append.
S1.3 FRONTIER_FILTER(T_edge, -, deferral=immediate)              ir:107-110
  Effects: vector:drain(V34); flags:read(T_edge,NetDeleted={kDel,kAdd});
  vector:append(V42). deferral=immediate (non-recursive lower table; no kAdd
  writer chain follows).
S1.4 FRONTIER_FILTER(T_edge, +, deferral=immediate)              ir:111-114
  Effects: vector:drain(V38); flags:read(T_edge,NetAdded={kAdd,kDel,kInI});
  vector:append(V46).
S1.5 SEED_FOLD(rule=tc:edge, delta_pos=0, sign=-, class=NonRecursive) ir:116-119
  Effects: vector:drain(V42); counter-(T_tc,NonRecursive); flags:read(T_tc,kInI)
  [crossing]; vector:append(V53). body: Loop(V42, Fold(T_tc,-,NonRec,{From,To}))
  — single-atom, no ACCESS.
S1.6 SEED_FOLD(rule=tc:edge, delta_pos=0, sign=+, class=NonRecursive) ir:121-124
  Effects: vector:drain(V46); counter+(T_tc,NonRecursive); flags:read(kInI);
  vector:append(V57). body: Loop(V46, Fold(T_tc,+,NonRec,{From,To})).

--- S2 OVERDELETE FixpointRound region (phase=OVERDELETE) ---
REGION FIXPOINT_ROUND(scc, phase=OVERDELETE)                     ir:125-201
  test_vecs (re-exported) = {V59, V60, V61}                      ir:127
  body_ops = [S2.D1..S2.D10]; output_ops = [S2.R1..S2.R3]. region effect = ∪ children.

S2.D1 CLAIM_DRAIN(T_tc, -, form=in-round)                        ir:132-138
  Effects: vector:clear(V59); vector:drain(V53); flags:read(T_tc,C_nr) +
  flags:write(T_tc,kDel|kDelNow) [TryClaimDel C_nr<=0, F17]; vector:append(V62)
  AND vector:append(V59) [DUAL-APPEND, G10 — two append effect entries].
S2.D2 CLAIM_DRAIN(T_join, -, form=in-round)                      ir:140-146
  Effects: clear(V60); drain(V66); read(C_nr)+write(kDel|kDelNow); append(V67)
  AND append(V60) [dual].
S2.D3 CLAIM_DRAIN(T_proj, -, form=in-round)                      ir:148-154
  Effects: clear(V61); drain(V72); read(C_nr)+write(kDel|kDelNow); append(V73)
  AND append(V61) [dual].
S2.D4/S2.D5 FIXPOINT_FIRE(join=tc⋈tc, sign=-, class=Recursive, arms=[a0,a1]) ir:156-173
  ONE op; OWNS input frontier V59 + output queue V66 (G2). Two arms:
  Effects (union): vector:drain(V59); [arm0] flags:read(T_tc,AliveAtClaim=
  {kInI,kDel,kDelNow}); [arm1] flags:read(T_tc,SurvivesSoFar={kInI,kDel});
  counter-(T_join,Recursive); flags:read(T_join,kInI); vector:append(V66)
  [ONE shared output, G2].
  arm0 (delta_pos=0, tc(F,X) delta) PlanTree:                    ir:156-164
    Loop(V59{From:78,X:79},
      Access(T_tc, bound={X:79}, index=idx80[From,_], pivot-col=X:82,
             pred=AliveAtClaim, polarity=member, lowering=keyed-section-walk,
        child= Fold(T_join, -, Recursive, {X:79,From:78,To:83})))
    guards: scan idx80 → if-compare {X:82}={X:79} → if-member AliveAtClaim →
    if-crossed → append V66.
  arm1 (delta_pos=1, tc(X,T) delta) PlanTree:                    ir:165-173
    Loop(V59{X:85,To:86},
      Access(T_tc, bound={X:85}, index=idx87[_,To], pivot-col=X:90,
             pred=SurvivesSoFar, polarity=member, lowering=keyed-section-walk,
        child= Fold(T_join, -, Recursive, {X:85,From:89,To:86})))
  (Two PlanTrees under ONE op, differing ONLY in index/pivot-col — G3.)
S2.D6 CHAIN_FOLD(source-claimed=V60 → T_proj, -, Recursive, proj={From,To}) ir:174-177
  Effects: vector:drain(V60); counter-(T_proj,Recursive); flags:read(T_proj,kInI);
  vector:append(V72). No ACCESS body (pure projection dropping X). DISTINCT from
  SEED_FOLD (G7): delta is a CLAIMED-* round frontier, not a lower net_*.
S2.D7 CHAIN_FOLD(source-claimed=V61 → T_tc, -, Recursive, proj={From,To}) ir:178-181
  Effects: vector:drain(V61); counter-(T_tc,Recursive); flags:read(kInI);
  vector:append(V53) [MERGE arm copy T_proj→T_tc; the tc delQ back-edge].
S2.D8 RETIRE(T_tc, -, after=fires)   drain(V59); write clear kDelNow   ir:182-183
S2.D9 RETIRE(T_join, -, after=fires) drain(V60); write clear kDelNow   ir:184-185
S2.D10 RETIRE(T_proj, -, after=fires) drain(V61); write clear kDelNow  ir:186-187
  (RETIRE band at round-body tail, table-id order — G5.)

--- REDERIVE (hosted in OVERDELETE region.output) ---
S2.R1 REDERIVE(T_tc)   drain(V62); read(T_tc,RecursivelySupported=C_r>0); append(V57)  ir:190-193
S2.R2 REDERIVE(T_join) drain(V67); read(T_join,C_r>0); append(V111)                    ir:194-197
S2.R3 REDERIVE(T_proj) drain(V73); read(T_proj,C_r>0); append(V116)                    ir:198-201
  (A COUNTER READ, not a search.)

--- S2 INSERT FixpointRound region (phase=INSERT) ---
REGION FIXPOINT_ROUND(scc, phase=INSERT)                         ir:202-290
  test_vecs = {V121, V122, V123}  ir:204; body_ops=[S2.A1..A10]; output_ops=[S2.F1..F6].

S2.A1 CLAIM_DRAIN(T_tc, +, form=in-round)                        ir:209-215
  Effects: clear(V121); drain(V57); flags:read(Total)+write(kAdd|kAddNow)
  [TryClaimAdd Total>0, F17]; append(V124) AND append(V121) [dual].
S2.A2 CLAIM_DRAIN(T_join, +, form=in-round)  clear(V122);drain(V111);read(Total)+write(kAdd|kAddNow);append(V128)+append(V122)  ir:217-223
S2.A3 CLAIM_DRAIN(T_proj, +, form=in-round)  clear(V123);drain(V116);read(Total)+write(kAdd|kAddNow);append(V133)+append(V123)  ir:225-231
S2.A4/S2.A5 FIXPOINT_FIRE(join=tc⋈tc, sign=+, class=Recursive, arms=[a0,a1]) ir:233-250
  ONE op; owns V121 + V111 (G2).
  Effects: vector:drain(V121); [arm0] flags:read(T_tc,InNewSansFrontier=
  {kInI,kDel,kAdd,kAddNow}); [arm1] flags:read(T_tc,InNewWithFrontier=InNew=
  {kInI,kDel,kAdd}); counter+(T_join,Recursive); flags:read(T_join,kInI);
  vector:append(V111).
  arm0 (delta_pos=0) PlanTree:                                   ir:233-241
    Loop(V121{From:138,X:139},
      Access(T_tc, bound={X:139}, index=idx80[From,_], pivot-col=X:141,
             pred=InNewSansFrontier, polarity=member, lowering=keyed-section-walk,
        child= Fold(T_join,+,Recursive,{X:139,From:138,To:142})))
  arm1 (delta_pos=1) PlanTree:                                   ir:242-250
    Loop(V121{X:144,To:145},
      Access(T_tc, bound={X:144}, index=idx87[_,To], pivot-col=X:148,
             pred=InNewWithFrontier, polarity=member, lowering=keyed-section-walk,
        child= Fold(T_join,+,Recursive,{X:144,From:147,To:145})))
S2.A6 CHAIN_FOLD(V122 → T_proj, +, Recursive, {From,To})  drain(V122);counter+(T_proj,Rec);read(kInI);append(V116)  ir:251-254
S2.A7 CHAIN_FOLD(V123 → T_tc, +, Recursive, {From,To})    drain(V123);counter+(T_tc,Rec);read(kInI);append(V57)[tc addQ back-edge]  ir:255-258
S2.A8 RETIRE(T_tc, +, after=fires)   drain(V121); write clear kAddNow  ir:259-260
S2.A9 RETIRE(T_join, +, after=fires) drain(V122); write clear kAddNow  ir:261-262
S2.A10 RETIRE(T_proj, +, after=fires) drain(V123); write clear kAddNow ir:263-264

--- DEFERRED frontier filters (hosted in INSERT region.output; E-17) ---
S2.F1 FRONTIER_FILTER(T_tc, -, deferral=add-loop-output)  drain(V62);read(T_tc,NetDeleted={kDel,kAdd});append(V166)DEAD  ir:267-270
S2.F2 FRONTIER_FILTER(T_join, -, add-loop-output)  drain(V67);read NetDeleted;append(V170)DEAD ir:271-274
S2.F3 FRONTIER_FILTER(T_proj, -, add-loop-output)  drain(V73);read NetDeleted;append(V175)DEAD ir:275-278
S2.F4 FRONTIER_FILTER(T_tc, +, add-loop-output)    drain(V124);read NetAdded;append(V179)DEAD ir:279-282
S2.F5 FRONTIER_FILTER(T_join, +, add-loop-output)  drain(V128);read NetAdded;append(V183)DEAD ir:283-286
S2.F6 FRONTIER_FILTER(T_proj, +, add-loop-output)  drain(V133);read NetAdded;append(V188)DEAD ir:287-290
  deferral=add-loop-output for ALL (SCC tables): a re-added row's kAdd must be
  final before the − filter reads NetDeleted (V-DEFER; §4.3).

--- S3 commit band ---
S3.1 COMMIT_SWEEP(T_tc, flavor=differential, publish-target=found_path/2)  ir:291
  Effects: flags:read/write(T_tc,kInI)[Commit sets kInI:=Present];
  flags:write(clear kDel|kAdd|kDelNow|kAddNow); table:compact. Publishes was!=now.
S3.2 COMMIT_SWEEP(T_join, differential, no publish)  ir:292
S3.3 COMMIT_SWEEP(T_edge, differential, no publish)  ir:293
S3.4 COMMIT_SWEEP(T_proj, differential, no publish)  ir:294
  ORDER = table-id 4,8,13,17 (NEITHER stratum NOR SCC order; V-SWEEP-ORDER, G9).

======================================================================
§IV. DEPENDENCE EDGES  (§4; derived from §2 effect intersection)
======================================================================
Each edge: X→Y  KIND  on RESOURCE  (why). kInI reads generate NO WAR/WAW
(frozen; §2) — only a RAW against COMMIT at the band boundary.

--- ingest → S1 (RAW on the SCC-external add/del queues) ---
S0.2→S1.2  RAW on V28 (append→drain);   S0.3→S1.1  RAW on V32
--- S1 intra-stratum chains (RAW on the produced Vec) ---
S1.1→S1.3  RAW on V34 (overdelete-set append→drain);  S1.2→S1.4  RAW on V38
S1.3→S1.5  RAW on V42 (net-removal append→drain);      S1.4→S1.6  RAW on V46
--- S1 → SCC entry (RAW on tc queues; the stratum-order edge) ---
S1.5→S2.D1  RAW on V53 (tc delQ append→drain);  S1.6→S2.A1  RAW on V57
--- claim gate flag hazards within a table (WAR on kDel/kAdd) ---
S1.1→S1.3  WAR on T_edge.kDel (claim writes kDel; NetDeleted reads kDel)
S1.2→S1.4  WAR on T_edge.kAdd
--- OVERDELETE round internal ---
S2.D1→S2.D4, S2.D1→S2.D5  RAW on V59 (dual-append frontier→fire drain) [§4.5]
S2.D1→S2.D8  RAW on V59 (frontier→retire drain)
S2.D4→S2.D8  WAR on T_tc.kDelNow (arm0 reads AliveAtClaim{kDelNow}; retire clears
             kDelNow) — retire-after-fires, §4.2 / V-RETIRE-AFTER
S2.D5→S2.D8  NO kDelNow WAR: arm1 reads SurvivesSoFar={kInI,kDel} (NOT kDelNow).
             The retire-after WAR is arm0-ONLY. (Precise graph fact.)
S2.D4/D5→S2.D2  RAW on V66 (shared fire output→next-round T_join claim drain)
S2.D2→S2.D6  RAW on V60;  S2.D6→S2.D3  RAW on V72;  S2.D3→S2.D7  RAW on V61
S2.D7→S2.D1  RAW on V53 (tc delQ back-edge→tc claim drain, next round) [feedback]
S2.D2→S2.D9, S2.D3→S2.D10  RAW on V60/V61 (frontier→retire)
--- OVERDELETE → REDERIVE (§4.4) ---
S2.D4/D5→S2.R2  RAW on T_join.C_r (fire counter- writes C_r; REDERIVE reads C_r>0)
S2.D6→S2.R3  RAW on T_proj.C_r;  S2.D7→S2.R1  RAW on T_tc.C_r
S2.R1→S2.A1  RAW on V57;  S2.R2→S2.A2  RAW on V111;  S2.R3→S2.A3  RAW on V116  [§4.4]
--- INSERT round internal (mirror of OVERDELETE) ---
S2.A1→S2.A4/A5  RAW on V121; S2.A1→S2.A8 RAW on V121 (→retire)
S2.A4→S2.A8  WAR on T_tc.kAddNow (arm0 reads InNewSansFrontier{kAddNow}) — §4.2
S2.A5→S2.A8  NO kAddNow WAR: arm1 reads InNewWithFrontier=InNew={kInI,kDel,kAdd}.
S2.A4/A5→S2.A2  RAW on V111 (shared output→next-round join claim)
S2.A2→S2.A6→S2.A3→S2.A7→S2.A1  RAW chain on V122/V116/V123/V57 (feedback)
--- INSERT → deferred filters (§4.3 E-17) ---
S2.A1→S2.F1  RAW on T_tc.kAdd (claim writes kAdd; NetDeleted reads kAdd) — forces
             the − filter INTO INSERT.output after ALL kAdd writers
S2.A1→S2.F4  RAW on V124 (addition-set append→drain) + RAW on T_tc.kAdd
S2.F* all:   outputs V166..V188 have NO out-edge — terminal/DEAD
--- COMMIT (last; WAR against every flag reader, WAW on kInI) ---
every flags:read(T)→COMMIT_SWEEP(T)  WAR on T's flag-set
S2.F1→S3.1  WAR on T_tc flags; COMMIT writes kInI:=Present (WAW vs prior kInI).
COMMIT order 4,8,13,17 pinned INDEPENDENT of dep graph (no inter-sweep Vec/flag
edge forces it — a pure pinned WAW-free ordering; V-SWEEP-ORDER).

======================================================================
§V. PINNED IDENTITY LINEARIZATION  (§4.6; = program.ir emission order)
======================================================================
A topological order of §IV. Certified by V-LINEAR. Region-internal orders are
the region's own checked linearizations (black boxes to the outer linearizer).

^entry:21 :  [S0.2 ‖ S0.3]  (par; ir:46)
^flow:198 :
  epoch-bump @22+=1 (ir:96)
  S1.1, S1.2, S1.3, S1.4, S1.5, S1.6                              ir:97-124
  REGION OVERDELETE (ir:125-201):
    body:   S2.D1, S2.D2, S2.D3, S2.D4, S2.D5, S2.D6, S2.D7, S2.D8, S2.D9, S2.D10
            (fires before retire band; §4.2)
    output: S2.R1, S2.R2, S2.R3
  REGION INSERT (ir:202-290):
    body:   S2.A1, S2.A2, S2.A3, S2.A4, S2.A5, S2.A6, S2.A7, S2.A8, S2.A9, S2.A10
    output: S2.F1, S2.F2, S2.F3, S2.F4, S2.F5, S2.F6
  S3.1, S3.2, S3.3, S3.4  (commit band, table-id order)
  return-true

Forced-vs-pinned notes:
 - S1.1<S1.3<S1.5 forced by V34/V42 RAW; S1.5<S2.D1 forced by V53 RAW.
 - Within OVERDELETE body the claim-drain order D1,D2,D3 is PINNED (no Vec edge
   forces D1 before D2 in round 0 — the round-boundary V66/V72 RAWs are next-
   round). WAW-free ⇒ table-id order is the pinned tiebreak.
 - REDERIVE strictly between rounds forced by C_r RAW (in) + addQ RAW (out).
 - COMMIT last forced by WAR against all flag readers.

======================================================================
§VI. VALIDATOR CHECKLIST — EVALUATED ON THIS ARTIFACT  (§5)
======================================================================
V-XOVER-ONE (B-3.1):  N/A-PASS. No negate ⇒ zero CROSSOVERs; POS vacuous, NEG holds.
V-PROD-MONO (B-3.2):  N/A-PASS. No PRODUCT_ARM.
V-PROD-CLASS (B-3.2): N/A-PASS. No product Fold leaf.
V-RETIRE-AFTER (B-3.3): PASS. Same-round fire→retire WAR present & respected:
  S2.D4→S2.D8 WAR(T_tc.kDelNow), D8 at round tail (ir:182 > ir:164);
  S2.A4→S2.A8 WAR(T_tc.kAddNow), A8 at tail (ir:259 > ir:241). NEG: no RETIRE
  precedes any same-round fire. sub-check (sort-unique per-vec): PASS — the 10
  sort-unique-at-drain vecs (V28,V32,V42,V46,V53,V57,V66,V72,V111,V116) each
  have VectorUnique immediately before their drain (ir:97,102,115,120,132,209,
  140,148,217,225 = exactly the 10 G6 sites); every multiset vec has none.
  Byte-matches ir.
V-SEED-DRAIN (B-3.4): N/A-PASS (no CROSSOVER/PRODUCT_ARM). The analogous NON-
  recursive seed→drain RAW (S1.5→S2.D1 on V53, S1.6→S2.A1 on V57) is present &
  respected, but this validator is scoped to crossover/product; vacuous here.
V-MEMBER-ID (B-3.5): PASS. T_tc.member_views={tc:edge, tc:proj}=2 identity-
  distinct (POS |list|==2). T_proj is a SEPARATE model from T_tc by identity
  though structurally πFrom,To equal (group_ids CSE guard) — NEG: not
  structurally deduped. T_join/T_edge lists=1 each.
V-CLAIM-GATE (F17): PASS. Every CLAIM_DRAIN carries its gate as DATA: S1.1/
  S2.D1/D2/D3 = TryClaimDel(C_nr<=0); S1.2/S2.A1/A2/A3 = TryClaimAdd(Total>0).
  NEG: no del drain gates on Total, no add drain gates on C_nr, none gate-less.
V-NEG-CTX (F18/F-5): N/A-PASS. No NEGATE_GATE op.
V-ONE-FOLD (F-6): PASS. Each PlanTree has exactly one Fold leaf: D4.arm0,
  D4.arm1, A4.arm0, A4.arm1 each terminate in ONE Fold(T_join); CHAIN_FOLD/
  SEED_FOLD bodies are a single Fold. NEG: no section walk with two folds.
V-READY (F-6): PASS. Every op reads only Vecs/tables from a lower-or-same SCC.
  T_edge (lower) feeds T_tc seeds — lower→higher, legal; intra-SCC fires read
  T_tc/T_join/T_proj (same SCC). NEG: no RAW edge FROM a strictly-higher SCC.
V-R3-DERIVER (F-6,R3): DORMANT. No aggregate table.
V-SEED-SUP (§6.4): PASS — THE CENTERPIECE. tc(F,T):tc(F,X),tc(X,T) has ALL
  sides in the SCC ⇒ NO lower position ⇒ §6.2 emits ZERO SEED_FOLDs into
  T_join; §6.3 emits its FIXPOINT_FIRE (S2.D4/D5, S2.A4/A5). POS: all-same-SCC
  join has ≥1 FIXPOINT_FIRE (2 arms) + 0 SEED_FOLDs into T_join — CONFIRMED (no
  edge→T_join seed anywhere in ir). NEG: no SEED_FOLD delta is a lower frontier
  feeding T_join — CONFIRMED (V42/V46 feed T_tc via S1.5/S1.6, never T_join).
  G1 structurally impossible here.
V-DEFER (§4.3): PASS. SCC tables' both-sign filters (S2.F1..F6) hosted in INSERT
  region.output (ir:265-290, after add loop quiesces). Non-recursive T_edge
  filters (S1.3/S1.4) float immediately after their drains (deferral=immediate).
  Derived from the kAdd-writer RAW chain, not a flag.
V-LINEAR (§4.6): PASS. §V pinned order is a topo sort of §IV: every RAW respected
  (no op precedes a producer it reads); no RETIRE precedes a same-round fire; no
  drain precedes its seed. Region-internal orders self-certify.

======================================================================
§VII. §9 REWRITE-TEST LIST — EVALUATED AS GRAPH FACTS
======================================================================
(a) FRONTIER-FILTER / CLAIM-DRAIN FUSION — which fuse, naming permitting edges:
  Fusion legal when the filter's drain-source Vec is the SAME Vec the claim
  appends AND no intervening op reads/writes the shared flag — a single RAW
  chain claim→filter with no third party on the flag set.
  • S1.1 CLAIM_DRAIN(T_edge,-) ⋈ S1.3 FRONTIER_FILTER(T_edge,-): FUSIBLE.
    Permitting edges: RAW(V34) S1.1→S1.3 + WAR(T_edge.kDel) S1.1→S1.3; NOTHING
    else drains V34; deferral=immediate (no kAdd writer between). Classic edge-
    frontier fusion.
  • S1.2 ⋈ S1.4 (T_edge,+): FUSIBLE, symmetric — RAW(V38)+WAR(T_edge.kAdd), V38
    single-use, deferral=immediate.
  • SCC filters (S2.D1⋈S2.F1, S2.A1⋈S2.F4, …): NOT FUSIBLE. S2.F1's source V62
    is ALSO drained by REDERIVE (S2.R1), and the − filter is deferral=add-loop-
    output — the RAW(T_tc.kAdd) chain from every INSERT writer (S2.A1,…) forces
    S2.F1 past the whole INSERT round (§4.3). The intervening kAdd writers (a
    third party on the flag) block fusion.
  NET: 2 fusible sites (S1.1/S1.3, S1.2/S1.4) permitted by single-use RAW +
  immediate deferral; 6 non-fusible by deferral + shared-source (V62/V67/V73
  each dual-drained by REDERIVE and a filter).

(b) G8 DEAD FILTERS as zero-use defs — six Vecs whose def is a FRONTIER_FILTER
    and whose use list is []:
    V166 (S2.F1, T_tc net-removal), V170 (S2.F2, T_join), V175 (S2.F3, T_proj),
    V179 (S2.F4, T_tc net-addition), V183 (S2.F5, T_join), V188 (S2.F6, T_proj).
  DCE keys on |uses|==0. R2 still emits them for identity (V-LINEAR keeps them
  in pinned order). Contrast LIVE lower-table filters V42/V46 (used by S1.5/
  S1.6): the liveness distinction is a Vec-use-edge fact, not a suppression flag.

(c) MONOTONE-SIDE GATE ELISION SITES: NONE. Every table is differential (§I) —
  no monotone side whose InNew is trivially true, hence no CHECKMEMBER a
  monotone-elision pass could drop. The join's same-SCC sides are BOTH T_tc
  (differential), so every fire ACCESS carries a genuine frontier-flavored pred
  (AliveAtClaim/SurvivesSoFar/InNewSansFrontier/InNewWithFrontier) that CANNOT
  be elided — load-bearing exactly-once gates, not trivially-true monotone
  reads. (Contrast d5rn: edge/src monotone ⇒ the InNew gates at d5rn ir:251/340
  are the elision candidates.) Zero elision sites IS the fact: this fixture is
  the two-differential-side complement d5rn lacked.

(d) WHERE AN ACCESS-PLAN TREE WOULD CHANGE UNDER A SEEK/WCOJ LOWERING:
  Four FIXPOINT_FIRE PlanTrees (S2.D4.arm0/arm1, S2.A4.arm0/arm1); each is
  Loop(delta)→Access(keyed-section-walk over idx80/idx87)→Fold.
  • Access.lowering flips keyed-section-walk → seek (reserved D5 value): the
    scan-index+if-compare pair (e.g. ir:157-158) becomes a seekable-iterator
    advance keyed on the pivot, collapsing the if-compare re-test into the seek
    predicate. The carried (index-id, pivot-col) data (idx80/X:82, idx87/X:90)
    is exactly what a seek needs — no new attribute required.
  • The self-join's TWO PlanTrees (arm0 idx80[From,_], arm1 idx87[_,To]) are the
    WCOJ joint-ordering decision point: a WCO plan would REORDER the two Access
    nodes into a single variable-ordering (bind X first, intersect both tc
    sections) rather than two independent delta-driven walks. That changes TREE
    SHAPE (two sibling Loops → one nested variable-elimination spine) — exactly
    why §3/F-7 makes bodies access-plan TREES, so the tree is the object a WCOJ
    pass rewrites.
  • Lower-table seeds (S1.5/S1.6) and CHAIN_FOLDs (S2.D6/D7/A6/A7) have NO ACCESS
    (single Fold leaf) — seek/WCOJ CANNOT touch them; Loop→Fold is invariant
    under join-ordering.
  NET: 4 fire trees seek/WCOJ-mutable; 6 fold-only trees invariant.

## Spec-gaps flagged (resolved by v3-spec §A)

- FIXPOINT_FIRE arm-count ambiguity: §2.1's op is FIXPOINT_FIRE(join,sign,arms:[Arm]) and §6.3 says 'emit ONE Sigma-term arm per same-stratum position i'. For tc⋈tc the two same-stratum body positions ARE both tc(F,X) and tc(X,T), giving 2 arms (correct here). But the spec never states the arm-COUNT = number of same-stratum atoms bearing a delta; for an n-way self-join over one recursive table this is n arms all over the SAME frontier vec, and V-ONE-FOLD is per-arm not per-op. Nothing in §5 asserts |arms| == count of same-SCC delta positions (a V-FIRE-ARMS validator is missing).
- Per-arm effect refinement is not expressible: §2.1 gives FIXPOINT_FIRE ONE effect set 'union over arms', but arm0 reads AliveAtClaim (touches kDelNow) while arm1 reads SurvivesSoFar (does NOT touch kDelNow). The retire-after-fires WAR (§4.2) is therefore arm0-ONLY on T_tc.kDelNow; the union effect set over-approximates and cannot say which arm creates the hazard. V-RETIRE-AFTER checks 'every same-round fire→retire WAR', but with a unioned effect set it cannot distinguish an arm that genuinely reads kDelNow from one that does not — the dependence graph loses precision (conservatively still correct, but the spec's 'DERIVED from effect sets' claim is weaker than stated for multi-arm ops).
- Vec element_shape lacks an id+cols-vs-values-vs-ids discriminator grounded in the IR text: the spec's ElementShape {values, ids, id+cols} is asserted per-vec, but program.ir vectors are typed only by column signature (<u64,u64> vs <u64,u64,u64>) — nothing in the emitted IR distinguishes a 'values' tuple from an 'ids' row-id vector (both are <u64...>). The shape is a semantic annotation the constructor must ATTACH, not read off the IR; for the claim-frontier vecs (V59 etc) 'ids' vs the fire-output 'id+cols' (V66) is a constructor decision with no IR witness. If wrong it is invisible to a byte-identity gate (they lower to the same <u64...> vector-define), so V-OLD-EQUIV cannot catch a shape misassignment. A shape-consistency validator against the def op's role is needed but unspecified.
- CHAIN_FOLD's V53/V57 back-edge multiplicity vs V-SEED-SUP scope: V53 has TWO def edges (S1.5 seed + S2.D7 CHAIN_FOLD) and V57 has THREE (S1.6 seed + S2.R1 REDERIVE + S2.A7 CHAIN_FOLD). §1's def(Vec) says 'the single op that PRODUCES the vec's contents this band', but these queue vecs are legitimately multi-def (a seed def in S1 AND an intra-round fold def in S2). The spec's 'single def edge = identity' (§1) contradicts the observed multi-append queue vecs; the round vecs are excused ('cleared then re-appended each round') but V53/V57 are NOT cleared between S1 and S2 — they accumulate across the stratum boundary. The def/use model needs an explicit multi-def (accumulating-queue) vec category distinct from the clear+reappend round vec; §1 as written under-specifies it.
- COMMIT_SWEEP order is declared pinned-table-id-order (G9/V-SWEEP-ORDER) with NO dependence edge forcing it, yet §4.6 says 'PinnedOrder is a topological sort of DepGraph'. A pinned order with no forcing edges is under-constrained: V-LINEAR (topo-sort check) is satisfied by ANY permutation of the four commit sweeps, so it cannot certify the specific 4,8,13,17 order that byte-identity requires. The spec needs either synthetic WAW-ordering edges among sweeps or a separate V-SWEEP-ORDER assertion pinning table-id order as data — §5 lists V-SWEEP-ORDER but §4.6's 'checked linearization is a topo sort' does not give it teeth for order-free op sets.
- Monotone-side gate ELISION has no representation as an op attribute in this all-differential fixture, so the spec's elision hook is untestable here: §7(c) shows zero elision sites, but the spec provides no Access-level 'gate-elidable-because-monotone' flag anywhere — the elision decision lives implicitly in whether pred resolves to a trivially-true monotone InNew. A future monotone-elision pass has no attribute to key on (the Access.pred is just kInNew whether the side is monotone or differential). d5rn would exercise it; tc cannot, and the vocabulary has no marker distinguishing an elidable monotone gate from a load-bearing differential one.