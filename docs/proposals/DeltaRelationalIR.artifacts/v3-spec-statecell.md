======================================================================
v3-spec-statecell.md — the BINDING R3 spec extension (aggregates + KV)
======================================================================
Written 2026-07-16 at the delta-relational-IR epoch, DOCS-ONLY. This
file EXTENDS docs/proposals/DeltaRelationalIR.artifacts/v3-spec.md — it
fills the reserved `statecell: fold/emit/old` effect sub-domain
(v3-spec §2 ln123-126) and the reserved `kStateFold/kStateEmit/kStateOld`
EffKind tail (v3-spec §7.1 ln619) with a normative, implementable-from
specification. It is the coverage-judge R3 line in DeltaRelationalIR.md
§9 ("R3 = separate matrix extension: valued sealed reads + value-fold
op") made concrete.

It BINDS over, and RESOLVES the eight self-flagged gaps of,
average_weight.post-r3.drir.md (G-1..G-8), which is the requirements
list. It is the design ledger AggregatingFunctors.md §2/§3/§4.1 lowered
onto the v3 object model. Where AggregatingFunctors and the v3 object
model CONFLICT, §0 records it loudly; everything after §0 assumes those
resolutions.

Sources re-derived end-to-end for this file: AggregatingFunctors.md
(§2 GROUP-UPDATE/state-cell semantics + worked stratum-phase diff; §3
declared algebra + engine-owned state + batch ABI; §4.1 mutable(a));
average_weight.post-r3.drir.md (G-1..G-8, the double-aggregate +
KV-index fixture); v3-spec.md §2 (effect domain; the ten predicates
E-14; TryClaimDel/Add F17), §7.1 (enum/struct sketch), §A/§B amendments
(esp. B-8: engine scalars get a value-node home, global:rmw); the ledger
§9 (R3 acyclic-band billing, F-6 sole-deriver assert); StackSafeNegation
§5.5 (Commit sweep); include/drlojekyll/Runtime/Table.h (Table::Seal /
kInI watermark / DiffTable::Commit / touched / CompactRowsInPlace);
Query.h (QueryAggregate group/config/summary; QueryKVIndex key/value/
NthValueMergeFunctor); lib/DataFlow/Stratify.cpp:267-296 (the negate
unstratified reject to mirror); bin/Oracle/Main.cpp (the from-scratch
semi-naive path + .oracle.stdout/.monotone.stdout sidecars);
lib/ControlFlow/Build/Build.cpp:955-956/1015-1053 (the F14 pre-pass).


======================================================================
§0. CONFLICTS: AggregatingFunctors §2/§3 vs the v3 object model
======================================================================
LOUD, per the mandate. Three genuine conflicts, each resolved in favor
of the v3 factoring (v3-spec §9 F-1: ordering/dataflow are DERIVED
dependence facts, not enum attributes or named vectors), plus one
non-conflict recorded so it is not re-litigated.

C-0a  CONFLICT (object identity): AggregatingFunctors §2 says the
      aggregate "instantiates a (notional) C++ OBJECT with two levels of
      group-by" — an object-per-(group,config) with methods
      init/fold/unfold/merge/emit (§3 ENGINE-OWNED STATE). The v3 object
      model has NO per-tuple object; it has TYPED IR VALUES with
      defs/uses and an effect set (v3-spec §2/§7.1). A per-group object
      is NOT an IR value.
      RESOLUTION: the object is the SEMANTIC CONTRACT and the
      @recompute physical fallback, NOT an IR value and NOT the default
      layout (AggregatingFunctors §3 already says exactly this — "the
      object-per-group model is the semantic contract and the
      opaque-functor fallback, not the default physical layout"). In the
      v3 IR the aggregate's standing state is ONE new IR value: a
      `StateCell` engine scalar-family value (v3-spec §B-8 relaxed the
      StateCell reservation to "engine scalars", effect kind global:rmw).
      The columnar-by-dense-group-id blob store IS that value's runtime
      backing; the notional object is a debug/oracle view of one key's
      slice. NO CONFLICT once the object is read as contract-not-value.

C-0b  CONFLICT (state as a hazard vs constant): AggregatingFunctors §2's
      old()="sealed batch-start value" is presented like kInI — a frozen
      read. v3-spec §2 makes kInI a DISTINGUISHED always-legal read that
      "never participates in WAR/WAW" (a scheduler CONSTANT). But a
      StateCell's fold() WRITES the working value in the SAME band that
      old() reads the sealed value, and emit() reads the working value
      AFTER all folds. So a StateCell is NOT a pure constant like kInI:
      it has a real intra-band WAR (old before fold-that-overwrites... no
      — old reads the SEALED copy, fold writes the WORKING copy; they are
      TWO storage words) and a real RAW (emit after fold).
      RESOLUTION: split the cell into TWO value words per key —
      `sealed` (frozen, kInI-like, RAW-only against STATE_SEAL, never a
      hazard within a flow band, exactly the v3-spec §2 kInI-frozen
      treatment) and `working` (a real read/write IR value: fold writes
      it, emit reads it, RAW/WAR/WAW tracked normally). old(g) reads
      `sealed`; emit(g)/fold(g) touch `working`. This is the ONLY way to
      keep v3-spec's "kInI is a constant, real state is a hazard"
      dichotomy intact. See §1 (the two-word cell) and §5 (the effect
      table splits fold/emit/old across the two words accordingly).

C-0c  CONFLICT (value-fold ≠ counter-fold): AggregatingFunctors §2's
      "state(g).fold(±, summary)" and v3-spec §2's `table: counter±(T)`
      look alike but are NOT the same effect. A counter fold is a
      presence-crossing RMW (Table.h FoldAt: crosses on total 0↔1); a
      StateCell fold is a VALUE mutation with NO presence crossing — the
      crossing (a table event) happens later, at emit_touched, and only
      if new≠old (average_weight.post-r3 G-2). v3-spec's EffKind already
      reserves `kStateFold` DISTINCT from `kCounter` for exactly this.
      RESOLUTION: GROUP_UPDATE's input arm emits `statecell:fold`
      (EffKind kStateFold), NEVER `table:counter±`. The counter±(agg
      table) folds live ONLY in emit_touched's two ordinary UPDATECOUNTs
      (§2 one-net-pair). This resolves G-2: "value-fold without immediate
      crossing" is a first-class effect kind, not an overloaded
      UPDATECOUNT. G-2 CLOSED.

C-0d  NON-CONFLICT (recorded so R3c does not re-open it): the KV-index
      is the degenerate aggregate (group=key, config=(), summary=value,
      algebra=merge functor) — AggregatingFunctors §2 last bullet and
      §4.1 recommendation (a) AGREE with the v3 lowering (one
      GROUP_UPDATE, no separate KVINDEX op). No conflict; §3 records the
      concrete node-deletion recommendation.

C-0e  CONFLICT (algebra-attribute vs pure-dependence IR): §9 F-1 wants
      NO ordering-as-enum-attribute. But the @invertible/@commutative/
      @idempotent/@recompute algebra (AggregatingFunctors §3) DOES sit
      on GROUP_UPDATE/StateCell as an attribute (average_weight.post-r3
      §3 (3)). Is that the enum-attribute anti-pattern §9 rejects?
      RESOLUTION: NO. §9 F-1's target is ORDERING/DATAFLOW encoded as
      attributes (seed-before-drain, deferral, dual-append). Algebra is
      neither ordering nor dataflow — it is a LOWERING SELECTOR for the
      fold/emit implementation of ONE op (fold-via-unfold vs
      fold-via-rescan), exactly like ACCESS's `Lowering how`
      (point-test | section-walk | scan | seek, v3-spec §2 ln159). It
      picks an implementation; it does not pin an order or a def/use
      edge. So it is a legal attribute in the v3 sense. G-4's "the
      lowering fork is unspecified" is closed by §1.3 making the fork
      explicit. G-4 CLOSED (fork spec'd in §1.3; the cost-model
      THRESHOLD within @recompute stays a lowering knob, not IR).


======================================================================
§1. THE STATECELL STORE  (the one genuinely new runtime object)
======================================================================
Per AGG view (QueryAggregate or desugared QueryKVIndex), ONE
StateCellStore, a peer of the agg's DiffTable, NOT part of it. It is the
runtime backing of the v3 `StateCell` engine-scalar-family IR value
(v3-spec §B-8). This is the SINGLE new emitted primitive R3 adds
(average_weight.post-r3 KEY FACT 4); everything downstream of
emit_touched's two UPDATECOUNTs rides existing machinery.

----------------------------------------------------------------------
§1.1 KEYING, DENSE IDS, NON-ALIASING (resolves G-1 + compaction)
----------------------------------------------------------------------
Keyed by (group cols ++ config cols) → a DENSE GROUP ID (0..num_groups),
allocated on first touch exactly like RowStore::FindOrAdd allocates row
ids (Table.h:152). The dense group id indexes the columnar blob arrays.

NON-ALIASING INVARIANT (the compaction answer): StateCell dense group
ids are a SEPARATE id space from the agg DiffTable's row ids. The
DiffTable holds output rows (g.cols ++ summary-value); the StateCell
holds standing per-group state keyed by (group ++ config) ONLY (no
summary value in the key). These are DIFFERENT keys over DIFFERENT id
spaces:
  - DiffTable row = (X, Sum)      keyed by the WHOLE tuple incl. value.
  - StateCell key  = (X)           the group only; its value is the blob.
A DiffTable COMPACTION (Table.h CompactDead, renumbers row ids) DOES NOT
TOUCH the StateCell store: no StateCell id is a DiffTable row id, and no
DiffTable index projects a StateCell id. The cursor-contract renumbering
(CLAUDE.md data-structures epoch) is a DiffTable-row-id concern; group
ids are stable across it. STATE THE INVARIANT in the runtime header
comment and assert it in debug: `cell.KeyAt(gid)` never appears as a
`table.Find`-able row-id argument and vice versa.
Do StateCell stores compact at all? A group id, once allocated, is
retained for the program's life even if the group empties (its working
value returns to the algebra's identity, e.g. Sum=0 / Count=0). This is
the same "monotone tables never compact" rationale (CLAUDE.md): group
ids are an append-only dense namespace; churn is in the WORKING value,
not the id set. If a future COST measurement shows dead-group bloat,
StateCell compaction is a SEPARATE renumbering with its own moved()
callback (mirror CompactRowsInPlace) — explicitly OUT of R3 scope, noted
as a D5-style residue. G-1 CLOSED (StateCell is a first-class runtime
object with its own id space; compaction non-interaction stated + to be
asserted).

----------------------------------------------------------------------
§1.2 THE TWO-WORD CELL: sealed vs working (resolves C-0b + G-3)
----------------------------------------------------------------------
Per dense group id, the store holds:
  - working blob : the current-epoch folded value (algebra-declared
    layout; e.g. i32 Sum, i32 Count, or an opaque N-byte blob). MUTATED
    by fold(); READ by emit(). A real read/write IR value (hazarded).
  - sealed blob  : the batch-start snapshot. WRITTEN only by Seal().
    READ by old(). A frozen value, kInI-like: RAW only against
    STATE_SEAL, never a within-band hazard (v3-spec §2 kInI-frozen
    treatment, C-0b).
  - touched set  : dense group ids folded this epoch (mirror of
    DiffTable::touched, Table.h:313/657). emit_touched iterates it
    SORT-UNIQUE (average_weight.post-r3 §2; G-8).

This directly answers the v3-spec §A open question ("is `old` another
frozen read, or does it need a per-epoch snapshot value node?"): old()
is a FROZEN READ of a SEPARATE sealed word — the same shape as kInI on a
Table (Table.h:249 id<sealed watermark), lifted from a boolean
membership to a VALUED read. A Table's kInI answers "was this ROW
present at batch start" with one id-compare; StateCell.old answers "what
was this GROUP's VALUE at batch start" with one sealed-blob load. The
ten membership predicates (E-14) are all boolean; old() is the FIRST
valued sealed read — this is precisely the "valued sealed reads" half of
the §9 coverage-judge R3 line, and G-3 CLOSED: the matrix cannot express
a valued frozen read, so R3 ADDS old()/emit() to the read vocabulary as
VALUED reads outside the boolean-predicate matrix (§2 below names them).

Why two words and not one + a log: a SUM cell folded -1 then +1 within a
batch must still let old() report the pre-batch value while working
already moved — two words make new≠old exact and O(1) with no history.
For @recompute algebra (§1.3) working is not a scalar but a membership
accrual; sealed is still the prior emit() result (a scalar), so old() is
uniform across algebras.

----------------------------------------------------------------------
§1.3 THE ALGEBRA LOWERING FORK  (resolves G-4; MIN/MAX = G-5, §6)
----------------------------------------------------------------------
The algebra attribute selects fold()/emit() bodies (C-0e: a lowering
selector, not an ordering fact). THREE lowering shapes; the
per-group-recompute THRESHOLD within @recompute is a lowering knob (not
IR), mirroring the subgraph-instance recompute mode (AggregatingFunctors
§3 PER-GROUP RECOMPUTE THRESHOLD):

  (I) @invertible (abelian: fold+unfold O(1) both ways — COUNT/SUM/AVG):
      working is the running reduction. fold(+,v)=working⊕v;
      fold(-,v)=working⊖v (the declared unfold). emit()=working (or a
      declared finalizer, e.g. AVG=Sum/Count computed at emit). O(1)
      per input delta; touched-group emit is O(touched). THIS is what
      average_weight.dr's sum_i32/count_i32 use.

  (II) @commutative+@associative WITHOUT @invertible, and NOT @recompute
      (e.g. a mergeable sketch with no inverse): working is a mergeable
      accumulator. fold(+,v)=working merge v is O(1); fold(-,v) has NO
      O(1) inverse ⇒ this class DEGRADES to (III) on any retraction in
      the group this epoch (detected: the group saw a `-` fold). R3 MAY
      ship this class as "invertible-until-first-retraction, else
      rescan"; simplest correct R3 ships only (I) and (III) and rejects
      declared-(II)-without-invertible at a `-`-reachable input with a
      diagnostic (see §5 V-ALGEBRA). RECOMMENDATION: ship (I)+(III) in
      R3; fold (II) in with sketches later.

  (III) @recompute (opaque fallback; used for new_weight_i32 whose
      invertibility is undeclared — average_weight.post-r3 §7 G-4/G-7):
      working holds the GROUP MEMBERSHIP (a Vec of DiffTable row ids, or
      the raw summary values, for the group). fold(+,v) appends; fold(-,
      v) marks/removes; emit() RE-RUNS the functor over the surviving
      members (the from-scratch per-group reduction). This is the
      genuinely different physical lowering G-4 named. Cost: emit is
      O(group size), fired once per touched group per epoch — often wins
      for small groups (COST honesty). The membership is keyed by group
      id in the SAME store; sealed still snapshots the prior emit scalar.

The fork is CARRIED as GROUP_UPDATE's `algebra` attribute; the runtime
StateCellStore is TEMPLATED on an Algebra policy type (compile-time
dispatch, zero-cost — repo idiom, no vtables). G-4 CLOSED.

----------------------------------------------------------------------
§1.4 C++ CLASS SKETCH  (repo idiom; new Runtime header)
----------------------------------------------------------------------
New file include/drlojekyll/Runtime/StateCell.h (peer of Table.h; same
dependency-free style; hyde::rt namespace; HYDE_RT_BENCH_COUNT seams).
Sketch (~50 lines, matching Table.h idiom):

  namespace hyde::rt {

  // Standing per-group state for one aggregating functor (a QueryAggregate
  // or a desugared QueryKVIndex). Keyed (group ++ config) -> a DENSE group
  // id, columnar working/sealed blobs indexed by that id. NON-ALIASING:
  // group ids are a SEPARATE id space from the agg DiffTable's row ids; a
  // DiffTable compaction NEVER touches this store (§1.1). `Algebra` is a
  // compile-time policy: Working, Sealed, Fold(w,sign,summary...),
  // Emit(w)->Summary, and (for @recompute) membership accrual.
  template <typename Key, typename Algebra>
  class StateCellStore {
   public:
    using Working = typename Algebra::Working;   // e.g. {i32 sum}; or a Vec
    using Sealed  = typename Algebra::Sealed;    // the prior emit scalar(s)
    using Summary = typename Algebra::Summary;   // emit() result columns

    explicit StateCellStore(Allocator a)
        : keys(a), working(a), sealed(a), touched(a) {}
    StateCellStore(const StateCellStore &) = delete;
    StateCellStore &operator=(const StateCellStore &) = delete;

    // Dense group id for `key`, allocating (identity-init working+sealed)
    // on first touch. Mirrors RowStore::FindOrAdd (Table.h:152).
    uint32_t FindOrAddGroup(const Key &key);

    // VALUE-fold, NO presence crossing (C-0c). Records the group in
    // `touched` exactly once (mirror DiffTable::Touch, Table.h:657).
    template <typename... Summary_>
    void Fold(uint32_t gid, int32_t sign, Summary_ &&...s) {
      Touch(gid);
      Algebra::Fold(working[gid], sign, s...);   // ⊕/⊖ or membership accrue
    }

    // Current working value reduced to output columns.
    Summary Emit(uint32_t gid) const { return Algebra::Emit(working[gid]); }

    // Batch-start snapshot (frozen; kInI-analogue, VALUED). old().
    Summary Old(uint32_t gid) const { return Algebra::OldOf(sealed[gid]); }

    // End-of-epoch: for each touched group, sealed := Emit(working); clear
    // touched. Mirror of Table::Seal (Table.h:277) / DiffTable Commit's
    // kInI advance (Table.h:543-546). Emitted at the commit-sweep tail
    // (STATE_SEAL), AFTER emit_touched has fired all pairs THIS epoch.
    void Seal() {
      for (uint32_t gid : touched) { sealed.Set(gid, Algebra::SealFrom(working[gid])); }
      touched.Clear();
    }

    // Iteration for emit_touched: the SORT-UNIQUE touched set (G-8).
    const Vec<uint32_t> &Touched() const { return touched; }
    const Key &KeyAt(uint32_t gid) const { return keys[gid]; }

    void DebugValidate() const;   // §1.5 property checks

   private:
    void Touch(uint32_t gid);     // append-once, mirror DiffTable::Touch
    Vec<Key> keys;                // group id -> key (debug + KeyAt)
    Vec<Working> working;         // columnar by dense group id
    Vec<Sealed> sealed;           // batch-start snapshot (old())
    Vec<uint32_t> touched;        // groups folded this epoch
    // + the open-addressing key->gid slot array (mirror RowStore slots).
  };

  }  // namespace hyde::rt

What it deliberately does NOT hold (average_weight.post-r3 §4): no signed
derivation counters, no kDel/kAdd scratch, no claim frontiers, no
indices. Presence/frontier semantics live entirely on the agg DiffTable,
reached through emit_touched's UPDATECOUNTs. The cell is standing state.

----------------------------------------------------------------------
§1.5 DEBUG VALIDATION  (the §3 declared-algebra property checks)
----------------------------------------------------------------------
StateCellStore::DebugValidate() (#ifndef NDEBUG; called by the generated
commit sweep alongside DiffTable::DebugValidateCounts, Table.h:610):
  - ALGEBRA LAWS on sampled groups (AggregatingFunctors §3 "annotations
    can lie"): for @invertible, fold(-,v) after fold(+,v) restores
    working bit-for-bit (unfold inverts fold); for @commutative, two
    fold orders of the same multiset agree; for @idempotent, a duplicate
    fold(+,v) is a no-op on emit(). Sample a bounded number of groups per
    epoch (cost-bounded, like DebugValidateCounts' per-row scan).
  - SEAL COHERENCE (V-CELL-SEAL, §5): after Seal(), touched is empty and
    every group's sealed == the emit() that the just-fired emit_touched
    observed as `new`. Asserted structurally: Seal reads working, which
    emit_touched already read as new; so sealed==new by construction —
    the assert guards ORDER (no fold after emit_touched within the epoch).
  - NON-ALIASING (§1.1): a debug set of "ids handed to any DiffTable
    Find" intersected with the StateCell's live gids is empty (cheap
    build-time-typed guard preferred; the runtime assert is the belt).


======================================================================
§2. THE MATRIX EXTENSION: valued reads + the value-fold op
======================================================================
The ten membership predicates (E-14, v3-spec §2) are BOOLEAN. R3 adds
TWO VALUED reads and ONE new op, all named-and-auditable like the ten.

----------------------------------------------------------------------
§2.1 TWO NEW VALUED READS  (named like the ten predicates; resolves G-3)
----------------------------------------------------------------------
  old(g)   : SC.Old(gid)  — the SEALED (batch-start) summary value of
             group g. A VALUED frozen read; RAW-only against STATE_SEAL;
             NEVER a within-band hazard (kInI-frozen treatment, C-0b).
             EffKind kStateOld.
  emit(g)  : SC.Emit(gid) — the WORKING (current, post-fold) summary
             value of group g. A VALUED read of the working word; RAW
             after every fold(g) this epoch (hazarded). EffKind kStateEmit.
These are NOT in the boolean Pred enum (v3-spec §7.1 ln621-623); they are
a SEPARATE valued-read family on the StateCell value, mirroring how kInI
sits apart from the ten. Auditability: each names its StateCell value id
and its group-key projection, exactly as an ACCESS names (table, pred,
bound-cols).

----------------------------------------------------------------------
§2.2 THE GROUP_UPDATE VALUE-FOLD OP  (the one new op; resolves G-2/G-6)
----------------------------------------------------------------------
OP: GROUP_UPDATE(agg A, statecell SC, group G, config C, summary S,
                 algebra Alg, class=NonRecursive, provenance∈{over,kv})
  ONE op per QueryAggregate/desugared-QueryKVIndex view. TWO bands:

  BAND (a) frontier_in — over the INPUT view's net frontiers.
    Structurally a THIRD frontier-vector consumer of the input stratum,
    sibling of CROSSOVER/PRODUCT_ARM (average_weight.post-r3 §5), but
    WITH NO position-keyed partner read (single input source, no join
    partner). Per SIGN arm (both, since inputs are differential):
      − arm: vector:drain(input table's D\A = NetRemovals frontier);
             statecell:fold(SC, −1, summary-proj)   [EffKind kStateFold];
             (touch g is a side effect of Fold — NOT a separate effect).
      + arm: vector:drain(input table's A\D = NetAdditions frontier);
             statecell:fold(SC, +1, summary-proj)   [EffKind kStateFold].
    The ONLY input membership predicates consumed are kNetAdded/kNetDeleted
    (v3-spec §2; the E-14 first-class frontier predicates, Program.h:620-
    621) — via the drained frontier vecs, NOT re-read. NO table:counter±
    here (C-0c: fold is value-mutation, not presence-crossing). This is
    the "value-fold without immediate crossing" G-2 asked for.

  BAND (b) emit_touched — over SC.Touched() (SORT-UNIQUE, G-8).
    Per touched group g:
      read new := emit(g)  [kStateEmit];  read old := old(g)  [kStateOld].
      if new != old  (a TUPLECMP-shaped guard):
        counter−(agg DiffTable, NonRecursive) over (g.cols ++ old);
          flags:read(agg, kInI); vector:append(agg delQ).   ← UPDATECOUNT
        counter+(agg DiffTable, NonRecursive) over (g.cols ++ new);
          flags:read(agg, kInI); vector:append(agg addQ).   ← UPDATECOUNT
    EXACTLY ONE net pair per touched group per epoch regardless of fold
    count (AggregatingFunctors §2 OUTPUT CONTRACT; average_weight.post-r3
    §6). If new==old, emit nothing (netting composes — OQ3; a group
    touched only by an annihilated adds∩removes pair sees no state change).
    These TWO folds are ORDINARY UPDATECOUNTs (v3-spec §2 table:counter±;
    Program.h:629) into the agg's OWN DiffTable — the whole tail rides the
    EXISTING claim/frontier/commit machinery. emit_touched carries NO new
    effect kind beyond the two valued reads; its writes are the reserved-
    already table:counter± + vector:append.

  provenance∈{over,kv}: records whether this GROUP_UPDATE came from an
    over(){} aggregate or a desugared mutable()/KVINDEX (resolves G-7:
    the merge-functor's REQUIRED declared algebra is checked differently
    for kv — a kv GROUP_UPDATE with an un-annotated merge functor is
    rejected at §5 V-ALGEBRA; an over aggregate defaults to @recompute if
    the functor declares nothing). A carried attribute, not an ordering.

  SHARED-INPUT FUSION (resolves G-6): when two GROUP_UPDATEs drain the
    SAME input net-frontier vec (average_weight's sum_i32 + count_i32 both
    over edge_weight), their frontier_in bands MAY be fused into one drain
    folding both cells. In the v3 IR this is NOT a lowering note — it is a
    DERIVED dependence fact: two GROUP_UPDATE.frontier_in ops with the
    SAME `vector:drain(V)` effect on the same V are fusion candidates,
    surfaced by the effect graph exactly as G-6 wanted (a CSE-like pass on
    the drain effect). R3 MAY leave them unfused (correctness-identical:
    two drains of the same vec are independent reads); fusion is a v3
    rewrite the substrate now EXPRESSES. G-6 CLOSED (expressible as an
    effect-graph fact; fusion optional in R3).

----------------------------------------------------------------------
§2.3 FULL EFFECT TABLE  (GROUP_UPDATE, both bands)
----------------------------------------------------------------------
  op                     effects
  GROUP_UPDATE.in(−arm)  vector:drain(input.NetRemovals);
                         statecell:fold(SC, −1)          [kStateFold, RW working]
  GROUP_UPDATE.in(+arm)  vector:drain(input.NetAdditions);
                         statecell:fold(SC, +1)          [kStateFold, RW working]
  GROUP_UPDATE.emit      statecell:emit(SC)  [kStateEmit, R working];
                         statecell:old(SC)   [kStateOld, R sealed — frozen];
                         table:counter−(agg, NonRecursive); flags:read(agg,kInI);
                           vector:append(agg.delQ);
                         table:counter+(agg, NonRecursive); flags:read(agg,kInI);
                           vector:append(agg.addQ)
  STATE_SEAL(SC)         statecell:seal(SC)  [global:rmw sealed:=working, v3 §B-8]

Per A-3 (v3-spec) each ARM carries its own effect set; the op-level union
is coarse-scheduling only.

----------------------------------------------------------------------
§2.4 DEPENDENCE EDGES  (derived, per v3-spec §4; not attributes)
----------------------------------------------------------------------
  E1  input-frontier RAW: the input stratum's FRONTIER_FILTER writes
      input.NetRemovals/NetAdditions; GROUP_UPDATE.in DRAINS them ⇒ RAW
      (filter-before-drain). Same shape as CROSSOVER's drain of a negated
      table's frontier vecs.
  E2  fold-before-emit RAW: every GROUP_UPDATE.in fold WRITES working;
      GROUP_UPDATE.emit READS working (emit(g)) ⇒ RAW. This is the
      band-order edge (all folds before any emit) — DERIVED from the
      working value's def/use, NOT a "seed-before-drain"-style attribute.
  E3  emit-before-agg-drain (seed-before-drain analogue): emit_touched's
      two counter± APPEND to agg.delQ/addQ; the agg DiffTable's CLAIM_DRAIN
      drains them ⇒ RAW, and the pair MUST precede the drain (the phantom-
      drop of the claim gates depends on the seed preceding the drain —
      average_weight.post-r3 §6). Same edge shape as CROSSOVER/PRODUCT_ARM
      seed-before-drain (v3-spec §B-3.4 V-SEED-DRAIN); reuse that validator.
  E4  agg-drains-after-emit: the whole agg-table acyclic band (claim
      drains, frontier filters, commit) is downstream of emit_touched.
      DERIVED from E3 transitively.
  E5  seal-after-emit (V-CELL-SEAL): STATE_SEAL(SC) reads working
      (sealed:=working); it MUST run AFTER emit_touched read working as
      `new` (else old() next epoch is wrong) ⇒ WAR/RAW ordering STATE_SEAL
      after GROUP_UPDATE.emit. And STATE_SEAL is a TRAILING band with the
      commit sweeps (v3-spec §A-2 commit band, table-id order): no old()
      reader is scheduled after any Seal (mirror V-COMMIT-TRAILS).
  E6  input-stratum-final (V-CELL-SEAL half 2): no old()/emit() read of SC
      before the INPUT stratum's frontier filters are FINAL — old() is
      only meaningful once the input net frontiers exist. DERIVED from E1
      (GROUP_UPDATE.in drains input frontiers; emit is E2-after-in).

----------------------------------------------------------------------
§2.5 VALIDATORS  (always-on graph checks, per v3-spec §5 style)
----------------------------------------------------------------------
  V-AGG-SOLE (F-6 sole-deriver, ledger §9 F-6): the agg DiffTable's
      member-view list == exactly the one agg view; GROUP_UPDATE is the
      SOLE producer of counter±(agg). No other op writes that table ⇒
      exactly one deriver, always NonRecursive. Structural (mirror the
      F-6 R3 sole-deriver assert the ledger already names).
  V-AGG-NET (OQ3 netting composes): a group whose folds net to no working
      change (new==old) emits NO counter± ⇒ no agg-table event. Checked as
      the emit_touched guard's contract (new!=old ⇒ pair; else nothing).
  V-TOUCH-SORTED (G-8): emit_touched iterates SC.Touched() sort-unique;
      the emitted output order is therefore canonical (or driver-sorted
      per the cursor contract). Pins golden determinism (G-8; the oracle
      §4 referees the value, the sort pins the order).
  V-CELL-SEAL (E5+E6): STATE_SEAL after all emit_touched reads within the
      epoch (E5); no old()/emit() before input-stratum frontiers final
      (E6). Two ordering asserts on the SC value's def/use.
  V-ONE-NET-PAIR: emit_touched contributes AT MOST one counter− and one
      counter+ per touched group (average_weight.post-r3 §6; the one-net-
      pair contract vs the claim gates — a clean acyclic crossing pair,
      no phantom, because GROUP_UPDATE has ONE input position so the
      §5.1.1 two-same-stratum-atom phantom class cannot arise).
  V-ALGEBRA (G-7 + §1.3 fork): a kv-provenance GROUP_UPDATE whose merge
      functor carries NO declared algebra is REJECTED (AggregatingFunctors
      §4.1: merge functors must carry declared algebra or be rejected); an
      over-provenance GROUP_UPDATE with no declared algebra DEFAULTS to
      @recompute (III). A declared-(II)-without-invertible at a
      retraction-reachable input is rejected (§1.3) or degraded per the
      shipped policy.
  V-STATECELL-EFFECT (R3-totality): every statecell:fold/emit/old effect
      names a StateCell value; no R1/R2 op declares one (v3-spec §2 ln124
      "no R1/R2 op declares a statecell effect" stays true — GROUP_UPDATE
      is the first and only).


======================================================================
§3. KV-INDEX DESUGARING  (resolves G-7; the node-deletion call)
======================================================================
Per AggregatingFunctors §4.1 recommendation (a) — KEEP mutable() as
sugar. A QueryKVIndex desugars to a GROUP_UPDATE:
  group   = KeyColumns()            (Query.h:910)     ; the key
  config  = ()                       (a KV index has no inner group-by)
  summary = ValueColumns()          (Query.h:913)     ; the merged value
  algebra = NthValueMergeFunctor(0)  (Query.h:926)    ; the merge functor
                                     with its declared @-algebra (or
                                     @recompute fallback — new_weight_i32
                                     is un-annotated ⇒ @recompute, §1.3-III)
  provenance = kv
This is IDENTICAL to average_weight.post-r3 stratum 0 (edge_weight) and
to pairwise_average_weight's sole aggregate (average_weight.post-r3 §2):
one GROUP_UPDATE + an ORDINARY §5 differential self-join. NO separate
KVINDEX lowering (average_weight.post-r3 §2 confirmed).

WHERE the desugaring happens — CONCRETE RECOMMENDATION on the KVINDEX
dataflow node:
  RECOMMEND: do NOT delete the QueryKVIndex DATAFLOW node in R3. Desugar
  at DR-IR CONSTRUCTION time (BuildDRFlow, v3-spec §6/§7.2), NOT in the
  dataflow layer. Rationale:
    (1) The dataflow KVINDEX node already reifies key/value/merge cleanly
        (Query.h:902-926); parse+dataflow build+canonicalization handle
        it (AggregatingFunctors §1). Deleting it forces a parse/dataflow
        rewrite that touches the optimizer's canonicalization — OUT of
        R3's acyclic-band scope and a golden-churn risk across ALL modes.
    (2) The DR-IR construction is EXACTLY where over(){} aggregates and
        KV indices UNIFY (both become GROUP_UPDATE). Desugaring here keeps
        one lowering path and preserves the `provenance` distinction
        (§2.2) for the V-ALGEBRA check.
    (3) Deleting the node is a SEPARATE, later simplification (a dataflow
        pass that rewrites KVINDEX→AGGREGATE upstream) with its own golden
        review — record it as a residue, do not couple it to R3.
  So: QueryKVIndex STAYS in dataflow; BuildDRFlow maps BOTH QueryAggregate
  and QueryKVIndex to GROUP_UPDATE. The mutable() surface syntax (§4.1
  decision (a)) is preserved. G-7 CLOSED (provenance carried; node kept;
  desugar at construction).


======================================================================
§4. ORACLE REFEREE  (per-group from-scratch recompute; golden sidecars)
======================================================================
The oracle (bin/Oracle/Main.cpp) ALREADY has the exact machinery: an
independent FROM-SCRATCH semi-naive stratified set evaluation
(Main.cpp:1639+) that recomputes the whole materialization from
accumulated explicit facts, cross-checked against the incremental path
every batch. It currently REJECTS aggregates (Main.cpp:611-613) and KV
indices. R3d LIFTS that rejection by teaching BOTH paths the aggregate:

  FROM-SCRATCH path (the referee — this is the per-group recompute the
    mandate asks for): for each QueryAggregate/QueryKVIndex view, after
    its input stratum's from-scratch materialization is final, GROUP the
    input rows by (group ++ config), and for each group RUN THE FUNCTOR
    from scratch over the group's members (sum/count/merge). This is the
    DEFINITIONAL aggregate — no counters, no cells, no old/new. The
    aggregate output rows are (group ++ functor-result). This is the
    ground truth the incremental StateCell path is checked against.
  INCREMENTAL path: the oracle gains a StateCell interpreter mirroring
    §1-§2 (fold on input net frontiers, emit_touched one-net-pair). Every
    batch it asserts: incremental agg-table presence == from-scratch agg
    presence for every group (the existing per-view presence assert,
    Main.cpp:21-22, extends to agg views for free once both paths produce
    the view's rows), AND the emitted summary VALUE matches. This makes
    the oracle the property-check host for §1.5's declared-algebra laws
    too (run the from-scratch reduction two ways for @commutative, etc.).
  @recompute in the oracle: trivially the from-scratch path IS recompute
    — the incremental @recompute lowering (§1.3-III) is validated by the
    same equality.
  MIN/MAX retraction (§6): the from-scratch path handles MIN/MAX with NO
    special case (it recomputes the group min from scratch); the
    incremental path's MIN handling (whichever §6 policy R3 ships) is
    checked against it. This is precisely the AggregatingFunctors §5
    hand-trace requirement made EXECUTABLE.

GOLDEN SIDECAR SHAPE (mirrors the existing .oracle.stdout/.monotone.stdout
convention, CLAUDE.md golden-master section; sidecars sit beside the
case): average_weight.dr and pairwise_average_weight.dr become OptDiff
cases with:
  - <name>.dr + <name>.main.cpp          (the driver; sorts keyed drains
      per the cursor contract, CLAUDE.md — average_incoming_weight is a
      keyed query, MUST sort before printing)
  - <name>.batches                        (the .batches sidecar; grammar
      per Oracle Main.cpp: +/- add_edge From To Weight lines in batch/end
      blocks — exercises deletion so the − arms and @recompute retraction
      fire)
  - <name>.stdout                         (the 4-mode golden, byte-compared
      across all optimization modes)
  - <name>.oracle.stdout                  (the oracle's DumpRelations —
      final agg rows in canonical sorted form; the incremental-vs-from-
      scratch cross-check must pass to PRODUCE it)
  - <name>.monotone.stdout                (the monotone projection — the
      from-scratch evaluation over all-facts-ever, Main.cpp:2028+)
  ALL goldens are BLESSED FROM ORACLE TRUTH (CLAUDE.md: new fixtures from
  oracle truth; AggregatingFunctors §5 verification): run the oracle,
  confirm OK, then bless. NEVER hand-author the expected agg values.
  average_weight.dr is the STRONGEST fixture (average_weight.post-r3 KEY
  FACT 1: a KV GROUP_UPDATE at stratum 0 feeding TWO summary GROUP_UPDATEs
  at stratum 1 — twice-nested-in-strata); pairwise is the pure-KV case.


======================================================================
§5. STRATIFICATION + DIAGNOSTICS  (AGG placed like NEGATE; F14 retires)
======================================================================
Stratify (lib/DataFlow/Stratify.cpp) places AGG EXACTLY like NEGATE
(AggregatingFunctors §2 STRATIFICATION): the summarized INPUT view must
be STRICTLY LOWER-stratum than the aggregate; an in-SCC aggregation
(the aggregate over its own recursive result) is REJECTED with a
diagnostic that is the SIBLING of the unstratified-negation error
(Stratify.cpp:267-296). Concretely, mirror that loop for aggregates:

  for each QueryAggregate/QueryKVIndex agg:
    if agg.stratum == input_view.stratum:   // same SCC
      log.Append(agg.Functor().SpellingRange())
        << "Aggregate is recursively derived from its own result "
           "(unstratified aggregation)";
      // + a Note, mirroring the negate Note: "The summarized relation
      //   must be fully computable before the aggregate runs; break the
      //   recursion through the aggregate."

RELAXATION QUESTION (recorded, AggregatingFunctors §5 critique item):
whether a MONOTONE aggregate over MONOTONE input may relax the strict-
lower rule (a monotone COUNT over a monotone relation has no retraction
and could in principle live nearer its input). RECOMMENDATION: DO NOT
relax in R3 — place ALL aggregates strictly-lower like negation. The
corpus (average_weight, pairwise) is all differential-input anyway; the
relaxation is a later optimization with its own soundness proof. Keep the
diagnostic scope identical to negation's.

F14 PRE-PASS RETIREMENT (Build.cpp:1015-1053): when R3c lands the
GROUP_UPDATE lowering, the two F14 rejections RETIRE:
  - the `for (auto agg : query.Aggregates())` "Aggregating functors are
    not yet supported" append (Build.cpp:1020-1023) is DELETED.
  - the `for (auto kv : query.KVIndices())` "Relations with mutable-
    attributed parameters are not yet supported" append (:1025-1032) is
    DELETED.
  - the region-dispatch asserts Build.cpp:955-956 ("TODO(pag):
    Aggregates") and :1024/:1028 (IsAggregate/IsKVIndex assert-false)
    become REAL lowering (call into BuildDRFlow's GROUP_UPDATE path) —
    the F14 comment already notes the pre-pass "dominates those asserts
    (they remain as internal-invariant backstops)"; those backstops now
    become the lowering entry. The MAP-impure and on-cycle-@product F14
    rows STAY (unrelated gaps). Retire ONLY the agg+kv rows.
  The oracle's matching rejection (Main.cpp:611-613) retires in R3d in
  the same diff that teaches the oracle the aggregate.


======================================================================
§6. MIN/MAX: what R3 ships  (resolves G-5; the §5 hand-trace mandate)
======================================================================
CONCRETE RECOMMENDATION: R3 ships MIN/MAX via @recompute (§1.3-III), NOT
via sorted-multiset state. Rationale + the two required hand-traces:

  WHY @recompute for MIN/MAX in R3: MIN/MAX are non-invertible (you
  cannot O(1)-unfold the removed min and know the new min without the
  group's other values — OQ12 "retraction-aware per-key state"). The two
  honest options are (a) a sorted-multiset / heap per group (retraction =
  O(log n) rebalance, standing ordered state), or (b) @recompute (emit
  re-scans the group's members). R3 ships (b): it needs NO new standing
  structure beyond the @recompute membership the store already has
  (§1.3-III), it is the universal fallback, and it is often the COST win
  for small groups (AggregatingFunctors §3). The sorted-multiset state
  (b') is a LATER optimization (a fourth algebra lowering) with its own
  COST justification and the seekable-iterator residue (memory:
  seekable-iterators-wcoj / D5). G-5 CLOSED (R3 = @recompute MIN/MAX; the
  vocabulary gap "retraction needs a rescan vs O(1) unfold" is exactly
  the §1.3 (I) vs (III) fork, now explicit — not a missing attribute).

  HAND-TRACE 1 — MIN retraction under @recompute (the §5 mandate):
    group X with members {5, 3, 8}; sealed MIN old(X)=3.
    Batch retracts the 3 (edge carrying value 3 deleted):
      frontier_in − arm drains input.NetRemovals, sees row(...,3):
        SC.Fold(gX, −1, 3)  →  @recompute: mark 3 removed from gX's
        membership; touch gX.
      emit_touched over touched={gX}:
        new := emit(gX) = MIN over surviving members {5,8} = 5   (RESCAN).
        old := old(gX) = 3  (sealed).
        new(5) != old(3):
          counter−(agg, NonRecursive) over (X, 3)  → retract old MIN row.
          counter+(agg, NonRecursive) over (X, 5)  → assert new MIN row.
      one net pair; rides claim/frontier/commit; STATE_SEAL sets
      sealed(gX):=5. Next epoch old(X)=5. CORRECT — the rescan is the
      whole point; no O(1) unfold could have known 5 was next.

  HAND-TRACE 2 — @invertible SUM (the §5 mandate):
    group X, members {5,3,8}; SUM working=16, sealed old(X)=16.
    Batch: retract 3, add 10:
      − arm drains NetRemovals, row(...,3): SC.Fold(gX,−1,3) →
        working = 16 ⊖ 3 = 13 (O(1) unfold); touch gX.
      + arm drains NetAdditions, row(...,10): SC.Fold(gX,+1,10) →
        working = 13 ⊕ 10 = 23 (O(1) fold); gX already touched.
      emit_touched over {gX}:
        new := emit(gX)=23; old := old(gX)=16; 23!=16:
          counter−(agg) over (X,16); counter+(agg) over (X,23).
      one net pair despite TWO folds (average_weight.post-r3 §6: k
      touches → one pair). STATE_SEAL sealed:=23. CORRECT and O(1) per
      delta — the @invertible payoff.
    ANNIHILATION sub-case (OQ3 / V-AGG-NET): if the same batch retracts 3
    and re-adds 3 (adds∩removes annihilate at ingest, §5.0), gX is either
    never touched (annihilated before the frontier) or touched with
    working 16⊖3⊕3=16=old ⇒ new==old ⇒ NO pair emitted. Netting composes.


======================================================================
§7. IMPLEMENTATION SEQUENCE  (R3a..R3d; epoch discipline gates)
======================================================================
Aggregates are the INAUGURAL delta-relational-IR operator family
(AggregatingFunctors §4) — the first whose delta semantics is NOT a §5.1
counter schema, forcing exactly the StateCell story and nothing else
(average_weight.post-r3 KEY FACT 4). R3 gates on the ACYCLIC families
being cut over (ledger §9 F-2: "R3 gates only on the acyclic families").

  R3a  RUNTIME STORE + UNIT TESTS (no dataflow/CF change).
       - include/drlojekyll/Runtime/StateCell.h (§1.4), templated on
         Algebra policies: Invertible<SUM/COUNT/AVG>, Recompute<opaque>.
       - DebugValidate (§1.5): algebra-law property checks + seal
         coherence + non-aliasing.
       - Unit tests in the DrTest mini-GoogleTest idiom (tests/DrTest,
         ASSERT_EQ on structs per the intent-communicating-asserts
         memory): fold/unfold round-trip; two-word sealed vs working
         (old() frozen while working moves); touched sort-unique; @recompute
         emit == from-scratch reduction; MIN retraction (§6 trace 1) as a
         unit test; SUM @invertible + annihilation (§6 trace 2).
       GATE: unit tests green; runtime still dependency-free; suite
       UNAFFECTED (no generated code touches StateCell yet). Bench
       harness untouched (COUNTERS seam off is a verified no-op).

  R3b  DATAFLOW / STRATIFY (diagnostics; still no lowering).
       - Stratify (§5): place AGG like NEGATE; add the unstratified-
         aggregation reject (mirror Stratify.cpp:267-296). in-SCC agg is
         now a clean diagnostic BEFORE the F14 pre-pass (the F14 rows
         still catch the acyclic in-corpus cases until R3c).
       - V-ALGEBRA (§2.5): reject kv-provenance with no declared algebra;
         default over-provenance to @recompute.
       GATE: OptDiff suite PASS (the two corpus files still hit F14 —
       expected-diagnostic, encoded in runall.sh as they are today until
       R3c); new stratification diagnostics get directed reject cases.

  R3c  DR-IR FAMILY + LOWERING (the cutover; F14 agg+kv rows retire).
       - BuildDRFlow (v3-spec §6/§7.2): map QueryAggregate AND
         QueryKVIndex → GROUP_UPDATE (§2.2/§3); provenance carried.
       - Lowering (v3-spec §7 LowerDRFlow, R2 acyclic-family style):
         frontier_in → a VECTORLOOP over the input net-frontier vecs
         calling SC.Fold (the ONE new emitted primitive, average_weight.
         post-r3 lowering trace); emit_touched → a VECTORLOOP over
         SC.Touched() with a new!=old TUPLECMP guard around TWO ordinary
         ProgramUpdateCountRegion nodes (Program.h:629; IsAdd=false then
         true, NonRecursive) — IDENTITY to existing UPDATECOUNT emission.
         STATE_SEAL → SC.Seal() at the commit-sweep tail (Procedure.cpp
         commit band), alongside COMMIT_SWEEP.
       - Retire F14 agg+kv rows (Build.cpp:1020-1032); the region-dispatch
         asserts (:955-956/:1024/:1028) become the lowering entry (§5).
       - The agg DiffTable, its claim drains / frontier filters / commit
         are IDENTITY to existing acyclic-table lowering (Stratum.cpp
         emission band) — no new emitted region there.
       GATE: the family-cutover discipline (ledger §9 F-2): this cutover
       DELETES the F14 agg+kv rejections in the SAME diff it adds the
       lowering. The suite's expected-diagnostic encoding for average_
       weight/pairwise in runall.sh is REMOVED in the same diff (they now
       COMPILE). DebugValidateCounts + StateCell::DebugValidate stay in
       every driver (F-8). NO golden exists yet for these two cases — they
       are blessed in R3d from oracle truth.

  R3d  ORACLE + CORPUS GOLDENS FROM ORACLE TRUTH.
       - Teach both oracle paths the aggregate (§4); retire the oracle's
         agg/kv rejection (Main.cpp:611-613) in this diff.
       - Add the .batches sidecars (deletion-exercising) for average_
         weight + pairwise; run the oracle; confirm ORACLE: OK (the
         incremental-vs-from-scratch cross-check passes — this is the
         real correctness proof).
       - BLESS <name>.stdout / .oracle.stdout / .monotone.stdout FROM the
         oracle run (never hand-authored). Add both as OptDiff cases with
         drivers that SORT keyed drains (cursor contract).
       GATE: SUITE: PASS with the two new cases green in all 4 modes +
       oracle + monotone; the AggregatingFunctors §5 hand-traces (§6)
       are now regression-covered by the .batches deletion cases. Bench:
       optionally add average_weight to bench/ (COST of @recompute vs
       @invertible per group) — perf only, never gates.

  DISCIPLINE across R3a-d (epoch rules, CLAUDE.md): opus for traces/
  critique/lowering/review, sonnet for mechanical audits + suite gates;
  goldens change ONLY via explicit `runall.sh --bless` after review,
  never to make a red case green; never rebuild the compiler mid bench
  run; the interleaved-binary A/B is normative for <10% bench deltas.


======================================================================
§8. GAP-CLOSURE LEDGER  (G-1..G-8 → resolution pointer)
======================================================================
  G-1 statecell not a §5 object      → §1 (first-class runtime object,
                                        own id space) + §2 (matrix ext).
  G-2 value-fold ≠ crossing          → C-0c + §2.2 (kStateFold, distinct
                                        from counter±; crossing at emit).
  G-3 old() valued sealed read       → §1.2 two-word cell + §2.1 (old()/
                                        emit() as VALUED reads, kInI-lifted).
  G-4 @recompute lowering fork        → C-0e + §1.3 (three algebra shapes;
                                        fork explicit; threshold=lowering).
  G-5 MIN/MAX retraction              → §6 (R3 ships @recompute MIN/MAX;
                                        sorted-multiset = later; 2 traces).
  G-6 shared-input fusion             → §2.2 (a DERIVED effect-graph fact:
                                        same drain(V) ⇒ fusion candidate;
                                        optional in R3).
  G-7 KV provenance / node deletion   → §2.2 provenance + §3 (keep KVINDEX
                                        node; desugar at construction).
  G-8 touched-set order determinism   → §1.2/§2.5 V-TOUCH-SORTED (sort-
                                        unique) + §4 oracle referees value.


======================================================================
END v3-spec-statecell.md
======================================================================
Files read (all read-only; ONE file written — this one):
AggregatingFunctors.md (full), average_weight.post-r3.drir.md (full),
v3-spec.md (§2/§7.1/§A/§B + open questions), DeltaRelationalIR.md §9,
StackSafeNegation.md §5.5, Table.h (full), Query.h (QueryAggregate/
QueryKVIndex accessors), Stratify.cpp:260-296, bin/Oracle/Main.cpp
(header + from-scratch path + sidecar grammar), Build.cpp:955-956/
1015-1053, data/examples/average_weight.dr + pairwise_average_weight.dr.

======================================================================
§C. AMENDMENTS (2026-07-16, after the r3cd-critique + owner decisions;
    BINDING over the sections above)
======================================================================
C-1 (OCCUPANCY — fixes the critique's CRITICAL batch-1 abort): the
    store tracks per-group OCCUPANCY (empty | occupied). The
    emit_touched guard generalizes:
      birth  (empty→occupied): emit ONLY +new (no phantom −old);
      death  (occupied→empty): emit ONLY −old (no phantom +new);
      change (occupied, new≠old): the one net pair (−old, +new);
      no-op  (occupancy and value unchanged, incl. OQ3 annihilation
              and zero-sum-nonempty states): emit nothing.
    Occupancy is part of the SEALED word's snapshot semantics (old()
    is defined only for sealed-occupied groups). StateCell.h grows the
    occupancy bit + birth/death/zero-sum-nonempty unit traces (R3a
    follow-up, landing inside the R3c-ii+R3d unit).
C-2 (V-ALGEBRA placement): the declared-algebra policy check runs in
    the Build.cpp PRE-PASS slot the retiring F14 rows vacate (the
    Aggregates()/KVIndices() loops give over-vs-kv provenance; the
    num_errors→nullopt gate is already downstream). over() undeclared
    → default @recompute; kv undeclared → clean reject (§7.1).
C-3 (STATE_SEAL pin): Seal() lands at the commit-sweep tail; the
    fold→emit→seal edges are pinned as specified (E2/E5), with the
    linearizer band keys for kGroupUpdate (emit_stratum, its own band)
    and kStateSeal (trailing band, V-READY skip) made explicit diffs.
C-4 (AGGREGATE INPUTS, owner-decided): monotone inputs SUPPORTED via
    FindMonotoneNegatedTables-style enrollment (net-additions frontier
    + Seal); QueryAggregateImpl sets can_produce_deletions
    unconditionally (value churn implies downstream retraction);
    induction-owned inputs get a clean feature-gap diagnostic this
    epoch.
C-5 (FUNCTOR ABI, owner-directed — the free-function/ADL trick
    generalized to functors): aggregate/merge reduction bodies are
    DRIVER-SUPPLIED FREE FUNCTIONS over DRIVER-OWNED state types,
    resolved by unqualified call from generated template context
    (the DatabaseLog deduction contract: no inheritance, no virtuals,
    no members). Generated code requires: agg_fold(State&, summary...),
    agg_emit(const State&) -> value, agg_unfold(State&, summary...)
    REQUIRED iff @invertible, agg_init(State&) or
    default-construction. The engine owns layout via
    StateCellStore<Key, State, Algebra> over the driver's POD state
    (trivially-copyable enforced). @recompute functors supply the
    rescan reduction instead. The corpus drivers define these before
    their message calls (new-driver review gate applies). RECORDED
    FOLLOW-ON (not this epoch): migrating MAP functors out of
    DatabaseFunctors onto the same free-function surface (touches
    every corpus driver — its own diff/epoch).
