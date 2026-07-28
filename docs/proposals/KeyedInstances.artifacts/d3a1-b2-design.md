# D3.a.1 STAGE (b) — LANE b2: DEATH MINT→LOWERING→EMITTER + THE DEMAND kNetRemovals FRONTIER + V-INST-DRAIN

> Tip **95251825** (verified `git rev-parse HEAD`, tree clean). Every anchor below
> re-verified at code this session (not inherited from the substrate). Binding
> context honored, not re-litigated: d3a1-substrate.md (§5/§6/§7 incl. the d2
> CO-ACTIVATION ruling + XC-3), d3a-ruling-brief.md (OD-15), d3a-substrate.md
> §7:888-1001, d3a0-design.md §3. ONE-COMMIT UNIT: this design assumes b1's d1
> toggle lands in the same commit (XC-3 forbids d1-only; no temporary fence is
> designed — none is needed, see §2.4).
>
> Lane boundary: b1 = toggle+writer+netting; b3 = band (drop scan / partition
> belt / diff-bit selector inside EmitSubgraphInstance); b4 = witness/fences/
> gates. Interfaces I EXPECT from siblings are in §8.

---

## §0 HEADLINE DECISIONS

- **D-1 (M1, region shape): NO new region kind, NO mode flag — the death lowers
  as band-(a0) INSIDE the existing SUBGRAPHINSTANCE region, on the SEAL
  PRECEDENT.** kInstanceSeal is already a DR op with no region of its own,
  self-lowered at the region tail (HP-1/OD-5; enrollment Procedure.cpp:344-345,
  emission Database.cpp:2461). The death is its exact head-mirror: a DR op
  self-lowered at the region HEAD, enrolled by the same lowering. The 3-op DR
  store protocol {death?, instantiate, seal} maps to ONE region whose internal
  band order (a0 → a1 → a2 → b → Seal) realizes the pinned DR order
  (OD-2 death-before-instantiate, seal band 11) BY CONSTRUCTION — textual order
  inside one region is unreorderable by any region-flattening pass, which a
  standalone pre-instantiate sibling region would not get for free. (The
  d3a1-substrate §3.2 item-4 phrase "its own pre-instantiate emission" was an
  as-landed gap observation, not a shape ruling; the seal precedent is the
  binding idiom.) Cost avoided: a new Program.h class + visitor + Optimize
  overload + IR print + codegen dispatch, all for a 4-line band.
- **D-2 (region signal): the death band keys on DEATH-OP PRESENCE (a nullable
  removal-frontier member), NEVER on `region.differential`.** The d2 ruling
  keeps the two predicates separately spelled (P-DEATH = Rel.cpp:1139
  demand-keyed; P-STORE = :1055/:1059 pub-keyed; do NOT fold). Lowering-side
  the op's existence IS P-DEATH's value — no re-evaluation, no shared helper.
  b3's (T,F) scan/belt keys on the region diff bit (P-STORE) independently;
  co-activation (the d2 theorem) makes them agree at this slice.
- **D-3 (M3, frontier provisioning): ZERO ControlFlow pre-provisioning; the
  differential machinery provisions everything; V-INST-DRAIN is REGIME-SPLIT
  instead of fed.** Under d1 the demand table is differential ⇒ BuildDRInventory
  automatically mints its DR vec sextet incl. kNetRemoval/kNetAddition
  (Rel.cpp:1814-1829) and its ± kFrontierFilter producers (:2540-2543, acyclic
  arm — the demand chain is acyclic on the slice, FENCE (i)). The ControlFlow
  VECTORs are lazily minted by the memoized `TableDeltaVector`
  (Build.cpp:745-777) from inside `EmitFrontierFilter` (Stratum.cpp:745-750)
  during the stratum lowering, and `LowerSubgraphInstances` fetches the SAME
  memoized objects afterwards (existing demand/input fetches Procedure.cpp:
  287-294; the new kNetRemovals fetch, §1.2). So the XC-3 timing problem
  (VECTORs first minted at Stratum.cpp:2477/:2489, after ValidateDROps :2186)
  is discharged by making the validator check, per regime, THE PRODUCER THAT
  ACTUALLY FEEDS THE DRAIN — monotone: the CF vec the eager boundary append
  provisioned (Build.cpp:999); differential: the DR vec + its frontier-filter
  producer, which DO exist at validator time. Green by construction (the
  producers are minted unconditionally per differential table by the same
  BuildDRInventory that minted the death), NOT by weakening (each regime's
  check names its real feeder; a missing feeder still aborts).
- **D-4 (M2, emitter): the death band is FindInstance + RecycleCurrent ONLY;
  the full (T,F) retract is DELIVERED BY band-(b)'s drop scan (b3).** Per
  OQ-DEATH-VS-REBUILD + the §4.3 coupling table: RecycleCurrent's Touch marks
  the iid touched (suppressing a1/a2 same-epoch rescans) and leaves `current`
  empty, so the dead key reaches band-(b) with dropped = frozen∖current =
  ALL frozen rows — death is the DEGENERATE CASE of the generic drop scan, and
  the pub-side −1 counters/queue appends live where the DR model already puts
  them: on the INSTANTIATE op's diff-arm effects (Rel.cpp:829-848), realized
  once by b3 for every drop (death-caused or rebuild-caused). The death op's
  own zero-counter signature (DeathEffects, Rel.cpp:861-884) stays exactly
  faithful to what the band emits: one drain, one demand read, one frozen read,
  one −1 store write — nothing else.
- **D-5 (dead-key FindInstance semantics): SKIP on kNoInstance, silently.**
  Grounds in §4.2.
- **D-6 (render): the kInstanceDeath `.rel` arm (Format.cpp:883-893) renders
  as-is — ZERO new `.rel` token spellings (audit §5.1). The ControlFlow `.ir`
  region line gains one only-when-present `death <vec>` token (§5.2) — a
  grammar note is owed (E-71 discipline applied to the ir-dump format doc).**
- **D-7 (id contract): the b2 sub-diff allocates ZERO new ids itself.** The
  death lowering's TableDeltaVector(demand, kNetRemovals) call is a memoized
  FETCH (the frontier-filter lowering already minted it earlier in the same
  build); the death band is pure text. Every id/byte movement in the slice
  traces to b1's d1 flip (differential machinery engaging for the demand+pub
  closure), which is witness-gated (§7.1).

---

## §1 (iii) M1 — THE DEATH LOWERING + ENROLLMENT (closes G-6, V-INST-EMITTED)

### 1.1 Iteration source

`LowerSubgraphInstances` (Procedure.cpp:265-347) keeps its instantiate-only
loop (`dr_flow.SubgraphInstances()`, Rel.cpp:585-593 — kSubgraphInstantiate
only). The death ops are pre-bucketed by store id with the EXISTING generic
accessor `DRFlowGraph::OpsOfKind` (Rel.cpp:595-603) — no new Rel.h surface:

```cpp
// D3.a.1 (M1): the death ops by store id. V-INST-PAIR (Rel.cpp:4390-4407)
// guarantees at most one death per sid, so a plain map is total.
std::unordered_map<unsigned, const DROp *> death_by_sid;
for (const DROp *dop : dr_flow.OpsOfKind(DROpKind::kInstanceDeath)) {
  death_by_sid.emplace(dop->instance_store_id, dop);
}
```

placed at the head of `LowerSubgraphInstances`, before the loop.

### 1.2 Per-instantiate death wiring (insert after the input_frontier Emplace,
Procedure.cpp:301, before the key/row plumbing)

```cpp
// D3.a.1 (M1/M2): the death band's drain source — the demand table's NETTED
// net-removals frontier (the commit-band frontier-filter product,
// EmitFrontierFilter Stratum.cpp:745-778; the memoized fetch returns the SAME
// VECTOR that filter's lowering already minted). Null under R-MONO: the band
// and the member stay absent, and the emitter keys on presence (D-2 — the
// death is keyed by the OP, never by the region diff bit; d2 ruling).
if (auto dit = death_by_sid.find(sid); dit != death_by_sid.end()) {
  const DROp *const death = dit->second;
  // V-INST-DEATH-COHERENCE [ALWAYS-ON]: the death op must name the SAME
  // demand/pub tables as its instantiate — a drifted mint would drain the
  // wrong table's frontier or retract into the wrong pub. fprintf+abort,
  // survives NDEBUG (the V-INST-DIFF-COHERENCE :274-285 mold).
  if (death->demand_table != op->demand_table ||
      death->table_op_table != op->table_op_table) {
    std::fprintf(stderr,
                 "error: SUBGRAPHINSTANCE store %u: kInstanceDeath tables "
                 "(demand/pub) disagree with its kSubgraphInstantiate\n", sid);
    std::abort();
  }
  si->removal_frontier.Emplace(
      si, TableDeltaVector(impl, context, death->demand_table,
                           VectorKind::kNetRemovals));
  context.emitted_instance_ops.push_back(
      {sid, static_cast<uint8_t>(DROpKind::kInstanceDeath)});
}
```

The enrollment closes V-INST-EMITTED (Procedure.cpp:435-464): the enrolled side
already counts kInstanceDeath (:446-450, verified); the emitted side now gains
the matching `{sid, kInstanceDeath}` entry exactly when a death op exists. The
multiset compare is sorted, so push position is free; we push inside the `if`
to keep emission==enrollment a single-site truth. The stale comment at
Procedure.cpp:341 ("Death is not minted under R-MONO") is updated to name the
three-kind protocol.

### 1.3 Region member + public accessor

- `lib/ControlFlow/Program.h` (impl, after `input_frontier` :1175):
  `UseRef<VECTOR> removal_frontier;  // BAND (a0) death drain source (netted
  demand net-removals). NULL under R-MONO — presence == a kInstanceDeath op
  for this store (D3.a.1).`
- `include/drlojekyll/ControlFlow/Program.h` (public wrapper, after
  `InputFrontier()` :861): `std::optional<DataVector> RemovalFrontier(void)
  const noexcept;` (nullable-accessor precedent: `WorkerId()` :554). Impl in
  `lib/ControlFlow/Program.cpp` beside DemandFrontier/InputFrontier.
- **Hash/Equals/IsNoOp (Operation.cpp:512-543): UNTOUCHED.** Same argument as
  the D3.a.0 diff bit: Equals keys (store_id, pub_table) and removal_frontier
  is a pure function of the store's demand table + its differentialness — two
  Equals-equal regions cannot differ in it (one instantiate deriver per pub,
  V-INST-SOLE; one death per sid, V-INST-PAIR). Record in the commit message.
- ExtractPrimaryProcedure threads the new UseRef automatically (the
  read/written-by-primary vec collection + replacement, Procedure.cpp:700-726
  — the same route the LowerSubgraphInstances :263-264 comment pins for the
  demand frontier). CONSEQUENCE (correctness, not just plumbing): the removal
  frontier becomes a primary-proc-LOCAL vector, fresh-empty per epoch — a
  stale removal row can never survive into a later epoch to kill a reborn key.

### 1.4 The id contract

New ids minted by this sub-diff: **none** (D-7). The kNetRemovals VECTOR id is
minted by `EmitFrontierFilter`'s existing `TableDeltaVector` call
(Stratum.cpp:748-750) at the demand table's stratum — a mint that fires for
EVERY acyclic differential table already and reaches the demand table only
under d1 (witness-only). `LowerSubgraphInstances` runs later
(Procedure.cpp:432) and FETCHES it memoized. The death band emission
(Database.cpp) allocates no ids (`next_ins_id` untouched; no VAR/REGION
creation). Therefore every pinned dump outside the witness is untouched: on
non-`-demand-instance` programs no instance op exists (flag-off,
Rel.cpp:1009-1017 gate) and the demand message is monotone (b1's toggle off),
so neither the death map scan nor any new call site executes with effect.

---

## §2 (i) M3 — THE DEMAND kNetRemovals FRONTIER (closes G-3/G-4; the XC-3 discharge)

### 2.1 The provisioning chain under d1+d3 (all sites verified at tip)

```
DR layer (BuildDRInventory, BEFORE ValidateDROps at Stratum.cpp:2186):
  demand table differential + non-induction-owned (acyclic demand, FENCE (i))
    -> sextet minted incl. {kNetRemoval, kNetAddition}    Rel.cpp:1814-1829
    -> ± kFrontierFilter producers minted                 Rel.cpp:2511-2538, 2540-2543
       (- arm: drain OverdeleteSet -> FlagRead NetDeleted -> append kNetRemoval)
  DeathEffects' kVecDrain{demand, kNetRemoval} (Rel.cpp:865-869) now RESOLVES
  in ResolveVecIdx (:4638ff) -> a real RAW dep edge frontier-filter -> death
  (the ~0u silent-no-edge of d3a1-substrate §2.3 is gone by construction).

ControlFlow layer (AFTER ValidateDROps — the XC-3 timing fact):
  stratum loop (Stratum.cpp:2446-2490) -> LowerDRFlow -> EmitFrontierFilter
    -> TableDeltaVector(demand, kNetRemovals) MINTS the VECTOR (:748-750)
       and appends the netted rows (full COLUMN-VALUE tuples: row_vars = one
       VAR per table column, :757-777 — same element shape as the monotone
       route's boundary appends, so band-(a1)'s existing Key_<id>{k...}
       construction idiom carries over to the death band unchanged).
  PublishDifferentialMessageVectors -> LowerSubgraphInstances (:432)
    -> memoized fetch into si->removal_frontier (§1.2).
```

The demand kNetAdditions frontier switches routes identically (the
Build.cpp:999 `!TableIsDifferential` gate turns the eager boundary append off;
the + frontier filter takes over). No a1 code change: the VECTOR keeps its
shaping authority (`TableDeltaVector` shapes BOTH routes, Build.cpp:752-777),
and the semantic content upgrades to the POST-NETTING commit-band product —
exactly the OD-15 birth-trigger shape (d3a1-substrate §4.1's noted rider).

### 2.2 Why no pre-provisioning block is designed

An unconditional pre-ValidateDROps `TableDeltaVector` sweep would make
V-INST-DRAIN a tautology (the validator would test a vec the sweep itself just
created — "green by weakening", explicitly forbidden by the lane charter). A
differential-only sweep would still test allocation, not production. The
regime-split validator (§3) instead checks the PRODUCER per route; the CF
VECTOR's existence then follows from the producer's lowering, which
V-PRED-XCHECK Site 4 (Stratum.cpp:731-743) and the frontier filter's own
census already police.

### 2.3 Runtime data-timing (the epoch position, substrate §4.4 re-affirmed)

Per epoch: handler NETBATCH (b1) → ingest folds park the demand deltas in the
demand DiffTable's queues → stratum phases: claim drains re-test
(TryClaimDel/TryClaimAdd) → frontier filters fill demand kNetRemovals /
kNetAdditions (NETTED: only rows whose presence actually crossed) → transmit
publishes → **instance region: band-(a0) death drain → a1 → a2 → band-(b) →
Seal** → commit sweeps (incl. the demand table's was!=now publish). The death
drain consumes a frontier filled strictly earlier in the same procedure — the
same relation band-(a1) already relies on for its additions drain.

### 2.4 XC-3 status under this design

With d1+d3 co-landed there is no intermediate abort. For the record (and the
stage-(d) checkpoint, §7.4): a d1-only build under this design no longer
aborts at V-INST-DRAIN (the differential arm's producers exist automatically);
its FIRST abort moves to **V-INST-EMITTED** (death minted, never lowered) —
i.e., XC-3's "not independently landable" verdict stands with a reordered
abort chain, and observing that abort in the prototype worktree is the
designed liveness proof for the V-INST-EMITTED death leg (§7.4-L2).

---

## §3 (ii) THE V-INST-DRAIN EXTENSION (+ two strengthenings)

### 3.1 The regime-split rewrite (Rel.cpp:4496-4524 block)

The block keeps its per-instantiate loop and gains (a) a regime split on the
demand arm, (b) a NEW kInstanceDeath clause via a PURE core (unit-testable,
the CheckInstanceOrder mold). Sketch (replaces :4499-4524):

```cpp
  // V-INST-DRAIN (HP-2, §3.3; REGIME-SPLIT at D3.a.1 — XC-3): every instance
  // drain must name a PROVISIONED frontier, checked against the producer that
  // actually feeds it in its regime.
  //  - MONOTONE demand/input: the eager boundary append (Build.cpp:999)
  //    provisioned the ControlFlow VECTOR during the walk — it must exist NOW.
  //  - DIFFERENTIAL demand: the frontiers are commit-band products; their
  //    ControlFlow VECTORs are first minted inside the stratum lowering,
  //    AFTER this validator (XC-3) — so check the DR-side vec + the
  //    kFrontierFilter producer of the right sign, both minted by
  //    BuildDRInventory before any validation.
  const auto cf_ok = [&](TABLE *t, VectorKind kind) -> bool {
    auto it = context.table_delta_vecs.find(t);
    return it != context.table_delta_vecs.end() &&
           it->second.count(static_cast<unsigned>(kind)) &&
           it->second.at(static_cast<unsigned>(kind)) != nullptr;
  };
  const auto dr_ok = [&](TABLE *t, VecRole role, int sign) -> bool {
    auto tv = flow.table_vecs.find(t);
    if (tv == flow.table_vecs.end() || !tv->second.count(role)) {
      return false;
    }
    for (const DROp &f : flow.ops) {
      if (f.kind == DROpKind::kFrontierFilter && f.table_op_table == t &&
          f.table_op_sign == sign) {
        return true;
      }
    }
    return false;
  };
  for (const DROp &op : flow.ops) {
    if (op.kind != DROpKind::kSubgraphInstantiate) {
      continue;
    }
    const bool demand_ok =
        TableIsDifferential(op.demand_table)
            ? dr_ok(op.demand_table, VecRole::kNetAddition, +1)
            : cf_ok(op.demand_table, VectorKind::kNetAdditions);
    if (!demand_ok) {
      ValidatorFail("V-INST-DRAIN: an instantiate's demand net-additions "
                    "frontier was never provisioned (OD-7/§2.2 gap)");
    }
    // [R-REBUILD-a2] input(edge) arm UNCHANGED: monotone this slice (a
    // differential input is V-INST-SOLE-rejected upstream, :4335-4340;
    // D3.a.2 owns the split here).
    if (!cf_ok(op.input_table, VectorKind::kNetAdditions)) {
      ValidatorFail("V-INST-DRAIN: an instantiate's input(edge) net-additions "
                    "frontier was never provisioned (OD-4/R-a2 gap)");
    }
  }
  CheckInstanceDeathFrontier(flow);  // D3.a.1 death clause (pure core, below)
```

### 3.2 The pure death clause `CheckInstanceDeathFrontier` (NEW, Rel.cpp +
Rel.h declaration beside `CheckInstanceOrder`, Rel.h ~:1214 area)

```cpp
// V-INST-DRAIN death clause (D3.a.1, G-4/G-5): every kInstanceDeath must
// drain a PROVISIONED demand net-removals frontier — DR vec present AND the
// `-` kFrontierFilter producer minted. PURE over the flow (no impl/context,
// no TABLE deref — pointer identity only) so the negative space is
// death-testable in tests/RelValidators (the CheckInstanceOrder mold).
void CheckInstanceDeathFrontier(const DRFlowGraph &flow) {
  for (const DROp &op : flow.ops) {
    if (op.kind != DROpKind::kInstanceDeath) continue;
    bool vec_ok = false;
    if (auto tv = flow.table_vecs.find(op.demand_table);
        tv != flow.table_vecs.end()) {
      vec_ok = tv->second.count(VecRole::kNetRemoval) != 0u;
    }
    bool producer_ok = false;
    for (const DROp &f : flow.ops) {
      if (f.kind == DROpKind::kFrontierFilter &&
          f.table_op_table == op.demand_table && f.table_op_sign == -1) {
        producer_ok = true;
        break;
      }
    }
    if (!vec_ok || !producer_ok) {
      ValidatorFail("V-INST-DRAIN: a death's demand net-removals frontier was "
                    "never provisioned (no DR vec / no - frontier-filter "
                    "producer) — G-DEMAND-NEG");
    }
  }
}
```

(Exact `flow.table_vecs` value type: whatever `MintTableVec`'s
`flow.table_vecs[table][role]` map is — `.count(role)` per :605-611 usage.)

### 3.3 Strengthening A — the G-8 source-aware death drain (V-INST-EFFECT)

The death arm (Rel.cpp:4347-4369) counts drains but does not source-check
(d3a1-substrate §2.2's noted d3 opportunity). Mirror the instantiate's
source-aware drain (:4281-4293) inside the death's `case EffKind::kVecDrain:`:

```cpp
case EffKind::kVecDrain:
  ++drains;
  if (fx.vec_role != VecRole::kNetRemoval ||
      fx.value_table != op.demand_table) {
    ValidatorFail("V-INST-EFFECT: a death kVecDrain is not a net-removals "
                  "drain of the demand frontier");
  }
  break;
```

### 3.4 Strengthening B — V-INST-DEATH-COHERENCE at the lowering (§1.2)

Graph-side V-INST-PAIR checks sid multiplicity only; the lowering-side
always-on table-identity check (§1.2) closes the drifted-mint hole at the
single point where the drain vector is actually chosen.

---

## §4 (iv) M2 — THE DEATH EMITTER (band-(a0); RecycleCurrent's FIRST codegen caller)

### 4.1 The emission (Database.cpp, EmitSubgraphInstance — insert between the
`emit_instance_rescan` lambda definition (:2350) and the band-(a1) block (:2352))

```cpp
  // band-(a0) DEATH [D3.a.1]: drain the NETTED demand net-removals frontier
  // (rows == the instance key columns, the a1 mold). RecycleCurrent = Touch +
  // current.Reset (InstanceStore.h:216-219, its FIRST codegen caller):
  //  - Touch sets TouchedFlag => the dead key's a1/a2 rescans SKIP this epoch
  //    (the OD-15 suppression) and Seal visits the iid;
  //  - current stays EMPTY => band-(b)'s drop scan retracts the key's WHOLE
  //    frozen set (dropped = frozen \ current = frozen) — the full (T,F)
  //    retract into pub's delete side rides the generic drop scan (b3);
  //  - V-INST-FRESH unchanged: nothing here fills current (OD-15 coupling).
  // FindInstance is NON-adding; kNoInstance SKIPS (see design §4.2).
  // Netting kills same-batch demand flap upstream (handler NETBATCH + the
  // commit-band was!=now filter), so a removal row is a genuine standing-
  // demand death.
  if (auto removal = region.RemovalFrontier(); removal) {
    const auto death_arity =
        static_cast<unsigned>(removal->ColumnTypes().size());
    std::vector<std::string> dbinds;
    for (unsigned i = 0u; i < death_arity; ++i) {
      dbinds.push_back("d" + std::to_string(i));
    }
    cc << cc.Indent() << "for (const auto &[" << JoinExprs(dbinds, ", ")
       << "] : " << VecName(*removal) << ") {\n";
    cc.PushIndent();
    cc << cc.Indent() << "const auto iid = " << sname << ".FindInstance(Key_"
       << id << "{" << JoinExprs(dbinds, ", ") << "});\n";
    cc << cc.Indent() << "if (iid != ::hyde::rt::kNoInstance) {\n";
    cc.PushIndent();
    cc << cc.Indent() << sname << ".RecycleCurrent(iid);\n";
    cc.PopIndent();
    cc << cc.Indent() << "}\n";  // found
    cc.PopIndent();
    cc << cc.Indent() << "}\n";  // band-(a0) death drain
  }
```

No new ids, no new emitter locals beyond the lambda-scope strings. The
frontier's element shape is the full demand-table column tuple (§2.1), which
IS the instance key (the fabricated demand relation's columns are exactly the
bound params) — the same identity band-(a1) already assumes
(Database.cpp:2352-2358 "rows == the instance key columns"). Both vectors are
shaped by the one `TableDeltaVector` authority on the same table, so their
arities cannot diverge from each other.

### 4.2 D-5 grounds: kNoInstance ⇒ SKIP (not abort)

A net-removed demand key always has an instance at tip semantics: presence in
kNetRemovals ⇒ the row WAS present (commit-band was!=now) ⇒ its net-ADDITION
crossed in some earlier epoch ⇒ band-(a1) FindOrAddInstance minted the iid
(iid namespace is append-only, no tombstones — OD-15). Same-batch add+retract
annihilates upstream (NETBATCH) or nets to no-crossing (frontier filter). So
kNoInstance is unreachable TODAY — but the skip is chosen over an always-on
abort because (i) it is the exact HP-5 mold band-(a2) already pins for a
non-adding probe (:2394-2397), (ii) death is idempotent by contract
(RecycleCurrent comment :210-215) and a skip preserves idempotence under any
future frontier reordering (D3.a.2's input-triggered recycles interleave with
demand deaths), and (iii) a wrong skip is not silent-wrong: the RAT-7
partition belt (b3) + the eqgate flat==nested oracle both trip on any
divergence a phantom-death/missed-death could cause. Adjudicator note: if the
fleet prefers positive-space teeth here, the alternative is an always-on
generated abort in the kNoInstance arm (V-INST-FRESH mold); I recommend
against it for reason (ii).

### 4.3 Ordering proof obligations (all discharged by construction)

- Death before a1/a2 for the store: textual band order inside ONE region
  (D-1); DR-side the OD-2 sign sort (Rel.cpp:5103-5109 fall-through + :5117
  sign line) + V-INST-ORDER (:4758-4786, unit-death-tested) pin the model
  order; the lowering realizes the 3-op protocol in one region, so no
  optimizer can interleave.
- Death's Touch vs V-INST-FRESH: Recycle leaves current empty; the a1/a2
  rescan (where the WorkingOccupied abort lives, :2314-2320) never runs for
  the dead key (TouchedFlag set). Other keys are untouched by the death band.
  V-INST-FRESH text/semantics UNCHANGED — the pinned OD-15 three-way coupling
  stated here verbatim: netting kills same-batch flap; TouchedFlag suppresses
  dead-key a2; V-INST-FRESH keeps its abort because Recycle leaves current
  empty.
- Death vs band-(b)/Seal: the dead iid is in `touched` (Touch), so band-(b)
  visits it (drop scan fires, born scan finds nothing) and Seal swaps in the
  empty current ⇒ frozen empties, SealedOccupied ⇒ false (the unit-pinned
  DeathHalfRecycleThenPartialReaddDropsRows shape, InstanceStoreTest.cpp:
  317-349). Rebirth in a later epoch is ordinary a1 (FindOrAdd re-binds the
  same iid; frontier vectors are per-epoch-fresh primary-proc locals, §1.3).
```

---

## §5 (v) THE kInstanceDeath RENDER ARM — AUDIT + E-71 NOTES

### 5.1 `.rel` (lib/Rel/Format.cpp) — NO EDIT NEEDED; zero new token spellings

The arm exists and is complete (Format.cpp:883-893, verified): header
`sign= ctx= stratum= i#N`, then `emit_reads` (prints NOTHING — DeathEffects
carries no kFlagRead; the empty-elision behavior is the kCommitSweep
precedent in the live witness dump), then `emit_effects`, then
`args: demand=<t> pub=<t> store=I#N`. Every terminal it can produce already
renders live elsewhere: kind name `kInstanceDeath` (Format.cpp:114) appears in
EVERY pinned census line today as `kInstanceDeath=0`; `sign=-` renders on
ingest folds/claim drains; effect spellings `kVecDrain`/`kNetRemoval`
(Format.cpp:69)/`kInstanceDemand`/`kStateOld`/`kInstanceRebuild` and the
`demand=`/`pub=`/`store=I#` args all render on instantiate/frontier-filter
blocks. **E-71 grammar note owed: NOT for a new spelling but for the FIRST
LIVE PRODUCTION of the kInstanceDeath block** — one line in the grammar
record stating the block's sublines (header, effects, args; no reads:, no
spine:) are now witness-exercised. The stale `// R-DIFF only; never rendered
at D1.b/D2.b` comment (:883) is updated (comment-only).

Predicted death block on the witness nested `.rel` (table ids per the tip
dump, SUBJECT to the b1-flip table-set hazard, §7.2; mint order gives the
death the op index right after its instantiate, render order per OD-2 puts it
BEFORE the instantiate):

```
op.1 kInstanceDeath sign=- ctx=seed stratum=<S> i#0
    effects: {kVecDrain(%table:8, kNetRemoval), kInstanceDemand(%table:8), kStateOld(%table:4), kInstanceRebuild(%table:4, -)}
    args: demand=%table:8 pub=%table:4 store=I#0
```

(`<S>` = `instance_stratum[0]` = the same value the instantiate renders —
both kinds resolve the one map entry, Rel.cpp:4734-4746; its numeric value
shifts with the d1 drain-strata lift and is a stage-(d) observable.)

### 5.2 `.ir` (lib/ControlFlow/Format.cpp:659-672) — one new only-when-present token

The `subgraph-instance` line renders the two frontiers today. Add, in band
order (before `demand`), only when present:

```cpp
if (auto removal = region.RemovalFrontier(); removal) {
  os << " death " << *removal;
}
```

giving `subgraph-instance i#0 death %vec:N demand %vec:M input ... seal`.
Grammar note owed in the ir-dump format record (t2-dump-spec.md /
ir-dump-formats — same E-71 discipline): token `death <vec>`, only-when-
present, position after `i#N`. No pinned `.ir` golden carries a
subgraph-instance line (demand_tc_witness/symrec_tie_1 are flat), so this is
[BYTE] on all pinned `.ir` surfaces.

---

## §6 EDIT-SPEC INVENTORY (b2 sub-diff, exact files/anchors at tip 95251825)

| # | file : anchor | edit |
|---|---|---|
| E1 | lib/ControlFlow/Program.h:1174-1186 | + `UseRef<VECTOR> removal_frontier;` member (after input_frontier) + class doc line |
| E2 | include/drlojekyll/ControlFlow/Program.h:861 | + `std::optional<DataVector> RemovalFrontier(void) const noexcept;` |
| E3 | lib/ControlFlow/Program.cpp (beside DemandFrontier/InputFrontier impls) | + RemovalFrontier impl (nullptr → nullopt) |
| E4 | lib/ControlFlow/Build/Procedure.cpp:265-267 | + `death_by_sid` map build via `OpsOfKind(kInstanceDeath)` (§1.1) |
| E5 | lib/ControlFlow/Build/Procedure.cpp:301 (after input_frontier Emplace) | + death wiring block: V-INST-DEATH-COHERENCE abort + removal_frontier Emplace (memoized `TableDeltaVector(demand, kNetRemovals)`) + `{sid, kInstanceDeath}` enrollment (§1.2); update the :340-341 comment |
| E6 | lib/CodeGen/CPlusPlus/Database.cpp:2350/2352 seam | + band-(a0) death drain emission (§4.1); update the :2260-2289 region doc comment to name band-(a0) |
| E7 | lib/Rel/Rel.cpp:4496-4524 | V-INST-DRAIN regime split (§3.1) + call `CheckInstanceDeathFrontier` |
| E8 | lib/Rel/Rel.cpp (new fn near CheckInstanceOrder :4758) + lib/Rel/Rel.h (~:1214, beside the CheckInstanceOrder decl) | + `CheckInstanceDeathFrontier(const DRFlowGraph &)` pure core (§3.2) |
| E9 | lib/Rel/Rel.cpp:4353 (death kVecDrain case) | + G-8 source check (§3.3) |
| E10 | lib/Rel/Format.cpp:883 | comment-only: retire "never rendered" |
| E11 | lib/ControlFlow/Format.cpp:661 | + only-when-present `death <vec>` token (§5.2) |
| E12 | tests/RelValidators/ + CMakeLists | + `DeathFrontierTest.cpp` (fork/waitpid, InstanceOrderTest mold): death arm = flow with kInstanceDeath + NO kNetRemoval vec/producer → SIGABRT; positive arm = vec entry + `-` kFrontierFilter present → exit 0 (pointer-identity fake TABLE*, no deref — the pure core guarantees this is safe) |
| E13 | grammar records | E-71 note (.rel first-production, §5.1) + ir-dump note (`death` token, §5.2) |

Not in b2 (explicit non-edits): Rel.cpp:1139 mint gate (UNCHANGED per d2);
Rel.cpp:861-884 DeathEffects (UNCHANGED — the effect model is already exactly
what §4 emits); OD-2 sort/V-INST-ORDER/V-INST-PAIR/census exp_death
(:3975-3997) all UNCHANGED (they were built for this and go live as-is);
Operation.cpp Hash/Equals (§1.3); Build.cpp:999 gate (stays — differential
tables must NOT get boundary appends, comment :986-987).

---

## §7 PREDICTIONS + GATES

### 7.1 [BYTE] surfaces (grounds: the §1.4 id contract + gating chain)

- ALL 174 non-witness cases × 4 modes (`.stdout`), all pinned `.rel`/`.irgold`/
  `.df`/`.ir`/`.h` goldens incl. demand_tc_witness (its `.drflags` stays bare
  `-demand`; b4 must NOT add the retract flag there) and all `data/` compiles:
  byte-identical. Chain: no `-demand-instance` ⇒ no instance op ⇒ E4/E5/E6/E7
  new arms all no-op; no b1 toggle ⇒ demand monotone ⇒ P-DEATH false ⇒ no
  death op even on the nested witness arm; V-INST-DRAIN's monotone arm is a
  pure refactor of the existing checks (same predicates, same abort strings
  for the two existing legs).
- The nested witness arm WITHOUT b1's toggle (i.e., if eqgate ran mid-stack):
  byte-identical too — this is the designed intermediate-commit safety even
  though the slice lands as one commit.
- The census lines of the eleven pinned `.rel` goldens keep `kInstanceDeath=0`
  (no new op minted anywhere flag-off).

### 7.2 [STRUCT] surfaces (witness-only, WITH b1's toggle + b4's .drflags/batches)

- Nested witness `.rel`: + the §5.1 death block; instantiate effects flip to
  the diff arm (12 effects — b1's flip, V-INST-EFFECT's expected regime);
  census line prediction, CONDITIONAL on the table set staying {4,8,11} and
  the eager web shape surviving the d1 flip (the flip may change DataFlow
  persistence choices — a b1-owned hazard the stage-(d) prototype must pin):

```
census: kCrossover=0 kProductArm=0 kSeedFold=0 kFixpointFire=0 kChainFold=0 kClaimDrain=4 kRetire=0 kRederive=0 kFrontierFilter=4 kCommitSweep=3 kNegateGate=0 kPivotAssemble=0 kIngestFold=3 kGroupUpdate=0 kStateSeal=0 kSubgraphInstantiate=1 kInstanceDeath=1 kInstanceSeal=1 kEagerForward=2 kEagerInsert=0 kEagerCompare=0 kEagerGenerate=0 kEagerUnion=0 kEagerSelect=0 kEagerJoin=0 kEagerProduct=0 kIngestLoop=0 kJoinEmit=0 kProductEmit=0
```

  (deltas vs the tip dump taken this session: kClaimDrain 0→4 + kFrontierFilter
  0→4 = 2 per acyclic differential table × {demand, pub} (negate_1 mold);
  kCommitSweep 2→3 (pub gains a differential sweep); kIngestFold 2→3 (the
  demand receive's stage-1 pair replaces its monotone fold);
  **kInstanceDeath 0→1 — the b2-owned token, UNCONDITIONAL given the toggle**.)
- Nested witness generated `datalog.h`: + the band-(a0) drain (§4.1 text), +
  b3's band-(b) delete side, + the `, false` store ctor (d5, automatic via
  co-activation). Answer identity gated by the LIVE eqgate ×4 (the standing
  oracle) — the witness stdout golden re-bless is b4's (flat arm churns too).
- deps: section gains the frontier-filter→death RAW edge (+ WAR/WAW epoch
  edges vs the seal) — shape-predicted, exact lines stage-(d).

### 7.3 Gate families

SUITE PASS(175) × 4 modes, debug + release + ASAN both surfaces; eqgate ×4
LIVE (flat==nested==golden, now incl. retract/rebirth probes — b4); ctest
(now +1: DeathFrontierTest) debug + ASAN; pinned-golden byte-grep (7.1); the
config-invariance single-hash on demand_tc_witness; permcheck N/A (no
published-delta reordering designed).

### 7.4 Liveness-by-perturbation obligations CREATED by b2

- **L1 (landed unit): V-INST-DRAIN death clause** — DeathFrontierTest (E12)
  death-tests the pure core both ways. This is the permanent negative-space
  proof; no env-gated scaffolding.
- **L2 (stage-(d) checkpoint, recorded not landed): V-INST-EMITTED death leg**
  — in the prototype worktree, build d1 WITHOUT E4/E5 and compile the
  witness: the FIRST abort must be V-INST-EMITTED (Procedure.cpp:456-462)
  with 3 enrolled vs 2 emitted (§2.4's reordered XC-3 chain). Observing it
  proves the enrolled-side kInstanceDeath leg (:447-449) has teeth. Then
  restore E4/E5.
- **L3 (stage-(d) checkpoint): V-INST-DRAIN differential-arm producer check**
  — temporary worktree perturbation: suppress the demand table's `-`
  frontier-filter mint (comment out one mint_filter call) and observe the
  §3.2 abort; revert. (The landed unit L1 covers the same predicate at the
  pure-core level; this checkpoint proves the wiring from ValidateDROps.)
- **L4 (runtime, b4-owned but b2-specified): the kill probe** — after a
  retract batch, the witness driver must observe ZERO rows for the dead key
  (flat AND nested), then a rebirth batch re-probes non-empty. This is the
  death band's end-to-end liveness; the a2-suppression coupling is separately
  visible as byte-level absence of re-publish (belt counters green).
- Inherited-not-created: V-INST-DIFF-COHERENCE + OWN-3 + partition-belt
  perturbation with a TRUE bit (d7) — b3/b4 per the ledger; V-INST-ORDER
  liveness already landed (InstanceOrderTest).

---

## §8 SIBLING-INTERFACE EXPECTATIONS (for the adjudicator)

- **FROM b1 (toggle/writer/netting):** (i) a gate — working name
  `-demand-retract` (per d3a-ruling-brief SUB-SLICE ORDER) — under which the
  fabricated demand message carries `differential_attribute`; OFF ⇒
  bit-for-bit tip everywhere (my 7.1 [BYTE] predictions assume this); (ii)
  the injector del_vec writer + handler NETBATCH engagement (the death
  trigger's netting premise, §2.3/§4.2); (iii) b1 owns the d1-flip blast
  radius on the witness (table set / persistence / eager-web changes) that my
  7.2 census prediction is conditioned on. b1 must NOT add any frontier
  provisioning — D-3 needs none.
- **FROM b3 (band):** (i) the (T,F) drop scan must run per touched iid BEFORE
  the born scan (OQ-PUBLISH-ORDER) and handle the empty-current dead key as
  the full-drop degenerate case (D-4 relies on it); (ii) the partition belt
  must accept the death shape (born=0, carried=0, dropped=frz.NumRows); (iii)
  the pub DelQueue/AddQueue VECTORs should be fetched in LowerSubgraphInstances
  via memoized `TableDeltaVector(pub, kDeleteQueue/kAddQueue)` — same
  post-validator route as my removal frontier (pub's sextet + claim drains
  exist under co-activation; NO V-INST-DRAIN clause needed for them,
  V-INST-EFFECT already counts the appends); (iv) whichever of b2/b3 lands
  first adds `ProgramSubgraphInstanceRegion::IsDifferential()` (G-12) — b2
  does NOT need it (D-2), so I have not claimed it; if b3 wants it, spec:
  `bool IsDifferential(void) const noexcept;` reading the :1189 impl bit.
- **FROM b4 (witness/gates):** (i) `.drflags` gains the b1 flag for
  demand_neighborhood_witness ONLY (never demand_tc_witness); (ii) retract
  batches + the L4 kill/rebirth probes; (iii) the census-line and death-block
  checks of §5.1/7.2 as review gates (a nested `.rel` pin is b4's call — none
  exists today, so my predictions are run-observables, not golden diffs);
  (iv) G-15 witness constraint: any published output over the demanded
  closure must be `@differential` (Differential.cpp:174-179) — the witness
  publishes none today; keep it that way or mark the transmit.
- **Shared with adjudicator:** if b1 chooses a different toggle spelling or
  scope (e.g. always-on under `-demand-instance` instead of a flag), 7.1's
  [BYTE] set must be re-derived — demand_tc_witness (flat, pinned `.rel`/
  `.ir`/`.h`/`.df` goldens) flips wholesale under any toggle that reaches
  plain `-demand`. My design is toggle-shape-agnostic; the gating chain only
  needs "OFF on every pinned surface except the witness".

