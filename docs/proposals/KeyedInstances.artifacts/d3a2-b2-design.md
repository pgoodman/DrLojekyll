# D3.a.2 — LANE b2 DESIGN: the a2' removal arm + the mold's Present conjunct + the band-(b) net-retraction ride

> **House banner.** Tip **b4d08307** (branch keyed-instances; code bytes ==
> the D3.a.1 landing 33cabcf1, every commit atop docs-only — so every anchor
> below is live at the frozen-A binary
> `/private/tmp/.../scratchpad/frozenA/drlojekyll-debug`). BINDING context read
> end-to-end, not re-litigated: d3a2-substrate.md §7 (**R-A2-TRIGGER = TWO
> DRAINS, NO RECYCLE**; gate-set identity; band order a0→a1→a2→a2'; the
> Present rider; the ungated-late-Recycle FORBIDDEN fence), d3a2-substrate.md
> §5 (H-10/H-11/H-12/H-14 = this lane), §4 (E-D/E-E/E-F1/E-F2/E-F3 + OB1-OB8),
> d3a-ruling-brief.md (OQ-INPUT / OQ-MODEL / OQ-DEATH-VS-REBUILD),
> d3a1-substrate.md §7 (d2 CO-ACTIVATION — P-STORE/P-DEATH never folded) + §8
> (e1-e8), d3a1-design.md (the MOLD: edit-spec granularity, §3.2 gate, §6
> prediction tables, §6.4 L-table), ledger §20(AK)/(AL)/(AM). Mold idiom per
> d3a1-design.md; anchors inline; per-surface [BYTE]/[STRUCT] pre-registered.

**LANE b2 IN ONE SENTENCE.** In `EmitSubgraphInstance` (Database.cpp
:2341-2654) — the ONLY code home this lane touches — emit the **band-(a2')**
edge-net-REMOVALS drain (a faithful net-removals clone of band-(a2), NO
RecycleCurrent), add the **`Present(s)` conjunct** to the ONE shared rescan
mold under an input-differential codegen selector, and verify the landed
**band-(b)** drop scan + V-INST-PARTITION belt + Seal + store-ctor + the a2
demand-liveness gate all ride the new regime UNCHANGED — walking E-D/E-F2/E-F3
and the e5 divergence (P-STORE true ∧ P-DEATH false) end-to-end in the
generated-code shape against the flat oracle.

Three edit specs, all in Database.cpp `EmitSubgraphInstance`: **E2a** (hoist
the input-removal frontier + the `input_diff` selector), **E2b** (the mold
Present conjunct), **E2c** (the band-(a2') arm). No new DROp kind, no new dump
spelling, no new validator, no runtime store change. The removal *producer*
(free, C7/ADV-5), the region `InputRemovalFrontier()` accessor + provisioning
fences, and the `InstantiateEffects` removal leg are **b1**'s (cross-lane
contracts §6); the **e4 quiescence lemma** and the gate-comment rider rewrite
are **b3**'s; the **e6 witness** is **b4**'s.

===============================================================================
## §0 THE HOME, READ FULLY (Database.cpp:2341-2654) — WHAT b2 CHANGES
===============================================================================

The landed band, in emission order (verified verbatim at frozen-A):

| block | lines | b2 disposition |
|---|---|---|
| header decls (`diff`, `input_front`, `input`, `pub`, `input_member`, `in_key`, `in_row`) | 2343-2358 | **E2a APPENDS** `input_removal` + `input_diff` after :2350 |
| `emit_instance_rescan` mold (V-INST-FRESH + full scan + key filter + `cur.TryAdd`), **NO presence filter** | 2366-2404 | **E2b GROWS** the `cond` string with `&& <input>.Present(s)` under `input_diff` |
| band-(a0) DEATH drain (`RemovalFrontier`, FindInstance, RecycleCurrent) | 2406-2440 | UNCHANGED (bytes untouched — R-A2-TRIGGER §7(4)) |
| band-(a1) demand-births drain → `emit_instance_rescan(kbinds)` | 2442-2461 | UNCHANGED bytes; its rescan gains the E2b conjunct **only for diff-input programs** |
| band-(a2) edge-net-ADDS drain + R-3 gate fork → `emit_instance_rescan(ekeyexprs)` | 2463-2523 | UNCHANGED bytes; its rescan gains the E2b conjunct for diff-input programs |
| **band-(a2') edge-net-REMOVALS drain** | — | **E2c APPENDS a new block after :2523** |
| band-(b) drop scan + born arm + V-INST-PARTITION belt (gated `diff`=PUB) | 2525-2645 | UNCHANGED; §3 verifies it publishes E-D's net retraction |
| Seal + DebugValidate | 2649-2653 | UNCHANGED |

Two orthogonal codegen selectors after E2a (never folded — the d2 §7
discipline extends to the input axis):
- **`diff`** = `region.IsDifferential()` == `TableIsDifferential(pub)` (P-STORE,
  :2347). Gates band-(b) drop-scan/belt/signed-publish AND the a2/a2'
  demand-liveness gate fork. Already hoisted.
- **`input_diff`** = `region.InputRemovalFrontier().has_value()` (NEW, E2a) ==
  `TableIsDifferential(input_table)` (the ADV-6 THIRD axis). Gates the mold
  Present conjunct AND the *existence* of the band-(a2') arm.

By O-1 (the Differential.cpp closure) **`input_diff ⇒ diff`** on every
accepted program (a differential input forces a differential pub) — verified
empirically below — so within the band-(a2') arm `diff` is always true (F2a).

===============================================================================
## §1 THE EDIT SPECS (E2a, E2b, E2c) — verbatim generator code
===============================================================================

### E2a — hoist the input-removal frontier + the `input_diff` selector

**File** `lib/CodeGen/CPlusPlus/Database.cpp`, `Generator::EmitSubgraphInstance`.
**Insertion point** immediately after the `input` decl (:2350), before
`pub` (:2351) — so the mold lambda (:2366) captures `input_diff` in scope.

```cpp
  const DataTable input = region.InputTable();
  // D3.a.2 [R-A2-TRIGGER]: the input(edge) net-REMOVALS frontier — present IFF
  // the summarized input is DIFFERENTIAL (b1 emplaces input_removal_frontier
  // ONLY under the input-diff regime, mirroring removal_frontier for demand
  // death). Its presence IS the codegen input-diff selector (single source of
  // truth: the E2b mold conjunct and the E2c band-(a2') arm both key on it —
  // definitionally co-gated, F2c). == TableIsDifferential(input_table), the
  // ADV-6 THIRD axis, NEVER folded into `diff`/P-STORE/P-DEATH (d2 §7).
  const auto input_removal = region.InputRemovalFrontier();
  const bool input_diff = input_removal.has_value();
```

`[BYTE]` on every landed (monotone-input) pin: `input_removal` is
`std::nullopt` for all 177 corpus cases → `input_diff` false → E2b/E2c inert.
Anchor for the accessor contract: mirrors `RemovalFrontier` (Program.cpp
:762-768; the `std::optional<DataVector>` idiom). **b1 provides
`InputRemovalFrontier()`** (X-b1-1, §6).

### E2b — the mold Present conjunct (H-10 / ADV-3, RULED `Present`)

**File** Database.cpp, inside the `emit_instance_rescan` lambda, in the
`cond`-building block (currently :2380-2391). **Replace** the block:

```cpp
    std::string cond;
    {
      auto sep = "";
      for (unsigned j = 0u; j < in_key.size(); ++j) {
        cond += sep + std::string("ir.") + input_fields[in_key[j]] +
                " == " + keyexprs[j];
        sep = " && ";
      }
    }
    if (cond.empty()) {
      cond = "true";
    }
```

**with** (adds the `if (input_diff)` conjunct clause; everything else
unchanged, so the monotone-input path is byte-identical):

```cpp
    std::string cond;
    {
      auto sep = "";
      for (unsigned j = 0u; j < in_key.size(); ++j) {
        cond += sep + std::string("ir.") + input_fields[in_key[j]] +
                " == " + keyexprs[j];
        sep = " && ";
      }
      // ADV-3 (RULED spelling = Present): under a DIFFERENTIAL input the
      // physical row log at `input_member` still holds THIS-EPOCH-retracted
      // rows — CompactDead runs only at the epoch-boundary commit-sweep tail
      // past the 4096 floor, NEVER in-band (XC-7; Table.h NeedsCompaction
      // ~:593-608, CompactDead :614), so `RowAt(s)` enumerates dead rows too.
      // `Present(s)` (DiffTable: counts[s] > 0, Table.h:421-424) is the LIVE
      // filter, and mid-band it EQUALS post-commit `kInI` because every input
      // counter write precedes the bands (explicit folds in the ingest proc;
      // a derived acyclic input's seed folds run in the ready_after-lifted
      // stratum ahead of the band — the OB8 lemma, b3). `s` is the RowAt
      // iteration id, so `Present(s)` reads the row just bound as `ir`.
      // Gated on input_diff at CODEGEN time => monotone-input emission is
      // byte-identical (d5-selector discipline). ONE shared mold => this
      // conjunct rides ALL THREE rescan sources: band-(a1) birth (the E-F2
      // rebirth-from-shrunken-input cell), band-(a2) edge-adds, band-(a2')
      // edge-removals.
      if (input_diff) {
        cond += sep + input_member + ".Present(s)";
        sep = " && ";
      }
    }
    if (cond.empty()) {
      cond = "true";
    }
```

Notes on the spelling, verified at code:
- **Id correctness.** The mold loops `for (uint32_t s = 0; s < input_member
  .NumRows(); ++s)` (:2376) and binds `const auto ir = input_member.RowAt(s)`
  (:2379). `DiffTable::Present(uint32_t id)` returns `Total(counts[id]) > 0`
  (Table.h:421-424) — indexed by the **row id**, which is exactly `s`. So
  `input_member.Present(s)` filters the row bound as `ir`. (`Find(ir)` would be
  a redundant hash probe for the same id; `Present(s)` is O(1) and exact.)
- **No "true &&" wart.** Present is appended with the running `sep`, so a keyed
  input (the normal case, `in_key` non-empty) yields
  `ir.<k> == <expr> && <input>.Present(s)`; a keyless diff input yields bare
  `<input>.Present(s)`; the `empty()→"true"` fallback fires only for a keyless
  MONOTONE input (unchanged).
- **Monotone-input Present compiles too** (the XC-9 degeneration): `Table::
  Present(id)` (Table.h:261) is `assert(id<NumRows); return true;` — but it is
  never emitted for a monotone input because `input_diff` is false. The clean
  shape gates on `input_diff`, exactly as the substrate ruled.

### E2c — the band-(a2') edge-net-REMOVALS drain (H-11 / OB1 / R-A2-TRIGGER)

**File** Database.cpp. **Insertion point** immediately after the band-(a2)
block closes (after `cc << cc.Indent() << "}\n";  // edge drain` at :2522-2523),
before the band-(b) publish comment at :2525. **Append verbatim** (a faithful
net-removals clone of band-(a2) :2468-2523, differing ONLY in the drained
vector and carrying NO RecycleCurrent):

```cpp
  // band-(a2') [R-A2-TRIGGER §7] drain the input(edge) net-REMOVALS frontier
  // (REBUILD-on-shrink): a live-demanded key whose summarized input LOST a row
  // full-rescans exactly as band-(a2) does — the ONE shared rescan mold, now
  // its THIRD drain SOURCE (a1 births, a2 edge-adds, a2' edge-removals).
  // WITHOUT this arm a pure edge-retract epoch mints no input net-ADDITIONS
  // row, band-(a2) never fires, the E2b-filtered rescan never runs, and the
  // doubled pub counter parks present (substrate §3 silent-miscompile (i),
  // OB1). WITH it + the E2b Present conjunct the rescan reads the epoch-net
  // (shrunken) input, so band-(b)'s drop scan publishes the net retraction
  // (E-D). Removals never mint a death (ADV-2, OQ-DEATH-VS-REBUILD): an input
  // shrink is a REBUILD, not a demand death.
  //
  // R-A2-TRIGGER §7(3) DESIGN FENCE — **NO RecycleCurrent HERE**. `current` is
  // provably EMPTY at first touch (V-INST-FRESH + Seal/Recycle sole emptiers),
  // so Recycle is DEATH-ONLY (band-(a0)). An UNCONDITIONAL / ungated Recycle
  // in this arm would silently full-retract a same-epoch co-added key
  // (interleaving E-E): the add arm rescans K (cur = net content), an ungated
  // Recycle then wipes cur, dedup skips the re-rescan, band-(b) drops K's
  // entire frozen set — and NO landed belt catches it (V-INST-PARTITION
  // balances 0/0/frz; V-INST-FRESH never fires because Recycle did not
  // rescan). FORBIDDEN. The gate set below is IDENTICAL to band-(a2) (binding
  // R-A2-TRIGGER §7(2)); a divergence between the two a2 arms is a design
  // ERROR (the d7 gate-identity perturbation).
  if (input_removal) {
    // ADJ-R1 (as band-(a2)): the outer bind is the FULL EDGE ROW; the key is
    // projected from the edge's own key cols e<in_key[j]>.
    const auto edge_arity =
        static_cast<unsigned>(input_removal->ColumnTypes().size());
    std::vector<std::string> ebinds;
    for (unsigned i = 0u; i < edge_arity; ++i) {
      ebinds.push_back("e" + std::to_string(i));
    }
    std::vector<std::string> ekeyexprs;
    for (unsigned j = 0u; j < in_key.size(); ++j) {
      ekeyexprs.push_back("e" + std::to_string(in_key[j]));
    }
    cc << cc.Indent() << "for (const auto &[" << JoinExprs(ebinds, ", ")
       << "] : " << VecName(*input_removal) << ") {\n";
    cc.PushIndent();
    cc << cc.Indent() << "const auto iid = " << sname << ".FindInstance(Key_"
       << id << "{" << JoinExprs(ekeyexprs, ", ") << "});\n";
    if (diff) {
      // DEMAND-LIVENESS gate (R-3), IDENTICAL to band-(a2): a dead key still
      // binds an iid (append-only, no tombstone — OD-15; TouchedFlag resets at
      // Seal), so gate the rebuild on the demand table's POST-COMMIT Present.
      // Diff demand: QUIESCENT in an edge epoch (channel disjointness, T-3) =>
      // Present == committed presence. MONOTONE demand (the e5 carrier): Find
      // hits and Present is trivially true — liveness is IRREVOCABILITY, never
      // retracts (XC-9). E-F3: an edge-removal for a DEAD key closes here and
      // no-ops (its neighborhood died at the demand-death epoch). The e4
      // quiescence lemma (b3) discharges the former D3.a.2 RIDER.
      const auto demand_member = table_member[region.DemandTable().Id()];
      // Fable review [I]: the demand probe nests INSIDE the iid check so the
      // common stray/undemanded-edge rows pay no hash probe.
      cc << cc.Indent() << "if (iid != ::hyde::rt::kNoInstance) {\n";
      cc.PushIndent();
      cc << cc.Indent() << "const auto dq = " << demand_member << ".Find({"
         << JoinExprs(ekeyexprs, ", ") << "});\n";
      cc << cc.Indent() << "if (dq != ::hyde::rt::kNoRow && " << demand_member
         << ".Present(dq) && !" << sname << ".TouchedFlag(iid)) {\n";
    } else {
      // Provably unreachable for a real D3.a.2 program (input_diff ⇒ diff via
      // O-1); retained verbatim for gate-set byte-symmetry with band-(a2).
      cc << cc.Indent() << "if (iid != ::hyde::rt::kNoInstance && !" << sname
         << ".TouchedFlag(iid)) {\n";
    }
    cc.PushIndent();
    // The SAME rescan as band-(a1)/band-(a2); key filter compares the edge
    // row's own key cols (E2b appends the Present conjunct under input_diff).
    emit_instance_rescan(ekeyexprs);
    cc.PopIndent();
    cc << cc.Indent() << "}\n";  // live && !TouchedFlag
    if (diff) {
      cc.PopIndent();
      cc << cc.Indent() << "}\n";  // iid != kNoInstance (review [I] nest)
    }
    cc.PopIndent();
    cc << cc.Indent() << "}\n";  // band-(a2') edge-removal drain
  }
```

**Gate-set identity is auditable line-by-line**: the `if (diff) {...} else
{...}` fork above is a character-for-character copy of band-(a2) :2486-2510;
the only inter-arm differences are the drained vector
(`VecName(*input_removal)` vs `VecName(input_front)`) and the header comment.
This satisfies R-A2-TRIGGER §7(2) by construction of copy; the d7 identity
perturbation (§8) is the standing referee.

**Why a clone, not a shared helper.** R-A2-TRIGGER §7(4) pins the landed
a0/a1/a2 *emission bytes* untouched ahead of the new arm (id-stream stability,
the (d0) baseline). Appending E2c leaves :2406-2523 byte-identical. A lambda
refactor of band-(a2) would keep GENERATED bytes identical too, but it churns
the landed a2 generator source and entangles the a2-specific header comment
(net-ADDS) with the a2'-specific one (net-REMOVALS + the NO-Recycle fence).
The clone keeps the landed source untouched and the two header comments
truthful; the ~24-line gate duplication is the same shape the D3.a.1 [F]
retract/forcer dedup deferred — this lane records it as a **candidate for the
D3.a.3 [F] dedup sweep** (F2b), not a blocker.

===============================================================================
## §2 THE Present-CONJUNCT ADJUDICATION (H-10, maximum care) — CONFIRMED
===============================================================================

- **Spelling = `Present`** (RULED four-lane + §20(AM); not re-litigated).
  DiffTable::`Present(s)` = `counts[s] > 0` (Table.h:421-424), the ONLY
  predicate equal to post-epoch committed content mid-band (T-5/T-7): `InI(s)`
  is stale-TRUE for this-epoch retracts and stale-FALSE for this-epoch adds;
  `InNew(s)` is claim-flag-derived (drain-order dependent). Grounded on the
  frozen-A flat oracle: `pt` is `DiffTable<Row7>` (table_7), whose counter-based
  Present is the flat web's own observable (O-4/O-7).
- **Id.** `s` is the `RowAt` iteration id (:2376-2379); `Present(s)` reads the
  row just bound as `ir`. Correct by construction.
- **CompactDead interplay (XC-7).** Dead rows persist at `RowAt(s)` until the
  epoch-boundary sweep (4096-row floor; never in suite-sized runs), so the scan
  DOES enumerate this-epoch-retracted rows — precisely why the filter is
  mandatory. `Present(s)` excludes them; `NumRows()` still counts them.
- **All THREE sources.** The conjunct lives in the ONE shared mold, so it rides
  band-(a1) birth (E-F2), band-(a2) edge-adds, and band-(a2') edge-removals
  automatically. E-F2 is the load-bearing a1 case: rebirth-after-edge-retract
  must NOT resurrect the retracted edge — the a1 rescan's Present conjunct reads
  the shrunken net input.
- **Monotone-input byte-identity.** The `if (input_diff)` guard is a
  CODEGEN-TIME C++ bool; for the 177 monotone-input pins the conjunct string is
  never appended → `[BYTE]`. The runtime cost (one O(1) `counts[s]>0` per scanned
  row) exists only for diff-input programs.

===============================================================================
## §3 BAND-(b) + Seal UNDER THE NEW REGIME — UNCHANGED; E-D/E-F2 WALKED
===============================================================================

**Claim: band-(b) (drop scan :2564-2588 + born arm :2590-2630 + V-INST-PARTITION
belt :2635-2645) and Seal are BYTE-UNCHANGED; the (T,F) drop scan already
publishes E-D's net retraction because the E2b-filtered rescan makes `cur`
smaller.** No b2 edit here.

### E-D walked end to end (edge retract of a live-demanded key), generated shape

Epoch: an `-pt(K,V)` retract for a standing demand `K`. One entry call = one
epoch (T-1); the edge handler writes only the edge channel (T-3).

1. **Ingest (flow head).** `pt_2` NetBatches (O-3), then the stage-1 SubExplicit
   fold crosses `(K,V)` down on `pt` (table_7): `counts` → 0, and the input
   `-` kFrontierFilter mints the `(K,V)` row into the **input net-removals
   frontier** (free producer, C7/ADV-5; b1 drains it into `input_removal`). By
   band time the retracted row is at `RowAt(s)` with `Present(s)` FALSE.
2. **band-(a0) death.** `RemovalFrontier()` empty (no demand write this epoch)
   → skip.
3. **band-(a1) demand births.** Demand frontier empty → skip.
4. **band-(a2) edge-ADDS.** `input_front` empty (pure retract) → skip.
5. **band-(a2') edge-REMOVALS (E2c).** `*input_removal` holds `(K,V)`.
   `FindInstance(Key{K})` → the standing iid (append-only). Gate: `diff` true
   (O-1) → `dq = demand.Find({K})` hits, `Present(dq)` true (demand standing),
   `!TouchedFlag(iid)` true → **rescan fires**. The mold scans `pt`, and E2b's
   `Present(s)` **filters OUT** the retracted `(K,V)` → `cur` = net input =
   frozen `\ {(K,V)}` (smaller). `TouchedFlag(iid)` set.
6. **band-(b) drop scan (:2571-2588).** `frz` holds `(K,V)`; `cur.Find(drow)`
   for `(K,V)` → `kNoRow` → `++dropped`, `pub.SubDerivation((K,V),
   kNonRecursive)`, `DelQueue.Add((K,V))` → **−1 on pub** (getpt) for the
   retracted answer. Every carried row hits `else ++carried`.
7. **V-INST-PARTITION** (:2635): `born=0, carried=|cur|, dropped=1`;
   `0+carried==cur.NumRows()` ✓ and `1+carried==frz.NumRows()` ✓ → no abort.
8. **Seal** swaps `cur→frz`. Next epoch's frozen == the shrunken set.

**Symmetric ±2 (OB3/OB4, ADV-4).** Flat and nested co-derive pub. Flat: the
guard-web's NetDeleted del arm (`demand.InI && pt.InI && pt.NetDeleted →
SubDerivation` on ans → getpt, O-4) fires the SAME −1. Nested: band-(b)'s drop
scan fires its −1. Both fire IFF the retracted row was a supported answer row
for a live-demanded key — the drop scan's `cur.Find==kNoRow` gate and the flat
web's `NetDeleted ∧ demand.InI` gate are the same predicate expressed two ways.
The doubling stays EXACTLY 2× (the eqgate's premise), and `was!=now` publishes
one net retraction. **O-4/O-5 preserved**: a retract of a NON-demanded key
mints an input-removal row, band-(a2') FindInstance→kNoInstance (never demanded
→ never FindOrAdded) → silent skip; flat's `demand.InI` false → no
SubDerivation. Over-retraction impossible on both arms.

### E-F2 walked (rebirth from shrunken input)

ep1: `-pt(K,V)` (E-D above; `frz` shrinks). ep2: re-demand `K` (demand birth) →
band-(a1) rescans. The a1 rescan carries the SAME E2b Present conjunct → reads
the shrunken net input → the rebuilt `cur` does NOT contain `(K,V)` → band-(b)
births only the surviving rows. **Rebirth reflects net input**, no resurrection.
This is why the conjunct MUST ride band-(a1), not just the a2 arms.

### E-F3 (edge retract for a DEAD key) — the a2' arm's gate closes

ep1: demand death of `K` (band-(a0) RecycleCurrent + band-(b) full (T,F)
retract of `K`'s frozen set — the D3.a.1 death path). ep2: `-pt(K,V)`. The
band-(a2') arm: `FindInstance(Key{K})` → the still-bound iid (append-only), but
`dq = demand.Find({K})` → `Present(dq)` FALSE (K died) → gate CLOSES → no-op.
Net 0. The R-3 gate — now guarding the REMOVAL path too — is the cross-epoch
belt; the retracted edge's neighborhood was already retracted at K's death.

===============================================================================
## §4 V-INST-DIFF-COHERENCE + STORE-CTOR + THE e5 DIVERGENCE — UNCHANGED, AUDITED
===============================================================================

**e5 carrier = plain `-demand` on `demand_diff_input_1`** (once e1 lifts the
fence): the FIRST program with **P-STORE true ∧ P-DEATH false**. Empirically
grounded at frozen-A (flat `-demand`, this session): the demand relation
`demand__getpt_bf` is `Table<Row4>` (**monotone**, table_4) while `pt`/`ans`/
`getpt` are `DiffTable` (table_7/11/15) — O-1 confirmed.

- **Store ctor.** P-STORE = `TableIsDifferential(pub)`; diff input forces diff
  pub (O-1) → P-STORE true → the store constructs `instance_<id>(allocator_,
  false)` (differential regime) **ALREADY, by co-activation** — no b2 edit, no
  new selector. d5's `, false` is live.
- **No death op minted.** P-DEATH = `TableIsDifferential(demand)` is FALSE (mono
  demand) → NO kInstanceDeath op → `RemovalFrontier()` absent → band-(a0) not
  emitted → **V-INST-PAIR n_death==0** (already allowed, d3a1-design §2/X11).
  The predicates DIVERGE exactly as the §7 d2 ruling foresaw; b2 relies on
  nothing folding them.
- **V-INST-DIFF-COHERENCE UNCHANGED.** Same predicate both sides (stamp ==
  `region.IsDifferential()`); it is a stamp-vs-live drift guard, oblivious to
  the input axis. No b2 edit.
- **Every band belt behaves under mono-demand × diff-input:**
  - *band-(b) drop scan / born arm / V-INST-PARTITION* — gated on `diff`=PUB,
    true (O-1) → they RUN (the store legitimately shrinks on edge retract).
    Correct; belt balances (§3 step 7).
  - *V-INST-FRESH* (mold, :2368) — unchanged teeth; `current` is empty at every
    band-(a) first touch (OQ-MODEL full-rescan; only monotone `TryAdd` fills it,
    only Seal/Recycle empties it), so N-1 (OB4) STAYS EXACT — `WorkingOccupied
    = NumRows()>0` never degenerates. **b2 adds no Recycle to any input arm, so
    band-(a) entry stays empty** — the N-1 close (H-15) holds by this lane's
    NO-RECYCLE discipline.
  - *the a2/a2' demand-liveness gate* — emitted because `diff`=PUB true (XC-9);
    it probes the MONOTONE demand member (table_4). `Table::Find` (RowStore,
    Table.h:80) and `Table::Present` (:261, trivially true) BOTH compile. **e4
    cross-ref sentence (b3 owns the lemma):** *for a monotone demand the gate's
    soundness is IRREVOCABILITY, not quiescence — a bound iid ⟺ a standing
    demand (mono demand never retracts), so `Find({K})` hitting is itself the
    liveness proof and `Present(dq)` is a trivially-true no-op; the R-3 gate
    thus degenerates to the R-MONO `FindInstance != kNoInstance` test it
    generalized, and is correct.* No per-regime gate emission split — the gate
    is PUB-keyed and emits identically; only the demand member's dynamic type
    (monotone vs diff) differs, and both satisfy the `.Find/.Present` surface.

===============================================================================
## §5 TouchedFlag / EPOCH COUPLING — b2's MECHANISM-LEVEL PIECE OF e4
===============================================================================

(The OB8 lemma STATEMENT is b3's; these are the mechanism facts it rests on,
all verified at code / the frozen-A flat oracle.)

- **Same-batch ± of ONE row nets away (O-3/O-6).** `pt_2` NetBatches at the
  message boundary (Vec.h:176-218) before any fold, so `{+ (K,V), − (K,V)}`
  produces NO frontier row on either side — the row never appears in both
  `input_front` and `*input_removal` in one epoch. Matches flat's no-op.
- **TouchedFlag dedups across ALL rescan sources (T-8).** One append-once,
  Seal-reset flag set, already shared by a0/a1/a2 and now a2'. Interleaving
  **E-E** (batch `{+ (K,x), − (K,y)}`, same key K, different rows): whichever
  arm (a2 or a2') first touches K does the ONE full Present-filtered rescan —
  which reads the NET input (both the add and the removal already crossed in
  the ingest proc, T-4/T-5) — and the other arm's `!TouchedFlag(iid)`
  short-circuits. Drain order a2-before-a2' is therefore behavior-neutral
  (R-A2-TRIGGER §6(i)); the single rescan is net-correct.
- **Death epoch vs edge epoch disjoint (T-3).** No entry writes both channels,
  so band-(a0) (demand death) and band-(a2)/(a2') (edge) never co-fire for one
  key in one epoch. The a0-before-a2/a2' order preserves OD-15 death-wins
  suppression for any FUTURE combined entry (which cannot occur today); b2 does
  NOT rely on it and does NOT discharge the hypothetical-combined-entry LOUD
  cell (OB8's fence-or-proof stays b3's).
- **The a2' dead-key row no-ops (E-F3).** Covered in §3: the gate closes on
  `!Present(dq)`. No tombstone, no bespoke arm.

===============================================================================
## §6 CROSS-LANE CONTRACTS (what b2 CONSUMES / DEFERS)
===============================================================================

| # | contract | provider | b2 dependency |
|---|---|---|---|
| X-b1-1 | `region.InputRemovalFrontier() -> std::optional<DataVector>`, present IFF `TableIsDifferential(input_table)` (Emplace gated exactly on input differentiality; mirrors `removal_frontier`/`RemovalFrontier` Program.h:1176 / include:866 / Program.cpp:762-768; + ClassifyVector read-set arm Procedure.cpp:196-211) | b1 | E2a consumes it as the `input_diff` selector AND the a2' drain source. **F2c**: the Emplace condition IS b2's correctness precondition — if it ever emplaces under a non-input-diff condition, E2b's conjunct mis-fires. |
| X-b1-2 | provisioning: the `(input_table, kNetRemovals)` CF vector is fence-guarded pre-minted (the :318 orphan-mint idiom, H-8) so `*input_removal` is a real drainable vec, never a silent orphan | b1 | E2c drains `VecName(*input_removal)`; b2 adds no orphan belt (symmetry with band-(a0)'s trust of its optional) — the mint fence is b1's. |
| X-b1-3 | `InstantiateEffects` grows a second `kVecDrain{input, kNetRemoval}` leg (C3/H-6, the DeathEffects Rel.cpp:868 mold) under an `input_diff` effect selector; V-INST-EFFECT `input_drains==2` / V-INST-DRAIN input-arm regime split (C5/C6) | b1 | b2's `.rel` [STRUCT] prediction (§7) depends on the effect leg rendering. |
| X-b3-1 | the **e4 OB8 quiescence lemma** + rewrite of the a2/a2' gate-fork "D3.a.2 RIDER" comment to cite the LANDED lemma (discharged) | b3 | E2c's gate comment carries the interim wording; b3 rewrites the shared sentence. |
| X-b4-1 | the **e6 witness** exercises E-D / E-E / E-F1 / E-F2 / E-F3 in BOTH regimes (diff-input × mono-demand [e5] AND × diff-demand, O-9) and carries a `@differential nbhd_out` tap (eqgate = answer + sorted published-delta identity, O-7) | b4 | b2's d7 L-rows (§8) drive this witness; **F2b**: the witness MUST include an E-F2 rebirth phase or the a1-source Present conjunct is untested. |

**NO NEW DROp KIND** (challenged at code, per the substrate): the a2' arm is
pure codegen consuming the free input-net-removals frontier via b1's effect
leg. No `kInstance*` op is minted by this lane; `kSubgraphInstantiate` stays 1.
**NO NEW DUMP SPELLING / NO E-71 NOTE OWED by b2**: the effect leg renders as
`kVecDrain(%input, kNetRemoval)` — both terminals already render (kNetRemoval
appears on the demand death drain; verified in the mono-witness dump's
`effects:` line). The a2' arm emits generated C++, not a dump token.

===============================================================================
## §7 PRE-REGISTERED PREDICTIONS ([BYTE]/[STRUCT], per surface + gate family)
===============================================================================

| surface | verdict | rationale + gate family |
|---|---|---|
| 177 landed-pin stdouts × 4 modes | **[BYTE]** | all monotone-input → `input_diff` false → E2a/E2b/E2c inert; no accepted program changes. Referee: OptDiff suite. |
| 20 pinned dump surfaces + 14 `.irgold` + the `.rel`/`.h` regen set | **[BYTE]** | flag-off / monotone-input: no id mint, no op mint, census keys unchanged; the band emits byte-for-byte the landed a0/a1/a2/b (R-A2-TRIGGER §7(4)). Referee: 20/20 carrier-golden regen. |
| `data/` corpus (36 × 4) | **[BYTE]** | none carries a differential summarized demanded input. |
| the **witness (e6) generated header**, NESTED arm, diff-input × mono-demand | **[STRUCT]** | +1 band-(a2') `for` loop over the removal vec (FindInstance + the R-3 `Find`/`Present`/`!TouchedFlag` nest + the rescan call, NO Recycle); +1 `Present(s)` conjunct term inside the rescan key-filter, appearing at ALL THREE rescan call-sites' emitted text (a1/a2/a2'); store ctor `(allocator_, false)` (co-activation, already predicted by d5). |
| the witness NESTED arm, diff-input × diff-demand | **[STRUCT]** | IDENTICAL a2' arm shape (gate forks on `diff`=PUB, true in both regimes); additionally the demand member is a `DiffTable` and band-(a0) death is present (b1/b2 D3.a.1 machinery). The a2' arm's bytes are regime-invariant. |
| the witness `.rel` (unpinned; eyeball + census) | **[STRUCT]** | the `kSubgraphInstantiate` op's `effects:` line grows by ONE — a third `kVecDrain(%input, kNetRemoval)` beside the two `kNetAddition` drains (X-b1-3). **Census line UNCHANGED at the KEY level**: `kSubgraphInstantiate=1`, no new key, no kInstance* delta from the a2' arm (effects are op-attributes, not census kinds — verified: the census counts DROp KINDS). |
| the witness `.ir` (unpinned) | **[STRUCT]** | the removal drain renders in the region's vector list; no new IR token from b2. |
| ctest / config-invariance / release==debug | **[BYTE]** unchanged binaries; single-hash on both witness arms | b2 adds no test binary. |

**Census delta summary (the point-6 question, answered):** ZERO census-key
changes attributable to lane b2. The a2' drain is EFFECT-level; effects render
on the op's `effects:` line and are NOT census keys. The instantiate op's
effect count rises by 1 (the kNetRemoval leg, X-b1-3); the census KIND tally is
untouched by b2. E-71 grammar notes owed by b2: **NONE**.

===============================================================================
## §8 d7 LIVENESS-BY-PERTURBATION (b2's rows; run at stage (d), texts recorded,
##    all reverted; NESTED-arm vehicle per the L3/D3.a.1 amendment)
===============================================================================

| # | belt / mechanism | perturbation (scratch worktree, WIP-commit FIRST) | expected |
|---|---|---|---|
| Lb2-1 | **E2b mold Present conjunct** | drop the `if (input_diff) cond += ... Present(s)` clause; rebuild; run the e6 witness E-D/E-F2 batches nested | eqgate GOLDEN-DIVERGE: the rescan re-adds this-epoch-retracted rows (over-materialization) AND E-D never drops (stuck present) — the flat oracle catches it. Landed conjunct = permanent teeth. |
| Lb2-2 | **E2c band-(a2') arm** | comment out the whole `if (input_removal) {...}` block; rebuild; run E-D | eqgate GOLDEN-DIVERGE (stuck-present, OB1): a pure edge-retract epoch never rebuilds; the doubled pub counter parks at 1. |
| Lb2-3 | **the FORBIDDEN ungated-late Recycle (§7(3) negative test)** | INSERT `sname.RecycleCurrent(iid);` UNGATED (outside `!TouchedFlag`, after the rescan) in the a2' arm; rebuild; run interleaving E-E | eqgate GOLDEN-DIVERGE (**silent** full-retract) AND **V-INST-PARTITION does NOT abort** (0/0/frz balances) AND **V-INST-FRESH does NOT fire** — demonstrating no landed belt catches it, which is WHY the fence exists. |
| Lb2-4 | **gate-set identity (R-A2-TRIGGER §7(2))** | drop the `Present(dq)` conjunct from the a2' gate ONLY (leave band-(a2) intact); rebuild; run E-F3 | eqgate GOLDEN-DIVERGE: a dead key's edge-removal re-materializes on the removal path (zombie-rebirth on a2' while a2 stays sound) — the two-arm divergence is the "design ERROR" the ruling names. |
| Lb2-5 | **E-F2 a1-source coverage (positive, LANDED)** | the e6 witness's rebirth-after-edge-retract phase — every suite run | golden + eqgate green; proves the a1 rescan's Present conjunct reads shrunken input (no resurrection). |
| Lb2-6 | **e5 divergence lives (positive, LANDED)** | compile `demand_diff_input_1` (or the e6 mono-demand witness) `-demand -demand-instance`; DebugValidate | compiles; store `(…, false)`; NO kInstanceDeath op (V-INST-PAIR n_death==0); band-(a2') + band-(b) run against a monotone demand member — green. |

===============================================================================
## §9 GATE ROLL-CALL (families this lane exercises / relies on)
===============================================================================

- **V-INST-FRESH** (mold, Database.cpp:2368) — always-on generated fprintf+abort;
  now also entered via band-(a2'); teeth unchanged (NO-RECYCLE keeps entry
  empty; N-1/OB4 exact).
- **V-INST-PARTITION** (band-(b), :2635) — always-on generated; refereed E-D
  (0/carried/1) and the FORBIDDEN-Recycle blind spot (Lb2-3, deliberately
  belt-invisible → eqgate-only).
- **the R-3 demand-liveness gate** (a2 AND a2', PUB-keyed) — the cross-epoch
  belt guarding both edge paths; E-F3/E-F1 charter.
- **eqgate** (b4) — the standing flat==nested answer + sorted-delta oracle; the
  sole referee for the silent shapes (Lb2-1/2/3/4). UPGRADED to delta identity
  via the `@differential nbhd_out` tap (O-7).
- **V-INST-DIFF-COHERENCE / V-INST-PAIR / store ctor** — UNCHANGED; §4 audits
  the e5 divergence rides them with n_death==0.

**b2 adds NO new validator or belt** (the substrate's "effects suffice" claim
holds at code: the free removal producer + b1's effect leg + the landed
V-INST-FRESH/PARTITION/R-3 gate cover the a2' arm; the input_diff selector is
definitionally coupled to `InputRemovalFrontier().has_value()`, so no coupling
belt is needed).

===============================================================================
## FINDINGS (b2)
===============================================================================

- **F2a (fact, intentional).** Within band-(a2') the `else` (monotone-demand)
  gate fork is PROVABLY UNREACHABLE: `input_removal` present ⇒ `input_diff` ⇒
  `diff` (P-STORE true) via the O-1 Differential.cpp closure (verified
  empirically: diff input → diff pub). It is RETAINED verbatim for
  character-level gate-set symmetry with band-(a2) (R-A2-TRIGGER §7(2)
  auditable-by-copy). Not dead code to delete — deleting it would make the two
  a2 arms textually divergent and weaken the identity audit.
- **F2b (coverage obligation → b4).** The E2b Present conjunct rides band-(a1)
  birth, not only the a2 arms. The e6 witness MUST carry an **E-F2
  rebirth-after-edge-retract phase** or the a1-source conjunct is untested
  (Lb2-5). Also: the a2' gate duplication (~24 lines) is a **D3.a.3 [F] dedup
  candidate** — recorded, not blocking (matches the deferred D3.a.1 [F]).
- **F2c (precondition on b1).** `input_diff` is derived as
  `InputRemovalFrontier().has_value()` (single source of truth). This is sound
  IFF b1 Emplaces `input_removal_frontier` EXACTLY under
  `TableIsDifferential(input_table)`. If b1 ever gates it on a broader/narrower
  condition, E2b's Present conjunct silently mis-fires (mono input → spurious
  filter, or diff input → missing filter → over-materialization). b1 must pin
  the Emplace condition and the commit message must state the coupling.
- **F2d (no folding — d2 §7 extends).** `input_diff` is a THIRD predicate axis
  (ADV-6), kept SEPARATE from `diff`/P-STORE/P-DEATH. E2a introduces it as its
  own local read; it is NOT folded into any shared helper. The e5 carrier is
  the first program where the axes diverge (P-STORE true, P-DEATH false,
  input_diff true) and the three-way separation is what keeps each site correct.
