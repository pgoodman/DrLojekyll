# D3.a.2 — LANE b3 DESIGN: the e4 quiescence re-derivation + the e5
# divergence-goes-live audit + the N-1 close (the ARGUMENT lane)

> **House banner.** Tip **b4d08307** (branch keyed-instances; docs-only atop
> the D3.a.1 landing 33cabcf1 — every code anchor below is live at the landed
> binaries; anchors personally re-read at this checkout). Binding context:
> d3a2-substrate.md (§4 OB1-OB8 + the epoch catalogue; §6 fact base; §7 the
> R-A2-TRIGGER ruling — TWO DRAINS, NO RECYCLE), d3a-ruling-brief.md
> (OQ-MODEL / OQ-INPUT / OQ-DEATH-VS-REBUILD), d3a1-substrate.md §7 (the d2
> CO-ACTIVATION anti-fold ruling) + §8 e1-e8, KeyedInstances.md §20(AK)-(AM).
> This lane is the ARGUMENT lane: proof text + pinned coupling statements +
> comment-edit specs. It mints NO band code. Its edit specs are E3a-E3d, all
> COMMENT edits ([BYTE] on every generated artifact and binary — none of the
> touched comments are `cc <<`-emitted into generated code; verified below).

## §0 SCOPE — what b3 owns, what it does NOT

OWNED (this doc): the e4 soundness lemma for every admitted epoch shape (OB8);
the in-source RIDER discharge (Database.cpp:2494-2495, E3a); the e5
divergence-goes-live audit table (H-16) with per-site n_death==0 verdicts; the
N-1 close (OB4/ADV-7/H-15, InstanceStore.h E3b/E3c); the demand_diff_input_1
in-file comment edit (ADV-8, E3d, conditional on b4's disposition); the pinned
five-way COUPLING STATEMENT; the d7 L-rows for the e5 carrier.

NOT OWNED (sibling lanes — cited as dependencies, never specified here):
- **b1** — F-A lift (e1), V-INST-SOLE split (e2/ADV-1), V-INST-EFFECT third
  axis (H-3/ADV-6), V-INST-DRAIN input-arm split (H-4), H-5 statement.
- **b2** — InstantiateEffects removal leg (H-6/C3), `input_removal_frontier`
  UseRef + accessor + ClassifyVector arm (H-7/C9), provisioning fences
  (H-8/C10), the a2' removal drain + the shared-mold **`input.Present(s)`
  conjunct** (H-10/ADV-3, H-11), the band-(b) net-retraction ride (H-14 code
  half). b3's lemma is the SOUNDNESS PRECONDITION of every one of these; the
  code is b2's.
- **b4** — the e6 witness family, demand_diff_input_1 disposition (H-17), the
  eqgate referee half of the symmetric-firing argument (H-14), G-INPUT-NEG /
  G-STALE discharge record (H-18/e7), the e8 perturbation vehicles.

The b3 lemma has ONE standing precondition it does not itself enforce but
depends on: the **recursive-content fence survives F-A's lift** (Build.cpp
:1533-1540, b1's e1 keeps it) — a fixpoint-refired input would break
"input counters final at band time" (OB8(i)). If b1 ever weakened it, this
lane's L-EDGE/L-MONO proofs would need re-derivation. Stated as a pinned
cross-lane obligation (§5, C-REC).

===============================================================================
## §1 THE e4 LEMMA (OB8) — a2-gate soundness for every admitted epoch shape
===============================================================================

### 1.0 The gate under audit + the single timing fact it rests on

The band-(a2) rebuild gate, differential arm (Database.cpp:2500-2505,
`diff == region.IsDifferential() == TableIsDifferential(pub) == P-STORE`):

```cpp
    if (iid != ::hyde::rt::kNoInstance) {
      const auto dq = demand_member.Find({ekeyexprs...});
      if (dq != ::hyde::rt::kNoRow && demand_member.Present(dq) &&
          !instance_<id>.TouchedFlag(iid)) { <rescan> }
    }
```

Per the R-A2-TRIGGER ruling clause (2), the NEW a2' removal arm (b2) carries
this **identical gate set**. So the lemma below covers a2 (edge-additions) and
a2' (edge-removals) uniformly; band-(a1) births carry the same `Present`
rescan-mold conjunct (b2/ADV-3, the E-F2 rebirth cell) but no demand-liveness
gate (its drain rows are presence GAINS by construction — d3a1-design §3.2).

**THE ONE LOAD-BEARING TIMING FACT (T-5, re-verified at code).** `Present` is
counter-based (DiffTable::Present, Table.h:421-424 `Total(counts[id]) > 0`)
and the counter flips AT THE INGEST FOLD (ingest proc, before the flow proc),
NOT at the commit sweep; the flow's claim drains touch FLAGS only (kDel/kAdd),
never counts. A table written only in the ingest proc therefore has counters
FROZEN through the whole flow, so `Present` at band time == the row's
POST-COMMIT `kInI` for this epoch (Commit sets `kInI := counts>0`). This is
the single fact the four sub-lemmas below share; it makes "Present at band
time" equal to "epoch-NET committed-equivalent presence" for BOTH the demand
table AND (via T-7) the input table.

### 1.1 (L-EDGE) — edge epochs, differential demand: the D3.a.1 argument, now
### also covering edge-RETRACT

Claim: in any epoch that writes ONLY the edge channel — add OR RETRACT — the
demand table is untouched, so `Present(dq)` == committed demand presence and
the gate is the D3.a.1 argument verbatim.

Proof. Channel disjointness (T-3): every generated entry point is one
`BuildIOProcedure` writing exactly one channel; `add_edge`/edge-retract writes
ONLY the edge (input) table, never the demand table. So the demand table's
counters are frozen at their pre-epoch committed values throughout the flow
(T-5) ⇒ `demand_member.Present(dq)` reads committed demand presence. The
**retract** direction changes NOTHING demand-side: a differential edge input's
retract epoch fires SubExplicit on the INPUT table in the ingest proc and drives
the input's net-removals frontier (b2's a2' arm), but the edge handler still
writes no demand row — T-3 holds identically for edge-retract as for edge-add.
Therefore a dead-but-iid-bound key (demand retracted in a PRIOR epoch) has
`Present(dq)` FALSE at band time and the gate skips both a2 and a2'. ∎
(Verified: the retract epoch's only demand-side effect would be through the
demand ingest fold, which an edge-only entry never invokes.)

### 1.2 (L-MONO) — the e5 carrier, monotone demand: soundness = IRREVOCABILITY
### (XC-9)

Claim (spell the sentence, XC-9): on the e5-divergence carrier (differential
input, MONOTONE demand — P-STORE true, P-DEATH false), the diff-arm gate is
STILL emitted (selector is `diff == region.IsDifferential() == P-STORE`, TRUE
because a differential input flips pub through the Differential.cpp closure),
and `demand_member` is the MONOTONE demand `Table`. There
`demand_member.Present(dq)` binds `Table::Present` (Table.h:261 — "Always true
for stored rows"), so the conjunct degenerates to `true` and the gate reduces
to `iid != kNoInstance && dq != kNoRow && !TouchedFlag(iid)`.

> **THE IRREVOCABILITY SENTENCE (XC-9, e4-normative).** When the demand table
> is monotone, a key's demand, once forced, is NEVER retracted; a bound iid
> therefore always implies a live demand and the demand row that minted it persists
> monotonically (so `Find` succeeds and `Present` is unconditionally true).
> The gate is sound not by QUIESCENCE (the demand table has no retract epoch
> to be quiescent across) but by IRREVOCABILITY — there is no dead-key-still-
> binds-iid hazard because there are no dead keys. The zombie-rebirth the R-3
> gate defends against (D3.a.1 HIGH-1) is UNREACHABLE under monotone demand;
> the emitted `Present`/`Find` conjuncts are correct-but-redundant belts, not
> load-bearing filters.

Corollary (why the gate is emitted anyway, and correctly): the selector is
pub-keyed by the d2 ruling (one authority, Rel.cpp:1055), so the e5 carrier
emits the diff arm without a demand-differentiality branch. This is BENIGN:
`Table::Present` compiles and returns true; no monotone-demand special case is
needed in the emitter (a d5-selector win — the emission stays byte-identical
across the demand-regime axis, only the demand table's own flavor differs).
`region.DemandTable()` is non-null for e5 because `si->demand_table.Emplace`
sits inside the `if (inst.differential)` block (Procedure.cpp:341-369) and
inst.differential == P-STORE == true. ∎

### 1.3 (L-DEMAND) — demand epochs: input frontiers empty, a2/a2' never fire

Claim: in a demand epoch (query/retract injector — writes ONLY the demand
channel), the input net-additions AND net-removals frontiers are empty, so
band-(a2) and band-(a2') iterate zero rows and are vacuously sound.

Proof (vec lifecycle, verified at code). The frontier VECTORs are
`proc.DefinedVectors()` of the flow/primary procedure, emitted as FRESH
stack-local `::hyde::rt::Vec<...>` at procedure-body entry
(Database.cpp:1840-1843: `::hyde::rt::Vec<...> <name>(allocator);`) — one fresh
empty vector per epoch, never a persistent Database member. They are FILLED
only by the commit-band frontier filter (`mint_filter`, Rel.cpp:2511-2538) of
the INPUT table, which produces rows only when the input table's counters moved
this epoch. A demand epoch writes no input row (T-3), so the input's overdelete/
addition sets are empty, the frontier filter appends nothing, and both input
frontiers stay empty ⇒ the `for (... : VecName(input_front))` /
`VecName(input_removal_frontier)` loops execute zero iterations. Symmetrically
the demand-birth frontier (band-a1) is filled only in a demand epoch, so in an
edge epoch a1 is idle — the same per-epoch-fresh-locals mechanism, the exact
D3.a.1 argument (§3.2). ∎ (This is why in a demand-retract/death epoch a2/a2'
cannot resurrect the dying key: their frontiers are empty; TouchedFlag is the
same-epoch belt, `Present`/the empty frontier are the cross-epoch belts.)

### 1.4 (L-COMBINED) — the hypothetical combined demand+input entry: a
### DOCUMENTARY fence (no emitting-code-reachable structural assert)

Claim: a single entry that writes BOTH channels in one epoch is the ONE shape
where the "Present == committed presence" justification of L-EDGE fails; NO
such entry exists today; the fence pinning it is DOCUMENTARY (there is no cheap
structural assert reachable from emitting code), per the OD-15 pinned-coupling
idiom.

Where it would arise. A future multi-message / combined-batch entry API — one
generated ingest proc that runs both the demand ingest fold AND the edge ingest
fold before one flow proc. Then, in an epoch with a demand write and an edge for
the same key, `Present(dq)` at band time would reflect this epoch's POST-FOLD
demand state (T-5: the demand fold ran in the same ingest proc), i.e. the
epoch-NET demand presence INCLUDING the uncommitted same-epoch write — NOT the
pre-epoch committed presence L-EDGE assumes. (Note: the net-post-fold reading
is arguably the MORE correct liveness signal — it answers "is this key live
after this epoch?" — but the specific D3.a.1 quiescence CHAIN OF REASONING no
longer certifies it; a combined entry demands a fresh proof, not a fresh line.)

What pins it TODAY. T-3 channel disjointness is a property of the GENERATED
ENTRY-POINT STRUCTURE: `BuildIOProcedure` emits one ingest proc per message,
each writing exactly one channel (verified: one `io_vec` per receive; a
combined entry would require a codegen change to emit a multi-channel ingest
proc). No such generator exists.

Why the fence is DOCUMENTARY (no structural assert). The band cannot attest
channel provenance: `Present` is counter-based and carries NO "written this
epoch" bit; a runtime "the demand frontier is empty in an edge epoch" check
would be a per-epoch cost that STILL could not distinguish a legitimate future
combined entry from a bug, and it would fire on the very API we would be
intentionally adding. There is no cheap emitting-code-reachable assert. Per the
OD-15 pinned-couplings precedent (and the substrate's explicit instruction),
the fence is therefore stated as a DOCUMENTARY coupling: it lives in the RIDER
discharge (E3a) and the §5 coupling statement, and it BINDS any future
multi-message batch API to re-derive this gate's committed-vs-post-fold
`Present` spelling before landing. This lane does NOT discharge OB8's
combined-entry obligation with a proof-of-impossibility beyond T-3's
entry-structure argument; it discharges it as a NAMED FENCE (the R-A2-TRIGGER
ruling clause (4) already scoped it out of the ruling: "does NOT discharge it —
that stays e4's"; e4's answer is: documentary, here).

### 1.5 EDIT SPEC E3a — the in-source RIDER discharge (Database.cpp:2494-2495)

The RIDER is a SOURCE comment in the codegen (a `//` block at Database.cpp
:2487-2499 describing the emitted gate; the emitted bytes are :2500-2505, which
carry NO comment). Editing it changes NOTHING in any generated artifact —
[BYTE] on all 20 pinned surfaces + both witness arms + every corpus stdout.

REPLACE the two rider lines (Database.cpp:2494-2495, currently):
```cpp
      // joins against the absent demand row). D3.a.2 RIDER: a differential
      // input interleaving re-derives this quiescence argument.
```
WITH (line-count-flexible; the preceding :2487-2493 "Sound because … QUIESCENT
… Present == committed presence" lines STAY — they remain truthful for L-EDGE:
an edge epoch, add or retract, never writes demand, T-3):
```cpp
      // joins against the absent demand row).
      // D3.a.2 DISCHARGE (e4 lemma, d3a2-design §b3): a differential
      // (deletable) summarized input does NOT break this gate. Present is
      // counter-based and flips at the ingest fold (T-5), so at band time it
      // reads the epoch-NET committed-equivalent presence. Sound for every
      // admitted epoch shape:
      //   L-EDGE  an edge ADD or RETRACT epoch leaves demand FROZEN (channel
      //           disjointness: the edge handler never writes demand) =>
      //           Present(dq) == committed demand presence (the argument
      //           above, verbatim; retract changes nothing demand-side);
      //   L-MONO  a MONOTONE demand (the e5 carrier: diff input, P-STORE true,
      //           P-DEATH false) => Present degenerates to Table::Present
      //           (always-true); the gate is sound by IRREVOCABILITY — a
      //           monotone demand key is never retracted, so a bound iid
      //           always implies a live demand (no zombie-rebirth to defend);
      //   L-DEMAND a demand epoch leaves the input frontiers EMPTY
      //           (per-epoch-fresh flow-proc Vec locals) => this arm and the
      //           a2' removal arm iterate zero rows: vacuously sound.
      // The ONE unhandled shape — a hypothetical COMBINED demand+input entry
      // (both channels one epoch) — cannot arise: BuildIOProcedure emits one
      // ingest proc per message, each writing a single channel. That fence is
      // DOCUMENTARY (no band-reachable assert can attest provenance — Present
      // carries no "written this epoch" bit); a future multi-message batch API
      // MUST re-derive this gate's committed-vs-post-fold Present spelling
      // before it lands.
```

Pre-registered verdict: **[BYTE]** on every generated surface and every binary
(source-comment only; the emitted gate at :2500-2505 is untouched). Gate family:
none engaged (documentation edit). Confirmed by construction: the two rider
lines are inside the `if (diff) { // comments … }` source block, above the
first `cc <<` of the arm.

===============================================================================
## §2 THE e5 DIVERGENCE-GOES-LIVE AUDIT (H-16) — every P-STORE-side site that
##    could assume a death exists, verified against n_death == 0
===============================================================================

The e5 carrier is the FIRST program with P-STORE (`TableIsDifferential(pub)`)
true AND P-DEATH (`TableIsDifferential(demand)`) false: a differential input
flips pub (O-1, empirical), while the demand stays monotone. It exercises the
FULL P-STORE-side machinery — `, false` ctor, InstantiateEffects diff arm
(2 counters/2 crossings/2 appends), del/add queues, the (T,F) drop scan, the
V-INST-PARTITION belt, the a2 diff-arm gate — with ZERO death: no kInstanceDeath
op, no band-(a0), no removal_frontier, no demand net-removals frontier. Every
site below was re-read at code; the question is whether each tolerates
n_death == 0 / a death-free differential store.

| # | site (anchor) | what it could assume | e5-carrier behavior | verdict |
|---|---|---|---|---|
| A1 | **V-INST-PAIR** Rel.cpp:4405-4407 | a differential store has a death op | `n_death > 1u` is the ONLY reject; `n_inst==1 && n_seal==1 && n_death==0` passes | **OK** (allows 0, verified — the "{instantiate, seal} (R-MONO)" arm of the message covers a death-free store) |
| A2 | **V-INST-EMITTED balance** Procedure.cpp:546-575 | enrolled == emitted keyed on the store being differential | `enrolled` counts flow ops of kind {instantiate, death, seal}; `emitted` is exactly what LowerSubgraphInstances pushed. Death NOT minted ⇒ enrolled = {inst, seal} = 2, emitted = {inst, seal} = 2 (the death enrollment Procedure.cpp:407-408 is inside `if death_by_sid.find`, not entered). BALANCE IS KEYED ON OP-PRESENCE (P-DEATH), NEVER ON P-STORE | **OK, NOT edit-needed** — the "3 vs 2" is the diff-DEMAND case (P-DEATH true); e5 is 2 vs 2. See F3a. |
| A3 | **V-INST-DEATH-COHERENCE** Procedure.cpp:379-390 | reachable per differential store | guarded by `if (auto dit = death_by_sid.find(sid); dit != end)`; e5's death_by_sid is empty ⇒ block not entered | **OK** (death-op-presence-keyed ⇒ vacuous) |
| A4 | **CheckInstanceDeathFrontier** Rel.cpp:4789-4813 (called unconditionally :4550) | called ⇒ a death frontier must exist | loops over `kInstanceDeath` ops ONLY (`if (op.kind != kInstanceDeath) continue`); e5 has none ⇒ loop body never runs. The unconditional CALL is a no-op on a death-free flow | **OK** (op-keyed loop; the call site being unconditional is harmless) |
| A5 | **band-(a0) emission** Database.cpp:2421 | emitted per differential store | guarded `if (auto removal = region.RemovalFrontier(); removal)`; e5's removal_frontier is null (Procedure.cpp Emplaces it only inside the death-op block :404-406) ⇒ no a0 emitted | **OK** (null-guarded on the region member, not on the diff bit) |
| A6 | **ClassifyVector kSubgraphInstance arm** Procedure.cpp:196-211 | classifies all region vecs incl. removal | `si->removal_frontier.get()` is nullptr for e5 ⇒ the removal branch never matches a real `vec`; demand/input frontiers + del/add queues classify correctly | **OK for existing vecs.** CROSS-LANE: the NEW `input_removal_frontier` (b2/H-7) needs its own read-classify arm — that is b2's edit; b3 flags it as a dependency (F3b), not an e5-death issue |
| A7 | **CollectEffects / hazard model, kInstanceDemand** Rel.cpp:801-804 (mint) + :4965-4966 (hazard switch) | the demand-table read models a DiffTable | `kInstanceDemand` → `break; // frozen read … NO hazard (HP-8)` — differentiality-agnostic; e5's monotone demand read is a no-hazard frozen read, modeled fine. The a2 probe of `demand.Present` is a second realization of this ALREADY-declared read (per-(op,table,kind) granularity), model-covered — no effect-set change | **OK** (review-[D]: the diff regime's declared demand read is legal over a MONOTONE demand table — a read effect needs no counters/queues on the target) |
| A8 | **the commit band** Procedure.cpp:544 (LowerCommitSweeps from kCommitSweep ops) | assumes symmetric diff tables | pub + input are differential ⇒ their commit sweeps run and drain the band-(b) del/add queue appends; the demand table is monotone ⇒ Seal-only, no sweep. No site assumes the demand table is differential | **OK** (per-table, driven by each table's own flavor) |
| A9 | **V-INST-DIFF-COHERENCE** Procedure.cpp:297-304 | — | `inst.differential == TableIsDifferential(pub)`; e5: both true. Same predicate both sides (the d2 tautology guard); death-independent | **OK** (unaffected — it never reads P-DEATH) |
| A10 | **store ctor `, false`** Database.cpp:1460-1467 (descriptor IsDifferential == P-STORE) | — | e5 P-STORE true ⇒ `, false` ⇒ HP-7 Seal belt OFF — CORRECT: an edge-retract legitimately shrinks the store; the [DBG] monotone belt must not fire | **OK** (P-STORE-keyed, which is exactly the right axis for the belt) |

**AUDIT VERDICT: ZERO edits needed on the P-STORE side for n_death == 0.**
Every death-touching site is keyed on OP-PRESENCE (the kInstanceDeath op) or on
a region member (removal_frontier) that is null under P-DEATH-false — never on
P-STORE. This is the §7 d2 anti-fold ruling paying off at code: because the two
predicates were kept separately spelled, the death machinery is P-DEATH-gated
and the store/effect machinery is P-STORE-gated, so their divergence at e5 is
absorbed with no new branch. The ONE genuinely new surface the e5 carrier needs
is b2's `input_removal_frontier` classify arm (A6/F3b) — an INPUT-side add, not
a death-side one.

### Findings

- **F3a (V-INST-EMITTED regime question — RESOLVED, NOT edit-needed).** The
  balance check (Procedure.cpp:546-575) compares two multisets that BOTH range
  over `{kSubgraphInstantiate, kInstanceDeath, kInstanceSeal}` and are BOTH
  populated only when the corresponding op is minted/lowered. For the e5
  carrier (P-DEATH false) both sides omit the death ⇒ 2 vs 2, PASS. The "3 vs
  2" the task flagged is the diff-DEMAND store, not e5. No edit; no directed
  d7 test (there is nothing to perturb — the check is already correct across
  the P-STORE/P-DEATH divergence). Had it been P-STORE-keyed it would have
  demanded a death for a death-free differential store and aborted the e5
  compile at V-INST-EMITTED — it is not.
- **F3b (cross-lane dependency, b2).** The e5 carrier's a2' removal arm drains
  `input_removal_frontier`; ClassifyVector (Procedure.cpp:196-211) must gain a
  `vec == si->input_removal_frontier.get() ⇒ read.insert` arm or the primary-
  proc threading mis-classifies it. This is b2's H-7 edit; recorded here
  because the e5 audit is where its ABSENCE would first bite (an unclassified
  read vec → wrong inout threading → the a2' arm drains an empty frontier
  silently, a stuck-present miscompile the eqgate catches). Pin: b2's classify
  arm is a SOUNDNESS precondition of L-EDGE's retract half.

===============================================================================
## §3 THE N-1 CLOSE (OB4 / ADV-7 / H-15) — discharged under full-rescan
===============================================================================

The InstanceStore.h:21-27 / :159-160 contingency note reserves the right to
re-introduce a signed per-instance member count because "under R-DIFF a
mid-batch retraction could make NumRows()-as-occupancy a lie." Under the RULED
OQ-MODEL (full-rescan) + the RULED R-A2-TRIGGER (NO RECYCLE in the input arms),
this cannot happen and the note CLOSES.

Proof (OB4). `current` is built ONLY by the band-(a) monotone rescan mold
(`cur.TryAdd(...)`, Database.cpp:2398) and emptied ONLY at Seal
(InstanceStore.h:174ff) or RecycleCurrent (death arm only, band-a0). An input
RETRACTION is expressed NOT as an incremental per-row delete on `current` but as
a full Touch+rescan of the a2' removal arm that reads the epoch-NET input (via
the `input.Present(s)` conjunct, b2/ADV-3) and rebuilds `current` from empty
(V-INST-FRESH guarantees empty at first touch; the ruling forbids any input-arm
Recycle). So within an epoch `current` only GROWS (monotone TryAdd) — no
mid-epoch dip below the true occupancy — and `WorkingOccupied = NumRows() > 0`
stays EXACT for a DiffTable input, exactly as under R-MONO. The N-1 signed-count
contingency reopens ONLY if band-(a2) ever becomes an incremental shrink of a
standing `current` (OQ-MODEL overturned) — which the ruling forbids. ∎

### EDIT SPEC E3b — InstanceStore.h:21-27 header note

REPLACE (InstanceStore.h:21-27):
```cpp
// working_count DROPPED (A.3.1; N-1 carried): StateCellStore tracks a signed
// per-group member count; here occupancy is `current->NumRows() > 0`. This is
// exact under R-MONO (band-(a) plus is a monotone TryAdd, no mid-epoch dip).
// N-1 (d1-pinned §CARRIED): when R-DIFF lands (D3.a) a mid-batch retraction
// could make NumRows()-as-occupancy a lie — revisit re-introducing a signed
// count then; recorded here so it is not lost.
```
WITH:
```cpp
// working_count DROPPED (A.3.1; N-1 CLOSED under OQ-MODEL, D3.a.2): occupancy
// is `current->NumRows() > 0`. EXACT under the FULL-RESCAN model
// (OQ-MODEL / d3a-ruling-brief.md): `current` is built ONLY by the band-(a)
// monotone rescan (TryAdd) and emptied ONLY at Seal / RecycleCurrent — it
// never shrinks mid-epoch, even for a DIFFERENTIAL (deletable) summarized
// input. An input retraction is a Recycle-FREE Touch+rescan (band-(a2')
// removal arm) that reads the epoch-net input and rebuilds `current` from
// empty, so NumRows()-as-occupancy stays a truth. REOPENS only if band-(a2)
// ever becomes an incremental per-row shrink of a standing `current`
// (OQ-MODEL overturned).
```

### EDIT SPEC E3c — InstanceStore.h:159-160 WorkingOccupied comment

REPLACE (InstanceStore.h:159-160):
```cpp
  // N-1 (carried): under R-DIFF a mid-batch retraction breaks this equivalence
  // — revisit a signed count at D3.a.
```
WITH:
```cpp
  // N-1 (CLOSED, OQ-MODEL, D3.a.2): under R-DIFF an input retraction is a
  // Recycle-free Touch+full-rescan that rebuilds `current` from empty (no
  // incremental shrink), so the equivalence holds. Reopens only if OQ-MODEL
  // is overturned.
```

Pre-registered verdict for E3b/E3c: **[BYTE]** — pure header comments, no
`constexpr`/template surface touched; the InstanceStore is header-only but no
byte of generated or compiled code depends on comment text. Gate family: none
(runtime-unit InstanceStoreTest unaffected; the WorkingOccupied SEMANTICS are
unchanged — the note only documents that they now hold under R-DIFF too). The
D3.a.1 unit `DeathRebirthCycleRebindsIidAndTogglesTouchedFlag` already exercises
Recycle+refill keeping NumRows exact; the e5 carrier (b4 witness) exercises
shrink-without-Recycle keeping it exact — the two together are the standing
liveness of this close (no new unit required; recorded as the L9-sibling
negative in §6).

===============================================================================
## §4 H-20 RIDER + demand_diff_input_1 (ADV-8) — the in-file comment edit
===============================================================================

### 4.1 EDIT SPEC E3d — demand_diff_input_1.dr comment (ADV-8)

demand_diff_input_1.dr:6 carries a stale anchor AND a stale disposition: it
cites `Build.cpp:1344` (the fence's live anchor is Build.cpp:1530/:1553-1555 per
substrate §3 F-A / XC-8; :1344 is D3.a.1-era drift) and describes the case as
"Rejected under -demand-instance" — which e1 FLIPS to compiling. Disposition is
b4's (H-17: promote to a full golden case vs keep as a lifted-fence compile
witness); b3 owns the in-file comment edit, conditional on which branch b4
takes. The `.drflags` is `-demand -demand-instance`.

**Branch A — b4 keeps it as a lifted-fence COMPILE witness** (the case still
compiles clean under `-demand-instance`, no stdout golden; it pins that the
fence LIFTED). REPLACE demand_diff_input_1.dr:3-10 (the header comment):
```
; demand_diff_input_1 -- fence-(iii) witness: a bound #query whose demanded
; body summarizes a @differential (deletable) message input. Rejected under
; -demand-instance by the Program::Build pre-pass CanReceiveDeletions() check
; (Build.cpp:1344); ALL FOUR modes diagnostic. This single-hop non-recursive
; body passes the flat demand transform, so it REACHES the control-flow fence
; (unlike the recursive-content shape). Stands as the D3.a belt: when the
; retraction surface lands, deletable demanded inputs become supported.
```
WITH:
```
; demand_diff_input_1 -- the LIFTED fence-(iii) witness: a bound #query whose
; demanded body summarizes a @differential (deletable) message input. As of
; D3.a.2 this COMPILES under -demand-instance (the input net-removals a2
; trigger + the Present-filtered rescan); the Build.cpp:1530/:1553-1555
; diff_input reject arm was lifted (F-A). This single-hop non-recursive body
; passes the flat demand transform and now lowers to a keyed InstanceStore
; over a differential input. Its cyclic/recursive-content siblings
; (demand_cyclic_1 / demand_recursive_content_1) still reject.
```
(and its runall.sh diagnostic-list entry is REMOVED — b1/b4 ride that per e1;
b3 flags the dependency, does not own the runall.sh edit.)

**Branch B — b4 promotes it to a 4-mode golden** (adds a `.main.cpp` driver +
`goldens/demand_diff_input_1.stdout`): then the comment above still applies
minus the "no stdout golden" implication; b3's spec is identical text, and b4
owns the driver/golden. Either branch uses the SAME corrected anchor/
disposition text above.

Pre-registered verdict: **[BYTE]** on all OTHER cases; this case's verdict flips
from all-4-modes-DIAGNOSTIC to COMPILE (Branch A) or GOLDEN (Branch B) — the
one expected non-[BYTE] line, owned by b1's fence lift + b4's disposition, NOT a
b3 behavior change (b3 edits only the descriptive comment). Gate family:
the runall.sh diagnostic-list / suite verdict.

### 4.2 H-20 stale cross-refs (rider)

The in-source "Build.cpp:999" cross-refs (Rel.cpp V-INST-DRAIN comment ~:4507
region; Procedure.cpp ~:326) are STALE-BY-DRIFT (XC-6: the live monotone append
is Build.cpp:1110-1114). These ride b1/b2 when those files are touched for the
fence lift / provisioning; b3 does not own them but records that E3a's
neighborhood (Database.cpp) carries no such stale ref (verified — the RIDER
block cites no Build.cpp line).

===============================================================================
## §5 THE PINNED COUPLING STATEMENT (the OD-15 idiom — one block every
##    affected future design must quote)
===============================================================================

> **THE D3.a.2 FIVE-WAY INPUT-QUIESCENCE COUPLING.** For a keyed-instance store
> over a differential input, five landed mechanisms interlock to make each
> touched key rebuild EXACTLY the epoch-net live content, once, with no
> resurrection and no over-retraction. State this in every design that touches
> the input rebuild band, the demand-liveness gate, or the entry-point
> structure:
>
> 1. **NETTING** (handler `NetBatch`, per channel, at the message boundary):
>    same-batch ± of one row annihilates before any fold, so every frontier row
>    (demand-add/removal, edge-add/removal) is a GENUINE net change — no arm
>    ever drains a self-cancelling pair. (O-3/OB7.)
> 2. **TouchedFlag** (append-once, Seal-reset): the SAME-EPOCH belt. Whichever
>    arm (a0 death, a1 birth, a2 edge-add, a2' edge-removal) first touches key K
>    does the ONE full rescan of K's net content; every later arm skips. Drain
>    order among the arms is therefore behavior-neutral — the property the
>    R-A2-TRIGGER two-drains-no-recycle ruling rests on.
> 3. **V-INST-FRESH** (band-entry belt, always-on generated fprintf+abort):
>    `current` is EMPTY at first touch (Seal/RecycleCurrent the sole emptiers).
>    This is why (a) the input arms need NO Recycle (the ruling), (b) the rescan
>    rebuilds from empty so occupancy stays exact (the N-1 close, §3), and (c) a
>    stray non-empty current (a real bug) still aborts.
> 4. **THE Present CONJUNCT** (`input.Present(s)` in the shared rescan mold,
>    ADV-3, ALL THREE sources incl. a1-birth): the mold materializes exactly the
>    epoch-net LIVE input rows (T-5/T-7: at band time `Present(s)` == post-commit
>    `kInI` for this epoch, because input counters are final at the fold and the
>    flow's claim drains touch flags only). No dead input row is resurrected on
>    any rescan — the OB8 lemma's operational content.
> 5. **THE DEMAND-LIVENESS GATE** (`demand.Present(dq)` on a2/a2', the R-3 gate):
>    the CROSS-EPOCH belt. A dead key still binds an iid (append-only, no
>    tombstone); the gate skips a rebuild whose demand is committed-absent
>    (differential demand) or trivially-present (monotone demand — sound by
>    IRREVOCABILITY, §1.2). This is the ONLY correct liveness signal; iid
>    existence is NOT one.
>
> **The interlock per epoch shape:** L-EDGE (add/retract) — 4+5 carry (input
> net-state via Present; demand frozen ⇒ gate reads committed); L-MONO (e5) —
> 4 carries, 5 degenerates to irrevocability; L-DEMAND — 2+3 carry, 4/5 idle
> (input frontiers empty). L-COMBINED (no such entry today) — 5's committed-vs-
> post-fold justification FAILS; pinned as a DOCUMENTARY fence (§1.4): any
> future multi-message batch API MUST re-derive it before landing.
>
> **C-REC (precondition, cross-lane).** Mechanisms 4 and the OB8 lemma require
> the input's counters be FINAL at band time — a fixpoint-refired (recursive-
> content) input would violate it. The recursive-content fence surviving F-A's
> lift (Build.cpp:1533-1540, b1's e1) is therefore LOAD-BEARING FOR CORRECTNESS,
> not just for scope. Any future slice that admits a recursive-content demanded
> input re-derives this coupling first.

This block subsumes and extends the D3.a.1 OD-15 three-way coupling
(netting/TouchedFlag/V-INST-FRESH) with the two D3.a.2 additions (the Present
conjunct + the gate's cross-epoch role) and the recursive-content precondition.

===============================================================================
## §6 d7 LIVENESS-BY-PERTURBATION ROWS (b3's — the e5 carrier as a LIVE vehicle)
===============================================================================

The e5 carrier (differential input × MONOTONE demand — b4's witness, e6) is a
first-class perturbation vehicle: it drives the full P-STORE-side machinery
with NO death, so it isolates the "belts fire on shrink without a death" claim.
Rows run at stage (d) in the prototype worktree, abort texts recorded, all
reverted (the mold's L-table idiom).

| # | belt / claim | procedure | expected |
|---|---|---|---|
| L3a | **V-INST-PARTITION belt under a death-free shrink** | e5 witness (diff input, mono demand) with an edge-RETRACT epoch on a live-demanded key; scratch-drop `++dropped;` (or `++carried;`) in the band-(b) drop scan (Database.cpp:2576/:2584) | generated `V-INST-PARTITION: instance %u …` abort on the FIRST e5 edge-retract epoch — proving the belt is LIVE for a differential store with n_death==0 (P-STORE, not P-DEATH, arms it) |
| L3b | **the a2' removal arm actually drains (F3b)** | e5 witness edge-retract epoch; scratch-remove b2's `input_removal_frontier` ClassifyVector read arm | the a2' arm drains an empty/mis-threaded frontier ⇒ stuck-present ⇒ eqgate (flat==nested) DIVERGES + golden diverges (no validator abort — this is a SILENT miscompile the oracle catches; records why F3b's classify arm is soundness-critical) |
| L3c | **the Present rescan conjunct (ADV-3), on the e5 carrier** | e5 witness edge-retract; scratch-drop the `input.Present(s)` conjunct from b2's rescan mold | the retracted edge's dead row is resurrected on the a2' rescan ⇒ over-materialization ⇒ eqgate + golden DIVERGE (the ADV-3 must-have, exercised on the monotone-demand carrier so the failure is not masked by a demand death) |
| L3d | **V-INST-EMITTED balance across the divergence (F3a — POSITIVE only)** | e5 witness compiled clean | exit 0, suite green: the 2-vs-2 balance passes with NO death for a differential store. NO negative perturbation (the check is already correct across P-STORE/P-DEATH; nothing to make abort) — recorded as an audit-verified POSITIVE, not a directed teeth test |
| L3e | **HP-7 disarm on the e5 carrier (negative, L9 sibling)** | debug e5 witness run post-`, false` ctor with an edge-retract shrink | the death-free store's shrink does NOT trip the monotone Seal belt (belt off for P-STORE stores); DebugValidate green — the N-1 close's standing liveness (§3) |

Notes on what is NOT a b3 L-row:
- The **RIDER discharge (E3a)** and the **N-1 close comments (E3b/E3c)** are
  DOC-ONLY: [BYTE], no runtime surface, no L-row (the mold: doc discharges get
  no perturbation).
- The **L-MONO gate degeneration** (§1.2): `demand.Present` is
  monotone-trivially-true, so there is nothing to perturb into a wrong answer
  (the redundant belt cannot be made load-bearing). Its correctness is the
  IRREVOCABILITY argument, refereed by the e5 witness's answer-identity
  (eqgate), not by a directed abort — recorded, no L-row.
- The **L-COMBINED fence** is documentary (no emitting-code assert); it has no
  perturbation vehicle by construction (no combined entry exists to run). Its
  teeth are the pinned coupling statement binding a future API — a review gate,
  not a d7 belt.

===============================================================================
## §7 PRE-REGISTERED VERDICTS (b3's touched surfaces) + FINDINGS ROLL-UP
===============================================================================

### 7.1 Per-surface verdicts (all b3 edits are comments — behavior-inert)

| surface | edit | verdict | gate family |
|---|---|---|---|
| Database.cpp:2494-2495 RIDER | E3a (discharge) | **[BYTE]** all generated artifacts + all binaries (source `//` comment; emitted gate :2500-2505 untouched) | none (doc) |
| InstanceStore.h:21-27 | E3b (N-1 close) | **[BYTE]** (header comment; no template/constexpr byte) | none (doc); InstanceStoreTest unaffected |
| InstanceStore.h:159-160 | E3c (N-1 close mirror) | **[BYTE]** | none (doc) |
| demand_diff_input_1.dr:3-10 | E3d (ADV-8) | case verdict flips DIAGNOSTIC→COMPILE/GOLDEN (owned by b1's fence lift + b4's disposition); the COMMENT edit itself is inert | suite diagnostic-list (b1/b4) |
| all other corpus / pinned / data | — | **[BYTE]** (b3 mints no code) | suite / 20-pin regen |

### 7.2 Findings

- **F3a** — V-INST-EMITTED balance is OP-PRESENCE-keyed (P-DEATH), not
  P-STORE-keyed; e5 balances 2 vs 2; NOT edit-needed. (§2 A2.)
- **F3b** — the e5 carrier's a2' arm needs b2's `input_removal_frontier`
  ClassifyVector read arm (H-7); its absence is a silent stuck-present
  miscompile the eqgate catches (L3b). A soundness precondition of L-EDGE's
  retract half; b2 owns the edit, b3 pins the dependency.
- **F3c** (recorded, no edit) — the whole e5 P-STORE-side audit passes with
  ZERO death-side edits because the §7 d2 anti-fold ruling kept the death
  machinery P-DEATH-gated and the store/effect machinery P-STORE-gated; the
  e5 divergence is absorbed with no new branch. This is the design payoff the
  d2 ruling predicted (d3a1-substrate §7 ground (1)); recorded as the audit's
  headline.

### 7.3 Cross-lane obligations b3 depends on (pinned, not owned)

1. **b1** keeps the recursive-content fence (Build.cpp:1533-1540) live through
   F-A's lift — C-REC; the OB8 lemma is false without it.
2. **b2** lands the shared-mold `input.Present(s)` conjunct on ALL THREE
   rescan sources (a1/a2/a2', ADV-3) — mechanism 4 of the coupling; L3c is its
   teeth.
3. **b2** lands the `input_removal_frontier` ClassifyVector arm — F3b; L3b is
   its teeth.
4. **b2** carries the R-A2-TRIGGER gate-set IDENTITY onto the a2' removal arm
   (the §1.0 gate copied verbatim) — the e4 lemma covers a2' ONLY under this
   identity; a gate divergence between a2 and a2' voids §1.
5. **b4** picks demand_diff_input_1's disposition (H-17); E3d's text serves
   both branches.

END b3-design.md
