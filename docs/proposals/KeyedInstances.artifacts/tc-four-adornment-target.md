# Four-adornment transitive closure — the POST-D3 dataflow target

Checkpoint step 3, artifact 3 (KeyedInstances epoch). Hand-written
DESIRED post-D3 dataflow graph for the four-adornment tc family, in
the BB-with-arguments dump form (ir-dump-formats.md §1). The D3 lift
is the multi-adornment slice: the >1-binding-pattern rejects (the
big-review belts) delete, and the per-adornment registries the injector
already binding-pattern-keys become load-bearing instead of the second
belt of a reject.

Ground truth read for this artifact:
- `data/self_testing_examples/transitive_closure.dr` (the prompt's
  `data/examples/transitive_closure.dr` — the file lives under
  `self_testing_examples/`; NOT `examples/`, which does not exist).
- `tests/OptDiff/cases/demand_tc_witness.dr` (the single-adornment
  landed witness; the D3 witness is its four-adornment sibling).
- fleet-d0/lane-pipeline.md (the verified single-adornment pipeline:
  the THREE GuardSite kinds — kReadAtTuple/kPushDown/kBaseAtom — and
  both guard families: the step-7 body guards vs the step-8
  query-projection guard).
- fleet-d0/lane-fabric-inject.md (the registry + fabrication reality;
  the injector's BindingPattern second belt at Build.cpp:465-474).
- DemandSeeds.md §1 E-41 (the four-adornment count); DemandSeeds.artifacts/
  d4s3-recipe.md AMENDMENTS F1 (the raw-seed-vs-d_p coincidence and where
  it diverges); d1-demand-seed-mechanism.md §3.x (the verified CSE
  reality: adornments differ STRUCTURALLY before group_ids).
- PerfRoadmap §18.5(D) (D3 as a diff against the landed pipeline).

A NOTE ON WHICH RECURSION SHAPE. There are TWO tc bodies in play and
they yield DIFFERENT adornment sets — this artifact keeps them distinct:

- The WITNESS body (`demand_tc_witness.dr`, and the D3 witness below):
  right-linear, From-preserving —
  `path(F,T) : path(F,M), edge_2(M,T)`.
  Its recursive subgoal reads `path(F,M)` — the bound F flows to the
  SAME position it was demanded at (From-preserving). One recursive p
  read per body.
- The E-41 body (`transitive_closure.dr`): NON-LINEAR self-join —
  `tc(F,T) : tc(F,X), tc(X,T)`.
  TWO recursive subgoals per body, and the second (`tc(X,T)`) re-keys
  the bound value SIDEWAYS. This is the shape whose literal
  left-to-right SIP yields FOUR adornments and whose raw-seed-vs-d_p
  divergence goes LIVE (§3 below). E-41 is explicit that "the
  load-bearing point (multi-adornment split mandatory) holds under
  either count" — the four-adornment claim is a property of the
  NON-LINEAR body.

D3's job is to make BOTH compile. The single-adornment slice already
handles one From-preserving adornment of the linear body; D3 lifts the
per-name multi-adornment reject AND (because the non-linear body's SIP
naturally produces >1 adornment) forces the raw-seed-vs-d_p divergence
to be handled correctly rather than coincidentally.

---

## 1. The adornment derivation, spelled out

### 1.1 The #query decls that exist

`transitive_closure.dr` declares THREE bound-or-free `#query`s over the
one `#local tc(From, To)`:

    #query reachable_from(bound u64 From, free u64 To)   ; adornment on tc: bf
    #query reaching_to   (free u64 From, bound u64 To)   ; adornment on tc: fb
    #query is_node       (free u64 Node)                 ; all-free (arity-1 projection)

E-41's correction (DemandSeeds.md §1): these three DECLS are not three
tc adornments — `reachable_from`/`reaching_to`/`is_node` are distinct
RELATIONS, each projecting from `#local tc`. The tc ADORNMENTS are what
SIP propagation over the tc rule bodies produces once these queries seed
demand. Under the current single-adornment slice, ANY of these being
present alongside another bound query is REJECT-2 (Demand.cpp:412,
"Multiple demanded (bound) queries"); `reachable_from` + `reaching_to`
alone trips it. D3 deletes that reject.

### 1.2 SIP propagation, per seeding query, over the NON-LINEAR body

The demand set is the fixpoint of: seed each bound query's demand onto
the relation it projects from, then propagate through each rule body
left-to-right, binding a subgoal's columns from already-bound variables.
The body under analysis (E-41 shape):

    tc(From, To) : tc(From, X), tc(X, To).      ; rule R1 (recursive)
    tc(From, To) : add_edge(From, To).          ; rule R2 (base)

SEED FROM `reachable_from(bound From, free To)`:
  - `reachable_from` projects tc at bf → seed `d_tc^bf` (bound col 0 =
    From).
  - Propagate bf into R1 with head adornment bf (From bound, To free):
    - Subgoal 1 `tc(From, X)`: From is bound (head-bound), X free →
      this subgoal is demanded at **bf**. It also BINDS X (X becomes
      available left-to-right after subgoal 1 produces it).
    - Subgoal 2 `tc(X, To)`: X now bound (from subgoal 1), To free →
      demanded at **bf** as well (its FIRST column is bound). BUT the
      bound value is X, NOT the query's From — this is the SIDEWAYS
      re-key. Same adornment string bf, DIFFERENT demand key value.
  - R2 (base): head bf → `add_edge(From, To)` with From bound. Base
    atom, no interior tc read. kBaseAtom guard site; NOT a demanding
    subgoal (adds no new tc adornment).
  - Fixpoint: bf reached; bf is closed (both R1 subgoals demand bf).

SEED FROM `reaching_to(free From, bound To)`:
  - `reaching_to` projects tc at fb → seed `d_tc^fb` (bound col 1 = To).
  - Propagate fb into R1 with head adornment fb (From free, To bound).
    Literal left-to-right (the E-41 order — this is where the
    right-to-left-optimal reordering is DELIBERATELY not done, per E-41
    "literal left-to-right magic-set propagation"):
    - Subgoal 1 `tc(From, X)`: From free, X free (X not yet bound) →
      demanded at **ff** (ALL-FREE). ff is inert (see 1.3). Subgoal 1
      binds From and X for later subgoals.
    - Subgoal 2 `tc(X, To)`: X bound (from subgoal 1), To bound (head) →
      demanded at **bb** (BOTH bound).
  - R2 base: head fb → `add_edge(From, To)`, To bound. kBaseAtom.
  - Fixpoint from fb: reaches {ff, bb}.
  - Now CLOSE ff and bb (they are new demanded adornments; SIP must
    propagate them too):
    - ff into R1 (From free, To free): subgoal 1 tc(From,X) → ff;
      subgoal 2 tc(X,To) after subgoal 1 binds X → bf. ff and bf both
      already present.
    - bb into R1 (From bound, To bound): subgoal 1 tc(From,X) → bf
      (From bound, X free); subgoal 2 tc(X,To) → bb (X bound by subgoal
      1, To bound). bf and bb already present.

FIXPOINT ADORNMENT SET over the non-linear body seeded by
{reachable_from bf, reaching_to fb}:

    { bf, fb, ff, bb }        ; E-41's four

`is_node(free Node)` (all-free) seeds `d_tc^ff` — the SAME ff already in
the set; it fuses (no fifth adornment). All-free `is_node` is the
demand-INERT consumer (1.3).

### 1.3 Why ff is inert, and what bb does

**ff (all-free) is inert.** An all-free adornment has an EMPTY bound-
column set → EMPTY instance key → the "demand relation" `d_tc^ff` would
carry an arity-0 payload (or a single sentinel row), and its guard
`d_tc^ff ⋈ tc` pivots on ZERO columns — a zero-pivot join, which under
this compiler appears only under `@product` and is otherwise a full
cross with a constant, i.e. it prunes NOTHING. Materializing tc under an
all-free demand is materializing the FULL closure: the guard is pure
overhead with no frontier restriction. This is exactly the tc.dr
negative-witness shape (DemandSeeds d1 §2.3, and the
`demand_tc_witness.dr` header note: "an all-free consumer of path would
make path demand-inert"). MECHANICALLY, the pass treats an all-free
query as the `bound_queries.empty()`-style benign skip for that query
(Demand.cpp:400-405 filters on `any param.Binding()==kBound`; an
all-free query is not a bound query and never enters `bound_indices`).
So `is_node` and any ff-demanded subgoal contribute NO d_tc^ff relation
and NO guard — ff is dropped from the emitted adornment set. The
emitted demand adornments are the three with a NON-EMPTY key:

    emitted = { bf, fb, bb }      ; ff pruned as inert

**bb (both-bound) is the fully-keyed point.** A bb demand carries the
WHOLE tuple (From, To) as its instance key — the maximally-restrictive
frontier: `d_tc^bb ⋈ tc` demands exactly the single (From,To) pair, so
tc materializes at most the rows needed to answer a ground membership
query. bb arises here ONLY as a PROPAGATED adornment (reaching_to's
subgoal 2 `tc(X,To)` with X bound sideways and To bound), never as a
seeded top-level query in this program — but D3 must mint `d_tc^bb`
and its guard because R1's second subgoal demands it. bb is the
adornment that makes the raw-seed-vs-d_p divergence live: its demand key
(a specific (X,To)) is NOT the query's issued seed key (the query issued
fb on To only), so guarding the query projection against the raw seed vs
against the union d_tc gives DIFFERENT answers (§3).

Summary table:

| adornment | bound cols | key arity | origin | emitted? | role |
|-----------|-----------|-----------|--------|----------|------|
| bf | {From} | 1 | reachable_from seed; reaching_to ff→bf prop | yes | From-frontier |
| fb | {To}   | 1 | reaching_to seed | yes | To-frontier |
| ff | {}     | 0 | reaching_to R1 subgoal1; is_node seed | NO (inert) | full materialization |
| bb | {From,To} | 2 | reaching_to R1 subgoal2 (sideways) | yes | ground/point demand |

---

## 2. The post-D3 dataflow graph (BB-with-arguments dump form)

The dump conventions are ir-dump-formats.md §1: one block per view in
view-id order, `^<kind>.<id>`; `=>` lines are tail-calls per user column
edge; MERGE is the block-args join point (φ), JOIN declares
`.lhs/.rhs/[pivot ...]` ports; SELECT carries the relation/stream +
receive kind; INSERT is terminal; a `producer=` attribute line marks
pass-minted views (load-bearing for D1/D3 review — the demand pass's
annotation sites become visible). Column ids here are ILLUSTRATIVE
(`c<n>`), chosen to make the fan-in/fan-out legible; the real dump uses
run-stable view/column ids. Per-adornment structure repeats, so the
graph is presented as: shared base structure once, then the three
emitted-adornment slices (bf, fb, bb), then the query-projection layer.

Because this is the DESIRED graph, it is the PRE-CSE minted shape with a
note on what Optimize fuses (§2.6). D3 mints, per emitted adornment α,
an independent fabricated message + local + d_tc^α MERGE + guard family;
the injector reaches each via its per-adornment registry entry.

### 2.1 The base program (shared; adornment-independent)

    ;; #message edge_2(u64 From, u64 To)     [the E-41 add_edge, renamed
    ;;                                          edge_2 to match the witness]
    ;; #local  tc(From, To)
    ;; R1: tc(F,T) : tc(F,X), tc(X,T)         (non-linear self-join)
    ;; R2: tc(F,T) : edge_2(F,T)              (base)

    select ^select.edge (io edge_2 recv) () -> (From:u64, To:u64)
      producer=SELECT ; message receive, user input
      => ^select.edge is read by R1-subgoals? NO — edge_2 only in R2.
         (E-41 body has no edge in R1; edges seed the base rule.)
      => ^tuple.R2base (From, To)             ; base rule body

    ;; tc's post-Connect MERGE — the union of all rule-body members.
    ;; Under -demand each recursive member is GUARDED (its member view
    ;; becomes a JOIN(d_tc^α ⋈ ...) restored to (From,To)); the base
    ;; member is guarded at its source atom. Shown fully per adornment
    ;; below; here the MERGE node itself:

    merge ^merge.tc (From:u64, To:u64)        ; TABLE %tc, class=differential
      producer=MERGE                          ; the tc relation
      ;; callers (members), post-D3, per adornment — see 2.3/2.4/2.5:
      ;;   R2 base member, guarded at edge_2 source per demanding adorn
      ;;   R1 recursive member, guarded at each recursive tc read
      => ^tuple.q_bf.read  (From, To)         ; reachable_from projection reader
      => ^tuple.q_fb.read  (From, To)         ; reaching_to projection reader
      => (the R1 recursive tc reads — themselves guarded, 2.3+)

Note the KEY POINT for D3: under multi-adornment demand, tc's ONE table
and ONE MERGE are shared, but each recursive member is guarded MULTIPLE
times — once per adornment that demands that subgoal. The single-
adornment slice guards each member once.

[AMENDED: HIGH/FINDING-2 — the landed guard machinery is ONE-SITE-PER-
MEMBER (Step 3b traces the member's bound OUTPUT columns to one source and
mints one GuardSite per member — REJECT-27 forces bound columns to share
one site, Demand.cpp:719-725, sites.push_back once per member :735; Step 7
mints one MintGuardJoin per site, :957). D3 does NOT "generalize one guard
per (member, adornment)" as a reuse of that machinery — it REPLACES the
member-OUTPUT-keyed guard pass with a PER-INTERIOR-READ guard pass. Each
full-width tc read inside a member is guarded `d_tc^α ⋈ tc` at THAT read,
with α assigned by the SIP walk to that specific read — and subgoal-1 and
subgoal-2 of R1 may carry DIFFERENT α (that is the whole point of the
split). This is a substantially larger rebuild than "delete rejects +
reuse the per-adornment registry" (§4.3, §18.5(D)); an implementer reusing
Step-3b/Step-7 unchanged would get ONE guard per member keyed on the
head-bound column's origin and would never guard subgoal-2's sideways read
at all. REJECT-18 (`p_reads > 1`) fires in Step 3a BEFORE any guard-site
work, so the whole non-linear guard picture below is a POST-LIFT DESIGN
TARGET for a rebuilt pass, not a diff against the existing one.]

This is the structural reason
the >1-adornment reject exists today (one guard family assumed per
member) and what D3 must BUILD: a per-interior-read guard pass that guards
each recursive tc read against the d_tc^α reader for the α the SIP walk
assigned to that read.

### 2.2 Per-adornment fabricated messages + locals

D3 fabricates one message + one local PER EMITTED adornment (Parse/
Demand.cpp FabricateDemandMessage/FabricateDemandLocal, the reserved
`demand__` prefix; the adornment string is appended at Demand.cpp:774-788
`base_name = "demand__" + name + "_" + adorn`). For this program:

    ;; adorn=bf (key arity 1: From:u64)
    io  ^io.d_tc_bf   (message demand__tc_bf/1, received, ABI-suppressed)
    select ^select.d_tc_bf (io ^io.d_tc_bf recv) () -> (From:u64)
      producer=DEMAND-RECEIVE

    ;; adorn=fb (key arity 1: To:u64)
    io  ^io.d_tc_fb   (message demand__tc_fb/1, received, ABI-suppressed)
    select ^select.d_tc_fb (io ^io.d_tc_fb recv) () -> (To:u64)
      producer=DEMAND-RECEIVE

    ;; adorn=bb (key arity 2: From:u64, To:u64)
    io  ^io.d_tc_bb   (message demand__tc_bb/2, received, ABI-suppressed)
    select ^select.d_tc_bb (io ^io.d_tc_bb recv) () -> (From:u64, To:u64)
      producer=DEMAND-RECEIVE

    ;; NO demand__tc_ff — ff is inert, pruned (1.3).

Each `demand__tc_<α>` local is its own `#local` relation (the d_tc^α
frontier), distinct decl, distinct DeclarationContext (CreateDerived
bypasses the redeclaration id-map — lane-fabric-inject §1e), so the
three demand relations never merge at the decl level. G3 uniquing (the
reserved-prefix collision scan, DemandFabricationWouldCollide) runs per
adornment BEFORE any fabrication for that adornment; the adornment suffix
makes the three names distinct (`demand__tc_bf` vs `_fb` vs `_bb`), so
they cannot collide with each other, only with a hypothetical user decl
of the same spelling+arity (still a clean reject).

### 2.3 The bf slice — d_tc^bf MERGE, readers, guards

`d_tc^bf` is demanded by: the reachable_from seed (root member) AND R1's
two recursive tc subgoals when propagated at bf (both re-key on a From-
position column). Its MERGE:

    tuple ^tuple.d_bf.root_head (From:u64)         ; over ^select.d_tc_bf
      producer=DEMAND-SEED
      => ^tuple.d_bf.root_member (From)
    tuple ^tuple.d_bf.root_member (From:u64)
      producer=INSERT
      => ^merge.d_tc_bf (From)                     ; MERGE member 0

    ;; propagation member: R1 subgoal-1 read of tc projecting From
    tuple ^tuple.d_bf.prop1 (From:u64)             ; From-col off a tc read
      producer=DEMAND-PROP
      => ^tuple.d_bf.prop1_member (From)
    tuple ^tuple.d_bf.prop1_member (From:u64)
      producer=INSERT
      => ^merge.d_tc_bf (From)                     ; MERGE member 1

    merge ^merge.d_tc_bf (From:u64)                ; TABLE %d_tc_bf
      producer=MERGE-INSERT                        ; callers: root_member, prop1_member
      => ^tuple.d_bf.reader (From)
    tuple ^tuple.d_bf.reader (From:u64)            ; the shared derived d_p reader
      producer=SELECT
      => ^join.g_bf.R1s1 .lhs(From)                ; guards R1 subgoal-1 tc read
      => ^join.g_bf.R1s2 .lhs(From)                ; guards R1 subgoal-2 tc read
      => ^join.g_bf.R2   .lhs(From)                ; guards R2 base source atom

Guard families for bf — one guard PER INTERIOR tc READ (the rebuilt
per-read model, FINDING-2), with the α the SIP walk assigns to THAT read.
Under the bf head both R1 reads are demanded at bf, so both guards pivot
against `^tuple.d_bf.reader`; the GuardSite kinds (the THREE kinds,
lane-pipeline §3) still classify each read:

  - R1 subgoal-1 `tc(From,X)` — SIP α = bf. The bound From reaches the
    recursive tc read THROUGH R1's body JOIN (tc ⋈ tc on X). Consumer =
    the JOIN → **kPushDown** (Demand.cpp:704). Guard `^join.g_bf.R1s1`
    guards THIS read alone, pivoting d_tc^bf.From against the read's
    From-column; a restoring TUPLE re-establishes (From,X); the JOIN is
    rewired onto it (MintRestoringTuple, step-7 else-branch :974).
  - R1 subgoal-2 `tc(X,To)` — SIP α = bf here (From-position bound is X).
    A SEPARATE guard on THIS read: bound value is X (sideways), also
    arriving through the body JOIN → **kPushDown**, pivoting d_tc^bf on the
    X-position. (In the From-preserving linear witness there is only ONE
    such read; the non-linear body has two, and each is guarded
    independently at its own read. Under DIFFERENT heads these two reads
    carry DIFFERENT α — e.g. the fb head sends subgoal-2 to bb, §2.5 —
    which is exactly why the guard must be per-read, not per-member.)
  - R2 base `edge_2(From,To)`: the bound From's source is the message-
    receive atom, no interior tc read → **kBaseAtom** (Demand.cpp:659),
    NOT added to pushdown_reads. Guard `^join.g_bf.R2` pivots d_tc^bf on
    the edge_2 read's From column; restoring TUPLE; the R2 member TUPLE
    rewired.

    join ^join.g_bf.R1s1 [pivot From:u64] {
      .lhs <- ^tuple.d_bf.reader (From)            ; demand side = joined_views[0]
      .rhs <- ^tuple.tc_read.R1s1 (From, X)        ; the guarded tc read = [1]
    } -> (From:u64, X:u64)                         ; pivots lead (C7)
      producer=DEMAND-GUARD
      => ^tuple.restore.g_bf.R1s1 (From, X)
    ;; ^join.g_bf.R1s2, ^join.g_bf.R2 analogous (pivot on the demanded
    ;; position; each producer=DEMAND-GUARD, each with a restoring TUPLE).

### 2.4 The fb slice — d_tc^fb MERGE, readers, guards

`d_tc^fb` is demanded by: the reaching_to seed (root member). R1's
propagation from fb produced ff (subgoal 1, inert) and bb (subgoal 2) —
NEITHER re-demands fb. So d_tc^fb has ONLY the root member (no
propagation member from this body). This is the key STRUCTURAL contrast
with bf: bf's frontier is fed by the recursion (self-sustaining), fb's
is fed ONLY by the seed and then hands off to bb.

    tuple ^tuple.d_fb.root_head (To:u64)           ; over ^select.d_tc_fb
      producer=DEMAND-SEED
      => ^tuple.d_fb.root_member (To)
    tuple ^tuple.d_fb.root_member (To:u64)
      producer=INSERT
      => ^merge.d_tc_fb (To)
    merge ^merge.d_tc_fb (To:u64)                  ; TABLE %d_tc_fb, ONE member
      producer=MERGE-INSERT
      => ^tuple.d_fb.reader (To)
    tuple ^tuple.d_fb.reader (To:u64)
      producer=SELECT
      => ^join.g_fb.R1s1 .lhs(To)                  ; guards R1 subgoal-1 at fb head... 

Guard family for fb — the head adornment fb over R1 means subgoal 1 is
demanded at ff (inert → NO guard minted for it; the fb frontier does not
restrict an all-free subgoal read) and subgoal 2 at bb (guarded by
d_tc^bb, NOT d_tc^fb — see 2.5). So the fb reader guards ONLY:
  - R2 base at fb: `edge_2(From,To)` with To bound → **kBaseAtom**,
    pivot d_tc^fb.To against edge_2's To column.
  - the reaching_to QUERY PROJECTION (§2.7) at fb.

The subtlety D3 must get right: an fb-headed R1 does NOT produce an
fb-guarded recursive tc read; it produces an ff read (dropped) and a bb
read (guarded by the bb slice). So `^tuple.d_fb.reader`'s only body-guard
consumer is the R2 base member. (The `... ` above is completed by the R2
guard, not an R1 subgoal guard.)

### 2.5 The bb slice — d_tc^bb MERGE, readers, guards

[AMENDED: HIGH/FINDING-1 — bb's demand-seed prop member was drawn as a
single DEMAND-PROP TUPLE projecting (X-as-From, To); that is structurally
impossible (the two key columns live in DIFFERENT views) and left bb — the
divergence-carrying adornment — with no correct seed producer, so d_tc^bb
would be UNDER-populated and reaching_to would silently UNDER-report. The
seed is now drawn as a magic-JOIN member, and the join-fed demand-seed
primitive is named as a NEW D3 obligation (not a reject-lift). The
superseded projection reasoning is retained below the corrected member.]

`d_tc^bb` (key arity 2: From,To) is demanded by: R1's subgoal-2 read at
adornment bb (propagated from the fb-headed R1). Its root member comes
from a PROJECTION of the demanding subgoal — but bb is NOT a top-level
seeded query, so its root seed member is the fb-body's subgoal-2 demand
join, not a fresh query receive... EXCEPT the D3 injector still
fabricates `demand__tc_bb/2` and a receive for it (2.2), because the
frozen-instance / seam machinery needs a real message.

THE CRITICAL STRUCTURAL POINT (FINDING-1). bb's demand key is (X, To). X
is column 1 of subgoal-1's read `tc(F,X)`; To is the fb-headed rule's
HEAD-bound To, which traces to the `d_tc^fb` demand frontier — NOT to any
column of subgoal-1's tc read. The two key columns live in DIFFERENT
views. A single projection TUPLE cannot combine them. Magic-sets is
explicit: the demand rule for fb's subgoal-2 is

    m_tc^bb(X, T) :- m_tc^fb(T), tc^ff(F, X).

a JOIN of the fb demand frontier (supplying T) with subgoal-1's tc
materialization (supplying X). So the bb propagation member is a
magic-JOIN, not a projection:

    tuple ^tuple.d_bb.root_head (From:u64, To:u64) ; over ^select.d_tc_bb
      producer=DEMAND-SEED
      => ^tuple.d_bb.root_member (From, To)
    tuple ^tuple.d_bb.root_member (From:u64, To:u64)
      producer=INSERT
      => ^merge.d_tc_bb (From, To)                 ; MERGE member 0

    ;; propagation member from the fb-headed R1 subgoal-2 demand:
    ;; a MAGIC-JOIN of the fb demand frontier (supplying To) with
    ;; subgoal-1's tc read (supplying X at col 1). This is a NEW
    ;; join-fed demand-seed primitive D3 must GROW — the landed pass
    ;; has no such emission path (see §4.3, §5.3).
    join ^join.d_bb.seed [pivot none] {              ; band-key on X's binding
      .lhs <- ^tuple.d_fb.reader (To)                ; fb frontier supplies To
      .rhs <- ^tuple.tc_read.R1s1 (From, X)          ; subgoal-1 read supplies X
    } -> (X:u64, To:u64)                             ; project (X-as-From, To)
      producer=DEMAND-PROP-JOIN                       ; NEW primitive (D3-grown)
      => ^tuple.d_bb.prop_member (From, To)          ; X re-labelled From
    tuple ^tuple.d_bb.prop_member (From:u64, To:u64)
      producer=INSERT
      => ^merge.d_tc_bb (From, To)                   ; MERGE member 1

    ;; SUPERSEDED (FINDING-1): the original draw was a single projection
    ;; TUPLE — kept here as the lane's reasoning, marked wrong:
    ;;   tuple ^tuple.d_bb.prop (From, To) producer=DEMAND-PROP
    ;;     "projects (X-as-From, To) off subgoal-1's binding of X + head To"
    ;; This elided the join between d_tc^fb and subgoal-1's read; the two
    ;; key columns are not co-resident in one view, so no TUPLE can mint it.

    merge ^merge.d_tc_bb (From:u64, To:u64)        ; TABLE %d_tc_bb
      producer=MERGE-INSERT
      => ^tuple.d_bb.reader (From, To)
    tuple ^tuple.d_bb.reader (From:u64, To:u64)
      producer=SELECT
      => ^join.g_bb.R1s2 .lhs(From, To)            ; 2-pivot guard

Guard family for bb — the bb-headed R1 (both From,To bound): subgoal 1
`tc(From,X)` → bf (guarded by the BF slice's reader, not bb); subgoal 2
`tc(X,To)` → bb. So d_tc^bb's reader guards the recursive tc read of
subgoal 2 with a TWO-PIVOT join (both From and To pivot):

    join ^join.g_bb.R1s2 [pivot From:u64, pivot To:u64] {
      .lhs <- ^tuple.d_bb.reader (From, To)        ; 2-column demand side
      .rhs <- ^tuple.tc_read.bb.R1s2 (From, To)    ; the guarded tc read
    } -> (From:u64, To:u64)
      producer=DEMAND-GUARD
      => ^tuple.restore.g_bb.R1s2 (From, To)

This is a **kPushDown** site with `pivot_pos = {0, 1}` (both bound
columns share one guard site — Demand.cpp:718-726 merges per-bound-column
sites into one when they share consumer+read; the site's `pivot_pos`
accumulates both positions). MintGuardJoin emits both pivots leading (C7),
`num_pivots=2`.

### 2.6 The query-projection guards (step-8, per bound query)

Each bound `#query` gets ONE query-projection guard (Demand.cpp:989-1004,
the step-8 family), pivoting the query's bound columns against a FRESH
receive-projection of its OWN adornment's raw seed (producer=DEMAND-RAW-SEED,
NOT the derived d_tc^α reader). Two bound queries → two such guards:

    ;; reachable_from (bf): guard its tc projection on From against the
    ;; RAW demand__tc_bf receive projection.
    tuple ^tuple.rawseed.bf (From:u64)             ; FRESH projection off ^select.d_tc_bf
      producer=DEMAND-RAW-SEED
      => ^join.q_bf .lhs(From)
    join ^join.q_bf [pivot From:u64] {
      .lhs <- ^tuple.rawseed.bf (From)             ; raw seed = demand side
      .rhs <- ^tuple.q_bf.read (From, To)          ; reachable_from's tc reader
    } -> (From:u64, To:u64)
      producer=DEMAND-GUARD
      => ^tuple.restore.q_bf (From, To)
      => ^merge.reachable_from (From, To)          ; rewired query-projection member

    ;; reaching_to (fb): guard its tc projection on To against the RAW
    ;; demand__tc_fb receive projection.
    tuple ^tuple.rawseed.fb (To:u64)
      producer=DEMAND-RAW-SEED
      => ^join.q_fb .lhs(To)
    join ^join.q_fb [pivot To:u64] {
      .lhs <- ^tuple.rawseed.fb (To)
      .rhs <- ^tuple.q_fb.read (From, To)
    } -> (From:u64, To:u64)
      producer=DEMAND-GUARD
      => ^tuple.restore.q_fb (From, To)
      => ^merge.reaching_to (From, To)

    ;; is_node (ff): ALL-FREE, demand-inert → NO query-projection guard.
    ;; is_node's projection reads tc UNGUARDED (full closure), by design.
    tuple ^tuple.q_isnode.read (Node:u64)          ; projects tc col 0 or col 1
      => ^merge.is_node (Node)                     ; unguarded — ff is inert

### 2.7 Shared vs duplicated structure; does CSE fold across adornments?

SHARED across all adornments (exactly one of each):
  - the base program: `^select.edge`, the tc MERGE `^merge.tc`, tc's
    TABLE, R1's body JOIN skeleton, R2's base atom read.
  - the recursive tc reads THEMSELVES are the same physical reads —
    each gets guarded once per demanding adornment, but the underlying
    full-width tc reader is shared; the guards fan out from it.

DUPLICATED per emitted adornment (one per α ∈ {bf, fb, bb}):
  - the fabricated message + local + IO + receive SELECT (2.2);
  - the d_tc^α MERGE + its root/prop members + the shared d_p^α reader
    (2.3/2.4/2.5);
  - the guard JOIN family + restoring TUPLEs;
  - the query-projection guard + raw-seed projection (2.6), one per
    BOUND query (bf, fb only — ff none).

**Does CSE fold anything ACROSS adornments? NO — the CONCLUSION is sound;
the argument is from the VERIFIED CSE reality (d1 §3.x).**

[AMENDED: MEDIUM/FINDING-3 — the mechanism paragraph mis-stated the CSE
machinery: it cited Join.cpp:467 (Join::Equals) for views that are TUPLEs,
and claimed Equals "short-circuits on structure BEFORE group_ids." Both
wrong. The demand-source projections are TUPLEs governed by
`QueryTupleImpl::Equals` (Tuple.cpp:270ff); in it, `InsertSetsOverlap`
(the group_ids test) is INSIDE the early-return `||` chain and runs BEFORE
the projected-column-edge check, and `columns.Size()` does NOT separate
equal-arity adornments. The load-bearing discriminator is
`ColumnsEq(input_columns)`. The conclusion (no cross-adornment merge) is
unchanged; the reason and anchor are corrected below.]

The demand-source PROJECTION views are TUPLEs, so the governing code is
`QueryTupleImpl::Equals` (Tuple.cpp:270ff), NOT Join::Equals. Its early-
return `||` chain is: `!that || can_receive_deletions≠ ||
can_produce_deletions≠ || columns.Size()≠ || InsertSetsOverlap(this,that)`,
and only AFTER that (past `eq.Insert`) does it compare the projected-
column EDGES via `ColumnsEq(input_columns)`. Three facts fix the
mechanism:

  - `columns.Size()` does NOT distinguish bf from fb — both are key-
    arity-1 projections (size 1), so the size short-circuit does not fire
    for them. (bb is size 2 and IS separated here, but the interesting
    equal-arity case is not.)
  - `InsertSetsOverlap` is a NON-BLOCKER, not a discriminator: the demand-
    source projections have EMPTY group_ids (only JOIN/AGG/KVINDEX seed
    group_ids — A18, Optimize.cpp:424), and InsertSetsOverlap returns
    false whenever either group_ids is empty (View.cpp:1479). So it does
    NOT separate the adornments; it merely fails to short-circuit them
    apart.
  - The SOLE discriminator for equal-arity adornments is
    `ColumnsEq(input_columns)` comparing the projected-column EDGES —
    which runs AFTER the group_ids clause, not before it.

  - bf's d-source projects `[From]` (col 0); fb's projects `[To]`
    (col 1); bb's projects `[From, To]` (both cols). DIFFERENT projected-
    column edges → `ColumnsEq(input_columns)` returns false. NO cross-
    adornment merge. SOUND. (The load-bearing weight is ColumnsEq, not a
    structural size/group_ids short-circuit.)
  - The bound-column set IS the adornment (d1 §3.x): two DIFFERENT
    adornments can never project the same column set by construction, so
    `ColumnsEq(input_columns)` always separates them regardless of the
    (non-blocking, empty-group_ids) InsertSetsOverlap clause. And if two
    subgoals of the SAME adornment project the
    same key (e.g. bf's two recursive tc reads both projecting From),
    they DO CSE-fuse — that is the desired shared-demand-frontier fusion
    (benign, wanted: fewer materialized demand rows).

The guard JOINs are a SEPARATE argument governed by Join::Equals
(Join.cpp:467-471), NOT the TUPLE Equals above. They additionally cannot
collapse onto their unguarded twins or across adornments: MintGuardJoin
gives each `num_pivots >= 1` plus the extra demand child, so structural
distinctness (E-32) holds through Optimize. A bf guard (1 pivot on From,
demand child d_tc^bf) and a bb guard (2 pivots, demand child d_tc^bb)
differ in num_pivots AND children → Join::Equals:467-471 short-circuits. VERIFIED-CSE conclusion: **the three
adornment slices stay structurally separate through Optimize; the only
fusion is intra-adornment same-key demand-frontier sharing (a feature).**

The CAVEAT for D3 (d1 §3.x verbatim intent): the transform must NOT rely
on group_ids to keep adornments apart — it relies on the projected-
column-set structural difference, which Equals() sees. D3 inherits this
correctness for free; the multi-adornment lift adds no new CSE hazard
because the adornment-distinguishing structure is exactly the bound-
column projection that Equals already discriminates.

---

## 3. The raw-seed-vs-d_p divergence made LIVE

### 3.1 The d4s3 F1 coincidence, restated

The landed single-adornment slice's query-projection guard (step-8)
pivots against the RAW demand-seed receive projection (producer=
DEMAND-RAW-SEED), NOT the derived d_p reader. The d4s3 recipe AMENDMENTS
F1 discharged WHY this is answer-correct in the witness AND flagged
exactly where it stops being a coincidence:

> For bf transitive closure the raw-seed guard (`d_path_bf(From) ⋈ path`)
> and a `d_path`-guard COINCIDE ... because `d_path` = the demand
> frontier = the closure of the seed under the recursion, which for the
> bf adornment PRESERVES `From` (every recursive rule re-keys on the SAME
> F it was demanded at). So path's materialized rows already have
> `From ∈ seed`, and pivoting on the raw seed vs the derived d_path
> yields the same `[From,To]` set.

The coincidence has TWO preconditions, both true in the single-adornment
witness:
  (P1) SINGLE adornment — d_path carries demand for exactly one query's
       key shape, so "the seed" and "the frontier keyed like the seed"
       are about the same demand.
  (P2) FROM-PRESERVING recursion — the bound column flows to the SAME
       position in every recursive subgoal, so the frontier's keys are a
       superset-closure of the seed's keys on that ONE position.

### 3.2 Where it BREAKS with two+ adornments

The non-linear body `tc(F,T) : tc(F,X), tc(X,T)` violates BOTH
preconditions the moment reaching_to (fb) is present:

  - (P1 broken) THREE adornments {bf, fb, bb} share the ONE `tc` table.
    d_tc^bf, d_tc^fb, d_tc^bb are DISTINCT frontiers. The union of all
    demand keyed onto tc is `d_tc^bf ∪ d_tc^fb-derived ∪ d_tc^bb` — but
    a given query issued demand at only ONE of them.
  - (P2 broken) reaching_to's fb-headed R1 subgoal-2 `tc(X,To)` re-keys
    SIDEWAYS: X (bound by subgoal 1) is a DIFFERENT value than the
    query's demanded To. So tc's materialized rows, under the union
    demand, carry `From` values that are reachable-INTO some demanded To
    but were NEVER the reaching_to query's issued key.

CONCRETE FAILURE. Suppose the driver probes `reaching_to(free, To=t0)`
(demand: "who reaches t0?"). The fb frontier seeds d_tc^fb = {(t0)}.
Propagation: subgoal 2 `tc(X, t0)` is demanded at bb for every X that
subgoal 1 `tc(From, X)` binds — i.e. tc materializes `(x, t0)` for all x
reaching t0, AND (subgoal 1, ff/bf) intermediate `(from, x)` rows. Now
consider the reaching_to PROJECTION reading tc. If it read the
d_tc-guarded tc alone (union frontier), it would see EVERY (From, To)
pair tc materialized to answer the demand — INCLUDING (from, x)
intermediate rows whose To=x ≠ t0, and rows demanded by a DIFFERENT
query (reachable_from's bf frontier co-resident in the same table). It
would OVER-REPORT: answering reaching_to(To=t0) with pairs whose To is
not t0.

The step-8 query-projection guard pivots reaching_to's projection
against the RAW d_tc^fb seed `{(t0)}` on the To column
(`^join.q_fb` in 2.6) — this PRUNES tc's rows to exactly `(From, t0)`,
the query's issued demand. That guard is NOW LOAD-BEARING, not
redundant. AMENDMENTS F1 says exactly this:

> if `p` is demanded at MULTIPLE adornments, or the recursion does NOT
> preserve the bound key ... then path's rows can carry a `From` that is
> reachable but was never the QUERY's demanded key — and the raw-seed
> guard on the query projection PRUNES those, whereas reading
> `d_path`-guarded `path` alone would over-report. There the query-
> projection guard is LOAD-BEARING, not redundant.

### 3.3 Which reads must move to d_p^α, and the seed schema

In the single-adornment coincidence, one could (wrongly) collapse the
raw seed and d_p: they were answer-equal. D3 must keep them DISTINCT and
route each read to the correct one:

RAW-SEED reads (per-query, the query's OWN issued demand):
  - The step-8 query-projection guard of each bound query pivots against
    that query's raw-seed receive projection — `^tuple.rawseed.bf`
    (`demand__tc_bf` receive) for reachable_from, `^tuple.rawseed.fb`
    (`demand__tc_fb` receive) for reaching_to. This is the query's issued
    key, and it is what makes the answer be "only the demand THIS query
    issued." These must NOT be re-pointed at the union d_tc^α reader.

d_p^α reads (the derived frontier, per-adornment):
  - Every BODY guard (step-7: the kPushDown recursive-read guards and the
    kBaseAtom base-atom guards) pivots against the DERIVED `^tuple.d_<α>.
    reader` for ITS adornment α — bf body guards against d_tc^bf, bb body
    guards against d_tc^bb. A body guard's job is to restrict the RULE's
    materialization to the frontier, which is the closure (d_p), not the
    raw seed. This was already true single-adornment (step-7 always used
    d_reader); D3's addition is that there are now THREE d_p^α readers and
    each body guard must pick the reader matching the adornment its SIP
    walk assigned to that read — NOT a single shared d_p.

THE SEED SCHEMA BECOMES per-adornment:
  - single-adornment: one receive `demand__p_α`, one root member into the
    one d_p MERGE, propagation members from every demanding subgoal, one
    d_p reader; the raw-seed guard and d_p reader happened to be answer-
    equal so the distinction was cosmetic.
  - post-D3: for each emitted adornment α, an INDEPENDENT
    {receive → root_head → root_member → d_tc^α MERGE ← prop_member(s) →
    d_tc^α reader} chain (2.2-2.5). The root member of d_tc^α is fed by
    α's OWN receive; propagation members are fed by whichever subgoals
    the SIP fixpoint demanded AT α (bf: two From-keyed recursive reads;
    fb: none — hands to bb; bb: one From/To-keyed recursive read). The
    raw-seed projection for a bound query is a SEPARATE fresh projection
    off α's receive (step-8), NEVER the root member and NEVER the
    d_tc^α reader.

So the schema change is: (a) the seed structure is replicated per emitted
adornment; (b) the propagation-member wiring is now adornment-directed
(the SIP walk labels each demanding read with its adornment, and the prop
member goes into THAT adornment's MERGE); (c) the raw-seed-vs-d_p
distinction that was cosmetic single-adornment is now SEMANTIC — the
step-8 guards MUST stay on the raw seed and the step-7 guards MUST stay
on d_p^α, and D3 cannot short-circuit either onto the other.

### 3.4 The one-instance-per-key reading (the epoch charter)

This is exactly the §18.5(D) note: "the raw-seed-vs-d_p guard DIVERGENCE
becomes live here (the d4s3 F1 coincidence argument no longer holds
sideways)." In the keyed-instance lowering (D1/D2), each d_tc^α frontier
ROW is an instance key; the divergence says: the query answer is the
projection of tc RESTRICTED to the instances the QUERY named (raw seed),
while the recursion populates ALL instances the SIP fixpoint reached
(d_p^α closure). The two-lowerings equivalence gate (§18.5(C)) must hold
under BOTH the flat guarded copy and the nested instance store for the
four-adornment witness — which is precisely why the four-adornment
witness is the D3 acceptance case: it is the smallest program where a
lowering that conflates raw-seed and d_p produces a WRONG answer, so the
oracle catches the conflation.

---

### What D3 must BUILD (not just delete) — the two structural obligations

[AMENDED: HIGH/FINDING-1 + HIGH/FINDING-2 — §18.5(D) frames D3 as "delete
the rejects + reuse the per-adornment registry." That undercounts the
work. Two NEW pieces of emission machinery are mandatory, neither a
reject-lift:]

  - **OBLIGATION A (join-fed demand seed, FINDING-1).** A propagated
    adornment whose demand key draws columns from TWO different views
    (bb: X from subgoal-1's read, To from the d_tc^fb frontier) needs a
    magic-JOIN demand-seed member — `m_tc^bb(X,T) :- m_tc^fb(T),
    tc^ff(F,X)` (§2.5). The landed prop-member primitive (Demand.cpp:898-
    916) projects columns off ONE p-read only; there is NO code path that
    emits a join-fed seed. D3 must GROW this primitive (`producer=
    DEMAND-PROP-JOIN`). Lifting REJECT-21/25 only stops the pass erroring
    on the sideways re-key; it does NOT synthesize the join that populates
    d_tc^bb. Without this, bb is under-populated and reaching_to under-
    reports.

  - **OBLIGATION B (per-interior-read guard pass, FINDING-2).** D3
    REPLACES the one-site-per-member guard machinery (Step 3b mints one
    GuardSite per member on the member's head-bound OUTPUT columns; Step 7
    mints one MintGuardJoin per site) with a per-recursive-read pass: each
    full-width tc read inside a member is guarded `d_tc^α ⋈ tc` at that
    read, α from the read's SIP label. Subgoal-1 and subgoal-2 of one R1
    member may carry different α. This is a guard-pass rebuild, not a
    reuse. REJECT-18 fires in Step 3a before any of it, so this is a design
    target for a rebuilt pass.

## 4. The per-adornment registry + injector picture

### 4.1 The registry becomes per-adornment natively

Today `QueryImpl::demand_forcings` holds ONE `QueryDemandForcing{query,
message, bound_params}` (lane-fabric-inject §2a-2b; include/.../Query.h:
950-962). The injector match (Build.cpp:465-474) ALREADY keys on BOTH
`entry.query == query` (DeclarationContext = name+arity only) AND
`ParsedDeclaration(entry.query).BindingPattern() ==
ParsedDeclaration(query).BindingPattern()` — the "second belt." Today
that second belt is redundant with the first-belt >1-adornment reject
(there is never more than one entry per name), existing purely so a
future fence-lift cannot re-open the cross-wire (demand_multi_adorn_1.dr
is its fire test: two adornments of `reachable_from` share one
DeclarationContext and would compare `==` EQUAL, so without the
BindingPattern belt the fb entry point would inherit the bf injector — a
silent wrong answer).

D3's lift: `demand_forcings` gains ONE entry PER emitted adornment
(three here: reachable_from@bf, reaching_to@fb, and — if the point-query
seam is exposed — a bb entry; bb has no top-level bound query in this
program, so its injector is only needed if a driver can issue bb demand
directly; otherwise bb is driven purely by SIP propagation and has NO
registry entry, only a fabricated message for the seam). The
CROSS-WIRE BELT becomes LOAD-BEARING instead of a redundant guard:

  - reachable_from's entry: `{query=reachable_from, message=demand__tc_bf,
    bound_params=[0]}`. Injector match at Build.cpp:467 finds it by
    name+arity AND BindingPattern bf.
  - reaching_to's entry: `{query=reaching_to, message=demand__tc_fb,
    bound_params=[1]}`. Different name → different DeclarationContext →
    the first-belt `==` already separates it here (distinct query NAMES,
    unlike demand_multi_adorn_1's same-name case).

The demand_multi_adorn_1 case (SAME name, two adornments) is where the
second belt is irreplaceable: reachable_from@bf and reachable_from@fb
share a DeclarationContext, so ONLY the BindingPattern equality
(:468-469) routes each driver call to its own injector. D3 turns THAT
exact program from a REJECT into a compiled two-adornment case; the belt
that was defensive becomes the dispatch mechanism.

### 4.2 The fabrication uniquing (G3 under multiple demand__ names)

Each adornment fabricates independently (2.2). The G3 pre-check
`DemandFabricationWouldCollide(msg_name, local_name, arity)` (Parse/
Demand.cpp:140-157) runs BEFORE each adornment's fabrication, scanning
`module->messages` and `module->locals` by (name-string, arity). The
adornment suffix (`_bf`/`_fb`/`_bb`) makes the three fabricated names
pairwise distinct, so:
  - inter-adornment: no collision (distinct suffixed names).
  - vs user decls: a user `#message demand__tc_bf/1` still cleanly
    rejects (the reserved-prefix guarantee holds per adornment).
  - `CreateDerived` (Demand.cpp:189-191) mints a fresh DeclarationContext
    per message, so the three demand messages never structurally merge
    with each other or with a same-spelling user message (lane-fabric-
    inject §1e).
  D3 loops the fabrication over emitted adornments; the single-shot
`MarkDemandFabricated()` (Demand.cpp:1053, N5 the flag admits one
fabrication PASS not one call) fires ONCE at pass end after ALL
adornments fabricated — NOT per adornment. The re-entry guard G2
(DemandMessagesFabricated at head) is unchanged.

### 4.3 The >1-adornment REJECT sites D3 deletes

From the verified single-adornment pipeline (lane-pipeline.md), the
rejects that fire on a multi-adornment program, each with its anchor:

  1. **REJECT-2** — `bound_queries.size() > 1`, Demand.cpp:412-416
     ("Multiple demanded (bound) queries"). Fires on reachable_from +
     reaching_to together. D3 DELETES this: the selection loop
     (Demand.cpp:394-446) now collects ALL bound queries and processes
     each; `bound_queries` becomes a per-query iteration, not a
     size-1 assertion.

  2. **REJECT-3** — `patterns.size() != 1`, Demand.cpp:429-439
     ("Multi-adornment demand ... >1 binding pattern"). This is the
     FIRST cross-wire belt (per-name). Fires on demand_multi_adorn_1
     (reachable_from at bf AND fb). D3 DELETES this: a query name with
     >1 binding pattern now mints one d_tc^α + one guard family + one
     registry entry PER pattern (the per-adornment registry, 4.1).

  3. **REJECT-21** — the recursive-read-at-different-position reject,
     Demand.cpp:645-649 (kReadAtTuple branch, `in_col->Index() != pos`
     → "Multi-adornment demand"). Fires when a recursive subgoal reads
     tc at a position OTHER than the demanded one (the sideways re-key,
     e.g. reaching_to's subgoal-2). D3 REPLACES this reject with:
     ASSIGN the read a DIFFERENT adornment (bb here) and route its prop
     member into that adornment's MERGE. The reject becomes an
     adornment-derivation branch.

  4. **REJECT-25** — the JOIN-traced-read-at-different-position reject,
     Demand.cpp:700-703 (kPushDown branch, `found_col->Index() != pos`
     → "Multi-adornment demand"). Same story as REJECT-21 but for the
     pushdown (through-JOIN) path — the sideways re-key arriving through
     R1's body JOIN. D3 REPLACES it identically: the differing position
     is a NEW adornment, not an error.

  These four are the "the >1-binding-pattern rejects (Demand.cpp ~:434 +
the two per-site rejects)" of §18.5(D). REJECT-21 and REJECT-25 are the
"two per-site rejects"; REJECT-3 (~:434) is the per-name belt; REJECT-2
is the multiple-bound-query gate that must also lift. The ControlFlow
BindingPattern second belt (Build.cpp:468-469) is NOT deleted — it
inverts from a defensive redundancy to the live per-adornment dispatch.

NOT deleted (still clean diagnostics under D3): NEGATE/AGG in a demanded
body (REJECT-16, Demand.cpp:602-605 — the demand sink); the stray-
consumer rejects (REJECT-28/29, Demand.cpp:754-764); self-join over-read
REJECT-18 (Demand.cpp:610-613, `p_reads > 1`) — CAUTION: the non-linear
tc body reads tc TWICE per R1 member, which today trips REJECT-18. D3
MUST also lift REJECT-18 for the multi-adornment case (two DISTINCT
adornments reading the same relation is exactly the non-linear tc shape,
"two distinct subgoals, two adornments — do not conflate," d4s3 §3.7
note). This is a FIFTH reject site D3 touches — see Open Questions.

---

## 5. Predictions: suite count, witness plan, open questions

### 5.1 Suite-count prediction for D3

Epoch-start suite is 168 (KeyedInstances.md §0). D3's landing adds the
four-adornment tc witness (below) and flips demand_multi_adorn_1 from
diagnostic to a compiled golden. Predicted deltas:

  - NEW golden case `demand_tc_four` (the four-adornment witness):
    `.dr` + `.main.cpp` + `.drflags(-demand)` + `.batches` +
    `.oracle.stdout` + `.monotone.stdout` → +1 case (goldens count
    +1; the four opt modes stay orthogonal to -demand).
  - `demand_multi_adorn_1` FLIPS from all-4-modes-diagnostic to a
    golden (the demand_multi_adorn analog of aggregate_1's R3 flip):
    it must be REMOVED from runall.sh's expected-diagnostic list and
    gain a `demand_multi_adorn_1.stdout` golden + a real `.main.cpp`
    driver probing both bf and fb. Case count unchanged (already
    counted), but the diagnostic-list arithmetic shifts: golden-less
    cases drop from 11 to 10 (demand_multi_adorn_1 leaves the list),
    plain goldens +1.

    [AMENDED: MEDIUM/FINDING-4 — the fb ADORNMENT of this case's body is
    demand-INERT and must be reconciled with §5.3 Q-ff-materialization.
    demand_multi_adorn_1's body is the RIGHT-LINEAR
    `path(F,T) :- path(F,M), edge_2(M,T)`, NOT the non-linear self-join.
    Under fb (F free, T bound): subgoal-1 `path(F,M)` is F-free/M-free →
    the recursive path read is demanded at **ff (INERT)**; subgoal-2
    `edge_2(M,T)` is the base atom. So the fb query cannot restrict the
    path materialization — fb reduces to FULL materialization (the §1.3 /
    Q-ff-materialization trap). DISPOSITION for this case: fb-over-right-
    linear COMPILES by full materialization, so demand_multi_adorn_1 is a
    CROSS-WIRE-DISPATCH witness (its two adornments route to their own
    injectors via the BindingPattern belt, §4.1), NOT a divergence
    witness — its fb answers are correct-but-UNRESTRICTED, and the golden
    must say so. It is NOT a Q-ff-materialization REJECT because BOTH of
    its queries share ONE relation and BOTH are #query consumers (there is
    no all-free co-consumer forcing an under-report); fb simply
    materializes the full path and reads the correct T-slice. The
    genuine-divergence + genuine-restriction witness is `demand_tc_four`
    (§5.2, the non-linear body), where bb IS restrictable. Q-ff-
    materialization's CLEAN REJECT applies to an ALL-FREE co-consumer
    (is_node), a DIFFERENT case from an fb #query over a right-linear
    body — see §5.3.]
  - Possibly a SECOND four-adornment sibling if the linear (From-
    preserving) vs non-linear split is worth two witnesses; the
    non-linear one is the load-bearing case (it exercises the
    divergence + REJECT-18 lift).

Predicted post-D3 suite: **170** (168 + demand_tc_four + one
optional sibling), demand_multi_adorn_1 reclassified. The IR-golden
directive (§0.5 directive 5) also wants `demand_tc_four.df.golden`
and `.deltarel.golden` mode-pinned to opt once (F) has restored
demand-ON IR determinism.

### 5.2 The witness plan (demand_tc_four)

`.dr` — the E-41 non-linear body with two bound queries (this is the
smallest program exercising ALL THREE emitted adornments + the
divergence):

    #message edge_2(u64 From, u64 To).
    #local tc(u64 From, u64 To).
    tc(F, T) : edge_2(F, T).
    tc(F, T) : tc(F, X), tc(X, T).       ; non-linear — bf, bb, (ff inert)
    #query reachable_from(bound u64 From, free u64 To) : tc(From, To).
    #query reaching_to(free u64 From, bound u64 To) : tc(From, To).

`.drflags` — `-demand` (identical to the landed witness).

`.main.cpp` driver shape (the force.dr ABI, both adornments): a fixed
edge stream sent once; then FIXED probe keys per adornment — never an
all-free enumeration (the tc.dr inertness shape). For each probe:
  - `reachable_from_bf(db, log, functors, f)` → cursor of To; sort;
    print. (bf: "what does f reach?")
  - `reaching_to_fb(db, log, functors, t)` → cursor of From; sort;
    print. (fb: "what reaches t?")
Each entry-point call injects its adornment's demand seed through its
OWN per-adornment injector (4.1), runs the flow, drains the cursor.
Cursor contract: drain fully before the next call; sort keyed drains.
The DIVERGENCE is exercised precisely because reaching_to(t) must
return ONLY pairs whose To=t (raw-seed guarded), even though tc's
table co-holds reachable_from's bf-demanded rows and the bb
intermediate rows.

`.oracle.stdout` / `.monotone.stdout` — ORACLE-BLESSED: the oracle
(`bin/Oracle`) evaluates the SAME program definitionally WITHOUT demand
(full closure), then for each probed key emits exactly the closure rows
matching that key's adornment. The demand-ON driver's per-key answers
MUST equal the oracle's per-key rows — the answer-identity referee. This
is the acceptance gate for the divergence: a lowering that conflates
raw-seed and d_p (§3) produces EXTRA rows for reaching_to and the oracle
diff catches it. `.batches` — the shared edge stream both the driver and
oracle consume.

Blessing per the standing discipline: `runall.sh --bless` after review,
never auto-on-red.

### 5.3 Open questions

  - **Q-join-fed-seed** [ADDED: HIGH/FINDING-1]: bb's demand seed is a
    magic-JOIN (`m_tc^bb(X,T) :- m_tc^fb(T), tc^ff(F,X)`, §2.5), not a
    projection. The landed prop-member primitive (Demand.cpp:898-916)
    projects columns off ONE p-read only and has NO join-fed path. D3
    must GROW a `DEMAND-PROP-JOIN` primitive: a JOIN whose lhs is the
    supplying adornment's d_p reader (d_tc^fb) and whose rhs is the
    interior tc read (subgoal-1), projecting the propagated key into the
    downstream adornment's MERGE (d_tc^bb). OPEN: what pivots the join
    (here X binds via subgoal-1's materialization — is the join keyed on
    X's binding or a full cross filtered by the frontier?), and does the
    LowerDRFlow product-arm machinery already have a shape this reuses, or
    is it a new DR-IR op? This is a first-class emission obligation, NOT
    subsumed by the REJECT-21/25 lifts (those only stop erroring; they do
    not synthesize the join).

  - **Q-guard-pass-rebuild** [ADDED: HIGH/FINDING-2]: D3 replaces the
    one-site-per-member guard machinery (Step 3b/Step 7) with a
    per-interior-read guard pass (§2.1, §4 Obligation B). OPEN: how much of
    Step 3b's GuardSite tracing (kReadAtTuple/kPushDown/kBaseAtom
    classification) survives when the site is keyed to a read rather than
    a member's head-bound output; does REJECT-27 (bound columns share one
    site) dissolve or re-scope to per-read; and does MintGuardJoin need a
    per-read α parameter it does not currently take. REJECT-18 must lift
    first (Q-reject18-lift).

  - **Q-bb-injector**: does bb get a registry entry / driver-callable
    injector, or is it SIP-propagation-only (no top-level bound query
    issues bb in this program)? Recommendation: bb has a fabricated
    message (for the seam/instance machinery) but NO registry entry
    unless a driver can issue a ground (From,To) point query. This
    program has no bb-adorned #query, so bb is propagation-only — its
    d_tc^bb is fed solely by the fb-body's subgoal-2 prop member (2.5),
    never by an injector. CONFIRM the injector builder tolerates a
    fabricated message with no QueryDemandForcing entry (it should: the
    handler is registered by the IO loop regardless — lane-fabric-inject
    §2f note; only the entry-point suppression consults IsDemandMessage,
    which scans demand_forcings and would NOT list bb → the bb message's
    public entry would NOT be suppressed. THIS IS A HOLE: an unsuppressed
    bb entry point leaks a driver-callable demand seam. D3 must either
    add a bb registry entry purely for suppression, or generalize
    IsDemandMessage to a separate fabricated-message set.).

  - **Q-reject18-lift**: REJECT-18 (`p_reads > 1`, self-join) currently
    blocks the non-linear body ENTIRELY, even single-adornment. Lifting
    it is a prerequisite for the four-adornment witness. Is the lift
    scoped to demand-mode only, or does it also affect the flag-off
    self-join handling? (The reject is inside ApplyDemandTransform, only
    reached under -demand, so flag-off is untouched — but confirm the
    two-distinct-adornments-reading-one-relation invariant does not
    break the stray-consumer accounting REJECT-28/29.)

  - **Q-linear-vs-nonlinear-witness**: keep the From-preserving linear
    witness (demand_tc_witness, single adornment) AND add the non-linear
    four-adornment one, or replace? Recommendation: KEEP both — the
    linear one is the (F)-determinism gate carrier (§0.5 directive 5)
    and its single-adornment coincidence is the baseline; the non-linear
    one is the divergence + multi-adornment gate. Different roles.

  - **Q-ff-materialization**: is_node (ff, inert) reads tc UNGUARDED
    (full closure). Under -demand with OTHER adornments present, tc is
    materialized ONLY to the union frontier — so is_node would see a
    PARTIAL closure (only rows some bf/fb/bb demand reached), not the
    full closure it semantically needs. This is the tc.dr inertness
    trap made concrete: an all-free consumer co-resident with demanded
    ones gets WRONG (under-reported) answers unless tc is fully
    materialized. D3 must decide: reject a program mixing an all-free
    consumer with demanded queries over the same relation (the safe
    clean diagnostic), or force full materialization when any inert
    consumer exists (defeating demand). Recommendation: CLEAN REJECT
    (an all-free consumer of a demanded relation) — matches the
    demand_tc_witness header's deliberate "NO all-free sibling" note.
    This makes `is_node` in transitive_closure.dr a reject-trigger under
    -demand, which is honest: that program cannot be demand-lowered
    while is_node co-consumes tc.
    [AMENDED: MEDIUM/FINDING-4 — scope note so §5.1 and §5.3 agree: this
    CLEAN REJECT is triggered ONLY by an ALL-FREE (ff) co-consumer sharing
    the relation. It is NOT triggered by an fb #query over a right-linear
    body (demand_multi_adorn_1's fb arm), which is demand-inert in the
    sense that fb reduces to full materialization but is still a well-
    formed bound #query that reads the correct T-slice — that arm
    COMPILES (correct-but-unrestricted), it does not reject. The
    distinction: an ff query has an EMPTY key and never enters
    bound_indices (§1.3), whereas an fb query has a non-empty key whose
    SIP walk merely fails to restrict a right-linear recursion. §5.1's
    demand_multi_adorn_1 flip stands under this reading.]

  - **Q-divergence-oracle-strength**: confirm the oracle's per-key
    projection actually distinguishes the conflated-vs-correct lowering
    on this witness (i.e. there EXISTS an edge stream where union-d_tc
    guarding over-reports for reaching_to). §3.2's concrete failure
    (intermediate (from,x) rows with To≠t0) is the construction; the
    `.batches` must include a graph where reachable_from's bf frontier
    and reaching_to's fb frontier OVERLAP in tc's table (shared
    intermediate nodes) so the divergence is observable, not vacuous.
