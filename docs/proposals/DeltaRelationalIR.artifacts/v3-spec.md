# DR-IR vocabulary v3 — binding pre-R1 spec (workflow-drafted, judge-checked)

======================================================================
DR-IR VOCABULARY V3 SPECIFICATION
Delta-relational IR epoch — R1 object model + construction contract
======================================================================
Status: v3, the binding pre-R1-code spec. Supersedes vocabulary v2
(DeltaRelationalIR.md §6.3) by folding in the §9 ADOPTED RESOLUTIONS
F-1/F-2/F-4/F-5/F-6/F-7/F-9. Every requirement below is derived from
ledger-cited fact + repo file:line, not restated aesthetically.
Read-only sources consulted: DeltaRelationalIR.md §5-§9; tc.drir.md
(G1-G11); d5_recursive_negate.drir.md; StackSafeNegation.md §5.1-§5.6;
include/drlojekyll/Runtime/Table.h; include/drlojekyll/ControlFlow/
Program.h (VectorKind, RowFlags); lib/ControlFlow/Build/Build.h
(Context), Stratum.cpp, Procedure.cpp.

The v3 factoring answers Fable's core verdict (§9): v2 "encodes
ordering as enum attributes and dataflow as shared named vectors,
reproducing one level up exactly what makes today's web
unoptimizable." v3 fixes exactly that: (a) vectors are typed IR values
with def/use edges (F-1/F-9); (b) every operator declares an EFFECT SET
(F-1); (c) fold bodies are ACCESS-PLAN TREES (F-7); (d) ordering is
DERIVED as WAR/RAW/WAW dependence over the effect sets, and the emitted
order is a CHECKED LINEARIZATION (F-1); (e) the B-3/F-6 asserts become
graph validators (F-1/F-6); (f) SEED_FOLD/FIXPOINT_FIRE terms are
DERIVED from the §5.1 telescoped expansion, sign/position reified
(F-2/§9 F-2-restatement).


======================================================================
§0. OBJECT MODEL OVERVIEW
======================================================================
A DR-IR unit is a FlowGraph per differential data-flow procedure
(one per Program::Build's ^flow proc plus the ^entry ingest band).
It is a bipartite def/use graph over two node families:

  VALUE nodes:   Vec (typed IR vectors, §1), Table (debug-labelled
                 differential/monotone models), StateCell (reserved
                 R3, §2 effect domain only).
  OP nodes:      the operators of §2, each carrying an EFFECT SET.

Edges are TYPED (RAW/WAR/WAW derive from effect intersection, §4).
Region containers (FixpointRound, Induction shell, ProcFrame) are OP
nodes that OWN a child subgraph and re-export a fixpoint-test vec set.

The FlowGraph is NOT an ordered list. Order is a separate artifact:
a CHECKED LINEARIZATION (§4.6) the constructor pins for identity and
the validators (§5) certify against the dependence graph.


======================================================================
§1. VECTORS ARE TYPED IR VALUES  (F-1, F-9)
======================================================================
A Vec is a first-class IR value with explicit def and use edges.
(table, VectorKind) — the v2 identity and today's
Context::table_delta_vecs key (Build.h:159-164) — is DEMOTED to a
debug-info annotation (Vec::debug_origin), never load-bearing in
scheduling or lowering. Identity/uniqueness comes from the def edge.

Vec ATTRIBUTES (all intrinsic, set at def, immutable):
  element_shape ∈ { values      -- raw column tuples, no table binding
                    ids          -- row ids into one Table (id-keyed)
                    id+cols }    -- row id + a projected column subset
      (F-9: the element-shape attribute; replaces the implicit
       "what's in the vector" that VectorKind smuggled.)
  role ∈ { param, net-removal(D\A), net-addition(A\D), delete-queue,
           add-queue, overdelete-set(D), addition-set(A),
           claimed-del-frontier(Δ_D), claimed-add-frontier(Δ_A),
           join-pivots, product-input, table-scan, message-output,
           empty }
      (role is DERIVED from the producing op's effect + the §5.1 band;
       it is NOT free-choice. The validators (§5) check role against
       the op that defines it.)
  unique-contract ∈ { multiset | sort-unique-at-drain }
      (G6 resolution: sort-unique is a PER-VECTOR ATTRIBUTE checked at
       the DRAIN site, not a floating emit decision. A drain op whose
       input Vec carries sort-unique-at-drain lowers a VectorUnique
       immediately before its VectorLoop; a multiset Vec does not.
       tc.drir G6's 10 sort-unique sites — ir:97,102,115,120,132,140,
       148,209,217,225 — are exactly the Vecs whose def→drain edge
       carries sort-unique-at-drain. Per-round fire-output vecs
       (:66/:111) are multiset at production and sort-unique-at-drain
       at the NEXT round's claim drain: the attribute lives on the
       def→use edge, so the same physical vec is multiset out of the
       fire and sort-unique into the next drain — no contradiction.)

Vec def/use edges:
  def(Vec):  the single op that PRODUCES the vec's contents this band
             (an op with a `vector:append` or `vector:clear`+append
             effect targeting it). Round vecs (Δ_D/Δ_A) are cleared
             then re-appended each round — def is the CLAIM_DRAIN in
             the round body; the FixpointRound region re-exports them.
  use(Vec):  every op with a `vector:drain` effect over it.

DUAL-APPEND (G10) is now two def edges from ONE op: a SCC CLAIM_DRAIN
appends each claimed row to BOTH its overdelete-set Vec (persistent,
survives the round; feeds REDERIVE + deferred frontier filters) AND
its claimed-del-frontier Vec (per-round; drives this round's fires).
Two `vector:append` effect entries, two def edges — the dual-append is
an EFFECT-SET fact, not a CLAIM_DRAIN sub-flag. A non-recursive
pre-induction drain (tc S1.1/S1.2; d5rn 2a) has ONE append effect
(overdelete-set only) — the single-vs-dual distinction is READ OFF the
effect set, resolving G10.


======================================================================
§2. EVERY OPERATOR DECLARES AN EFFECT SET  (F-1)
======================================================================
Effect domain (the five sub-domains, F-1 exact):

  vector:   append(V) | drain(V) | clear(V)
  table:    counter±(T, class∈{NonRecursive, Recursive, Explicit})
              -- an UPDATECOUNT-family RMW; the ± and class are the
                 §5.1 term's sign and RuleClass (Stratum.cpp:291)
  flags:    read(T, F) | write(T, F)  for F ∈ RowFlags
              (Table.h:36-45: kInI, kDel, kAdd, kDelNow, kAddNow,
               kExplicit, kTouched). Frozen sub-case below.
  kInI:     read-frozen(T)  -- kInI is batch-frozen (Table.h:37); a
              read of kInI is a distinguished, always-legal read that
              never participates in WAR/WAW (it has no writer inside a
              band; Commit is the sole writer, §4.5). Kept separate so
              the scheduler treats it as a constant, not a hazard.
  statecell: fold(SC) | emit(SC) | old(SC)   -- RESERVED for R3
              (aggregates, AggregatingFunctors §2/§3). No R1/R2 op
              declares a statecell effect; the domain exists so the
              scheduler and validators are R3-total from day one.

Membership-predicate reads are `flags:read` bundles. The TEN predicates
(E-14) each expand to a fixed flag-read set, semantics DERIVED FROM
Table.h ONLY (E-12), never the Program.h enum comment:
  Present            = counts>0                    (Table.h:393)
  InI                = kInI                        (Table.h:357)
  InNew              = (kInI&&!kDel)||kAdd          (Table.h:363)
  SurvivesSoFar      = kInI&&!kDel                  (Table.h:370)
  AliveAtClaim       = kInI&&(!kDel||kDelNow)       (Table.h:376)
  InNewWithFrontier  = InNew                        (Table.h:382)
  InNewSansFrontier  = (kInI&&!kDel)||(kAdd&&!kAddNow) (Table.h:386)
  RecursivelySupported = C_r>0                      (Table.h:399)
  NetDeleted         = kDel&&!kAdd                  (Table.h:405)
  NetAdded           = kAdd&&!kDel&&!kInI           (Table.h:429; the
                        !kInI conjunct is E-12, from Table.h:413-422 —
                        NOT the Program.h:621 comment)
The claim gates (F17), also flag/counter reads with side effects:
  TryClaimDel        proceeds iff C_nr<=0; writes kDel|kDelNow (Table.h:437)
  TryClaimAdd        proceeds iff Total>0; writes kAdd|kAddNow (Table.h:469)

----------------------------------------------------------------------
§2.1 THE FULL EFFECT TABLE  (every v2 §6.3 operator + NEGATE_GATE)
----------------------------------------------------------------------
Notation: T = target table; V = a vec; "→V" = append(V); "loop V" =
drain(V). Reads are flags:read unless kInI-frozen. class from RuleClass.

OP: ACCESS(table T, bound-cols S, pred P, context C)          [B-1, F-7]
  Effects: flags:read(T, P-expansion) [+ kInI:read-frozen if P uses kInI]
  No table-counter, no vector effect (ACCESS is a MEMBERSHIP/scan LEAF).
  bound-cols S = canonical column-id SET (B-1: GetOrCreateIndex
    SortAndUnique order, Data.cpp:349 — NEVER "bound-prefix"; scattered
    sets are live, e.g. reaching_to keys on col 2). S selects the index.
  Lowerings (attribute lowering∈): point-test (Find+pred) | keyed
    section walk (idx.First/Next + pivot re-test) | full scan | seek
    (reserved D5). For a section walk ACCESS ALSO carries (index-id,
    pivot-col) so a self-join's two scans are unambiguous (G3 fix:
    idx_80[From,_] vs idx_87[_,To] + which if-compare column).
  context C ∈ {eager, seed, fixpoint} — decides which membership
    flavor the pred resolves to when P is context-sensitive.

OP: NEGATE_GATE(negate N, context C∈{eager,seed,fixpoint}, hint H)  [F-5]
  This is an ACCESS whose pred is the negate forward gate, polarity =
  ABSENT. The pred is DERIVED from (C, H), NEVER sign-derived (B-3):
    C=eager,   H=normal : pred = InI      (Negate.cpp:91-94, F-5)
    C=eager,   H=@never : pred = Present   (Negate.cpp:93; F-5/§4)
    C=seed,    H=normal : pred = InI, BOTH signs (E-13/F18; Stratum.cpp:521)
    C=fixpoint,H=normal : pred = InNew, if-ABSENT (refire; d5rn ir:265/354)
    (@never in seed/fixpoint is unreachable — @never never retracts.)
  Effects: flags:read(N.negated_table, pred). No counter, no vector.
  F-5 correction adopted: eager/kInI cell (eager-normal) EXISTS and was
  missing from §4-as-was; @never/kPresent belongs to EAGER, not seed.

OP: PIVOT_ASSEMBLE(join J, sources V1..Vk → pivots-vec Vp)   [G2, v2]
  Effects: for each Vi: vector:drain(Vi); vector:clear(Vp)+append(Vp).
  Unions a join's delta sources (both-sign reach frontiers + monotone
  edge A\D projected to the pivot col; d5rn ir:165-174) into ONE
  join-pivots Vp with unique-contract=sort-unique-at-drain. Vp is the
  single input frontier the join's FIXPOINT_FIRE arms share (G2: the
  JOIN, not the arm, owns the pivot vec).

OP: SEED_FOLD(rule R, delta_pos i, sign s, class K, body: PlanTree)
  The §5.1 SEED schema arm (delta at a LOWER-stratum position i).
  Effects: vector:drain(delta-vec) [the lower pred's D\A or A\D
    frontier, or a message A\D/D\A]; the body PlanTree's ACCESS reads;
    counter±(head-table, K) [± = s]; flags:read(head, kInI) [crossing
    predicate]; vector:append(head's delQ if s=-, addQ if s=+).
  body PlanTree (F-7, §3): nested ACCESS over the non-delta positions,
    each read flavored by §5.1 SEED table (j<i: InNew / negated absent
    in InNew; j>i and same-stratum j: InI / negated absent in InI).
  seed-suppression (G1): a SEED_FOLD is OMITTED (not emitted) for a
    join rule ALL of whose sides share the target's SCC. This omission
    is DERIVED (§6.4), asserted by validator V-SEED-SUP, never a flag.

OP: FIXPOINT_FIRE(join J, sign s, class=Recursive, arms: [Arm])   [G2,G3]
  The §5.1 FIXPOINT schema (delta at a SAME-stratum position). ONE
  op per (join, sign); it OWNS the shared input claimed-frontier vec
  and the shared output queue (G2). Each Arm = one delta_pos section:
    Arm.delta_pos, Arm.body: PlanTree.
  Effects (union over arms): vector:drain(claimed-frontier of the
    delta table); per arm the PlanTree ACCESS reads (fixpoint flavors:
    j<i lower → InNew; same j<i → SurvivesSoFar[OD]/InNewWithFrontier
    [INS]; same j>i → AliveAtClaim[OD]/InNewSansFrontier[INS] —
    Table.h-exact, §5.1); counter±(J's table, Recursive);
    flags:read(J.table, kInI); vector:append(J.table's delQ/addQ)
    [ONE shared output queue for all arms, G2].

OP: CHAIN_FOLD(claimed-frontier → head, sign s, class K, proj)    [G7,v2]
  The intra-SCC projection / MERGE-arm fold (tc S2.D6/D7/A6/A7;
  d5rn T14→T24, T18→T4). DISTINCT from SEED_FOLD (G7 fix): its delta
  input is a CLAIMED-* round frontier of one SCC table, not a lower
  net_* frontier. Effects: vector:drain(claimed-frontier of source
  SCC table); counter±(head-table, K=Recursive); flags:read(head,
  kInI); vector:append(head's delQ/addQ). No ACCESS body (pure
  projection/copy) unless the arm carries a residual gate.

OP: CROSSOVER(negate N)  — an ARM-PAIR object                 [B-3.1]
  ONE per non-@never negate (B-3.1: exactly one arm-pair folds into
  each negate's table; validator V-XOVER-ONE). Two arms:
    − arm: vector:drain(negated table's A\D=NetAdditions);
           body ACCESS(positive-predecessor, kInNew) [seed context];
           counter−(negate-table, RuleClass(negate,{pred,negated}));
           flags:read(kInI); vector:append(negate delQ).
    + arm: vector:drain(negated table's D\A=NetRemovals); same ACCESS;
           counter+(...); vector:append(negate addQ).
  seed-before-drain (B-3.4): a checkable ordering attribute — the
  crossover's counter± into the negate table MUST precede that table's
  own claim drain (validator V-SEED-DRAIN, §4.1). NOTE the negated
  side's delta is ALREADY materialized as its frontier vecs — CROSSOVER
  does NOT read the negated table (d5rn: only ACCESS is the positive
  predecessor gate). The negate FORWARD gate is a SEPARATE lowering
  (NEGATE_GATE, refire context) attached to the FIXPOINT_FIRE/SEED_FOLD
  that fires the negate — decoupled from CROSSOVER (d5rn gap 3).

OP: PRODUCT_ARM(product P, side, sign s)                       [B-3.2]
  Acyclic differential @product frontier arm. minus arm iff the side
  is differential (B-3.2: no − arm for monotone sides; validator
  V-PROD-MONO). Effects: vector:drain(this side's signed frontier);
  body: full-scan ACCESS(other side, position-keyed sign-INDEPENDENT:
  j<i InNew, j>i InI — §4); counter±(product-table, class=NonRecursive
  ASSERTED, B-3.2 / V-PROD-CLASS); flags:read(kInI);
  vector:append(product delQ/addQ). seed-before-drain.

OP: CLAIM_DRAIN(table T, sign s, form F∈{single-pass,in-round})   [G10,F17]
  Effects: vector:drain(T's delQ if s=-, addQ if s=+);
    flags:read(T, C_nr/Total) + flags:write(T, kDel|kDelNow [s=-] or
    kAdd|kAddNow [s=+]) [the F17 claim gate: TryClaimDel C_nr<=0 /
    TryClaimAdd Total>0 — MANDATORY DATA on the op, asserted, never a
    template flourish, B-3];
    vector:append(T's overdelete-set/addition-set);
    IF form=in-round ALSO vector:append(T's claimed-*-frontier) [the
    dual-append, G10 — a SECOND append effect, read off the effect set].
  form is DERIVED from whether T is in a recursive SCC (single-pass for
  lower differential like dead/edge; in-round for SCC tables).

OP: RETIRE(band B, sign s, after=fires)                       [B-3.3, G5]
  Effects: vector:drain(the round's claimed-*-frontier); flags:write(T,
  clear kDelNow [s=-] / kAddNow [s=+]). ordering attribute after=fires
  (B-3.3): RETIRE is strictly after ALL same-round FIXPOINT_FIRE/
  CHAIN_FOLD (validator V-RETIRE-AFTER, §4.2 — retiring kDelNow before
  a same-round AliveAtClaim/SurvivesSoFar read corrupts the exactly-once
  counting, §5.1). Emitted as a BAND (one per SCC table, table-id order)
  at the round-body tail (G5).

OP: REDERIVE(table T)
  Effects: vector:drain(T's overdelete-set); flags:read(T,
  RecursivelySupported=C_r>0); vector:append(T's addQ). A COUNTER READ,
  not a search (§5.2). Hosted in the OVERDELETE FixpointRound's `output`
  region (§4.4).

OP: FRONTIER_FILTER(table T, sign s, deferral∈{immediate,add-loop-output})
  Effects: vector:drain(T's overdelete-set [s=-] / addition-set [s=+]);
    flags:read(T, NetDeleted [s=-] / NetAdded [s=+]); vector:append(T's
    net-removal [s=-] / net-addition [s=+]). deferral DERIVED (E-17):
    immediate for non-recursive lower tables (dead/edge); add-loop-output
    for SCC tables (BOTH signs deferred into the INSERT round's `output`,
    after the add loop quiesces — a re-added row's kAdd must be final
    before the − filter reads NetDeleted; validator V-DEFER, §4.3).
  liveness (G8): a FRONTIER_FILTER whose net_* output has NO downstream
  use edge is a DEAD op (tc S2.F* net_* sinks). v3 records this as a
  Vec-use-edge fact (dead = zero uses), giving the future DCE pass a
  hook while R2 still emits it for identity. NOT a suppression flag.

OP: FIXPOINT_ROUND(scc)  — STRUCTURED REGION (container)       [G4]
  Owns a child subgraph (the round body: CLAIM_DRAIN×|tables| →
  FIXPOINT_FIRE/CHAIN_FOLD/NEGATE_GATE fires → RETIRE band) and
  re-exports the fixpoint-TEST vec set = the per-SCC-table claimed-*
  frontiers (the loop's break condition, §5.2/5.3). Two SEPARATE
  FixpointRound regions per SCC: phase=OVERDELETE and phase=INSERT
  (tc ir:125 / ir:202). REDERIVE lives in the OVERDELETE region's
  `output`; the deferred FRONTIER_FILTERs live in the INSERT region's
  `output` (G4). Region effect = union of children's effects (used by
  the outer scheduler as a black box).

OP: COMMIT_SWEEP(table T, flavor, publish-target?)            [G9]
  flavor DERIVED from T's model: differential → Commit(sink)+
  DebugValidateCounts+CompactDead+reindex; monotone → Seal(). publish-
  target present iff T backs a @differential TRANSMIT view (not merely
  a query relation — d5rn T4 reach is a query, sink is a no-op).
  Effects: flags:read/write(T, kInI) [Commit sets kInI:=Present];
  flags:write(T, clear kDel|kAdd|kDelNow|kAddNow); table:compact.
  ORDER: table-id order (G9: 4,8,13,17 — NEITHER stratum NOR SCC
  order), pinned independent of strata (validator V-SWEEP-ORDER).

OP: NET_BATCH(message M)  — ingest-only
  Effects: vector:drain(both message vecs) + vector:clear + append
  (SET-net: dedup each, adds∩removes annihilate — OQ3). Lives on the
  ^receive path, not ^flow.

OP: INGEST_FOLD(table T, sign s, class, [explicit])  — entry-proc only
  The message→table seed fold (d5rn 2a). Effects: vector:drain(param
  queue); counter±(T, class) [explicit → update-count-explicit form];
  flags:read(kInI); vector:append(T's delQ/addQ or the message A\D/D\A).


======================================================================
§3. FOLD BODIES ARE ACCESS-PLAN TREES  (F-7)
======================================================================
SEED_FOLD and FIXPOINT_FIRE bodies are explicit nested ACCESS-PLAN
TREES mirroring EmitJoinFire's scan_next nesting (F-7: the only shape
admitting the D5/WCOJ joint-ordering decision later). This resolves
G3 (section walks first-class) and G11 (guard nesting is the op's
shape).

----------------------------------------------------------------------
§3.1 GRAMMAR
----------------------------------------------------------------------
  PlanTree  ::= Loop( delta-vec, PlanNode )        -- the outermost drain
  PlanNode  ::= Access( table, bound-cols, index?, pivot-col?,
                        pred, polarity∈{member,absent},
                        lowering, child: PlanNode )
              | Gate( pred, polarity, child: PlanNode )   -- membership
                        gate with no scan (a bare CHECKMEMBER)
              | Fold( head-table, sign, class, proj-cols )  -- LEAF

  Every non-leaf node carries an implicit `if-guard` (G11): Access
  emits {scan-index | Find} + `if-compare pivot / if-true`; Gate emits
  `if-member / if-absent`; Fold emits `if-crossed` around its
  counter±→append. The guard chain IS the tree spine — load-bearing
  control the lowering reproduces verbatim.

  A tree has EXACTLY ONE Fold leaf (validator V-ONE-FOLD, F-6:
  single-fold-per-section-walk, today structural at Stratum.cpp:586-593).

----------------------------------------------------------------------
§3.2 IDENTITY LOWERING
----------------------------------------------------------------------
  Loop(V, N)           => VectorLoop over V (sort-unique iff V's
                          def→drain edge carries sort-unique-at-drain)
  Access(T,S,idx,piv,  => scan-index select over idx keyed by S (a
    pred,pol,walk,ch)      keyed section walk); `if-compare {piv}={bound}`
                          re-test on the scan cursor id (D1 row-binding
                          scope stack); then the pred CHECKMEMBER
                          (pol=member→if-member, absent→if-absent);
                          then lower `ch`. (tc S2.D4: ir:157 scan idx_80
                          + ir:160 kAliveAtClaim gate.)
  Access(...,point-test)=> Find full-key + pred CHECKMEMBER, then `ch`.
  Gate(pred,pol,ch)    => CHECKMEMBER(pred) if-member/if-absent, then ch
  Fold(T,s,class,proj) => UPDATECOUNT ±class over proj cols, wrapped in
                          `if-crossed`, appending the crossed id to T's
                          delQ (s=-) / addQ (s=+).

  Self-join disambiguation (G3): the two FIXPOINT_FIRE arms of tc⋈tc
  are two PlanTrees under ONE FIXPOINT_FIRE, differing ONLY in
  Access.index (idx_80[From,_] vs idx_87[_,To]) and Access.pivot-col
  (which body atom is the delta). The (index,pivot-col) pair is carried
  data, so lowering is unambiguous.


======================================================================
§4. ORDERING IS DEPENDENCE — A CHECKED LINEARIZATION  (F-1)
======================================================================
No op carries an ordinal. The constructor emits (a) the dependence
graph (derived from §2 effect intersections) and (b) ONE pinned
linearization for identity. A validator certifies the linearization is
a topological order of the dependence graph; the emitter walks the
pinned order. This is the F-1 replacement for the scheduling fixpoint
(Stratum.cpp:1732) whose integer output "IS NOT emitted" (§2 ln153-155).

Dependence edges between ops X, Y over a shared Value W:
  RAW  (read-after-write): X writes W, Y reads W  ⇒ X→Y
  WAR  (write-after-read):  X reads W, Y writes W  ⇒ X→Y
  WAW  (write-after-write): both write W           ⇒ pinned order
W ranges over Vecs (append=write, drain/clear=read/write) AND table
flag-sets per (table,flag) (counter± and TryClaim* = write; membership
read = read). kInI reads are FROZEN (§2): they generate NO WAR/WAW
(constant within a band), only RAW against Commit at the band boundary.

Each ledger ordering law is now a NAMED dependence class, DERIVED:

----------------------------------------------------------------------
§4.1 seed-before-drain  (B-3.4)
----------------------------------------------------------------------
  CROSSOVER/PRODUCT_ARM/SEED_FOLD write counter±(T) and append T.delQ/
  addQ; the subsequent CLAIM_DRAIN(T) drains T.delQ/addQ.
  ⇒ RAW on T.delQ/addQ: seed op → claim drain. The "seed BEFORE drain"
  law is exactly this RAW edge. (Load-bearing for the claim gates'
  phantom-drop, §5.1.1.) Validator V-SEED-DRAIN asserts the edge exists
  for every seeded table and the pinned order respects it.

----------------------------------------------------------------------
§4.2 retire-after-fires  (B-3.3, G5)
----------------------------------------------------------------------
  A same-round FIXPOINT_FIRE reads AliveAtClaim/SurvivesSoFar =
  flags:read(T,{kDel,kDelNow}); RETIRE(T,-) writes clear(kDelNow).
  ⇒ WAR on T.kDelNow: fire → retire. Every same-round fire→retire WAR
  forces RETIRE to the round-body tail. Validator V-RETIRE-AFTER.

----------------------------------------------------------------------
§4.3 E-17 deferral  (deferred SCC frontier filters)
----------------------------------------------------------------------
  A re-added row's INSERT CLAIM_DRAIN/FIXPOINT_FIRE writes kAdd;
  FRONTIER_FILTER(T,-) reads NetDeleted = flags:read(T,{kDel,kAdd}).
  ⇒ RAW on T.kAdd: every INSERT-round writer of kAdd → the − filter.
  Because ALL insert-round kAdd writers precede it, the − filter is
  forced INTO the INSERT round's `output` (after quiescence). Validator
  V-DEFER: for an SCC table, both-sign FRONTIER_FILTERs are hosted in
  the INSERT FixpointRound.output; for a non-recursive table there is
  no such kAdd-writer chain, so the filter floats immediately after the
  drain (deferral=immediate). Deferral is DERIVED, never a placement flag.

----------------------------------------------------------------------
§4.4 REDERIVE placement
----------------------------------------------------------------------
  REDERIVE reads C_r (written by the OVERDELETE fires' counter−) and
  writes addQ (read by INSERT's CLAIM_DRAIN). ⇒ RAW(C_r): overdelete
  fires → REDERIVE; RAW(addQ): REDERIVE → insert drain. This forces
  REDERIVE strictly between the two rounds — hence its host is the
  OVERDELETE region `output`. Derived, not scheduled.

----------------------------------------------------------------------
§4.5 dual-append & frontier feedback
----------------------------------------------------------------------
  CLAIM_DRAIN's two appends (overdelete-set + claimed-frontier, G10)
  create two def edges; the claimed-frontier feeds same-round fires
  (RAW), the overdelete-set feeds REDERIVE + deferred filters (RAW
  across the round boundary). The d5rn frontier-feedback (reach's own
  D\A/A\D produced by the INSERT-round deferred filters, consumed by
  the NEXT seed pass's PIVOT_ASSEMBLE) is a RAW edge on reach's
  net-removal/net-addition Vecs — an intra-flow producer→consumer edge
  the pinned linearization must (and does) respect. This makes the
  single-pass feedback a graph fact, not implicit ordering (d5rn gap 1).

----------------------------------------------------------------------
§4.6 SCC ROUND STRUCTURE & the CHECKED LINEARIZATION
----------------------------------------------------------------------
  The FixpointRound region's fixpoint-test vec set (claimed-* frontiers)
  is re-exported; the region is a scheduling black box to the outer
  linearizer (its internal order is its own checked linearization). The
  two rounds are ordered by the REDERIVE addQ RAW chain (§4.4). COMMIT
  is last (WAR against every flag reader; WAW on kInI). Stratum order
  falls out of RAW edges on inter-stratum net_* frontiers (a lower
  table's FRONTIER_FILTER writes net_*; a higher SEED_FOLD drains it).
  The constructor emits: DepGraph + PinnedOrder. Validator V-LINEAR:
  PinnedOrder is a topological sort of DepGraph (positive: every edge
  respected; negative: no op precedes a producer it reads, no RETIRE
  precedes a same-round fire, no drain precedes its seed).


======================================================================
§5. VALIDATORS — GRAPH VALIDATORS, ALWAYS-ON  (B-3 §6.1 + F-6)
======================================================================
All are TigerBeetle-style (positive AND negative space), ALWAYS-ON
(not #ifndef NDEBUG — B-3 lifts Stratum.cpp:1616's arm-pair assert out
of the NDEBUG block; F-6 lifts the :1860-1875 readiness assert). They
run over the §0 object model at construction.

B-3 cluster (five silent-breakage properties):
  V-XOVER-ONE   (B-3.1): exactly ONE CROSSOVER arm-pair folds into each
     non-@never negate's table. POS: count==1. NEG: no negate table is
     a counter± target of two distinct CROSSOVERs; no @never negate has
     a CROSSOVER. (lift Stratum.cpp:1616.)
  V-PROD-MONO   (B-3.2): no PRODUCT_ARM with sign=- has a monotone side;
     POS: differential side ⇒ has both signs. NEG: monotone side ⇒ no −
     arm exists.
  V-PROD-CLASS  (B-3.2): every product Fold leaf class==NonRecursive.
  V-RETIRE-AFTER(B-3.3): §4.2 — every same-round fire→retire WAR present
     and respected; POS: RETIRE at round tail. NEG: no RETIRE before any
     same-round fire in PinnedOrder. sort-unique is a per-vec attribute
     asserted at each drain placement (POS: sort-unique-at-drain vec ⇒
     VectorUnique present; NEG: multiset vec ⇒ absent — G6).
  V-SEED-DRAIN  (B-3.4): §4.1 — every CROSSOVER/PRODUCT_ARM has a
     seed-before-drain RAW edge into its table's claim drain, respected
     in PinnedOrder.
  V-MEMBER-ID   (B-3.5): a table's member-relation LIST holds each view
     at most once BY IDENTITY; NEG: never structurally dedup (distinct-
     but-equal views sharing a model are intentional — the group_ids CSE
     guard). POS: |list| == count of identity-distinct feeder views.

F17/F18 mandatory data:
  V-CLAIM-GATE  (F17): every CLAIM_DRAIN carries its gate as DATA;
     TryClaimDel ⇒ C_nr<=0, TryClaimAdd ⇒ Total>0. NEG: no CLAIM_DRAIN
     lacks a gate; no del drain gates on Total or vice versa.
  V-NEG-CTX     (F18/F-5): every NEGATE_GATE pred is DERIVED from
     (context,hint) per §2.1. NEG: no NEGATE_GATE pred is sign-derived;
     no seed-context negate reads InNew (must be InI both signs);
     @never ⇒ Present and only in eager context.

F-6 additions (three new always-on):
  V-ONE-FOLD    (F-6): every PlanTree has exactly one Fold leaf
     (today structural, Stratum.cpp:586-593). NEG: no section walk
     with two folds; POS: count==1.
  V-READY       (F-6): every op reads only Vecs/tables produced in a
     LOWER-or-SAME SCC (reads-lower-or-same-SCC, today NDEBUG at
     :1860-1875). NEG: no RAW edge from a strictly-higher SCC.
  V-R3-DERIVER  (F-6, R3): an aggregate table's member list ==
     exactly the agg view (sole-deriver). Dormant until R3; the domain
     exists now.

Additional derived-property validators (from the DERIVED construction,
§6): V-SEED-SUP (§6.4), V-LINEAR (§4.6), V-DEFER (§4.3).


======================================================================
§6. CONSTRUCTOR DERIVATION FROM THE §5.1 TELESCOPED EXPANSION  (F-2)
======================================================================
SEED_FOLD/FIXPOINT_FIRE terms are DERIVED per rule from the §5.1
identity New(B1..Bn) − Old(B1..Bn) = Σᵢ New^{<i} ⊗ Δᵢ ⊗ Old^{>i}. The
sign/position ATTRIBUTES on the ops ARE the Σ terms reified, kept
EXACTLY — so G1-class omissions are structurally impossible (§9 F-2).

----------------------------------------------------------------------
§6.1 Position order (input to derivation)
----------------------------------------------------------------------
  For each rule, fix the total body-atom order (§5.1): lower-strata
  atoms first (stratum order, then syntactic), then same-stratum atoms
  syntactic. Negated atoms are always lower positions (Stratify
  guarantee). RuleClass(rule) = Recursive iff target and some read
  table share an SCC (Stratum.cpp:291), else NonRecursive.

----------------------------------------------------------------------
§6.2 SEED-schema rules (delta at a LOWER position i)  →  SEED_FOLD
----------------------------------------------------------------------
  For each lower position i, emit ONE Σ-term = SEED_FOLD(rule, i, s, K):
    delta ranges over position i's lower frontier: − elements over D\A
    (positive atom) / A\D (negated key); + elements over A\D / D\A.
    (The sign of the enumerated element sets s; − enumerated in
    OVERDELETE, + in INSERT — §5.1 seed table.)
    body PlanTree non-delta reads (§5.1 seed table, DERIVED):
      lower j<i → InNew (negated: absent-in-InNew)
      lower j>i → InI  (negated: absent-in-InI)
      same-stratum j (always j>i) → InI
    Fold leaf: counter±(head, K), if-crossed → head delQ/addQ.
  This is exactly how tc S1.5/S1.6 (tc:edge, delta_pos=0, ±, NonRec)
  and d5rn reach:-src (delta=src X, NonRec) are produced.

----------------------------------------------------------------------
§6.3 FIXPOINT-schema rules (delta at a SAME position i)  →  FIXPOINT_FIRE
----------------------------------------------------------------------
  For each same-stratum position i, emit ONE Σ-term arm; group the arms
  of one join under ONE FIXPOINT_FIRE(join, sign) (G2). delta ranges
  over the current claim round (Δ_D in OVERDELETE, Δ_A in INSERT).
    body PlanTree non-delta reads (§5.1 fixpoint table, DERIVED — these
    ARE the four frontier-flavored predicates):
      lower j<i        → InNew (negated: absent-in-InNew)
      same j<i         → SurvivesSoFar [OD] / InNewWithFrontier [INS]
      same j>i         → AliveAtClaim  [OD] / InNewSansFrontier  [INS]
    Fold leaf: counter±(join-table, Recursive) → join delQ/addQ.
  tc S2.D4/D5/A4/A5 are the two same-position arms of tc⋈tc, EXACTLY the
  two Σ-terms for the two same-stratum body positions. The kDelNow/
  kAddNow bits give same-round exactly-once (§5.1: earlier-as-delta →
  later passes AliveAtClaim; later-as-delta → earlier fails
  SurvivesSoFar) — DERIVED from the table flavors, not hand-placed.

----------------------------------------------------------------------
§6.4 SAME-SCC SEED SUPPRESSION — DERIVED AND ASSERTED, NEVER A FLAG (G1)
----------------------------------------------------------------------
  A join rule ALL of whose sides share the target's SCC has NO §5.1
  SEED-schema arm: there is no lower position for it (every side is
  same-stratum), so §6.2 emits ZERO SEED_FOLDs for it and §6.3 emits
  its FIXPOINT_FIREs. The round-0 firing is carried by the claim-round
  fire (Stratum.cpp:1541-57 / §2 ln148-149). Thus seed suppression is a
  CONSEQUENCE of the position analysis (no lower position exists),
  NOT a JoinEmission flag. Validator V-SEED-SUP asserts the positive+
  negative space: POS: an all-same-SCC join has ≥1 FIXPOINT_FIRE and
  0 SEED_FOLDs into its table; NEG: NO SEED_FOLD exists whose delta is
  a lower frontier feeding an all-same-SCC join's table (the exact
  "naive constructor wrongly emits edge→T_join seed" bug, tc G1). This
  makes G1 structurally impossible: a wrong seed has no Σ-term to derive
  from and trips V-SEED-SUP.

----------------------------------------------------------------------
§6.5 Phantom-pair coverage (§5.1.1) — a derivation invariant
----------------------------------------------------------------------
  The mixed-change instance (earlier atom added, later deleted) fires
  twice with canceling signs — the − term (§6.2/6.3 OD column) and the
  + term (INSERT column) are BOTH derived Σ-terms; no special op. The
  net-exactness is a property of the derived term set, asserted at
  Commit by DebugValidateCounts (unchanged), not the DR-IR — but the
  constructor's completeness (every Σ-term emitted) is what makes the
  phantom pair net to zero. V-LINEAR's "no missing producer" plus the
  per-position completeness of §6.2/6.3 is the structural guarantee.


======================================================================
§7. C++ SHAPE  (files, class sketches, Program::Build hook)
======================================================================
New directory lib/ControlFlow/DR/ (peer of lib/ControlFlow/Build/).
Header include/drlojekyll/ControlFlow/DR.h. Repo style: Node<>-wrapped
impl classes, unsigned ids, no third-party deps.

----------------------------------------------------------------------
§7.1 CLASS SKETCHES  (~50 lines; matching repo idiom)
----------------------------------------------------------------------
  // DR.h — the vocabulary-v3 object model.
  enum class ElementShape : uint8_t { kValues, kIds, kIdCols };
  enum class VecRole : uint8_t { kParam, kNetRemoval, kNetAddition,
    kDeleteQueue, kAddQueue, kOverdeleteSet, kAdditionSet,
    kClaimedDel, kClaimedAdd, kJoinPivots, kProductInput,
    kTableScan, kMessageOutput, kEmpty };
  enum class UniqueContract : uint8_t { kMultiset, kSortUniqueAtDrain };
  enum class EffKind : uint8_t { kVecAppend, kVecDrain, kVecClear,
    kCounter, kFlagRead, kFlagWrite, kInIReadFrozen,
    kStateFold, kStateEmit, kStateOld };   // R3 tail reserved
  enum class Ctx : uint8_t { kEager, kSeed, kFixpoint };
  enum class Pred : uint8_t { kPresent, kInI, kInNew, kSurvivesSoFar,
    kAliveAtClaim, kInNewWithFrontier, kInNewSansFrontier,
    kRecursivelySupported, kNetDeleted, kNetAdded };  // the ten (E-14)

  struct Effect { EffKind kind; unsigned value_id;      // Vec or Table
                  Pred pred; RowFlags flag; int sign;    // ± for counter
                  DerivClass klass; };                   // Nr/R/Explicit
  class DRVecImpl { ElementShape shape; VecRole role;
    UniqueContract uniq; TABLE *debug_table;             // (T,kind) demoted
    VectorKind debug_kind; unsigned def_op; std::vector<unsigned> uses; };
  class DRTableImpl { QueryView model; bool differential;
    std::vector<QueryView> member_views;                 // V-MEMBER-ID list
    std::vector<DataIndex> indices; };
  class PlanNode { TABLE *table; std::vector<unsigned> bound_cols;
    DataIndex *index; unsigned pivot_col; Pred pred; bool absent;
    Lowering how; std::unique_ptr<PlanNode> child; bool is_fold;
    int fold_sign; DerivClass fold_class; };              // §3 tree
  class DROpImpl { OpKind kind; std::vector<Effect> effects;
    std::unique_ptr<PlanNode> body;                       // §3 (fold ops)
    Ctx ctx; Pred gate; bool sort_before_drain;
    Deferral deferral; ClaimForm form; };                 // attrs per §2
  class FixpointRoundImpl : public DROpImpl {             // §2 region
    Phase phase; std::vector<unsigned> test_vecs;         // re-exported
    std::vector<unsigned> body_ops; std::vector<unsigned> output_ops; };
  class FlowGraphImpl { std::vector<DRVecImpl> vecs;
    std::vector<DRTableImpl> tables; std::vector<DROpImpl> ops;
    std::vector<std::pair<unsigned,unsigned>> dep_edges;   // §4 derived
    std::vector<unsigned> pinned_order; };                 // §4.6 identity

  // DRBuild.h — the constructor + validators + lowering entry.
  FlowGraphImpl *BuildDRFlow(ProgramImpl*, Context&, Query);   // §6 derive
  void ValidateDRFlow(const FlowGraphImpl&);   // §5, always-on
  void LinearizeAndCheck(FlowGraphImpl&);      // §4.6 topo + V-LINEAR
  // R2 entry point (dormant in R1):
  void LowerDRFlow(ProgramImpl*, const FlowGraphImpl&);  // dr → cf regions

----------------------------------------------------------------------
§7.2 WHERE CONSTRUCTION HOOKS INTO Program::Build  (§2/E-16)
----------------------------------------------------------------------
  BuildStratumPhases is nested inside BuildEntryProcedure
  (Procedure.cpp:769; E-16), ~872 lines of Stratum.cpp:1456-2328, ONE
  function with three interleaved stages sharing the mutable state
  {branches, joins, crossovers, products, drain_stratum, recursive_sccs,
  Context::table_delta_vecs}. That shared state IS the DR-IR object
  (E-16: "the R1 cut must externalize exactly that state").

  R1 replaces BuildStratumPhases' DISCOVERY half (Stratum.cpp:1461-1671)
  + the SCHEDULING FIXPOINT (:1732-1828) with:
      BuildDRFlow(impl, context, query)   // ComputeRecursiveSCCs +
        DiscoverBranches (as a WORKLIST/memoized form, §1.4 hazard fix)
        + crossover/product discovery → FlowGraphImpl (vecs/tables/ops)
      LinearizeAndCheck(flow)             // replaces the integer lift;
        emits dep_edges + pinned_order (the scheduling fixpoint's
        RESULT becomes a checked linearization, F-1)
      ValidateDRFlow(flow)                // §5 always-on
  The EMISSION half (Stratum.cpp:1936-2327) is UNTOUCHED in R1 (F-2:
  R2 cuts over lowering families later). LowerDRFlow is the R2 entry
  point, stubbed in R1.

----------------------------------------------------------------------
§7.3 WHAT R1 DOES NOT CHANGE — and the R1 discipline decision
----------------------------------------------------------------------
  DECISION: R1 CONSTRUCTS DR-IR ALONGSIDE the old discovery and
  VALIDATES against it — it does NOT replace discovery immediately.
  Justification (against the F-2/§9 risk #5 "two-webs-alive stall" and
  the golden gate):
   - EMISSION still runs from the OLD web until R2 cutovers (F-2:
     acyclic families first, each cutover deletes its Emit* in the same
     diff). So the old discovery's output (branches/joins/crossovers/
     products/drain_stratum) is STILL CONSUMED by the untouched
     emission half in R1. Ripping discovery out in R1 would force a
     same-diff R2 emission rewrite — violating F-2's incremental,
     family-at-a-time, delete-with-cutover rule and the §9 risk-#5
     mitigation.
   - R1's value is the CHECKED equivalence: BuildDRFlow runs, produces
     the FlowGraph, ValidateDRFlow + LinearizeAndCheck pass, AND a
     cross-check asserts the DR-IR's derived op set is ISOMORPHIC to
     the old discovery's shared state (same tables, same SCCs, same
     join/crossover/product inventory, same pinned order as the
     scheduling fixpoint's integer lift would emit). This makes R1 a
     ZERO-emission-change, byte-identical-golden hard gate (§7.1 owner
     policy) while proving the DR-IR captures the E-16 state faithfully
     BEFORE any lowering rides on it. The isomorphism check is itself an
     always-on validator (V-OLD-EQUIV), retired family-by-family as R2
     cutovers delete the old discovery paths.
   - Concretely R1 does NOT touch: Runtime headers; the emission half
     (Stratum.cpp:1936-2327); commit-sweep emission (Procedure.cpp);
     Induction.cpp; codegen; any golden. It ADDS lib/ControlFlow/DR/,
     the BuildDRFlow call + validators inside BuildEntryProcedure right
     after the existing discovery, and the V-OLD-EQUIV cross-check.

  This satisfies: F-4 (byte-identical program.ir for R1 construction —
  construction-only, no emission change); §7.1 owner policy (R1 is a
  byte-identical-stdout hard gate); §9 risk #3 (construction/scheduling
  divergence caught by V-READY + V-OLD-EQUIV); §9 risk #5 (no stall —
  the old web stays authoritative for emission, deleted only at each
  R2 family cutover).

======================================================================
§A. AMENDMENTS (2026-07-15, after the dependence-completeness judge;
    these RESOLVE the judge's findings and BIND over the sections above
    where they conflict)
======================================================================
A-1 (HIGH: the d5rn frontier-feedback edge). The dependence graph is
    PER-EPOCH (one flow invocation). A vec whose contents flow from one
    epoch's tail filters into the next epoch's seed band is a
    LOOP-CARRIED dependence: within one epoch the seed's drain reads
    the PREVIOUS epoch's contents and the filters refill afterwards, so
    the intra-epoch edge is a WAR (drain-before-refill) — which
    topo-sorts. The cross-epoch RAW is recorded on the edge as
    loop-carried=true and validated separately (V-LOOP: every
    loop-carried vec is drained before any same-epoch refill def).
A-2 (HIGH: commit-band order). PinnedOrder = (any topological sort of
    the per-epoch dependence graph) + LOWERING-DEFAULT TIE-BREAKS.
    Unconstrained bands (the commit band's table order — G9) are
    ordered by the lowering default (table-id), which lives in R2's
    lowering, never as a synthetic graph edge. V-LINEAR certifies
    topo-consistency ONLY; V-SWEEP-ORDER is a lowering-conformance
    check, not a graph validator.
A-3 (MEDIUM: per-arm effects). Each FIXPOINT_FIRE/SEED_FOLD Arm carries
    its OWN effect set; the op-level set is their union and is used
    only for coarse scheduling. Validators that key on frontier-flag
    reads (V-RETIRE-AFTER) evaluate at ARM granularity.
A-4 (MEDIUM: multi-def vecs). Append-accumulating vecs (delete/add
    queues, overdelete/addition sets) may carry MULTIPLE def edges —
    appends are commutative and unordered among themselves; every def
    orders before every drain (RAW per def). "Single def" holds only
    for clear+refill round vecs. Identity comes from the def-edge SET.
A-5 (MEDIUM: element_shape witness). element_shape is CONSTRUCTOR
    knowledge (from the producing op's effect), carried as an
    attribute; the emitted IR's column signature is its debug
    cross-check, not its source of truth. Join-pivot vecs whose two
    section walks project different column subsets are id+cols with
    the union column set (the per-walk projection lives in the
    access-plan tree, not the vec).
A-6 (LOW: monotone elision hook). ACCESS carries
    elidable=monotone-trivial when table flavor is monotone and the
    predicate is InNew/Present inside a same-table scan (the D1 class);
    R2 identity lowering IGNORES it, a future pass may not.


## Open questions (carried)

- V-OLD-EQUIV isomorphism granularity: §7.3 asserts DR-IR is isomorphic to the old discovery's shared state, but the old scheduling fixpoint (Stratum.cpp:1732) computes INTEGER strata whose only observable is emission order, while the DR-IR pins a topological linearization. Need to confirm the pinned order MATCHES the integer-lift emission order byte-for-byte on all 155 cases before R1 can claim byte-identical goldens — or the linearizer must be seeded from the integer lift rather than derived independently (a weaker, faster-to-land R1 that still de-risks R2).
- Element-shape (ids vs id+cols) for join-pivot vecs: tc's shared kJoinPivots ($claimed_del:59) carries {From,X} for one arm and {X,To} for the other section walk (tc.drir G2/G3) — is the pivot vec element_shape=id+cols with a per-arm projection, or two logical views over one id vec? The §1 attribute set needs a concrete answer before PIVOT_ASSEMBLE and FIXPOINT_FIRE arm bodies can be lowered identically.
- Frozen-kInI RAW-against-Commit modeling (§4): kInI reads are treated as constants within a band, with a single RAW edge to COMMIT_SWEEP. But COMMIT_SWEEP order is table-id (G9), not dependence-derived — so the kInI-RAW edge and the table-id pin could conflict for a table read by a higher stratum's seed after a lower table commits. Confirm commit sweeps are strictly a trailing band (all after all flow ops) so no kInI reader is scheduled after any Commit — the d5rn/tc artifacts show this but it should be a validator (V-COMMIT-TRAILS), not an assumption.
- R3 statecell effect domain (fold/emit/old) is reserved but unspecified: the AggregatingFunctors old()/valued-sealed reads (average_weight.post-r3 target) need the WAR/RAW semantics of `old` (read prior-epoch sealed value) defined against the frozen-kInI model — is `old` another frozen read, or does it need a per-epoch snapshot value node the scheduler tracks? Out of R1/R2 scope but the domain shape should be pinned so §2's five-domain claim is not later refactored.
- DiscoverBranches worklist/memoized rebuild (§1.4 latent hazard, exponential path-enumeration DFS at Stratum.cpp:352): §7.2 folds this into BuildDRFlow, but whether the memoized form produces the IDENTICAL branch set (hence identical seeds/fires) as the current no-visited-set DFS on reconvergent plumbing is unverified — a directed OptDiff case with reconvergent table-less plumbing is needed before R1's V-OLD-EQUIV can be trusted on that shape.
======================================================================
§B. AMENDMENTS (2026-07-15, after the identity-lowering judge —
    v3-judge-identity.md; binding over the sections above)
======================================================================
B-7 (HIGH, judge F1): CLAIM_DRAIN's IN-ROUND form gains a
    `vector:clear(input-queue)` effect — the drained accumulating
    queue is cleared after the drain so this round's fire appends
    don't replay next round. This is IR CONTENT (load-bearing for
    quiescence), and single-pass vs in-round is now distinguishable by
    BOTH the clear and the dual-append. New validator V-QCLEAR: every
    in-round-drained accumulating queue is cleared before round end.
B-8 (HIGH, judge F2): the epoch bump `g += 1` becomes ProcFrame
    prologue material with a declared lowering default ("a flow proc's
    body opens with the epoch bump"), and engine globals get a value-
    node home (the StateCell family, its R3 reservation relaxed to
    "engine scalars"; effect kind global:rmw).
B-9 (MEDIUM, judge F3): A-2 generalizes to a declared ROUND-BODY BAND
    TEMPLATE: bands in fixed order (claims, fires, chain-folds,
    retires; output bands sign-major-then-table-id), table-id within a
    band keyed by the DRAINED SOURCE vec's debug table, sign − before
    + where a band holds both. V-SWEEP-ORDER generalizes to a per-band
    lowering-conformance check.
B-10 (MEDIUM, judge F4): A-1 applies at EVERY region nesting level —
    epoch AND fixpoint round. Round-carried vecs (next-round RAW
    feedback) are loop-carried at round scope; V-LOOP checks
    drain-before-refill per round.
B-11 (LOW, judge F5): dual-append intra-seq order is a G11-family
    template default: persistent-set append before round-frontier
    append.
B-12 (LOW, judge F6): ProcFrame is specified: prologue = vec defines
    in id order + (flow procs) the epoch bump; epilogue = return token
    per proc kind; entry/receive band templates. Id allocation:
    "ids allocated sequentially at first touch in pinned order" is a
    DECLARED lowering default — verify the emitter's var-id stream
    matches before the R2 family-#1 byte-identity gate is trusted.
B-13 (R1 sequencing, judge §7 assessment): R1 SEEDS pinned_order from
    the existing scheduling fixpoint's integer lift (apples-to-apples
    V-OLD-EQUIV for family #1); the independent linearization deriver
    lands later behind the same validator. A directed
    reconvergent-plumbing OptDiff case gates trust in the
    DiscoverBranches memoized form.
