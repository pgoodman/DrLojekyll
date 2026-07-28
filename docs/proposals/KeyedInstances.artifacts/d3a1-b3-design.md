# D3.a.1 DESIGN LANE b3 — THE BAND: (T,F) DROP SCAN + RAT-7 PARTITION BELT + SELECTOR LIVE

> Lane b3, stage (b), tip **95251825** (verified `git rev-parse HEAD`, tree clean).
> Binding context honored, not re-litigated: d3a1-substrate.md (§5 gaps, §6 facts,
> §7 d2 ruling: CO-ACTIVATION, predicates NOT folded, XC-3 one-commit unit),
> d3a-ruling-brief.md (OQ-BELT/OQ-PUBLISH-ORDER/OQ-DEATH-VS-REBUILD),
> d3a-substrate.md §7:888-1001 (d4/d5/d7), d3a0-design.md §3 (inheritance).
> Every code claim re-verified at tip by direct read. Scope: sub-diff b3 =
> d4 (drop scan + belt) + d5 (selector live) + the G-12 accessor + the d7
> liveness plan. b1 owns toggle/writer/netting; b2 owns death mint→emitter +
> frontier + validators; b4 owns witness/fences/gates.

## §0 HEADLINE DECISIONS

1. **The (T,F) drop scan** is a second per-iid loop in band-(b) over `frz`,
   gate `cur.Find(drow) == kNoRow`, emitted BEFORE the born scan
   (OQ-PUBLISH-ORDER OVERDELETE-first). Delete side = the GROUP_UPDATE fold
   shape verbatim (Database.cpp:2251-2254 mold): `pub.SubDerivation(row,
   DerivClass::kNonRecursive)` + `VecName(DelQueue).Add(row)`. No index
   removal (SubDerivation removes no index entry — Database.cpp:2190-2192
   comment; readers gate on membership predicates; compaction rebuilds).
2. **The born scan flips APIs under the differential regime**: `DiffTable` has
   NO `TryAdd` (Table.h:255 is monotone-`Table`-only; DiffTable:329ff exposes
   only counter folds), so the differential born arm is the
   `emit_add_deriv` mold (Database.cpp:2204-2219): `AddDerivation(row,
   kNonRecursive)` + `added_row`-gated `EmitIndexAdds` + `AddQueue.Add(row)`.
   The monotone arm keeps `TryAdd` BYTE-IDENTICAL.
3. **The RAT-7 partition belt (`V-INST-PARTITION`)** is three `uint32_t`
   counters (`born`/`carried`/`dropped`) in the emitted differential band,
   checked per touched iid after both scans: `born + carried == cur.NumRows()
   && dropped + carried == frz.NumRows()`; on violation
   `std::fprintf(stderr, ...); std::abort();` in GENERATED code (the
   V-INST-FRESH idiom, Database.cpp:2316-2318) — survives NDEBUG. Emitted in
   the DIFFERENTIAL arm only (OQ-BELT: "lands WITH the (T,F) scan, the
   differential-mode invariant"); HP-7 stays the monotone stores' [DBG] belt
   (InstanceStore.h:177-194) with NO NDEBUG promise.
4. **Death needs NO special band arm**: b2's death (RecycleCurrent = Touch +
   current Reset, InstanceStore.h:216-219) leaves `cur` empty and the iid
   touched; the SAME drop scan then emits the full (T,F) retract
   (dropped == frz.NumRows, born == carried == 0 — the belt's death row).
   INTERFACE: b2's death emission must NOT itself retract pub rows.
5. **The G-12 accessor**: `ProgramSubgraphInstanceRegion::IsDifferential()`
   (public include/.../ControlFlow/Program.h + Program.cpp), reading the
   D3.a.0 write-only impl bit (lib/ControlFlow/Program.h:1188-1190). Plus TWO
   new region members `del_queue`/`add_queue` (UseRef<VECTOR>, Emplaced in
   LowerSubgraphInstances from the MEMOIZED `TableDeltaVector(pub,
   kDeleteQueue/kAddQueue)` — the same VECTOR objects pub's claim drains use)
   with public `DelQueue()`/`AddQueue()`, Emplaced ONLY when
   `inst.differential`. Hash/Equals UNTOUCHED (Operation.cpp:512-543 keys
   {op, store_id, pub_table}; the queues are a memoized pure function of
   pub_table — no new distinguishing state).
6. **d5 selector**: zero code change — the D3.a.0 emitter arm
   (Database.cpp:1460-1466) goes live when the descriptor bit
   (`desc.differential = inst.differential`, Stratum.cpp:2338) turns true
   under co-activation. Witness ctor line: `instance_0(allocator_)` →
   `instance_0(allocator_, false)`. HP-7 off for that store
   (InstanceStore.h:185 monotone gate); V-INST-FRESH unchanged (always-on,
   generated, occupancy-based — Recycle leaves current empty).
7. **IR observability**: the SUBGRAPHINSTANCE `.ir` render
   (lib/ControlFlow/Format.cpp:659-672) gains a DIFFERENTIAL-GATED suffix
   `diff-publish -> <delq> / <addq>` before ` seal` (the GROUP_UPDATE
   `emit-touched one-net-pair -> del / add` mold at :653-654). Monotone
   renders byte-identical. New dump-token spelling ⇒ E-71 grammar note owed.
8. **Named residual (adjudicator + b2)**: pub's claim drains/frontier filters
   mint per differential table (Rel.cpp:2540-2603) and LOWER in step 1
   (Stratum.cpp strata), BEFORE the band (step 2, Procedure.cpp:432). The
   band's queue appends are therefore RUNTIME-INERT this slice: the delta
   vectors are per-call proc locals, so the appends die at proc return and
   the next epoch's drains see fresh empty queues — publish correctness
   rides the commit sweep (counters/flags, Table.h Commit, emitted AFTER the
   band at Procedure.cpp:433). Emitted anyway: the ruling directs the fold
   shape, InstantiateEffects (Rel.cpp:829-848) declares appends==2 and
   V-INST-EFFECT expects them. Any FUTURE slice giving pub an in-flow
   downstream consumer must re-derive the band's schedule position (the DR
   graph orders drain-after-instantiate via the append defs; the emission
   places drains first — an F17-family divergence trap if ever consumed).

## §1 STATE AT TIP (verified anchors this lane relies on)

- `EmitSubgraphInstance` Database.cpp:2290-2465. Band-(b) :2409-2458: born-only
  monotone publish; `pub_exprs` hardcodes row var `row.c<r>` (:2420-2426);
  `(void) key;` :2433; `frz.Find(row) == kNoRow` born gate :2439; TryAdd arm
  :2441-2452 (`ins<N>` handle, `EmitIndexAdds(pub, ins, pub_exprs)` :2449);
  `Seal()` :2461; `#ifndef NDEBUG DebugValidate()` :2462-2464.
- GROUP_UPDATE producer mold: delete side :2251-2254 (`SubDerivation` +
  `VecName(region.DelQueue()).Add(...)`, ENQUEUE UNCONDITIONAL — dequeue
  re-tests are the dedup, Table.h TryClaimDel:465ff/TryClaimAdd:497ff);
  add side `emit_add_deriv` :2204-2219 (`gd<N>` handle from `next_ins_id`,
  `.added_row` gate, `EmitIndexAdds(agg, d, ...)`; queue append after, outside
  the gate, :2256-2257/:2264-2265). `DerivClass::kNonRecursive` throughout
  (the band is acyclic machinery).
- `RowExpr` = braced `{a, b}` (Database.cpp:353-363); `Insert{added,id}`
  (Table.h:247-250) vs `Delta{crossed, added_row, id}` (Table.h:331-335) —
  both carry `.id`, so `EmitIndexAdds` works for either handle; only the
  call-site gate spelling differs (`.added` vs `.added_row`).
- Region impl: 3-arg ctor with `const bool differential` :1162-1165/:1188-1190
  (lib/ControlFlow/Program.h), NO public accessor (include Program.h:848-880:
  StoreId/frontiers/tables/positions only), NO queue members (contrast
  GROUPUPDATE `del_queue`/`add_queue` :1120-1121). Hash :512-520 / Equals
  :527-543 (Operation.cpp) key {op, store_id, pub_table} only.
- Lowering: LowerSubgraphInstances Procedure.cpp:265-347 —
  V-INST-DIFF-COHERENCE :274-285; memoized `TableDeltaVector` fetches
  :287-294; 3-arg region ctor :296-298; enrollment :341-345. GROUP_UPDATE
  queue mold: Stratum.cpp:1388-1391 (`TableDeltaVector(agg_table,
  kDeleteQueue/kAddQueue)`) + :1414-1415 (Emplace).
- Descriptor: `desc.differential = inst.differential` Stratum.cpp:2338
  ("same DRInstance field as region"); emitter arm Database.cpp:1460-1466
  (`instance_<id>(allocator_` + `, false` iff `store.IsDifferential()`).
- Store: ctor flag InstanceStore.h:66 (gates ONLY the HP-7 belt :177-194,
  [DBG] + monotone-gated); `Current/Frozen/KeyAt/Touched/TouchedFlag`
  :130-169/:144-151; `RecycleCurrent` :216-219 (Touch + Reset, idempotent);
  Seal :174-208 (swap + `sealed_occupied = NumRows()>0` + flag reset).
- Band-(a2) gate `iid != kNoInstance && !TouchedFlag(iid)` Database.cpp:2397;
  band-(a1) `!TouchedFlag` :2365; V-INST-FRESH :2314-2320 [ALWAYS-ON].
- Runtime semantics for the coupling argument: `NetDeleted = kDel && !kAdd`,
  `NetAdded = kAdd && !kDel && !kInI` (Table.h:432-462) — COMMIT-relative and
  mutually exclusive per row, so a demand key is in AT MOST ONE of the demand
  table's net frontiers per epoch. `Commit` publishes `was != now` per touched
  row (Table.h:551ff).
- Witness nested arm regenerated at tip (scratchpad gen-nested/datalog.h,
  compiler `-demand -demand-instance`): flow proc `flow_46`; pub =
  `neighborhood_4` (`Table<Row4>`) + index `idx_45` (pub_has_indexes TRUE);
  demand table `table_8`, input `table_11`; a1 drains `vec29`, a2 `vec25`;
  band-(b) publishes `neighborhood_4.TryAdd({key.c0, row.c0})` under `ins2`;
  store ctor `instance_0(allocator_)`. The witness has NO published transmit
  (query-only sink) ⇒ no G-15 faithfulness constraint on the CURRENT .dr;
  retraction observability = the query surface + the eqgate answer identity.

## §2 EDIT SPECS

### E1 — public accessor + queue getters (include/drlojekyll/ControlFlow/Program.h:848-880)

Insert after `PubTable()` (:865):

```c++
  // D3.a.1: the store/pub differential regime — == DRInstance.differential ==
  // TableIsDifferential(pub) (V-INST-DIFF-COHERENCE-checked at lowering).
  // Gates the band-(b) (T,F) drop scan, the signed publish, and the
  // V-INST-PARTITION belt.
  bool IsDifferential(void) const noexcept;

  // Pub's delete/add queues (the band-(b) signed publish appends into them —
  // the GROUP_UPDATE DelQueue/AddQueue peer). Valid ONLY when
  // IsDifferential(); null refs for a monotone store.
  DataVector DelQueue(void) const noexcept;
  DataVector AddQueue(void) const noexcept;
```

### E2 — accessor definitions (lib/ControlFlow/Program.cpp, after :767 PubTable)

```c++
bool ProgramSubgraphInstanceRegion::IsDifferential(void) const noexcept {
  return impl->differential;
}
DataVector ProgramSubgraphInstanceRegion::DelQueue(void) const noexcept {
  return DataVector(impl->del_queue.get());
}
DataVector ProgramSubgraphInstanceRegion::AddQueue(void) const noexcept {
  return DataVector(impl->add_queue.get());
}
```

### E3 — region impl members (lib/ControlFlow/Program.h:1174-1186 area)

After `pub_table` (:1177):

```c++
  // D3.a.1 differential regime only (null under the monotone regime): pub's
  // delete/add queues — the band-(b) signed publish appends; the SAME memoized
  // TableDeltaVector objects pub's claim drains consume (per-epoch proc
  // locals, so the band's post-drain appends are inert this slice — the
  // commit sweep is the publish channel; see the b3 §0.8 residual).
  UseRef<VECTOR> del_queue;
  UseRef<VECTOR> add_queue;
```

Comment on :1189-1190 updates: `read by EmitSubgraphInstance in D3.a.1.
FALSE today.` → `read by EmitSubgraphInstance (the D3.a.1 drop-scan/belt
selector).` Hash/Equals (Operation.cpp:512-543) UNTOUCHED — the queues are a
memoized pure function of `pub_table`, which Equals already keys on; two
Equals-equal regions share pub, hence share the memoized queue VECTORs.

### E4 — LowerSubgraphInstances (lib/ControlFlow/Build/Procedure.cpp:296-311)

After the frontier Emplaces (:300-301), add (mirror Stratum.cpp:1388-1391 +
:1414-1415):

```c++
    // D3.a.1: under the differential regime the band publishes SIGNED deltas
    // into pub's own machinery (OQ-PUBLISH-ORDER) — fetch pub's memoized
    // delete/add queues. Monotone stores keep null refs (TryAdd publish).
    if (inst.differential) {
      si->del_queue.Emplace(
          si, TableDeltaVector(impl, context, op->table_op_table,
                               VectorKind::kDeleteQueue));
      si->add_queue.Emplace(
          si, TableDeltaVector(impl, context, op->table_op_table,
                               VectorKind::kAddQueue));
    }
```

Note: when `inst.differential`, pub is differential (co-activation), so the
(pub, kDeleteQueue/kAddQueue) VECTORs were ALREADY minted by the stratum
lowering (memoization returns the same objects — vector-decl set unchanged);
the region sits in the pre-split flow proc, so ExtractPrimaryProcedure threads
them exactly as it threads the frontiers (:263 comment). Under the monotone
regime the branch is dead ⇒ zero behavior change.

### E5 — EmitSubgraphInstance band-(b) (lib/CodeGen/CPlusPlus/Database.cpp:2409-2458)

(a) Parameterize the pub-expr builder by row var (discharges the G-9
row-var hardcode). Replace :2417-2426 with:

```c++
  const unsigned npub =
      static_cast<unsigned>(key_pos.size() + row_pos.size());
  // Map each pub position to its source expr (KeyAt slot or a store-row var).
  // Row-var-parameterized (D3.a.1): the born scan binds `row` (from cur), the
  // (T,F) drop scan binds `drow` (from frz).
  const auto pub_exprs_for = [&](const char *rv) {
    std::vector<std::string> es(npub);
    for (unsigned r = 0u; r < key_pos.size(); ++r) {
      es[key_pos[r]] = "key.c" + std::to_string(r);
    }
    for (unsigned r = 0u; r < row_pos.size(); ++r) {
      es[row_pos[r]] = std::string(rv) + ".c" + std::to_string(r);
    }
    return es;
  };
  const std::vector<std::string> pub_exprs = pub_exprs_for("row");
```

(`pub_exprs` values are BYTE-IDENTICAL to tip; the monotone arm is untouched.)

(b) The band body. Keep the loop head :2428-2433 verbatim (`Touched()` loop,
`cur`/`frz`/`key`, `(void) key;`). Then:

```c++
  const bool diff = region.IsDifferential();
  if (diff) {
    // RAT-7 / OQ-BELT: the V-INST-PARTITION counters. Always-on in generated
    // code (fprintf+abort, survives NDEBUG) — the differential-mode invariant
    // born+carried==cur && dropped+carried==frz. HP-7 stays the monotone
    // stores' [DBG] Seal belt.
    cc << cc.Indent() << "uint32_t born = 0u;\n";
    cc << cc.Indent() << "uint32_t carried = 0u;\n";
    cc << cc.Indent() << "uint32_t dropped = 0u;\n";
    // The (T,F) DROP SCAN (G-TF-PUBLISH), BEFORE the born scan per touched
    // iid (OQ-PUBLISH-ORDER OVERDELETE-first). dropped = frozen \ current;
    // delete side = the GROUP_UPDATE fold shape (SubDerivation +
    // del-queue append; enqueue unconditional — TryClaimDel re-tests at
    // dequeue). A DEAD key (RecycleCurrent left cur empty) retracts its
    // whole frozen buffer here — death needs no bespoke publish arm.
    const auto dpub = pub_exprs_for("drow");
    cc << cc.Indent() << "for (uint32_t r = 0; r < frz.NumRows(); ++r) {\n";
    cc.PushIndent();
    cc << cc.Indent() << "const auto &drow = frz.RowAt(r);\n";
    cc << cc.Indent() << "if (cur.Find(drow) == ::hyde::rt::kNoRow) {\n";
    cc.PushIndent();
    cc << cc.Indent() << "++dropped;\n";
    cc << cc.Indent() << pub_member << ".SubDerivation(" << RowExpr(dpub)
       << ", ::hyde::rt::DerivClass::kNonRecursive);\n";
    cc << cc.Indent() << VecName(region.DelQueue()) << ".Add(" << RowExpr(dpub)
       << ");\n";
    cc.PopIndent();
    cc << cc.Indent() << "} else {\n";
    cc.PushIndent();
    cc << cc.Indent() << "++carried;\n";
    cc.PopIndent();
    cc << cc.Indent() << "}\n";  // (T,F) gate
    cc.PopIndent();
    cc << cc.Indent() << "}\n";  // rows of frozen
  }
```

(c) The born scan: loop head + `(F,T)` gate lines (:2434-2440) verbatim; the
publish body branches:

```c++
  if (!diff) {
    // ... :2441-2452 verbatim (TryAdd / ins<N> / EmitIndexAdds) ...
  } else {
    cc << cc.Indent() << "++born;\n";
    // DiffTable has no TryAdd: the born side is the emit_add_deriv mold
    // (AddDerivation + added_row-gated index adds + add-queue append).
    if (!pub_has_indexes) {
      cc << cc.Indent() << pub_member << ".AddDerivation(" << RowExpr(pub_exprs)
         << ", ::hyde::rt::DerivClass::kNonRecursive);\n";
    } else {
      const auto d = "gd" + std::to_string(next_ins_id++);
      cc << cc.Indent() << "const auto " << d << " = " << pub_member
         << ".AddDerivation(" << RowExpr(pub_exprs)
         << ", ::hyde::rt::DerivClass::kNonRecursive);\n";
      cc << cc.Indent() << "if (" << d << ".added_row) {\n";
      cc.PushIndent();
      EmitIndexAdds(pub, d, pub_exprs);
      cc.PopIndent();
      cc << cc.Indent() << "}\n";
    }
    cc << cc.Indent() << VecName(region.AddQueue()) << ".Add("
       << RowExpr(pub_exprs) << ");\n";
  }
```

(d) After the born loop closes (:2456), before the iid loop closes (:2457):

```c++
  if (diff) {
    cc << cc.Indent() << "if (born + carried != cur.NumRows() || "
       << "dropped + carried != frz.NumRows()) {\n";
    cc.PushIndent();
    cc << cc.Indent() << "std::fprintf(stderr, \"V-INST-PARTITION: instance "
       << "%u born=%u carried=%u dropped=%u cur=%u frz=%u (store " << id
       << ")\\n\", iid, born, carried, dropped, cur.NumRows(), "
       << "frz.NumRows()); std::abort();\n";
    cc.PopIndent();
    cc << cc.Indent() << "}\n";
  }
```

Then `Seal()`/`DebugValidate()` (:2460-2464) unchanged. Also update the
function's doc comment (:2282-2289) for the two regimes.

Emission-structure proof of monotone byte-identity: the ONLY diff-gated
insertions are (b)'s block (skipped), (c)'s else-arm (not taken; the then-arm
is the tip text verbatim), (d)'s belt (skipped); (a) computes the same
`pub_exprs` strings.

### E6 — the `.ir` render (lib/ControlFlow/Format.cpp:659-672) [dump-token change]

Before the closing `" seal"` (:670):

```c++
  if (region.IsDifferential()) {
    os << "} diff-publish -> " << region.DelQueue() << " / "
       << region.AddQueue() << " seal";
  } else {
    os << "} seal";
  }
```

(GROUP_UPDATE's `emit-touched one-net-pair -> del / add` mold, :653-654.)
Grammar: `subgraph-instance i#<sid> demand <vec> input <vec> rescan <table> ->
publish <table> key@{..} row@{..} [diff-publish -> <delq> / <addq>] seal` —
**E-71 grammar note owed** for the `diff-publish` token. Monotone renders
byte-identical; the null-DataVector print is unreachable (token gated on the
same bit that gated the Emplace, and V-INST-DIFF-COHERENCE pins the bit to
the live predicate).

### E7 — d5, NO EDIT (goes live by co-activation)

The store-ctor arm Database.cpp:1460-1466 + descriptor Stratum.cpp:2338 +
`ProgramInstanceStoreInfo::IsDifferential()` are all landed (D3.a.0) and flip
together off the ONE Rel.cpp:1055/:1059 stamp when b1's d1 toggle makes
`TableIsDifferential(pub)` true (the §6 co-activation chain, ruled §7).
b3 adds nothing; the prediction is §3. HP-7 disarms via the ctor flag
(InstanceStore.h:66/:185, [DBG] — no NDEBUG teeth promised); V-INST-FRESH is
untouched in both arms (the shared rescan mold :2312-2350 is not edited).

## §3 PREDICTED GENERATED-TEXT DELTA — THE WITNESS (the ONE program whose bit flips)

Precondition (b1/b4 interface): the d1 toggle is PER-CASE OPT-IN (the ruling
brief's `-demand-retract` spelling) — an unconditional flip would (a) churn
`demand_tc_witness`'s pinned `.rel`/`.stdout` machinery and (b) HARD-ERROR any
plain-`-demand` case publishing a non-@differential output over the demanded
closure (Differential.cpp:174-179, G-15). With the flag only on
`demand_neighborhood_witness` (b4 adds it to `.drflags`), EXACTLY ONE case
changes text.

Whole-file [STRUCT]: the co-activation flip makes `table_8` (demand) and
`neighborhood_4` (pub) DiffTables in BOTH lowerings — member types, two-
polarity demand ingest folds, handler NETBATCH, claim drains + frontier
filters for both tables, `Commit(...)`-form sweeps replacing `Seal()`,
doubled receive vectors on proc signatures. That is b1/b2-owned FLAT-standard
machinery (predicted by shape, not byte, in their lanes). b3-owned line-level
predictions, against the tip nested emission (regenerated this session,
scratchpad gen-nested/datalog.h):

(1) STORE CTOR (d5): `      instance_0(allocator_) {}` →
    `      instance_0(allocator_, false) {}` — exactly one line, the
    Database.cpp:1461-1464 arm.

(2) BAND-(b) (d4), the `for (const auto iid : instance_0.Touched())` body —
    tip text:

```c++
    for (uint32_t r = 0; r < cur.NumRows(); ++r) {
      const auto &row = cur.RowAt(r);
      if (frz.Find(row) == ::hyde::rt::kNoRow) {
        if (const auto ins2 = neighborhood_4.TryAdd({key.c0, row.c0}); ins2.added) {
          idx_45.Add({key.c0}, ins2.id);
        }
      }
    }
```

    becomes (modulo `gd<N>` numbering from the shared `next_ins_id` stream and
    `vec<N>` ids from the upstream-shifted id stream; loop head + `cur`/`frz`/
    `key`/`(void) key;` lines UNCHANGED above it):

```c++
    uint32_t born = 0u;
    uint32_t carried = 0u;
    uint32_t dropped = 0u;
    for (uint32_t r = 0; r < frz.NumRows(); ++r) {
      const auto &drow = frz.RowAt(r);
      if (cur.Find(drow) == ::hyde::rt::kNoRow) {
        ++dropped;
        neighborhood_4.SubDerivation({key.c0, drow.c0}, ::hyde::rt::DerivClass::kNonRecursive);
        vecD.Add({key.c0, drow.c0});
      } else {
        ++carried;
      }
    }
    for (uint32_t r = 0; r < cur.NumRows(); ++r) {
      const auto &row = cur.RowAt(r);
      if (frz.Find(row) == ::hyde::rt::kNoRow) {
        ++born;
        const auto gdN = neighborhood_4.AddDerivation({key.c0, row.c0}, ::hyde::rt::DerivClass::kNonRecursive);
        if (gdN.added_row) {
          idx_45.Add({key.c0}, gdN.id);
        }
        vecA.Add({key.c0, row.c0});
      }
    }
    if (born + carried != cur.NumRows() || dropped + carried != frz.NumRows()) {
      std::fprintf(stderr, "V-INST-PARTITION: instance %u born=%u carried=%u dropped=%u cur=%u frz=%u (store 0)\n", iid, born, carried, dropped, cur.NumRows(), frz.NumRows()); std::abort();
    }
```

    where `vecD`/`vecA` are pub's kDeleteQueue/kAddQueue proc-local delta
    vectors (declared by the stratum lowering that also emits their claim
    drains; the band adds NO vector declarations).

(3) Bands (a1)/(a2) text UNCHANGED apart from vector renumbering — the input
    `table_11` stays a monotone `Table<Row11>` (its eager boundary append,
    Build.cpp:999-1004 route, is untouched: the :999 gate excludes only
    DIFFERENTIAL tables); the rescan mold + V-INST-FRESH strings are
    byte-identical. The a1 demand frontier drain reads the SAME (table_8,
    kNetAdditions) memoized VECTOR — now filled by table_8's frontier filter
    (the post-netting commit-band product) instead of the monotone boundary
    append (b2's provisioning re-route, XC-3).

(4) The witness's nested `.ir` dump (not a pinned golden): the region line
    gains ` diff-publish -> $vecD:... / $vecA:...` before ` seal`. Its `.rel`
    dump (not pinned either): b2 owns the death-op/frontier render deltas.

BEHAVIORAL note (belt algebra, all three run-modes of one iid-epoch):
birth: frz empty ⇒ dropped=carried=0, born=cur.NumRows ✓. Rebuild (monotone
input growth): frz ⊆ cur ⇒ dropped=0, carried=frz.NumRows, born=cur−carried ✓.
Death: cur empty (Recycle) ⇒ born=carried=0, dropped=frz.NumRows — the full
(T,F) retract ✓. Pub counters stay 0/1 per row on this shape (the pub row
EMBEDS the instance key — key@{0} — so two iids can never publish the same
pub row; the OQ-PUBLISH-ORDER cross-instance netting clause is vacuously
satisfied; the design does NOT rely on 0/1 — AddDerivation/SubDerivation are
multiset-correct if a future shape shares rows). Sub always crosses 1→0 into
the del queue; the commit sweep publishes was=1,now=0. Rebirth in a later
epoch: frz is EMPTY (death-epoch Seal swapped the empty current in), so the
born scan re-publishes everything — 0→1, was=0,now=1 ✓.

## §4 THE a2/TouchedFlag COUPLING (task (iv)) — VERIFIED DELIVERED

The OD-15 pinned three-way coupling, at code, given b2 places the death drain
BEFORE band-(a1) for the store (OD-2 graph order; §4.4 epoch position):

1. Same-batch death+re-demand: killed UPSTREAM — the handler NETBATCH
   (Procedure.cpp:571-575, Vec.h:176-218) annihilates add∩remove, and the
   demand table's net frontiers are COMMIT-relative and mutually exclusive
   per row (`NetDeleted = kDel && !kAdd` vs `NetAdded = kAdd && !kDel &&
   !kInI`, Table.h:432-462) — one key is in AT MOST ONE of
   {kNetAdditions, kNetRemovals} per epoch. Consequence: the §4.3
   interleaving table's WRONG-order row (a1 fills, then death empties a
   filled current) is UNREACHABLE at the frontier level; the order
   discipline (death first) stands as belt-and-suspenders.
2. Dead-key a2 suppression (the leg this lane must verify): death runs
   RecycleCurrent ⇒ `Touch(iid)` sets `touched_flag` (InstanceStore.h:
   216-219, :298-305). Band-(a2)'s emitted gate `iid !=
   ::hyde::rt::kNoInstance && !instance_0.TouchedFlag(iid)`
   (Database.cpp:2397-2398; witness text verbatim) then SKIPS the rescan for
   any same-epoch input edge touching the dead key ⇒ `cur` stays empty ⇒ the
   drop scan (§2 E5) publishes the full (T,F) retract. Band-(a1)'s
   `!TouchedFlag` (:2365) gives the same suppression on the (unreachable)
   add side. The gate is UNTOUCHED by this design — the suppression falls
   out of b2's death placement + the existing emission.
3. V-INST-FRESH unchanged: the guard keys `WorkingOccupied` (current
   non-empty) at band-(a) ENTRY (:2314-2320). Recycle leaves current EMPTY,
   so a death never trips it; a rescan after death is suppressed by (2)
   anyway. In the death epoch, Seal swaps the empty current in ⇒
   `sealed_occupied=false`; the store's occupancy stays exact (OQ-N1 closed,
   full-rescan model).

## §5 GATE FAMILIES + PRE-REGISTERED PREDICTIONS (b3 surfaces; b4 consolidates)

- **[BYTE] — all 175 non-witness cases × 4 modes** (`.stdout`), the eleven
  `.rel` + 14 `.irgold` pinned sidecars, all 20 pinned regen surfaces, all
  `.ir`/`.df` dumps, `data/` compiles. Grounds: every b3 edit is gated on
  `IsDifferential()` / `inst.differential` / `store.IsDifferential()`, which
  is FALSE program-wide off the witness (co-activation + the d1 per-case
  flag; the sole `-demand-instance` carrier today is the witness itself);
  the E5(a) refactor emits identical strings; E6's token is bit-gated;
  Hash/Equals untouched ⇒ CF dedup identical. The witness is in NO pinned
  set (its nested arm is refereed LIVE by run_eqgate; not among the eleven
  `.rel` pins — verified list).
- **[STRUCT] — the witness only**: generated header delta exactly as §3
  (ctor line + band-(b) block b3-owned; closure-wide DiffTable machinery
  b1/b2-owned); `.stdout` golden gains the d6 retract-phase output (b4
  blesses ONCE, flat-refereed); eqgate flat==nested==golden ×4 modes LIVE —
  the standing cross-lowering oracle for the retract batches (ruling-brief
  referee note).
- **SUITE PASS(175)** all 4 modes, debug + release + ASAN both surfaces;
  **ctest 6/6** debug + ASAN (no new unit this lane; the belt lives in
  generated code and is witness-exercised. OPTIONAL hand-off: an
  InstanceStore-level unit is NOT possible — the belt is codegen-side; the
  death-half runtime contract is already unit-pinned at
  InstanceStoreTest.cpp:317-349).
- **Config-invariance single-hash** on `demand_tc_witness` (which stays
  flag-off ⇒ fully [BYTE]).
- **E-71 grammar note** for `diff-publish` (E6) in the errata ledger, same
  commit.
- **Expected reds: NONE** at the landed commit (the slice lands as ONE unit
  per XC-3; intermediate abort states — V-INST-DRAIN then V-INST-EMITTED —
  never exist in a committed tree).

## §6 d7 — LIVENESS-BY-PERTURBATION PLAN (the §20(AF) §3.6 obligation, G-16)

All perturbations are TRANSIENT one-line edits, run at the gate stage,
REVERTED before commit (no env-gated scaffolding — house rule); each records
the observed abort text in the slice ledger entry.

1. **V-INST-DIFF-COHERENCE (Procedure.cpp:278-285), BOTH polarities, now
   reachable with a TRUE bit**: (a) stamp-false/live-true — negate the third
   ctor... rather, flip `inst.differential` at the :278 comparison
   (`!inst.differential !=` …) or stamp `!diff` at Rel.cpp:1059; compile the
   WITNESS (bit true) ⇒ must abort with the :279-283 message. (b)
   stamp-true/live-false — same perturbation, compile a MONOTONE carrier
   (the witness without `-demand-retract`) ⇒ must abort. Confirms the
   validator is a live drift guard in both directions, discharging the
   "vacuous-green" note (§20(AG)).
2. **V-INST-PARTITION (the new belt)**: perturb the EMITTER, not the
   runtime — drop the `++dropped;` line (E5(b)) ⇒ rebuild ⇒ run the witness
   driver's retract batch ⇒ generated abort `V-INST-PARTITION: instance ...
   (store 0)` must fire on the first death epoch (dropped+carried <
   frz.NumRows). Second perturbation (born side): drop `++born;` ⇒ abort on
   the first birth epoch. Depends on b4's d6 retract batches being in place;
   sequence AFTER the witness lands in the working tree, BEFORE commit.
3. **OWN-3 fold abort**: teeth already unit-proven by the D3.a.0 fork/waitpid
   death test (GuardAnnotationFoldTest, ctest); the differential bit does not
   change fold reachability (single adornment ⇒ the both-set arm stays
   corpus-dormant). Discharge = re-run ctest under this slice's tree (both
   build flavors) + record; no new perturbation is meaningful before D3.a.3.
4. **HP-7 disarm sanity (negative)**: with the witness store constructed
   `monotone=false`, a death epoch legitimately shrinks frozen⊇current; a
   debug-build witness run must NOT trip the Seal belt (it is gated off,
   InstanceStore.h:185) while `DebugValidate()` stays green — covered by the
   ordinary debug suite run, called out so a reviewer doesn't mistake the
   silent belt for a miss.

## §7 SIBLING-INTERFACE EXPECTATIONS (for the adjudicator)

- **b1 (toggle/writer/netting)**: (i) the d1 toggle is PER-CASE OPT-IN
  (`-demand-retract` or spelling of record) — b3's [BYTE] prediction for the
  175 and for `demand_tc_witness` RESTS on this; an unconditional flip also
  hits G-15 hard-errors on plain-`-demand` corpus cases. (ii) co-activation
  is b1's d1 + the landed Rel.cpp:1055/:1059 stamp — b3 reads the bit, never
  computes it. (iii) the netting + frontier-exclusivity argument (§4.1) is
  quoted from b1's machinery; if b1 alters the netted-channel shape, §4
  re-derives.
- **b2 (death mint→emitter + frontier + validators)**: (i) death emission
  runs BEFORE band-(a1) for its store and calls RecycleCurrent — the Touch is
  load-bearing for §4.2; (ii) death does NOT retract pub rows itself (the
  drop scan is the ONLY publisher of −1s; a second retract would double-Sub);
  (iii) b2 provisions demand kNetAdditions under the differential route
  (XC-3) — b3's a1 drain text assumes the same memoized VECTOR identity;
  (iv) b2 verifies the new instantiate-op queue-append DEFS (pub kDeleteQueue/
  kAddQueue) clear DeriveDRStrata + LinearizeAndValidateDRFlow without abort
  and are def-enrolled (or deliberately not) in flow.vecs — see the §0.8
  residual; (v) the `.rel` render of death/frontier tokens is b2's, with its
  own E-71 notes.
- **b4 (witness/fences/gates)**: (i) adds the toggle flag to the witness
  `.drflags` + the d6 retract batches + re-probes (drop-then-reprobe per iid;
  DEATH stays oracle-blind); (ii) blesses the ONE `.stdout` golden change
  flat-first (never from the nested arm; eqgate stays live-refereed);
  (iii) no published transmit is REQUIRED for observability (query surface
  suffices — §1 last bullet); if b4 adds one for extra coverage it MUST be
  `@differential` (G-15); (iv) runs §6.2's belt perturbation after the
  batches land; (v) owns the consolidated gate table — §5 pre-registers b3's
  rows.

## §8 WHAT b3 DELIBERATELY DOES NOT DO

- No predicate folding (d2 ruling: the death gate keeps Rel.cpp:1139; the
  store bit keeps :1055/:1059 — extensional equality is a theorem, not an
  invariant).
- No InstanceStore API changes (the full-rescan model needs none; N-1 closed).
- No same-epoch re-position of pub's claim tail (the §0.8 residual is
  RECORDED, not fixed — fixing it is unmotivated while pub is terminal and
  would move bytes on flat differential programs).
- No temporary fence: per the orchestrator's charter the slice lands as ONE
  commit (XC-3 forbids d1-only), and no hard blocker surfaced in this lane.
