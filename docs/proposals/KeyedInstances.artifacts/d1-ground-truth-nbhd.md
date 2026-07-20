======================================================================
COMMITTED AT THE §19 CHECKPOINT (2026-07-20, tip 1aaca896). The
orchestrator's ground-truth note for demand_neighborhood_witness's REAL
flat dumps (GT-1..GT-6), verbatim from the session scratchpad
(nbhd-dumps/GROUND-TRUTH.md), plus appendices embedding the dump bytes
and the R-DIFF donor probe. Emission provenance: frozen tip binary
1aaca896 debug; 3-run deterministic (1 hash); debug==release
byte-identical on all four surfaces.
======================================================================

# Orchestrator ground-truth note — demand_neighborhood_witness real dumps
(2026-07-19, frozen tip binary 1aaca896, debug; 3-run deterministic 1 hash;
debug==release byte-identical on all four surfaces df/deltarel/ir/h)

Dumps: run1/nbhd.{df,deltarel,ir} (demand-ON), off/nbhd.{df,deltarel,ir}
(flag-off), rel/ = release demand-ON. Witness source:
docs/proposals/KeyedInstances.artifacts/demand_neighborhood_witness.dr

## LOAD-BEARING DIVERGENCES from the paper-era witness-deltarel-target.md

GT-1 (THE BIG ONE): the fabricated demand table %table:8
(demand__neighborhood_bf) lowers MONOTONE in the real flat lowering —
class=monotone in the .df dump, kIngestFold with kCounter(+, NonRecursive)
in the .deltarel dump, NO frontier/claim/commit machinery anywhere. The
target's band-(a) minus arm drains $net-removal.21 of %table:8 — a vec the
flat lowering NEVER MINTS. Demand-retract has NO flat-lowering counterpart
on this witness as landed: the injector only ADDS seeds at query-call time,
the demand message's public entry is suppressed (IsDemandMessage), so no
driver-visible retraction path exists. INSTANCE DEATH'S TRIGGER IS
CURRENTLY UNREACHABLE. D1 must decide the retraction surface (fabricated
demand message becomes deletion-capable under the nested knob? death
deferred to D4 seams? a driver-visible retract ABI?) — owner decision.

GT-2: the ENTIRE demand-ON witness is monotone (add-only add_edge +
non-differential demand ⇒ TrackDifferentialUpdates marks nothing
deletable). The real .deltarel dump is TWO kIngestFolds + empty rounds/deps.
The target §2's assumption that %table:4 "rides the EXISTING acyclic
claim/frontier/commit tail" is FALSE for the witness as lowered today —
there IS no differential tail. If D1 makes demand deletion-capable, the
whole path flips differential (tables 8/15/4 become deletion-capable and
the tail appears). The nested design hinges on this choice.

GT-3: single-adornment raw_seed/d_reader CSE coincidence CONFIRMED LIVE:
join.6 (body guard: d_p ⋈ edge → guarded copy tuple.3/%table:15) and
join.7 (query-projection guard: d_p ⋈ neighborhood-read... actually
.in1=tuple.3 the guarded copy → insert.8/%table:4) BOTH read tuple.4
(%table:8) as .in0 — the fresh raw_seed TUPLE was CSE-folded into the
d_reader (the d4s3 F1 coincidence, exactly as D1.1(b)/D3 predicted).

GT-4: table map for the REAL demand-ON flat lowering (per-compile ids):
  %table:8  = demand__neighborhood_bf (demand relation, monotone)
  %table:11 = edge (monotone)
  %table:15 = the guarded copy (tuple.3, the flat "instance" content)
  %table:4  = the query answer table (insert.8's target)
The target's borrowed d3 numbering (%table:4=pub, %table:8=demand,
%table:11=edge) does NOT match: pub/answer is %table:15+%table:4 here.

GT-5: the flat demand flow is carried ENTIRELY by the hand-coded eager web
(both guard joins are eager monotone regions; DR-IR sees only the two
ingest folds). SUBGRAPH_INSTANTIATE therefore REPLACES eager-web emission,
not existing DR ops — the "excised JOIN" is a DataFlow view whose emission
is eager, and the §9 eager-web candidate is adjacent to D1's mold choice.

GT-6: .df rendering notes for authors: fabricated demand columns render as
c<id>:<type> tokens (c3, c8) — unnamed fabricated vars, not AutoVar_N here;
rename maps render as (c8=c3); recv comments `; recv #message <name>/<arity>`
at the byte-52 column. Flag-off .df = 3 blocks (select→tuple→insert into
%table:4=edge's table): the bound query is answered by a direct index scan,
no join exists flag-off.

## APPENDIX A — run1/nbhd.df (demand-ON flat, REAL bytes)

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

## APPENDIX B — run1/nbhd.deltarel (demand-ON flat, REAL bytes)

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

## APPENDIX C — run1/nbhd.ir (demand-ON flat, REAL bytes)

```
const u64 @0 = 0

const bool @1 = false

const bool @2 = true

global u64 @20

create %table:4[u64,u64]
  u64	%col:5	; Start
  u64	%col:6	; Node

  %index:7[u64,u64] on %col:5, %col:6
  %index:57[u64,_] on %col:5

create %table:8[u64]
  u64	%col:9

  %index:10[u64] on %col:9

create %table:11[u64,u64]
  u64	%col:12	; From
  u64	%col:13	; To

  %index:14[u64,u64] on %col:12, %col:13
  %index:32[u64,_] on %col:12

create %table:15[u64,u64]
  u64	%col:16	; Start
  u64	%col:17	; Node

  %index:18[u64,u64] on %col:16, %col:17
  %index:38[u64,_] on %col:16

init proc ^init:3()
  vector-define $empty:51<u64,u64>
  vector-define $empty:52<u64>
  seq
    call ^entry:19($empty:51<u64,u64>, $empty:52<u64>)
    return-false

proc ^entry:19($param:21<u64,u64>, $param:26<u64>)
  vector-define $pivots:25<u64>
  vector-define $pivots:29<u64>
  seq
    par
      vector-loop {@From:23, @To:24} over $param:21<u64,u64>
        update-count +nonrecursive {@From:23, @To:24} in %table:11[u64,u64]
          if-crossed
            vector-append {@From:23} into $pivots:25<u64>
      vector-loop {@28} over $param:26<u64>
        update-count +nonrecursive {@28} in %table:8[u64]
          if-crossed
            par
              vector-append {@28} into $pivots:25<u64>
              vector-append {@28} into $pivots:29<u64>
    vector-clear $param:21<u64,u64>
    vector-clear $param:26<u64>
    call ^flow:58($pivots:25<u64>, $pivots:29<u64>)
    return-false

proc ^receive:add_edge/2:42($param:43<u64,u64>)
  vector-define $empty:45<u64>
  seq
    call ^entry:19($param:43<u64,u64>, $empty:45<u64>)
    return-true

proc ^receive:demand__neighborhood_bf/1:46($param:47<u64>)
  vector-define $empty:49<u64,u64>
  seq
    call ^entry:19($empty:49<u64,u64>, $param:47<u64>)
    return-true

proc ^inject:53(@Start:54)
  vector-define $param:55<u64>
  seq
    vector-append {@Start:54} into $param:55<u64>
    call ^receive:demand__neighborhood_bf/1:46($param:55<u64>)
    return-true

proc ^flow:58($pivots:25<u64>, $pivots:29<u64>)
  seq
    @20 += 1
    vector-unique $pivots:25<u64>
    join-tables
      vector-loop {@From:31} over $pivots:25<u64>
      select {%col:9 as @33} from %table:8[u64] using %index:10[u64] where %col:9 = @From:31
      select {%col:12 as @From:34, %col:13 as @To:35} from %table:11[u64,u64] using %index:32[u64,_] where %col:12 = @From:31
        if-compare {@From:31, @From:31} = {@33, @From:34}
          if-true
            update-count +nonrecursive {@From:34, @To:35} in %table:15[u64,u64]
              if-crossed
                vector-append {@From:34} into $pivots:29<u64>
    vector-clear $pivots:25<u64>
    vector-unique $pivots:29<u64>
    join-tables
      vector-loop {@Start:37} over $pivots:29<u64>
      select {%col:9 as @39} from %table:8[u64] using %index:10[u64] where %col:9 = @Start:37
      select {%col:16 as @Start:40, %col:17 as @Node:41} from %table:15[u64,u64] using %index:38[u64,_] where %col:16 = @Start:37
        if-compare {@Start:37, @Start:37} = {@39, @Start:40}
          if-true
            update-count +nonrecursive {@Start:40, @Node:41} in %table:4[u64,u64]
    vector-clear $pivots:29<u64>
    return-true

```

## APPENDIX D — off/nbhd.df (flag-off baseline, REAL bytes)

```
dataflow

select ^select.0 (From:u64, To:u64)                ; recv #message add_edge/2
  ATTRIBUTES class=table-less stratum=0
  => ^tuple.1 (From, To)

tuple ^tuple.1 (From:u64, To:u64)
  ATTRIBUTES class=table-less stratum=1
  => ^insert.2 (From, To)

insert ^insert.2 (From:u64, To:u64) into %table:4
  ATTRIBUTES class=monotone stratum=2
```

## APPENDIX E — off/nbhd.deltarel (flag-off baseline, REAL bytes)

```
deltarel

op.0 kIngestFold sign=+ ctx=eager stratum=0
    effects: {kCounter(%table:4, +, NonRecursive)}
    spine: —
    args: table=%table:4 message=add_edge/2

rounds:

deps:

census: kCrossover=0 kProductArm=0 kSeedFold=0 kFixpointFire=0 kChainFold=0 kClaimDrain=0 kRetire=0 kRederive=0 kFrontierFilter=0 kCommitSweep=0 kNegateGate=0 kPivotAssemble=0 kIngestFold=1 kGroupUpdate=0 kStateSeal=0
```

## APPENDIX F — the R-DIFF donor probe (lane-ops): diff_demand_analog.dr
(a deletion-capable analog whose .deltarel shows the differential-tail
shape the nested R-DIFF stage rides; op.14 = the monotone ingest fold
with kVecAppend kNetAddition = the OD-7 mold)

```
; Copyright 2026, Peter Goodman. All rights reserved.
;
; diff_demand_analog -- D1 "ops" lane probe. Structurally mimics the
; demand_neighborhood_witness shape (one bound-shaped hop, a monotone edge
; joined against a "which keys to materialize" relation) BUT with the
; key-carrying relation sourced from an EXPLICITLY @differential message
; (`ask`) so the retraction surface the fabricated demand message SUPPRESSES
; is REACHABLE. This is the real differential-tail vocabulary the nested
; SUBGRAPH_INSTANTIATE / INSTANCE_DEATH band wiring must splice into:
;   - `ask` is @differential (deletable) -> its table + everything reachable
;     is deletion-capable -> TrackDifferentialUpdates marks it -> the flat
;     lowering grows claim drains / frontier filters / commit sweeps on the
;     ANSWER table.
;   - `edge` stays monotone (add-only, no @differential) -> the A5-fence
;     shape: differential KEY relation, monotone summarized input.
;   - `answer(Start, Node)` plays %table:4's role (the pub/answer table);
;     `want(Start)` plays the demand relation %table:8's role but is
;     deletion-capable.
;
; NOT itself a -demand program: no flag, no fabricated demand message, no
; JOIN-excision. It is the DIFFERENTIAL-TAIL DONOR: the flat differential
; join `want |x| edge` gives us the exact claim/frontier/commit ops the
; nested band (b) must feed (the queues) and the exact net-removal/
; net-addition frontier vecs band (a) must drain.

#message add_edge(u64 From, u64 To).
#message ask(u64 Start) @differential.

#local edge(u64 From, u64 To).
edge(From, To) : add_edge(From, To).

#local want(u64 Start).
want(Start) : ask(Start).

#local answer(u64 Start, u64 Node).
answer(Start, Node) : want(Start), edge(Start, Node).

#query neighborhood(bound u64 Start, free u64 Node) : answer(Start, Node).
```

## APPENDIX G — da.deltarel (the probe's REAL FLAG-OFF dump; corrected
## per E-84 — the donor's differential tail comes from its @differential
## message, not the demand transform, and the donor REJECTS under -demand)

```
deltarel

vec $delete-queue.0 <ids %table:4> uniq=sort-unique-at-drain def=[op.16] use=[op.3]
vec $add-queue.1 <ids %table:4> uniq=sort-unique-at-drain def=[op.15] use=[op.4]
vec $overdelete-set.2 <ids %table:4> uniq=multiset def=[] use=[op.5]
vec $addition-set.3 <ids %table:4> uniq=multiset def=[] use=[op.6]
vec $net-removal.4 <ids %table:4> uniq=multiset def=[] use=[op.0]
vec $net-addition.5 <ids %table:4> uniq=multiset def=[] use=[op.1]
vec $delete-queue.6 <ids %table:11> uniq=sort-unique-at-drain def=[] use=[op.7]
vec $add-queue.7 <ids %table:11> uniq=sort-unique-at-drain def=[] use=[op.8]
vec $overdelete-set.8 <ids %table:11> uniq=multiset def=[] use=[op.9]
vec $addition-set.9 <ids %table:11> uniq=multiset def=[] use=[op.10]
vec $net-removal.10 <ids %table:11> uniq=multiset def=[] use=[]
vec $net-addition.11 <ids %table:11> uniq=multiset def=[] use=[]
vec $join-pivots.12 <id-cols> uniq=sort-unique-at-drain def=[] use=[]

branch.0 src=%table:4 -> join.0 ends_at_join=true
branch.1 src=%table:7 -> join.0 ends_at_join=true

join.0 view=<Start,Node> pivot_vec=$join-pivots.12 targets=[%table:11]

op.16 kIngestFold sign=- ctx=eager stratum=0
    effects: {kCounter(%table:4, -, NonRecursive), kVecAppend(%table:4, kDeleteQueue)}
    spine: —
    args: table=%table:4 message=ask/1
op.15 kIngestFold sign=+ ctx=eager stratum=0
    effects: {kCounter(%table:4, +, NonRecursive), kVecAppend(%table:4, kAddQueue)}
    spine: —
    args: table=%table:4 message=ask/1
op.14 kIngestFold sign=+ ctx=eager stratum=0
    effects: {kCounter(%table:7, +, NonRecursive), kVecAppend(%table:7, kNetAddition)}
    spine: —
    args: table=%table:7 message=add_edge/2
op.3 kClaimDrain sign=- ctx=seed stratum=3 form=single-pass gate=kDelGateCnrNonPositive
    effects: {kVecDrain(%table:4, kDeleteQueue), kFlagWrite(%table:4, -), kVecAppend(%table:4, kOverdeleteSet)}
    args: table=%table:4
op.4 kClaimDrain sign=+ ctx=seed stratum=3 form=single-pass gate=kAddGateTotalPositive
    effects: {kVecDrain(%table:4, kAddQueue), kFlagWrite(%table:4, +), kVecAppend(%table:4, kAdditionSet)}
    args: table=%table:4
op.5 kFrontierFilter sign=- ctx=seed stratum=3 deferral=immediate
    reads: NetDeleted(%table:4)
    effects: {kVecDrain(%table:4, kOverdeleteSet), kVecAppend(%table:4, kNetRemoval)}
    args: table=%table:4
op.6 kFrontierFilter sign=+ ctx=seed stratum=3 deferral=immediate
    reads: NetAdded(%table:4)
    effects: {kVecDrain(%table:4, kAdditionSet), kVecAppend(%table:4, kNetAddition)}
    args: table=%table:4
op.0 kSeedFold sign=- ctx=seed stratum=4 src=%table:4 join_pivot
    effects: {kVecDrain(%table:4, kNetRemoval), kVecAppend($join-pivots.12)}
    spine: —
    args: src=%table:4 pivots=$join-pivots.12
op.1 kSeedFold sign=+ ctx=seed stratum=4 src=%table:4 join_pivot
    effects: {kVecDrain(%table:4, kNetAddition), kVecAppend($join-pivots.12)}
    spine: —
    args: src=%table:4 pivots=$join-pivots.12
op.2 kSeedFold sign=+ ctx=seed stratum=4 src=%table:7 join_pivot
    effects: {kVecDrain(%table:7, kNetAddition), kVecAppend($join-pivots.12)}
    spine: —
    args: src=%table:7 pivots=$join-pivots.12
op.7 kClaimDrain sign=- ctx=seed stratum=6 form=single-pass gate=kDelGateCnrNonPositive
    effects: {kVecDrain(%table:11, kDeleteQueue), kFlagWrite(%table:11, -), kVecAppend(%table:11, kOverdeleteSet)}
    args: table=%table:11
op.8 kClaimDrain sign=+ ctx=seed stratum=6 form=single-pass gate=kAddGateTotalPositive
    effects: {kVecDrain(%table:11, kAddQueue), kFlagWrite(%table:11, +), kVecAppend(%table:11, kAdditionSet)}
    args: table=%table:11
op.9 kFrontierFilter sign=- ctx=seed stratum=6 deferral=immediate
    reads: NetDeleted(%table:11)
    effects: {kVecDrain(%table:11, kOverdeleteSet), kVecAppend(%table:11, kNetRemoval)}
    args: table=%table:11
op.10 kFrontierFilter sign=+ ctx=seed stratum=6 deferral=immediate
    reads: NetAdded(%table:11)
    effects: {kVecDrain(%table:11, kAdditionSet), kVecAppend(%table:11, kNetAddition)}
    args: table=%table:11
op.11 kCommitSweep sign=· ctx=seed band=9 flavor=differential publish_target=false
    effects: {kInIReadFrozen(%table:4, InI, seed), kFlagWrite(%table:4)}
    args: table=%table:4
op.12 kCommitSweep sign=· ctx=seed band=9 flavor=monotone publish_target=false
    effects: {kFlagWrite(%table:7)}
    args: table=%table:7
op.13 kCommitSweep sign=· ctx=seed band=9 flavor=differential publish_target=false
    effects: {kInIReadFrozen(%table:11, InI, seed), kFlagWrite(%table:11)}
    args: table=%table:11

rounds:

deps:
  op.0 -> op.1 WAW epoch
  op.0 -> op.2 WAW epoch
  op.1 -> op.2 WAW epoch
  op.3 -> op.4 WAW epoch
  op.3 -> op.5 RAW epoch
  op.3 -> op.6 RAW epoch
  op.3 -> op.11 WAW epoch
  op.4 -> op.5 RAW epoch
  op.4 -> op.6 RAW epoch
  op.4 -> op.11 WAW epoch
  op.5 -> op.0 RAW epoch
  op.5 -> op.11 WAR epoch
  op.6 -> op.1 RAW epoch
  op.6 -> op.11 WAR epoch
  op.7 -> op.8 WAW epoch
  op.7 -> op.9 RAW epoch
  op.7 -> op.10 RAW epoch
  op.7 -> op.13 WAW epoch
  op.8 -> op.9 RAW epoch
  op.8 -> op.10 RAW epoch
  op.8 -> op.13 WAW epoch
  op.9 -> op.13 WAR epoch
  op.10 -> op.13 WAR epoch
  op.14 -> op.12 WAW epoch
  op.15 -> op.3 WAW epoch
  op.15 -> op.4 RAW epoch
  op.15 -> op.4 WAW epoch
  op.15 -> op.5 RAW epoch
  op.15 -> op.6 RAW epoch
  op.15 -> op.11 WAW epoch
  op.16 -> op.3 RAW epoch
  op.16 -> op.3 WAW epoch
  op.16 -> op.4 WAW epoch
  op.16 -> op.5 RAW epoch
  op.16 -> op.6 RAW epoch
  op.16 -> op.11 WAW epoch
  op.16 -> op.15 WAW epoch

census: kCrossover=0 kProductArm=0 kSeedFold=3 kFixpointFire=0 kChainFold=0 kClaimDrain=4 kRetire=0 kRederive=0 kFrontierFilter=4 kCommitSweep=3 kNegateGate=0 kPivotAssemble=0 kIngestFold=3 kGroupUpdate=0 kStateSeal=0
```
