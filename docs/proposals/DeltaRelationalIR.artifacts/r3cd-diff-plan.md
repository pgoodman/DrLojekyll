# R3c-ii+R3d as pseudocode diffs (2026-07-16) — REVISE-BEFORE-IMPLEMENTATION per the critique

╔══════════════════════════════════════════════════════════════════════╗
║  R3c-ii + R3d — ONE SUITE-GATED UNIT (per ledger §11 F9 disposition)  ║
║  Concrete +/- pseudocode diffs against the AS-LANDED model            ║
║  (delta-relational-ir @ ecfe674 committed; PENDING = the in-flight    ║
║   review-fixes+V-PRED-XCHECK diff, modeled below where it touches me) ║
╚══════════════════════════════════════════════════════════════════════╝

PROVENANCE / CITE-GROUNDING (file:line at HEAD I verified before writing):
  - F14 pre-pass to retire: Build.cpp:1024-1032 (agg append + kv append), the
    region-dispatch asserts at Build.cpp:955-956/:1024/:1028 become lowering entry.
  - Corpus: average_weight.dr:6-8 (sum_i32/count_i32/new_weight_i32 all UN-annotated),
    :12 mutable(new_weight_i32); pairwise_average_weight.dr:7,:9 same merge functor UN-annotated.
  - runall.sh case dispatch: :217 (aggregate_1|kvindex_2|kvindex_3|kvindex_4|... all-modes-diagnostic),
    :223-228 (kvindex_1 modesplit: run_vs_golden opt/nocf + expect_diagnostic nodf/none),
    :18-19 header comment list, :85-89 kvindex_1 bless path.
  - Mode-flag map: diffrun.sh:42-47 (opt="", nodf=-disable-dataflow-opt,
    nocf=-disable-controlflow-opt, none=both). CONFIRMED: kvindex_1.dr:1-4 says the KVINDEX
    canonicalizes AWAY (unused value col → TUPLE) — this happens ONLY when dataflow-opt is ON
    (opt/nocf). This is the mechanism behind the by-design mode split.
  - Oracle rejection: Main.cpp:611-619 (Aggregates()/KVIndices() Fail()).
  - StateCell fixed API (PENDING already committed at ecfe674): Touched() returns
    `const Vec<uint32_t>&` and SortAndUnique()s on read (StateCell.h:333-336) — the F7 fix
    IS in the committed subject; §3 below binds to that const contract.
  - DR.h OpKind enum (DR.h:114-125) — I add kGroupUpdate + kStateSeal.
  - EffKind (v3-spec §7.1 ln617-619): kStateFold/kStateEmit/kStateOld already RESERVED in the enum.
  - Pred vs ClaimGate: the PENDING diff added the dedicated ClaimGate enum (F2 fix); GROUP_UPDATE
    reuses kAddGateTotalPositive/kDelGateCnrNonPositive verbatim, NOT a Pred.


═══════════════════════════════════════════════════════════════════════
(1) BuildDRInventory + GROUP_UPDATE CONSTRUCTION
    — new B.II family, sits between PRODUCT_ARM (:740) and BRANCHES (:742)
═══════════════════════════════════════════════════════════════════════

DR.h — enum + op payload additions:

  DR.h:114-125 (OpKind):
+   kGroupUpdate,    // §2.2 GROUP_UPDATE: one per QueryAggregate/desugared-KVIndex view
+   kStateSeal,      // §2.3 STATE_SEAL: per statecell, commit-sweep tail (mirror kCommitSweep)

  DR.h — new provenance/algebra enums (near VecRole, ~:428):
+   enum class AggProvenance : uint8_t { kOver, kKv };   // §2.2 provenance∈{over,kv}
+   enum class Algebra : uint8_t { kInvertible, kRecompute };  // R3 ships (I)+(III) only (spec §1.3)
        // NOTE: kCommutative/kAssociative/kIdempotent are PARSED (R3c-i) but NOT lowering
        // selectors in R3 — a declared-(II)-without-invertible degrades to kRecompute (spec §1.3-II).

  DROpImpl (DR.h:638-641) — GROUP_UPDATE payload block (kind==kGroupUpdate):
+   // ---- GROUP_UPDATE data (kind == kGroupUpdate) ---------------------------
+   QueryView agg_view;             // the QueryAggregate OR QueryKVIndex (provenance-tagged)
+   AggProvenance provenance;       // kOver | kKv  (§2.2; drives V-ALGEBRA + oracle path)
+   Algebra algebra;                // kInvertible | kRecompute (§1.3 lowering fork)
+   TABLE *agg_table;               // the agg's own DiffTable (sole-deriver, V-AGG-SOLE)
+   unsigned statecell_id;          // index into flow.statecells (NEW vector, below)
+   std::vector<unsigned> group_cols;   // group ++ config, canonical col-id SET (§1.1 key)
+   std::vector<unsigned> summary_cols; // the folded value column(s)
+   QueryView input_view;           // the summarized relation (single source — no join partner)
+   // Two per-sign frontier_in arms + emit_touched carried on `effects`/`body` per A-3.
+   // For kProductArm-style arm carriage, use a mini arms[] as PRODUCT_ARM does; see §3.

+   struct DRStateCell { QueryView agg_view; Algebra algebra;
+     std::vector<unsigned> key_cols; unsigned summary_arity;
+     const ParsedFunctor *algebra_functor; };  // for codegen type binding (§4)
  FlowGraphImpl (DR.h:645-648):
+   std::vector<DRStateCell> statecells;   // one per GROUP_UPDATE; codegen instantiates a store each

DR.cpp — the constructor (new function BuildGroupUpdateOps, called from BuildDRInventory
after PRODUCT_ARM discovery, before BRANCHES at DR.cpp:742):

+ static void BuildGroupUpdateOps(DRFlowGraph &flow, QueryImpl *impl,
+                                 Context &context, Query query) {
+   // Iterate BOTH aggregate sources; unify to GROUP_UPDATE (spec §3, C-0d).
+   auto emit_group_update = [&](QueryView agg_view, AggProvenance prov,
+       std::vector<unsigned> group, std::vector<unsigned> summary,
+       QueryView input, const ParsedFunctor &algebra_functor) {
+     Algebra alg = SelectAlgebra(algebra_functor, prov);   // §1 V-ALGEBRA, below
+     unsigned sc = flow.statecells.size();
+     flow.statecells.push_back(DRStateCell{agg_view, alg, group,
+         (unsigned)summary.size(), &algebra_functor});
+     DROp op; op.kind = kGroupUpdate;
+     op.agg_view = agg_view; op.provenance = prov; op.algebra = alg;
+     op.agg_table = TableFor(agg_view);         // its OWN DiffTable (V-AGG-SOLE)
+     op.statecell_id = sc; op.group_cols = group; op.summary_cols = summary;
+     op.input_view = input;
+
+     // ---- frontier_in arms (spec §2.3) : two arms, − ALWAYS + + ALWAYS ----
+     // input is DIFFERENTIAL for every corpus case (add_edge is a message).
+     // Structurally a THIRD frontier-vec consumer, sibling of CROSSOVER/PRODUCT_ARM,
+     // but with NO position-keyed partner read (single source view). (average_weight.post-r3 §5.)
+     for (int sign : {-1, +1}) {
+       Arm arm; arm.sign = sign;
+       DRVec *front = (sign < 0) ? NetRemovalsVecOf(input)     // input.D\A frontier
+                                 : NetAdditionsVecOf(input);   // input.A\D frontier
+       arm.effects.push_back(Effect{kVecDrain, front->id});    // E1 input-frontier RAW
+       arm.effects.push_back(Effect{kStateFold, /*value_id=*/sc,
+           .sign = sign});                                     // C-0c: NOT kCounter
+       // touch(g) is a SIDE EFFECT of Fold (spec §2.3) — no separate effect node.
+       op.arms.push_back(arm);
+     }
+     // ---- emit_touched effects (spec §2.3) : the ONE-NET-PAIR band --------
+     // Reads: statecell:emit (working) + statecell:old (sealed, frozen).
+     op.effects.push_back(Effect{kStateEmit, sc});   // E2 fold-before-emit RAW (working def/use)
+     op.effects.push_back(Effect{kStateOld,  sc});   // frozen; RAW-only vs kStateSeal (§2.4 E5)
+     // TWO ordinary counter± into the agg's OWN DiffTable (reserved-already vocab, C-0c):
+     op.effects.push_back(Effect{kCounter, TableId(op.agg_table),
+         .sign = -1, .klass = kNonRecursive});       // retract old  (UPDATECOUNT)
+     op.effects.push_back(Effect{kFlagRead, TableId(op.agg_table), .flag = kInI});
+     op.effects.push_back(Effect{kVecAppend, DelQueueOf(op.agg_table)->id});  // E3 seed-before-drain
+     op.effects.push_back(Effect{kCounter, TableId(op.agg_table),
+         .sign = +1, .klass = kNonRecursive});       // assert new  (UPDATECOUNT)
+     op.effects.push_back(Effect{kFlagRead, TableId(op.agg_table), .flag = kInI});
+     op.effects.push_back(Effect{kVecAppend, AddQueueOf(op.agg_table)->id}); // E3 seed-before-drain
+     flow.ops.push_back(std::move(op));
+
+     // STATE_SEAL op — commit-band tail (mirror kCommitSweep, spec §2.3 STATE_SEAL):
+     DROp seal; seal.kind = kStateSeal; seal.statecell_id = sc;
+     seal.agg_view = agg_view;
+     seal.effects.push_back(Effect{kStateFold, sc, .sign = 0});  // global:rmw sealed:=working (§B-8)
+     flow.ops.push_back(std::move(seal));
+   };
+
+   for (auto agg : query.Aggregates()) {                        // QueryAggregate path
+     // group ++ config = GroupColumns() ++ ConfigColumns() (Query.h:655-656)
+     std::vector<unsigned> group = ColIds(agg.GroupColumns());
+     append(group, ColIds(agg.ConfigColumns()));
+     std::vector<unsigned> summary = ColIds(agg.SummaryColumns());   // Query.h:657
+     emit_group_update(QueryView(agg), kOver, group, summary,
+                       agg.InputView(), agg.Functor());
+   }
+   for (auto kv : query.KVIndices()) {                          // desugared KVINDEX (spec §3)
+     std::vector<unsigned> group = ColIds(kv.KeyColumns());     // Query.h:910  (config = ())
+     std::vector<unsigned> summary = ColIds(kv.ValueColumns()); // Query.h:913
+     emit_group_update(QueryView(kv), kKv, group, summary,
+                       kv.InputView(), kv.NthValueMergeFunctor(0)); // Query.h:926
+   }
+ }

  // WIRED into BuildDRInventory (DR.cpp, after PRODUCT_ARM, before BRANCHES ~:742):
+   BuildGroupUpdateOps(flow, impl, context, query);

  V-ALGEBRA POLICY (SelectAlgebra + reject — spec §2.5 / §7.1 owner decision):
+ static Algebra SelectAlgebra(const ParsedFunctor &f, AggProvenance prov) {
+   if (f.IsInvertible())  return kInvertible;   // Parse.cpp:1211 accessor (R3c-i)
+   if (f.IsRecompute())   return kRecompute;
+   // UNDECLARED:  over() defaults @recompute;  kv/mutable REJECTS (ratified §7.1).
+   if (prov == kKv) {
+     // V-ALGEBRA reject — sibling of the F14 diags, but this is a POLICY reject,
+     // NOT an internal-invariant abort. Placed HERE (construction), fires before lowering.
+     log.Append(f.SpellingRange())
+       << "mutable-attributed merge functor must declare an algebra "
+          "(@invertible or @recompute)";   // AggregatingFunctors §4.1
+     return kRecompute;   // continue so one diag per functor, not an abort
+   }
+   return kRecompute;      // over() default (§7.1)
+ }

  V-ALGEBRA PLACEMENT JUSTIFICATION: the reject must live where BOTH aggregate sources
  meet (construction), NOT in Stratify (R3b already placed the in-SCC reject there) and NOT
  in the F14 pre-pass (which retires). It runs during BuildDRInventory, which is inside
  Program::Build AFTER the F14 pre-pass — so the diag surfaces as a clean user diagnostic and
  Program::Build returns nullopt on a nonempty log (Build.cpp:1051 already checks num_errors).
  ⇒ V-ALGEBRA is in R3c-ii (ledger §11 F9(ii): "V-ALGEBRA must be explicitly in R3c-ii").


═══════════════════════════════════════════════════════════════════════
(2) THE STRATA LIFT RULE FOR GROUP_UPDATE
    — new B.III case in DeriveDRStrata (DR.cpp:1653); F9's HIGHEST-RISK piece
      (no old lift exists to cross-check → the rule is written from spec edges)
═══════════════════════════════════════════════════════════════════════

DeriveDRStrata additions (DR.cpp, in the init + fixpoint blocks):

  INIT (alongside crossover_stratum/product_stratum init, ~DR.cpp:1700):
+   // GROUP_UPDATE sits at its agg VIEW's stratum — the aggregate is placed
+   // STRICTLY ABOVE its input by Stratify (R3b, Stratify.cpp:297-334 mirrors
+   // the negate reject). So view.Stratum() is already input.Stratum()+1-or-more.
+   for (op : flow.ops where kind==kGroupUpdate)
+     group_update_stratum[op] = op.agg_view.Stratum();      // port of view.Stratum() init
+   // The agg TABLE is phase-owned differential → seed drain_stratum[agg_table]:
+     drain_stratum[op.agg_table] = owner_stratum(op.agg_table);  // = max view.Stratum()

  FIXPOINT (a new monotone lift body, alongside crossover lift at DR.cpp:~1800):
+   // GROUP_UPDATE readiness: the op's frontier_in DRAINS input.NetRemovals/NetAdditions,
+   // which are WRITTEN by the INPUT stratum's FRONTIER_FILTERs (E1). So the op — and
+   // hence its agg-table drain — cannot run until the input stratum's frontier filters
+   // are FINAL. This is EXACTLY crossover's ready_across shape (crossover drains a negated
+   // table's frontier vecs). Use STRICT ready_after (like PRODUCT, NOT crossover's SCC
+   // exemption): an aggregate over its own SCC is a Stratify REJECT (R3b), so no SCC
+   // self-read exemption is ever needed — the acyclic fence applies unconditionally.
+   for (op : kGroupUpdate ops):
+     unsigned need = ready_after(op.input_view's owner table);   // drain_stratum[input]+1
+     group_update_stratum[op] = max(group_update_stratum[op], need);
+     drain_stratum[op.agg_table] = max(drain_stratum[op.agg_table],
+                                       group_update_stratum[op]);  // agg drains AT/AFTER the update

  POST (stamp, alongside FIXPOINT_FIRE.scc_group stamping ~DR.cpp:1845):
+   for (op : kGroupUpdate) op.emit_stratum = group_update_stratum[op];
+   for (seal : kStateSeal) seal.emit_stratum = /* commit band — off-lattice, table-id order */;
        // STATE_SEAL is a TRAILING band with commit sweeps (spec §2.4 E5, V-COMMIT-TRAILS);
        // it does NOT participate in the ascending-stratum band, like kCommitSweep.

READINESS JUSTIFICATION (derived from the spec's dependence edges §2.4):
  E1 (input-frontier RAW): FRONTIER_FILTER(input) writes NetRemovals/NetAdditions BEFORE
     GROUP_UPDATE.frontier_in drains them ⇒ group_update_stratum ≥ input's drain_stratum+1.
     This is the STRICT ready_after — identical to PRODUCT's acyclic fence (DR.cpp:1774-1789
     STRICT ready_after with no SCC exemption). Encoded above.
  E2/E3 (fold-before-emit, emit-before-agg-drain): INTRA-op ordering, not a stratum edge —
     handled by the linearizer's band keys (§3), NOT the strata lift. Correctly NOT in this rule.
  E4 (agg-band-after-emit): the agg DiffTable's OWN claim/frontier/commit band lifts off
     drain_stratum[agg_table], which we set = group_update_stratum. So the agg's downstream
     consumers (stratum-2 SEED_FOLDs in average_weight — the JOIN over agg_S,agg_C) read
     ready_after(agg_table) = drain_stratum[agg_table]+1 through the EXISTING branch/join lift.
     ⇒ the double-nesting (KV stratum-0 → sum/count stratum-1 → join stratum-2) falls out of
     the SAME monotone fixpoint the existing crossover/product lift uses; no new fixpoint pass.

NEGATIVE SPACE — a WRONG rule and which oracle/golden catches it:
  WRONG-A (too low: group_update_stratum := input.Stratum(), forgetting the +1 ready_after):
    the agg-table drain would be scheduled IN THE SAME band as the input's frontier filters.
    frontier_in would drain input.NetAdditions/NetRemovals BEFORE the input's FRONTIER_FILTER
    populated them ⇒ empty drains ⇒ the aggregate sees NO input rows ⇒ agg_S/agg_C stay empty ⇒
    average_incoming_weight is empty. CAUGHT BY: the R3d oracle's incremental-vs-from-scratch
    per-view presence assert (Main.cpp:1779 "view <name> from-scratch: present=N incremental=0")
    AND the <name>.stdout golden (empty vs non-empty rows). The V-READY linearizer check
    (B.IV.c :3082) would ALSO likely trip: the kStateFold's drain of a not-yet-written frontier
    vec is a non-carried RAW with writer-stratum > reader-stratum.
  WRONG-B (too high: pin agg_table to the input's SCC group-max):
    an aggregate is NEVER in its input's SCC (Stratify reject), so pinning would over-delay the
    agg band behind unrelated recursive work — no CORRECTNESS failure (strata are a partial
    order; a too-late drain still drains after its writers) but a golden ORDER shift if any
    interleaving changed. CAUGHT BY: not correctness, but the 4-mode stdout agreement — if
    over-delay reorders emitted rows relative to a driver that does NOT sort, stdout diverges.
    (Corpus drivers DO sort keyed drains per the cursor contract, so this is masked for the
    corpus; the risk is real for a future un-sorted case — recorded.)
  WRONG-C (asymmetric: lift the − arm but not the + arm, or vice versa): both arms drain the
    SAME input stratum's filters (E1 is per-input-view, sign-independent). Splitting them would
    schedule +new rows before −old retractions were visible ⇒ transient double-presence within
    the epoch. CAUGHT BY: the oracle's per-batch counter equality (Main.cpp:21-22) on a .batches
    deletion case — the incremental agg counter would diverge from from-scratch mid-batch.
  The lift rule above is SIGN-INDEPENDENT and uses STRICT ready_after — it is WRONG-A/B/C-proof
  by the same argument that makes PRODUCT's strict acyclic fence sound (DR.cpp:1774; F5 verdict).


═══════════════════════════════════════════════════════════════════════
(3) THE LOWERING — frontier-drain fold band + emit_touched band
    — new B.V.a case in LowerDRFlow (Stratum.cpp:1213); GROUP_UPDATE is the
      FIRST family whose lowering CONSUMES op payload (F9: the key-only idiom ends here)
═══════════════════════════════════════════════════════════════════════

LowerDRFlow, new step between PRODUCTS (step 4) and ACYCLIC drains (step 5), Stratum.cpp:~1300:

+   // (4b) GROUP_UPDATES at this stratum. UNLIKE every prior family, this
+   //      lowering READS op payload — algebra, provenance, statecell_id,
+   //      group_cols, summary_cols — because the DR op is the SOLE authority
+   //      for the state cell (no surviving Emit* template re-derives it;
+   //      GROUP_UPDATE has no pre-epoch emitter to duplicate — F9 note).
+   for (op : flow.GroupUpdates() at stratum):
+     LowerGroupUpdate(impl, context, *op);

+ static void LowerGroupUpdate(ProgramImpl *impl, Context &ctx, const DROp &op) {
+   StateCell &sc = ctx.statecell_regions[op.statecell_id];  // the runtime store handle (§4)
+
+   // ---- BAND (a) frontier_in : two VECTORLOOPs over the input net frontiers ----
+   // E1 RAW: these vecs are the input stratum's FRONTIER_FILTER outputs (already final).
+   for (int sign : {-1, +1}) {
+     VECTOR *front = (sign < 0) ? ctx.NetRemovalsVecFor(op.input_view)
+                                : ctx.NetAdditionsVecFor(op.input_view);
+     auto *loop = impl->parallel_regions.Create(...);   // ProgramVectorLoopRegion
+     loop->vector = front;
+     // body: project (group ++ config) and (summary) from the drained row, then Fold.
+     //   NO CHECKMEMBER, NO position-keyed partner read (single source, spec §5 CRUCIAL DIFF).
+     //   group/summary are projections of the delta row itself.
+     body = SC.Fold(gid_of(project(row, op.group_cols)), sign,
+                    project(row, op.summary_cols));   // ← reads op.algebra via the store template
+     // Touch(g) is INTERNAL to Fold (StateCell.h:296) — no separate emitted region.
+   }
+
+   // ---- BAND (b) emit_touched : VECTORLOOP over SC.Touched() (sort-unique, G-8) ----
+   // E2 fold-before-emit RAW is honored by band order: this loop is emitted AFTER both
+   //    frontier_in loops (all folds precede any emit). The linearizer's band key already
+   //    orders (a) < (b) because kStateEmit reads the working value that kStateFold wrote.
+   auto *touched_loop = impl->parallel_regions.Create(...);
+   touched_loop->vector = SC.TouchedVector();   // ← FIXED Touched() API: const Vec<uint32_t>&,
+                                                //    SortAndUnique()'d on read (StateCell.h:333).
+                                                //    We consume it READ-ONLY (never re-append) —
+                                                //    the const contract the review-fix diff pins.
+   // per touched group g:
+   //   new := SC.Emit(g)  [kStateEmit, reads working];  old := SC.Old(g) [kStateOld, sealed]
+   //   TUPLECMP-shaped guard  new != old:                     (spec §2.3)
+   auto *cmp = impl->tuple_cmp_regions.Create(ComparisonOperator::kNotEqual, new, old);
+   //     OLD-vs-NEW TUPLECMP: the old row is (g.cols ++ old); the new row is (g.cols ++ new).
+   //     if new != old:
+   //       UPDATECOUNT(IsAdd=false, agg_table, (g.cols ++ old), kNonRecursive)  → E3 append delQ
+   //       UPDATECOUNT(IsAdd=true,  agg_table, (g.cols ++ new), kNonRecursive)  → E3 append addQ
+   //   These are IDENTITY to the existing seed-fold UPDATECOUNT emission
+   //   (ProgramUpdateCountRegion, Program.h:629) — the ONE-NET-PAIR riding EXISTING drains.
+   //   IsAdd=false BEFORE IsAdd=true: the −old retraction hits an in-I row (TryClaimDel C_nr≤0
+   //   crossing), the +new hits absent/new (TryClaimAdd total>0). Clean acyclic pair, no phantom
+   //   (GROUP_UPDATE has ONE input position — the §5.1.1 two-atom phantom class cannot arise,
+   //   spec §2.5 V-ONE-NET-PAIR).
+   cmp->body = SERIES(updatecount_minus, updatecount_plus);
+   touched_loop->body = cmp;
+ }

  E3 emit-before-agg-drain: the two UPDATECOUNTs append to agg.delQ/addQ; the agg table's
  CLAIM_DRAIN drains them. The emit_touched band MUST precede CLAIM_DRAIN(agg_table) — this is
  the SAME seed-before-drain edge as CROSSOVER/PRODUCT_ARM (spec §2.4 E3), and REUSES V-SEED-DRAIN
  (the existing validator). No new ordering attribute — E3 is a DERIVED def/use edge (kVecAppend
  on delQ before the agg's kVecDrain of delQ), surfaced by the existing linearizer (B.IV.c :2626
  dep-edge derivation). ⇒ the whole agg-table tail (claim drains, frontier filters, commit) is
  IDENTITY to existing acyclic-table lowering (B.V.a step 5) — NO new emitted region there.

LowerCommitSweeps addition (B.V.c, Stratum.cpp:1604 / Procedure.cpp:316):
+   // STATE_SEAL rides the commit-sweep tail alongside kCommitSweep (spec §2.3):
+   for (seal : flow.StateSeals()):
+     ctx.statecell_regions[seal.statecell_id].EmitSeal();   // → SC.Seal() call
  E5 seal-after-emit / emit-before-seal: STATE_SEAL reads working (sealed:=working) and MUST run
  AFTER emit_touched read working as `new` (else next epoch's old() is wrong). Because STATE_SEAL
  is in the TRAILING commit band (off the ascending-stratum lattice, table-id order) and every
  emit_touched is in the ascending band, seal is structurally after every emit — mirror
  V-COMMIT-TRAILS. This is E2/E3's fold-before-emit/emit-before-seal chain closed by band phase.

WHERE GROUP_UPDATE'S LOWERING CONSUMES OP PAYLOAD (the F9 first-consumer note, spelled out):
  - op.statecell_id → selects the runtime StateCell store handle (§4). No emitter re-derives this;
    the store is a NEW object with no pre-epoch template.
  - op.algebra → flows into the store's TEMPLATE type (Invertible vs Recompute) at codegen (§4);
    the fold/emit BODIES differ by algebra, and only the DR op knows the algebra.
  - op.group_cols / op.summary_cols → the projection of the drained row into the cell key/value.
  - op.provenance → NOT consumed by lowering (over and kv lower IDENTICALLY, C-0d); consumed only
    by V-ALGEBRA (§1) and the oracle (§6). Recorded so a reviewer does not expect a lowering fork.
  This is the diff that BREAKS the key-only idiom (big-review F1): the DR op is no longer a mere
  stratum/kind/sign KEY — its algebra/cell-ref payload is READ. Consequently GROUP_UPDATE needs NO
  V-PRED-XCHECK cross-check (there is no duplicated emitter to check against — the DR op IS the
  sole authority, which is exactly the go-forward shape F1 wanted). The PENDING V-PRED-XCHECK diff
  (sites 2/3, EmitJoinFire/EmitClaimDrain) is ORTHOGONAL to GROUP_UPDATE and needs no extension here.


═══════════════════════════════════════════════════════════════════════
(4) CODEGEN + RUNTIME BINDING (StateCellStore instantiation)
═══════════════════════════════════════════════════════════════════════

Database.cpp (lib/CodeGen/CPlusPlus) — per DRStateCell in flow.statecells:

  Type binding (from the DRStateCell payload, §1):
+   // Key type   = the tuple of group ++ config column C++ types (Vector<...> element type,
+   //              same synthesis as a Table's key index tuple).
+   // Algebra    = from op.algebra + the functor:
+   //   kInvertible → Invertible<Reduce_sum_i32> / Invertible<Reduce_count_i32>  (StateCell.h:84)
+   //   kRecompute  → Recompute<Reduce_new_weight_i32>                            (StateCell.h:141)
+   //   Reduce_<functor> is a generated policy struct: Working/Summary/Sealed typedefs +
+   //   Identity/Fold/Emit/SealFrom/OldOf, bodies emitted from the functor's declared shape
+   //   (sum_i32: Working={i32 sum}, Fold=w.sum+=sign*v; count_i32: Working={i32 n};
+   //    new_weight_i32 @recompute: Working=membership Vec, Emit re-runs new_weight_i32).
+   // Working/Sealed/Summary all FLOW from Algebra::* — codegen never names them directly;
+   //   it names StateCellStore<Key, Algebra> and lets the typedefs deduce (StateCell.h:199-201).

  Instantiation (in struct Database, alongside the DiffTable members):
+   StateCellStore<KeyTuple_<gu>, Algebra_<gu>> statecell_<gu>{allocator};
        // constructed with JUST the allocator (StateCell.h ctor, matches the generated-surface
        // epoch: Database sealed state struct constructed with the allocator only).

  Seal call at the commit-sweep tail (LowerCommitSweeps → EmitSeal, §3):
+   // In the generated commit sweep, AFTER every DiffTable::Commit and AFTER emit_touched fired:
+     statecell_<gu>.Seal();               // sealed := Emit(working) for touched groups; clear touched
+   // + (debug) statecell_<gu>.DebugValidate();  alongside DiffTable::DebugValidateCounts (F-8).

  #include: the generated datalog.h gains  #include <drlojekyll/Runtime/StateCell.h>
  (peer of Table.h; dependency-free; already landed in R3a).

  BINDING JUSTIFICATION: Working/Sealed/Summary come from the Algebra policy (StateCell.h:199-201
  `using X = typename Algebra::X`), so codegen only has to pick the Algebra template arg from
  op.algebra + name the Reduce_<functor> policy — no per-column type plumbing. This is the whole
  point of the compile-time Algebra dispatch (spec §1.3: zero-cost, no vtables, repo idiom).


═══════════════════════════════════════════════════════════════════════
(5) F14 RETIREMENT + runall.sh RE-STAGING (SAME UNIT as R3d goldens)
═══════════════════════════════════════════════════════════════════════

Build.cpp:1024-1032 — retire ONLY the agg+kv rows (MAP-impure / on-cycle-@product STAY):

- for (auto agg : query.Aggregates()) {
-   log.Append(agg.Functor().SpellingRange())
-       << "Aggregating functors are not yet supported";
- }
- for (auto kv : query.KVIndices()) {
-   log.Append(kv.NthValueMergeFunctor(0).SpellingRange())
-       << "Relations with mutable-attributed parameters are not yet supported";
- }
  // Build.cpp:955-956/:1024/:1028 region-dispatch asserts become REAL lowering:
  // the IsAggregate/IsKVIndex branches now route into BuildDRInventory's GROUP_UPDATE
  // path (the F14 comment already notes the pre-pass "dominates those asserts (they
  // remain as internal-invariant backstops)" — those backstops are now the lowering entry).

Oracle Main.cpp:611-619 — retire in THE SAME diff (§6):
- for (auto agg : query->Aggregates()) { Fail("aggregates are not supported..."); }
- for (auto kv : query->KVIndices())  { Fail("kv-indices ... not supported..."); }

THE EXACT RUNALL/GOLDEN SEQUENCE keeping every intermediate gate green (the F9 contradiction fix —
land R3c-ii+R3d as ONE atomic diff; the suite is NEVER run at an intermediate SHA):

  Because F14 retirement flips aggregate_1/kvindex_2/3/4 from compiling-to-a-diagnostic to
  compiling-to-a-program, their runall.sh dispatch (:217) must move OUT of the all-modes-diagnostic
  arm IN THE SAME COMMIT that adds their goldens. Sequence WITHIN the single commit's working tree
  (before the suite is ever invoked):

  Step 1 (code): retire F14 agg+kv (Build.cpp) + oracle rejection (Main.cpp) + land the
    GROUP_UPDATE construction/lift/lowering/codegen (§1-§4).
  Step 2 (cases): aggregate_1/kvindex_2/3/4 already EXIST as .dr+.main.cpp (they were the
    expected-diagnostic fixtures). They must now (a) have drivers that observe the compiled
    program AND sort keyed drains, and (b) get REAL goldens. If their existing .main.cpp were
    diagnostic-only stubs, replace with real drivers (cursor-contract sorted).
  Step 3 (bless from oracle truth — NEVER hand-authored):
      DR=build/debug/bin/drlojekyll tests/OptDiff/runall.sh /tmp/wr 6 \
        'aggregate_1|kvindex_2|kvindex_3|kvindex_4|kvindex_1|average_weight|pairwise_average_weight'
      # first run: goldens MISSING (red) — expected, not committed.
      tests/OptDiff/runall.sh --bless /tmp/wr \
        'aggregate_1|kvindex_2|kvindex_3|kvindex_4|kvindex_1|average_weight|pairwise_average_weight'
      # blesses .stdout (+ .oracle.stdout/.monotone.stdout for cases with .batches, §6).
  Step 4 (runall.sh dispatch edit) — MOVE the flipped cases:
      runall.sh:217:
-       aggregate_1|kvindex_2|kvindex_3|kvindex_4|agg_in_scc_1|kv_in_scc_1|algebra_dup_1|algebra_conflict_1|evm_func_parse|nonascii_1|truncated_decl_1)
+       agg_in_scc_1|kv_in_scc_1|algebra_dup_1|algebra_conflict_1|evm_func_parse|nonascii_1|truncated_decl_1)
      # aggregate_1/kvindex_2/3/4 fall through to the default `*)` arm → diffrun.sh 4-mode golden.
      # (agg_in_scc_1/kv_in_scc_1 STAY diagnostic — in-SCC aggregation is still a Stratify reject.)
      # Header comment (:18-19) updated to drop them from the expected-diagnostic list.
  Step 5 (kvindex_1 — THE BY-DESIGN MODE SPLIT, the spec's flagged open question):
      runall.sh:223-228 currently: run_vs_golden opt/nocf + expect_diagnostic nodf/none.
      CONFIRMED mechanism (kvindex_1.dr:1-4 + diffrun.sh:44-47): the KVINDEX is CANONICALIZED AWAY
      (unused value col → TUPLE) ONLY when dataflow-opt is ON (opt, nocf). With dataflow-opt OFF
      (nodf, none) the KVINDEX survives → today hits F14 → diagnostic.
      AFTER F14 retirement: nodf/none no longer error — the surviving KVINDEX now LOWERS via
      GROUP_UPDATE. So kvindex_1 becomes a PLAIN 4-mode golden case:
-       kvindex_1)
-         run_vs_golden opt || exit 1
-         run_vs_golden nocf || exit 1
-         expect_diagnostic nodf || exit 1
-         expect_diagnostic none || exit 1
-         echo "$NAME modesplit OK"
-         ;;
      # → falls through to default `*)` → diffrun.sh, which byte-compares ALL 4 modes to ONE golden.
      CROSS-MODE AGREEMENT (the resolution of the flagged question): opt/nocf emit a TUPLE-forwarded
      program (no value column, no GROUP_UPDATE); nodf/none emit a real GROUP_UPDATE over
      edge_weight. These are DIFFERENT emitted programs but has_edge(F,T) reads ONLY the key columns
      (kvindex_1.dr:11) — so BOTH produce the SAME has_edge rows ⇒ SAME stdout. The golden-master
      cross-mode agreement (CLAUDE.md: EVERY mode byte-compared to the SAME golden) HOLDS by
      construction because the observable projection is key-only. This is why kvindex_1 is safe to
      collapse to a plain golden case. (If a future kvindex case read the VALUE column, opt/nocf's
      canonicalized-away value would DIVERGE from nodf/none's merged value — that case would need
      to STAY mode-split. kvindex_1 specifically does not.)
  Step 6 (commit): the SINGLE commit contains code + moved dispatch + new goldens + kvindex_1
    collapse. Now run the full suite ONCE — it must end SUITE: PASS. At NO intermediate SHA is the
    suite red (the bless in step 3 happened in the working tree before the dispatch edit committed).

  This is the ledger §11 F9 disposition realized: "R3c-ii+R3d merged into one gated unit."


═══════════════════════════════════════════════════════════════════════
(6) THE ORACLE REFEREE (per-group recompute + sidecar shape)
═══════════════════════════════════════════════════════════════════════

Oracle Main.cpp — retire the rejection (§5) + teach BOTH paths:

  FROM-SCRATCH path (the referee — per-group recompute, Main.cpp:1639+ semi-naive):
+   // After the input stratum's from-scratch materialization is final, for each
+   // QueryAggregate/QueryKVIndex view: GROUP the input rows by (group ++ config),
+   // and for each group RUN THE FUNCTOR from scratch over the group's members.
+   //   sum_i32   → sum of the aggregated col over group members
+   //   count_i32 → member count
+   //   new_weight_i32 (kv merge) → fold new_weight_i32 over members (NthValue(0) semantics)
+   // Emit agg rows = (group ++ functor-result). This is the DEFINITIONAL aggregate —
+   // no counters, no cells, no old/new. It is the ground truth. (spec §4.)
+   // Because it is set-presence only, it slots into the EXISTING per-view presence
+   // structure (Main.cpp:323 from-scratch row set) with no counter machinery.

  INCREMENTAL path (the checked subject):
+   // A StateCell interpreter mirroring §1-§2: fold on input net frontiers (both signs),
+   // emit_touched one-net-pair guarded by new!=old. Every batch, the EXISTING per-view
+   // presence assert (Main.cpp:1779) extends to agg views for free once both paths produce
+   // the view's rows; ADDITIONALLY assert the emitted summary VALUE matches from-scratch.
+   // @recompute is trivially refereed: the from-scratch path IS recompute, so the §1.3-III
+   // incremental lowering is validated by the same equality (spec §4).
+   // MIN/MAX (not in corpus): from-scratch handles it with no special case; recorded for §6 traces.

  SIDECAR SHAPE (mirrors existing .oracle.stdout/.monotone.stdout, spec §4):
    For average_weight.dr and pairwise_average_weight.dr, add as OptDiff cases:
      - <name>.dr + <name>.main.cpp   (driver SORTS keyed drains — average_incoming_weight is a
                                        keyed query; MUST sort before printing, cursor contract).
      - <name>.batches                 (deletion-EXERCISING: +/- add_edge lines in batch/end blocks
                                        per the Main.cpp:57-73 grammar — exercises the − arms and,
                                        for edge_weight's @recompute merge, retraction re-merge).
      - <name>.stdout                  (4-mode golden, byte-compared across opt/nodf/nocf/none).
      - <name>.oracle.stdout           (oracle DumpRelations(from_scratch=false), Main.cpp:1817 —
                                        final agg rows canonical-sorted; PRODUCED only if the
                                        incremental-vs-from-scratch cross-check passes = ORACLE: OK).
      - <name>.monotone.stdout         (DumpRelations over all-facts-ever, Main.cpp:2009 monotone
                                        projection).
    ALL blessed FROM ORACLE TRUTH (never hand-authored): run oracle → confirm ORACLE: OK → bless.

  The .batches deletion cases make the AggregatingFunctors §5 hand-traces (spec §6: MIN retraction,
  SUM @invertible + annihilation) REGRESSION-COVERED — average_weight/pairwise's + and − arms and
  the @recompute re-merge all fire under deletion.


═══════════════════════════════════════════════════════════════════════
(7) CORPUS ANNOTATION (which pragmas on which functors)
═══════════════════════════════════════════════════════════════════════

data/examples/average_weight.dr:
  :6  #functor sum_i32(aggregate i32 Val, summary i32 Sum).
+       → #functor @invertible sum_i32(aggregate i32 Val, summary i32 Sum).
        (running sum, O(1) fold/unfold — spec §1.3-I; the @invertible payoff, §6 trace 2.)
  :7  #functor count_i32(aggregate i32 Val, summary i32 Count).
+       → #functor @invertible count_i32(aggregate i32 Val, summary i32 Count).
        (running count, O(1); same class as sum.)
  :8  #functor new_weight_i32(bound i32 OldWeight, bound i32 NewWeight, free i32 NewWeightOut) @range(.).
+       → #functor @recompute new_weight_i32(...) @range(.).
        (KV MERGE functor — invertibility UNDECLARED for a merge; @recompute is the ratified
         fallback, and mutable REJECTS if undeclared — §7.1 policy. So it MUST be annotated.)
        (NOTE: @recompute + @range coexist — @range is the functor's I/O mode, @recompute is the
         algebra selector; orthogonal, both round-trip per Format.cpp:132-144.)

data/examples/pairwise_average_weight.dr:
  :7  #functor new_weight_i32(...) @range(.).
+       → #functor @recompute new_weight_i32(...) @range(.).
        (the SOLE aggregate is the mutable() merge — MUST be annotated or the kv V-ALGEBRA
         reject fires; §7.1 "pairwise's merge functor gets annotated — corpus edit".)
  add_i32/div_i32 (:3-4) are pure @range MAPs, NOT aggregates → NO algebra pragma (correctly none).
  sum_i32/count_i32 do NOT appear in pairwise (it is KV-only) → no edit there.

  JUSTIFICATION: exactly the functors that back a GROUP_UPDATE get an algebra pragma; the merge
  functors are @recompute (undeclared-merge is a reject), the summary functors are @invertible
  (SUM/COUNT are the abelian §1.3-I class). MAP functors get nothing. This matches §7.1's "over()
  defaults @recompute, kv rejects undeclared" — sum/count are annotated @invertible to EXERCISE the
  §1.3-I path (the default would be @recompute, which is correct-but-slower and would not exercise
  the invertible fold/unfold the oracle's §1.5 algebra-law checks want to referee).


═══════════════════════════════════════════════════════════════════════
(8) PRE-REGISTERED PREDICTIONS (before any run — the COST/oracle honesty gate)
═══════════════════════════════════════════════════════════════════════

DIAGNOSTIC CHANGES (which cases flip):
  - aggregate_1, kvindex_2, kvindex_3, kvindex_4: FLIP from all-modes-diagnostic → 4-mode GOLDEN
    (they compile in all 4 modes once F14 agg+kv rows retire). Move out of runall.sh:217.
  - kvindex_1: FLIP from mode-split (2 golden + 2 diagnostic) → plain 4-mode GOLDEN (nodf/none
    stop erroring; cross-mode agreement holds because has_edge reads keys only — §5 step 5).
  - agg_in_scc_1, kv_in_scc_1: UNCHANGED (in-SCC aggregation is still a Stratify reject, R3b).
  - algebra_dup_1, algebra_conflict_1: UNCHANGED (R3c-i pragma-conflict diags, pre-lowering).
  - NEW diagnostic case candidate: an undeclared-merge kv reject (V-ALGEBRA) — add a directed case
    (e.g. kv_no_algebra_1) so V-ALGEBRA has a red-fixture; expected-diagnostic in runall.sh.

CASES THAT GAIN GOLDENS (blessed from oracle truth in this unit):
  - aggregate_1.stdout, kvindex_1.stdout, kvindex_2/3/4.stdout (4-mode).
  - average_weight.{stdout,oracle.stdout,monotone.stdout} + .batches.
  - pairwise_average_weight.{stdout,oracle.stdout,monotone.stdout} + .batches.

SUITE COUNT AFTER:
  Current: 162 (per R3c-i commit 50f936b). Net change:
    + average_weight, + pairwise_average_weight               (+2 new corpus cases)
    + kv_no_algebra_1 (V-ALGEBRA directed reject)             (+1 directed)
    aggregate_1/kvindex_1-4 are RE-CLASSIFIED, not added      (+0)
  ⇒ PREDICTED SUITE COUNT: 165. Must end SUITE: PASS with the two corpus cases green in all 4
    modes + oracle + monotone. (If the orchestrator chooses to add MORE directed V-ALGEBRA/algebra
    cases, count rises accordingly — 165 is the floor for the minimum unit.)

Q5 / FLAGSHIP (tc, the recursive-band flagship) EXPECTATIONS — NEUTRAL, and WHY:
  - GROUP_UPDATE is an ACYCLIC family (Stratify forbids in-SCC aggregation; every corpus agg folds
    kNonRecursive). It touches NEITHER LowerDRRounds (the recursive band) NOR EmitJoinFire's matrix
    NOR EmitChainStep's negate gate. tc and the recursive flagship have NO aggregate → their emitted
    code is BYTE-UNCHANGED. The new BuildGroupUpdateOps loop is over query.Aggregates()/KVIndices(),
    which are EMPTY for tc ⇒ zero ops constructed ⇒ zero lowering ⇒ zero codegen delta.
  - The DeriveDRStrata GROUP_UPDATE lift body only fires when a kGroupUpdate op exists; for tc the
    fixpoint is unchanged (no new lift entries), so the integer lift is byte-identical → region
    nesting identical → tc program.ir byte-identical.
  - StateCell.h is header-only and only #included when a store is instantiated; tc's generated
    datalog.h gains no #include and no member. Bench binaries for tc are unaffected.
  ⇒ PREDICTION: Q5 and every non-aggregate flagship are NEUTRAL (byte-identical emitted code,
    no wall-time delta beyond noise). The only perf surface is the two new aggregate cases, which
    are perf-only (optionally added to bench/ — COST of @recompute vs @invertible per group,
    spec §7 R3d) and NEVER gate. The interleaved-binary A/B is not needed (no <10% delta to
    adjudicate on existing cases — the delta is exactly zero on them).

CONFIDENCE / RESIDUAL RISKS (pre-registered so a miss is legible):
  - HIGHEST: the DeriveDRStrata GROUP_UPDATE lift (§2) has NO old lift to cross-check — a
    mis-stratified frontier drain is caught ONLY by the R3d oracle (WRONG-A above). This is the
    single place the byte-identical-gate discipline does not protect us; the oracle .batches
    deletion cases are the real proof.
  - MEDIUM: the @recompute membership store's group-id stability across rehash under a real arena
    (big-review F7 "untested") — first exercised for real by average_weight/pairwise @recompute
    merge under deletion; the .batches cases are the first arena-real exercise.
  - LOW: cross-mode agreement for kvindex_1 (argued key-only-safe in §5 step 5; the golden bless
    confirms).


## Open questions

- EMIT ORDERING OF THE TWO CORPUS AGGREGATES (G-6 shared-input fusion): average_weight's sum_i32 and count_i32 both drain edge_weight's SAME net-frontier vecs. R3 MAY leave them unfused (two independent drains, correctness-identical per spec §2.2) or fuse. UNFUSED is the safe R3 choice and what I specced, but the golden ORDER of agg_S vs agg_C rows depends on which GROUP_UPDATE lowers first (query.Aggregates() iteration order). Since the driver sorts keyed drains, stdout is deterministic regardless — but the .oracle.stdout / program.ir are NOT sorted. CONFIRM the aggregate iteration order is stable (it is derived from Query view order) so the blessed goldens are reproducible across re-blesses. If unstable, pin an explicit sort on flow.GroupUpdates() by agg_view id.
- V-ALGEBRA DIAGNOSTIC ANCHOR + WORDING: I placed the kv-undeclared reject at BuildDRInventory construction (SelectAlgebra), anchored at the merge functor's SpellingRange, so it composes with Program::Build's num_errors→nullopt gate (Build.cpp:1051). But R3b already added a Stratify-layer aggregate reject, and R3c-i added parser-layer @-conflict diags. CONFIRM with the owner that a THIRD diagnostic layer (DR construction) is acceptable, vs folding V-ALGEBRA into the parser (Functor.cpp, where IsInvertible/IsRecompute are already read) so the reject fires before dataflow — the parser has no provenance (over vs kv) yet, so it CANNOT distinguish 'over defaults @recompute' from 'kv rejects', which is exactly why I put it at construction where provenance exists. Flagging the layer choice for ratification.
- AGGREGATE_1 / KVINDEX_2-4 EXISTING DRIVER SHAPE: these were expected-diagnostic fixtures — their .main.cpp may be diagnostic-only stubs (never linked against generated code) rather than real drivers. If so, R3c-ii+R3d must AUTHOR real cursor-contract-sorted drivers for all four, not just re-classify them. I could not verify the .main.cpp contents in the time budget; the orchestrator should read tests/OptDiff/cases/{aggregate_1,kvindex_2,kvindex_3,kvindex_4}.main.cpp and confirm whether they compile-and-run or are stubs, which determines whether step 2 of §5 is a re-classify or an author-new.
- STATE_SEAL COMMIT-BAND POSITION vs DiffTable::Commit ORDER: spec §2.4 E5 constrains seal-after-emit-within-epoch but says the order relative to the agg table's Commit is otherwise free (post-r3 §4). I placed STATE_SEAL in the trailing commit band alongside kCommitSweep. CONFIRM whether STATE_SEAL(SC) must come strictly AFTER COMMIT_SWEEP(agg_table) or may interleave — both are correct for old()-next-epoch, but the emitted region ORDER affects program.ir byte-identity across re-blesses. Recommend pinning STATE_SEAL after all COMMIT_SWEEPs (statecell_id order) for determinism.