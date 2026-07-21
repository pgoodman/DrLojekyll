======================================================================
COMMITTED AT THE §20 EPOCH-CLOSE CHECKPOINT (2026-07-21). This is the
adjudicated BINDING implementation contract for its diff, verbatim from
the session scratchpad (d1b/d1b-design.md); its ADJUDICATION LEDGER records every
folded amendment, and the KeyedInstances.md §19(K)-(O) landing records
carry the owner rulings (RAT-1..RAT-10) that resolved its open items.
======================================================================

# D1.b DESIGN — DR-IR op family + EffKinds + validators + dump grammar

**Diff-on-pseudocode of `lib/DeltaRel/DeltaRel.{h,cpp}` + `lib/DeltaRel/Format.cpp`
at tip `4d92d255` (keyed-instances; D1.a landed).**

Scope (per KeyedInstances.md §19(C) D1.b block, d1-pinned §3 D1.b entry): land the
three keyed-instance DROpKinds, the EffKind additions HP-11 earns, the DROp/DRInstance
fields, `BuildSubgraphInstanceOps` (the mint) **GATED OFF**, the linearizer
key/stratum/effect cases, the five instance validators, and the dump-grammar rows —
with the mint **structurally unreachable now** so every prediction P-D1b.1..3 holds.

Precedence obeyed: **d1-pinned.md WINS** (HP-1..HP-3, HP-6, HP-8, HP-10, HP-11, HP-14,
HP-17) > d1-design-consolidated.md (§A.2/§A.5/§A.7, §B D1.b) > d1-desired-states.md
(§B.3 STAGE-1, §B.4 nested illustrative). Ledger rulings §19(I) (OD-2/OD-5/OD-6),
§19(K) (RAT-1/RAT-2 + the **dangling-handle caveat**, addressed in §3.6 below).
Grammar law: t2b-grammar.md + t2-dump-spec.md v3.4 pins p10-p14.

**DeltaRel unchanged since 99f211f5** (`git diff --stat 99f211f5 4d92d255 -- lib/DeltaRel/`
= empty), so every lane-deltarel.md anchor holds at tip. All line refs below re-verified
at `4d92d255`.

---

## §0. VERIFIED CURRENT-STATE PSEUDOCODE (the diff baseline)

### 0.1 DROpKind — 15 members, tail `kStateSeal` (DeltaRel.h:114-139)
```
enum class DROpKind : uint8_t {
  kCrossover(0) … kIngestFold(12), kGroupUpdate(13), kStateSeal(14)   // TAIL
};                                                                     // :139 closing }
```
Appending a kind is **not free** — it must also land in FIVE places or a build/runtime
tripwire fires (lane §1, re-verified):
- `DROpKindName` switch, Format.cpp:95-115 — else the no-`default` `-Wswitch` warning
  at compile AND the loud-abort at :113-114 at runtime.
- `kAllKinds[]`, Format.cpp:779-787 — else `census_total != flow.ops.size()` aborts
  (Format.cpp:796-802).
- `DROpStratum` switch, DeltaRel.cpp:3133-3203 (`default → 0u`).
- `op_band` lambda, DeltaRel.cpp:3360-3390 (`default → 0u`).
- the census recount in `ValidateDROps` (DeltaRel.cpp:2872-2882 `expect(...)` block) —
  a new kind with a nonzero count and no `expect` line mismatches nothing today (it is
  simply uncensused), so its recount line is a **correctness add**, not a tripwire.

**[ADJ:crit-correctness-2] (LOW — the FIVE list is NOT exhaustive; the extra sites are
moot at D1.b, INTENTIONAL defaults at D2.b).** Four MORE `switch(op.kind)` sites carry a
`default` the new kinds silently flow through: `:2432` validator dispatch (default:2638),
`:3221` op_pivot_hint (default → `~0u`), `:3497` DepScope (default → `kEpoch`), `:3765`
op_input_round (default → `nullopt`) — plus the `:581` op p-rule renderer, which the design
DOES extend (§4.5/§4.6). All moot at D1.b (0 minted). At D2.b these defaults are the INTENDED
classification for seed-context instance ops: op_pivot_hint `~0u` (no pivot), DepScope
`kEpoch` (seed, not round — a wrong scope only mis-scopes a hazard edge, with V-LINEAR/V-LOOP
as backstop, hence LOW), op_input_round `nullopt` (non-round). Recorded here as intentional so
"FIVE places" is not read as the closed set of touch-points.

### 0.2 EffKind — 10 members, kState* tail (DeltaRel.h:74-85)
```
enum class EffKind : uint8_t {
  kVecAppend(0), kVecDrain(1), kVecClear(2), kCounter(3), kFlagRead(4),
  kFlagWrite(5), kInIReadFrozen(6), kStateFold(7), kStateEmit(8), kStateOld(9)  // TAIL
};
```
Mirror-tables: `EffKindName` (Format.cpp:117-132, no `default` → `-Wswitch`) and the
render arg-shapes in `emit_effect` (Format.cpp:411-459). The linearizer hazard switch
(DeltaRel.cpp:3280-3320) handles all 10 explicitly — its `default: break;` (:3318-3319)
is **dead today** (this is P-D1b.3's baseline: no corpus flow reaches it).

### 0.3 key_of / op_table_id / key_less / bands (DeltaRel.cpp:3400-3477)
- `op_table_id` (:3400-3408): first non-null of
  `table_op_table ? : product_table : agg_table : negate_table : ingest_table : fire_table`,
  returns `t->id + 1u` (NULL → 0). **NO `pub_table` / `publish_target` branch** — HP-3's
  realization keys the instance ops on the EXISTING `table_op_table` arm.
- `op_sign` (:3409-3423): `table_op_sign → crossover_sign → product_sign → fire_sign`.
- `key_of` (:3445-3469): `struct Key{lead,stratum,band,table_id,sign,ctor}`. Cases:
  eager NegateGate → `{0,0,0,tid,0,oi}`; IngestFold → `{0,0,0,tid,ingest_sign,oi}`;
  **CommitSweep → `{2, max_stratum+1, 9, tid, 0, oi}`** (:3458-3459);
  **StateSeal → `{2, max_stratum+1, 10, tid, 0, oi}`** (:3461-3465, band 10 is a
  key_of-local constant — NOT from `op_band`); DEFAULT → `{1, op_stratum, op_band, tid,
  op_sign, oi}`.
- `key_less` (:3470-3477): lexicographic `lead→stratum→band→table_id→sign→ctor`; the
  **sign tie-break (5th, "− before +", :3475) fires ONLY when table_id is equal** — this
  is the entire mechanism HP-3/OD-2 rest on.
- `op_band` (:3360-3390) is the phase-op band (0..9); `default → 0`. Note kStateSeal's
  band 10 and (below) kInstanceSeal's band 11 come from **key_of**, not op_band — a
  StateSeal/InstanceSeal is never a `lead==1` phase op so op_band never sees it.

### 0.4 DROpStratum (DeltaRel.cpp:3133-3203)
Free helper shared by linearizer + dump. Phase kinds resolve a `*_stratum` map else
`return 0u`; `kStateSeal → 0u` (:3198); **`default → 0u`** (:3200-3201, negate gates /
commit sweeps / ingest folds). Per A.2.4/F-OPS-7 the two LIVE instance kinds must
**ValidatorFail on a missing `instance_stratum` entry**, never silently `return 0u`.
**[ADJ:crit-pins-1]** The GROUP_UPDATE case at :3186-3197 is NOT "the exact precedent" — on a
map-MISS it falls through to `return 0u` (:3197, verified); its false-negative-space fix only
gave it its OWN case (vs the shared `default`), NOT a fail-loud on miss. ValidatorFail-on-miss
is therefore a deliberate STRENGTHENING beyond the kGroupUpdate shape (justified: at mint the
`instance_stratum[sid]` entry is ALWAYS written §3.2, so a miss is a mint bug, exactly the
fail-loud-on-invariant house style, ValidatorFail callable at :32). Record it as an invention,
not a precedent-follow.

### 0.5 The effect-hazard switch (DeltaRel.cpp:3280-3320)
Per `op.effects` (the A-3 op-level union covers per-arm too). Enrolling: kVecDrain
(read-vec), kVecAppend/kVecClear (write-vec), kCounter (write-flag `counter_table`),
kFlagWrite (write-flag `write_table`), kFlagRead (read-flag `read_table`), **kStateFold
(write-flag `value_table`)**, **kStateEmit (read-flag `read_table`)**. No-hazard:
kInIReadFrozen (:3302-3303 `break`), **kStateOld (:3316-3317 `break`)**. `default: break`.

### 0.6 Census recount (DeltaRel.cpp:2676-2882 = the "2818-2851 mold")
`ValidateDROps` recomputes each kind's expected count as a **pure function of the
discovery inputs** (impl/context/query/scc_map), NEVER from `flow`:
- `count_kind` lambda (:2854-2862) + `expect(kind, want, what)` (:2863-2871) fprintf+abort
  on mismatch.
- `expect` calls (:2872-2882): seed/chain/fire/claim/retire/rederive/filter/sweep/ingest
  /group-update/state-seal. **state-seal expects `exp_group_update`** (:2882) — the
  bijection precedent for kInstanceSeal.
- The GROUP_UPDATE recount (:2833-2851 `add_gu_key`) iterates `query.Aggregates()` /
  `query.KVIndices()` and builds an **order-free key multiset** (agg_table, prov, algebra,
  view.UniqueId()); compared at :2911-2935. **This is the exact mold for the instantiate/
  death/seal recount over `query.RecognizedSubgraphs()`** — with the §3.6 liveness caveat.
- V-AGG-EFFECT (:2937-3010): per-op effect-multiset totality for kGroupUpdate/kStateSeal.
  **This is the V-INST-EFFECT mold.** kStateSeal's leg (:2992-2996) asserts
  `effects.size()==1 && effects[0].kind==kStateFold` — the seal-effect precedent.

### 0.7 Format.cpp census + section layout (Format.cpp:328-819)
Sections in order: `deltarel` header → vecs → branches → joins → **ops** → rounds:
→ deps: → census:. **Empty-section law (p11)**: vecs/branches/joins each render their
leading `\n` **only when non-empty** (`if (!flow.vecs.empty()) os << "\n"`, :342, :373,
:388; ops guarded at :575). rounds:/deps:/census: are **unconditional**. `kAllKinds[]`
(:779-787) is the 15 enum-order members; census renders `Name=count` for each even at 0
(:788-795); completeness abort at :796-802. Spelling tables (18 of them, :42-266) use
the **no-`default` + trailing fprintf+abort** idiom (compile-time `-Wswitch` + runtime
backstop) — the T2b law.

---

## §1. THE HP-11 DECISION — made BEFORE any spelling row

**Question (HP-11 / OD-6):** do `kInstanceEmit` / `kInstanceOld` earn existence, or
collapse into the landed `kStateEmit` / `kStateOld` (DeltaRel.h:83-84)? And, same bar,
re-examine `kInstanceSealSwap` under HP-1's self-lowered seal carrier. **Bar:** a
DISTINCT hazard target or a census distinction — the bar `kInstanceDemand` clears.
**Owner nudge (§19(I) OD-6):** *lean collapse unless a distinct hazard/census earns the
members.*

### 1.1 What the InstanceStore words ARE, mapped onto the kState* hazard vocabulary
The `InstanceStore` (A.3.1) is the `StateCellStore` transpose: a two-word cell per
instance — **`current` (= the working word)** and **`sealed`/`frozen` (= the sealed
word)**. The kState* effects are exactly a two-word-cell hazard vocabulary:
- `kStateFold` = a **WRITE of the working word**, hazard keyed on `value_table`
  (DeltaRel.cpp:3304-3311).
- `kStateEmit` = a **valued READ of the working word** (E2 RAW after the fold), keyed on
  `read_table` (:3312-3315).
- `kStateOld` = a **frozen READ of the sealed word, NO hazard** (:3316-3317 `break`).

The band-(b) partition reads (A.2.3, A.3.5) are:
- born `{(F,T)}` publish: scan `current` (Find in frozen) → **reads the working/current
  word** → this is a `kStateEmit` read of `current`.
- dropped `{(T,F)}` retract: scan `frozen` (Find in current) → **reads the frozen/sealed
  word** → this is a `kStateOld` read of `frozen`.

So the design's `kInstanceEmit` ("reads current, RAW after rebuild", A.2.2) and the
landed `kStateEmit` ("valued read of working, E2 RAW after every fold") are **the same
hazard on the same resource** — a read of the current/working word keyed on the state
table (here `pub_table`), producing a RAW after whatever wrote it. Likewise
`kInstanceOld` ("reads frozen, no hazard") and `kStateOld` ("frozen sealed read, no
within-band hazard") are **byte-identical in hazard semantics AND in spelling meaning**.

### 1.2 DECISION — COLLAPSE emit/old; COLLAPSE sealswap; KEEP rebuild/demand
| design kind          | ruling  | realized as (D1.b)                              |
|----------------------|---------|-------------------------------------------------|
| `kInstanceEmit`      | COLLAPSE| `kStateEmit(read_table = pub_table)`            |
| `kInstanceOld`       | COLLAPSE| `kStateOld(read_table = pub_table)`             |
| `kInstanceSealSwap`  | COLLAPSE| `kStateFold(value_table = pub_table, sign = 0)` |
| `kInstanceRebuild`   | **KEEP**| NEW EffKind (write, `value_table`+`sign`)       |
| `kInstanceDemand`    | **KEEP**| NEW EffKind (frozen read, `read_table`, no haz) |

**Net new EffKinds = 2** (`kInstanceRebuild`, `kInstanceDemand`), down from the design's
proposed five. Every spelling row and effect-multiset below follows from THIS table.

### 1.3 Why emit/old collapse (they do NOT clear the bar)
- **Hazard target — NOT distinct.** `kStateEmit` already accepts an arbitrary
  `read_table`; keyed on `pub_table` it reads the current word and produces the RAW after
  the rebuild write — the exact "RAW after rebuild" semantics A.2.2 asks for. `kStateOld`
  is already `break` (no hazard) — the exact "reads frozen, no hazard" semantics. There
  is nothing a `kInstanceEmit`/`kInstanceOld` case would do in the hazard switch that the
  existing `kStateEmit`/`kStateOld` cases do not already do. (Contrast `kInstanceDemand`:
  a `kInIReadFrozen` fold would **blind the census to the F1 silent-drop** — A.2.2 — so
  demand clears the bar; emit/old carry no such census freight.)
- **Census — NOT distinct.** V-INST-EFFECT counts effect KINDS per instantiate/death op
  in its OWN loop; it never sees kGroupUpdate ops and V-AGG-EFFECT never sees instance
  ops (they are disjoint `op.kind` branches — DeltaRel.cpp:2949/2992 vs the new §5.1
  loop). Reusing kStateEmit/kStateOld inside instance ops causes **no cross-validator
  collision**. V-INST-EFFECT still asserts "exactly 1 emit + 1 old" by counting
  kStateEmit==1 / kStateOld==1 — the HP-6 (F,T)/(T,F) partition is enforced by the
  LOWERING two-scan and the p-rule, never by the effect NAME.
- **Spelling honesty.** `kStateEmit(%table:4)` renders "reads current"; `kStateOld(%table:4)`
  renders "reads frozen sealed". For the InstanceStore, current==working and
  frozen==sealed, so the collapse is not merely hazard-safe but **semantically exact** —
  no dishonest reuse.

### 1.4 Why kInstanceSealSwap collapses to kStateFold (the mold-faithful choice)
HP-1 self-lowers the seal from `kInstanceSeal`'s own dispatch (not EmitCommitSweep's
exits) and enrolls it in V-INST-EMITTED. The seal's data action is `sealed := current`
for touched instances — a **WRITE of the state word**. The landed `kStateSeal` op already
realizes its own `sealed := Emit(working)` pointer-swap as a **`kStateFold(sign=0)`**
(DeltaRel.cpp:750-756, verified: `seal_fx.kind = EffKind::kStateFold`). So the faithful
realization of `kInstanceSealSwap` is **`kStateFold(value_table = pub_table, sign = 0)`** —
the same effect kind the peer seal already uses.
- **Hazard — not distinct.** A write of the state word keyed on the state table is exactly
  `kStateFold`'s shape. No new hazard target.
- **Census — not distinct.** V-INST-EFFECT can assert "kInstanceSeal carries exactly 1
  kStateFold(sign=0)" — the DIRECT analogue of V-AGG-EFFECT's kStateSeal leg
  (DeltaRel.cpp:2992-2996). The DROpKind `kInstanceSeal` (kept — OD-6, band 11, its own
  census counter, V-INST-EMITTED enrollment) already carries all the census signal the
  seal needs; a distinct EffKind adds nothing.

**One consequence, flagged for the D2.b desired-state refresh (HP-13(b) duty).**
`kStateFold` ENROLLS A WRITE hazard on `pub_table`. Because the seal trails at band 11
and the instantiate sits at band 0, this derives forward WAW/WAR belt edges
instantiate→seal (e.g. `op.<inst> -> op.<seal> WAW epoch`). **The illustrative
d1-desired-states §B.4 renders NO edge touching the seal — that block is WRONG on this
point** (it assumed a no-hazard sealswap). §B.4 is ILLUSTRATIVE/UNBLESSABLE; the mandatory
D2.b re-derivation corrects it. The kStateSeal precedent confirms the belt is real:
kStateSeal's kStateFold write already produces WAW/WAR edges to the emit_touched reads in
avg_weight. The band key already orders seal-after-instantiate; the edge is a derived
belt, forward by construction, never a false hazard (V-BAND-HAZARD green).

### 1.5 Why kInstanceRebuild / kInstanceDemand are KEPT (they clear the bar)
Pinned justified-as-is by HP-11 (line 205-207); I ratify with the bar test:
- `kInstanceDemand` — **census-distinct**: a frozen read of the drained demand KEY,
  distinct from the `kVecDrain` row read; folding it into `kInIReadFrozen` would blind the
  census to the F1 silent-drop (A.2.2). Clears the bar exactly as HP-11 states.
- `kInstanceRebuild` — **structural-distinct**: the ±1 STRUCTURAL sign is the death-vs-birth
  regime discriminant AND the store-op selector (TryAdd vs Recycle); V-INST-EFFECT's
  regime totality keys on its sign presence (birth `+1`, death `-1`). Keeping it a named
  kind (rather than overloading `kStateFold`, whose semantics is "value combine", for a
  structural row-set rebuild) keeps the lowering dispatch and the validator unambiguous.
  I considered collapsing it to `kStateFold(±1)` (the hazard alone would be correct, a
  write on the state word) and REJECT: it is the sole write on the instantiate/death op
  and the regime signal — a named kind is worth two enum slots. (HP-11 is a hard pin; I do
  not relitigate, only record the collapse was weighed and declined.)

**Result:** the EffKind family gains exactly `kInstanceRebuild`, `kInstanceDemand`. All
five spelling rows the design fleet expected shrink to two new rows + reuse of the three
landed kState* rows. Every §4/§5 artifact below is written to this outcome.

**[ADJ:crit-grammar-4] (LOW — the collapse restyles the §B.4 exemplar's EFFECT bytes too).**
Beyond the stale seal-EDGE §1.4 already flags, the HP-11 collapse renames the exemplar's
effect spellings: `kInstanceEmit → kStateEmit`, `kInstanceOld → kStateOld`,
`kInstanceSealSwap → kStateFold` (§B.4 d1-desired-states:485,492; consolidated §A.2.3
:329-354 now carry STALE effect names). Authorized by the ruling; hits NO D1.b golden
(`demand_neighborhood_witness` enters `cases/` only at D2.c, §5). Flagged so the D2.c golden
bless reflects the kState* spellings and the §B.4/§A.2.3 multisets are refreshed, not read as
current.

---

## §2. THE DIFF — exact insertion-point pseudocode (DeltaRel.h)

### 2.1 DROpKind — three kinds appended after kStateSeal (DeltaRel.h:138, before `};`)
```
  kStateSeal,      // (14) — existing tail
+ kSubgraphInstantiate,  // (15) keyed-instance BIRTH/REBUILD + band-(b) publish_touched.
+                        //   Sole deriver of pub_table. table_op_table=pub_table,
+                        //   table_op_sign=+1. (OD-6, A.2.1)
+ kInstanceDeath,        // (16) whole-instance DEATH, its OWN op (§18(B) mandate). NO
+                        //   fold/counter — the zero-counter totality is the teeth.
+                        //   table_op_table=pub_table, table_op_sign=-1. MINTED ONLY when
+                        //   TableIsDifferential(demand_table) (R-DIFF); ships INERT at
+                        //   D1.b/D2.b (HP-17).
+ kInstanceSeal,         // (17) trailing pointer swap (kStateSeal peer, band 11 via
+                        //   key_of). Self-lowered from its own dispatch (HP-1/OD-5).
};
```
Underlying `uint8_t`, 15→18 members, room to 255. **Belt (HP-10):** all three land in
`kAllKinds[]` at D1.b (§4.3) — pre-registered behind the census-line churn.

### 2.2 EffKind — two kinds appended after kStateOld (DeltaRel.h:84, before `};`)
```
  kStateOld,       // (9) — existing tail
+ kInstanceRebuild,  // (10) WRITE of the store `current` word; hazard keyed on pub_table
+                    //   (reuses value_table); carries sign ±1 (STRUCTURAL: birth +1 /
+                    //   death -1 → TryAdd vs Recycle). HP-11 KEEP.
+ kInstanceDemand,   // (11) frozen READ of the drained demand KEY (reuses read_table);
+                    //   NO hazard (HP-8, the kInIReadFrozen precedent). Census-distinct
+                    //   (F1 silent-drop). HP-11 KEEP.
};
```
(No `kInstanceEmit`/`kInstanceOld`/`kInstanceSealSwap` — collapsed per §1.)

### 2.3 DROp fields — the instance payload block (DeltaRel.h, near :523, after the
GROUP_UPDATE block)
Per HP-3: **NO new `pub_table` field** — both instance ops set `table_op_table = pub_table`
(reusing op_table_id's existing 1st `?:` arm) with `table_op_sign = ∓1`. The genuinely
new fields:
```
+ // ---- SUBGRAPH_INSTANTIATE / INSTANCE_DEATH / INSTANCE_SEAL (kind ==
+ //      kSubgraphInstantiate | kInstanceDeath | kInstanceSeal) --------------
+ std::optional<QueryView> demanded_view;   // the recognized subgraph's demanded MERGE
+ TABLE *demand_table{nullptr};             // the demand relation's model table (drained)
+ TABLE *input_table{nullptr};              // the summarized monotone input (rederive leaf)
+ unsigned instance_store_id{~0u};          // dense per-forcing store id (V-INST-ORDER key)
+ unsigned forcing_index{~0u};              // -> query.RecognizedSubgraphs()[i].forcing_index
+ // (table_op_table = pub_table, table_op_sign = ∓1 — REUSED, per HP-3.)
```
- `demand_table`/`input_table`/`instance_store_id` back the effect args + the DRInstance
  descriptor render + V-INST-ORDER.
- **`context_col_sources` is NOT added at D1.b** — CONFIRMED. The V-ALPHA binding-source
  machinery (`PlanNode::bound_col_sources`, the instantiate `context_col_sources`, arms
  A/B) is D2.b (d1-pinned §A.4; the α-consumer wiring lands with the lowering). At D1.b
  the `ik:`/`row:` dump tags render from the DRInstance descriptor's key-column positions
  (§4.4), NOT from binding sources — an independent, simpler source.

### 2.4 DRInstance descriptor + the id space (DeltaRel.h, new struct near DRStateCell :629)
```
+ // One per RecognizedSubgraph (the mint's subject). Codegen (D2.b) instantiates an
+ // InstanceStore<Key,Row> member from it. Peer of DRStateCell.
+ class DRInstance {
+  public:
+   QueryView demanded_view;            // the forcing's demanded MERGE
+   QueryView pub_view;                 // [ADJ:crit-grammar-1] answer INSERT target
+                                       //   VIEW (copy rs.pub_view at mint) — the ONLY
+                                       //   name source for the ik:/row: column tags.
+   unsigned forcing_index{~0u};        // -> query.RecognizedSubgraphs()[i]
+   TABLE *pub_table{nullptr};          // answer INSERT target (the published relation)
+   TABLE *demand_table{nullptr};       // demand relation model table
+   TABLE *input_table{nullptr};        // summarized monotone input
+   std::vector<unsigned> key_cols;     // α positions in the published row (the ik: set)
+   std::vector<unsigned> row_cols;     // the remaining published positions (the row: set)
+ };
```
**[ADJ:crit-grammar-1] (MANDATORY — a D1.b COMPILE fix, not merely D2.b).** The §4.4/§4.5
render mandates published COLUMN NAMES (`key_cols=[Start]`, `pub_row=[ik:Start,row:Node]`,
`nested=<Node>`). But `ColRender::Name/Typed` take a **`QueryColumn`**, not an `unsigned`
position (Format.cpp:308,318, verified; GROUP_UPDATE renders `col.Typed(op.group_cols[i])`
where `group_cols` are `QueryColumn`s, :593). A `TABLE*` carries NO variable names; a bare
`unsigned` position is not a name. To turn a `key_cols`/`row_cols` position into a renderable
`QueryColumn` the code needs the published VIEW: `col.Name(pub_view.Columns()[pos])`. So the
struct MUST carry `pub_view` (copied from `rs.pub_view` at mint) — else the §4.4/§4.5 rows the
design mandates as **compile-covered D1.b code do not compile** (a type error, not dead-code:
the render body compiles unconditionally even though it is never *reached* with
`flow.instances` empty). §2.3's "positions are an independent, simpler source" is HALF-RIGHT:
positions decide `ik:` vs `row:`, but they are not NAMES without a view. Alternative accepted:
precompute the name strings at mint time and store `std::vector<std::string>
key_col_names/row_col_names` — the dump then holds NO view handle, which ALSO closes the
render-side leg of [ADJ:crit-correctness-1]. **Add `pub_view` OR precompute strings; the
pseudocode as originally written is uncompilable.**
On `DRFlowGraph`, next to `statecells`:
```
+   std::vector<DRInstance> instances;                          // one per RecognizedSubgraph
+   std::unordered_map<unsigned, unsigned> instance_stratum;    // store_id -> derived stratum
```
**Id-space decision (OD-I2 / C-7): ONE id space, `i#N == store=I#N` collapsed.** The
GROUP_UPDATE `sc#N` precedent carries ONE id (statecell_id) rendered as `sc#0`; the
instance family follows — `instance_store_id` IS the DRInstance index, rendered `i#0` in
the op header and `store=I#0` in args from the SAME integer. (The single-adornment slice
is 1 forcing : 1 store : 1 op, so no information is lost by collapsing; the dual-render
was only ever a hedge — §A.2-F5.)

---

## §3. THE MINT — `BuildSubgraphInstanceOps`, GATED OFF (DeltaRel.cpp)

### 3.1 Structural unreachability at D1.b — stated exactly
The knob `-demand-instance` does **not exist until D2.b**. The mint reads a new
`Context` bool `demand_instance_enabled` (default **false**, plumbed onto `Context` but
**set true by NO code path at D1.b** — Main.cpp wires no flag to it until D2.b). The call
site in `BuildDRInventory`:
```
  BuildGroupUpdateOps(...);                       // existing R3 loop, ~:1081/:1095
+ if (context.demand_instance_enabled) {          // ALWAYS FALSE at D1.b
+   BuildSubgraphInstanceOps(flow, impl, context, query, scc_map);
+ }
```
Because the guard is unconditionally false, `BuildSubgraphInstanceOps` is **dead at
runtime on every corpus flow and every mode** — zero instance ops minted (P-D1b.1). The
function body is fully written and compiled (so D2.b flips one bool), but never executes.
This is legitimate semantic-predicate staging (HP-17), NOT env-gated scaffolding: the
gate is a real mode bit, not a debug toggle, and it is default-off production code.

### 3.2 The mint body (regime-correct for D2.b, never run at D1.b)
```
BuildSubgraphInstanceOps(flow, impl, context, query, scc_map):
  live = LiveViewPtrSet(impl)                     // §3.6 — set of uintptr_t(VIEW*), no deref
  for rs in query.RecognizedSubgraphs():          // append order = forcing order
                                                  // [ADJ:crit-pins-4] deterministic: the
                                                  // pass's own stamp order (Query.h:1054-1056);
                                                  // ties break on pub-table id in op_table_id
                                                  // (DeltaRel.cpp:3400-3408), never ctor order —
                                                  // discharges the HP-9 "opaque handles" caveat.
    if !live.count(rs.pub_view.UniqueId()) || !live.count(rs.demanded_view.UniqueId()):
      continue                                     // §3.6 dangling-handle pre-filter
    pub_table   = model_table(rs.pub_view)
    demand_table= model_table(rs.demanded_view)   // the fabricated demand relation
    input_table = model_table(summarized monotone input of rs)
    DIFF = TableIsDifferential(pub_table)          // A.2.3 regime discriminant
    sid  = flow.instances.size()
    flow.instances.push_back(DRInstance{rs.demanded_view, rs.pub_view,   // [ADJ:crit-grammar-1]
                                        rs.forcing_index, pub_table,
                                        demand_table, input_table, rs.key_cols, row_cols})
    // ---- kSubgraphInstantiate (birth/rebuild + band-(b) publish) ----
    inst = DROp(kSubgraphInstantiate)
    inst.ctx = kSeed; inst.table_op_table = pub_table; inst.table_op_sign = +1
    inst.demand_table = demand_table; inst.input_table = input_table
    inst.instance_store_id = sid; inst.forcing_index = rs.forcing_index
    inst.arms[0].body = <PlanTree: kAccess(input_table, section-walk keyed on key_cols)
                                    -> kFold(pub_table, +1, NonRecursive)>   // §A.2.3
    inst.effects = INSTANTIATE_EFFECTS(DIFF, pub_table, demand_table, input_table)  // §3.3
    if DIFF:  register pub-queue DEF-edges (the DR.cpp:744-747 mold)          // §3.3
    flow.ops.push_back(inst)
    // ---- kInstanceDeath (R-DIFF ONLY) ----
    if TableIsDifferential(demand_table):          // HP-17: runtime-gated, ships INERT
      death = DROp(kInstanceDeath)
      death.ctx = kSeed; death.table_op_table = pub_table; death.table_op_sign = -1
      death.demand_table = demand_table; death.instance_store_id = sid
      death.effects = DEATH_EFFECTS(pub_table, demand_table)                  // §3.3
      flow.ops.push_back(death)
    // ---- kInstanceSeal (always; self-lowered, HP-1) ----
    seal = DROp(kInstanceSeal)
    seal.ctx = kSeed; seal.table_op_table = pub_table; seal.instance_store_id = sid
    seal.effects = { kStateFold(value_table=pub_table, sign=0) }              // §1.4
    flow.ops.push_back(seal)
    // ---- stratum seed (A.2.4): lift above demand_table + input_table drains ----
    flow.instance_stratum[sid] = 1 + max(ready_after(demand_table),
                                         ready_after(input_table))            // strict A5
```
`row_cols` = the published positions not in `rs.key_cols`. (Under R-MONO the death branch
never mints — `TableIsDifferential(demand_table)` is false for the R-MONO-a witness — so
even with the knob ON at D2.b the family is `{instantiate, seal}`, census kInstanceDeath=0,
exactly d1-desired-states §B.4's op set.)

### 3.3 Effect multisets — REGIME-SPLIT per A.2.3, realized to the §1 collapse
`DIFF := TableIsDifferential(pub_table)`.
```
INSTANTIATE_EFFECTS(DIFF, pub, demand, input):
  always:  kVecDrain(demand, kNetAddition)             // requires OD-7 frontier (drain)
           kInstanceDemand(demand)                     // NEW kind, frozen read, no hazard
           kFlagRead(input, kPresent, kSeed)           // rederive leaf (existing kFlagRead)
           kInstanceRebuild(pub, +1)                   // NEW kind, WRITE current, sign +1
           kStateEmit(pub)                             // COLLAPSED emit — read current
           kStateOld(pub)                              // COLLAPSED old  — read frozen
  if DIFF: kCounter(pub, +1, NonRecursive), kCounter(pub, -1, NonRecursive)
           kInIReadFrozen(pub, ...) x2
           kVecAppend(pub, kAddQueue), kVecAppend(pub, kDeleteQueue)
           + queue DEF-edge registration (DR.cpp:744-747 mold)
  else (!DIFF, R-MONO): kCounter(pub, +1, NonRecursive)   // ONE counter; ZERO appends;
                                                          // NO TableVec on pub queues
                                                          // (TableVec ASSERTS — :595-601)

DEATH_EFFECTS(pub, demand):  // R-DIFF only
  kVecDrain(demand, kNetRemoval), kInstanceDemand(demand), kStateOld(pub),
  kInstanceRebuild(pub, -1)
  // EXACTLY ZERO {kStateEmit, kCounter, kInIReadFrozen, kVecAppend} — the zero-counter
  // death signature (§18(B) teeth).
```
Note the collapse rewrites A.2.3's `kInstanceEmit`→`kStateEmit(pub)`,
`kInstanceOld`→`kStateOld(pub)`. `kFlagRead(input,...)` renders on `reads:` (§4.4), never
in `effects:` (Format.cpp:529-531).

### 3.4 Linearizer cases (DeltaRel.cpp)
**op_table_id (:3400-3408): UNCHANGED** — both instance ops already key on the existing
`table_op_table` 1st arm (HP-3). No new `?:` branch.

**DROpStratum (:3133-3203): add the two LIVE kinds (ValidatorFail on miss, per F-OPS-7);
kInstanceSeal → 0 (trailing).**
```
+ case DROpKind::kSubgraphInstantiate:
+ case DROpKind::kInstanceDeath:
+   if (auto it = flow.instance_stratum.find(op.instance_store_id);
+       it != flow.instance_stratum.end()) return it->second;
+   ValidatorFail("DROpStratum: instance op has no instance_stratum entry");  // NOT 0u
+ case DROpKind::kInstanceSeal:
+   return 0u;  // trailing commit band (V-READY skip, like kStateSeal :3198)
```
**op_band (:3360-3390): no case needed** — instance ops are never `lead==1` phase ops
(instantiate/death are `lead==1` but the default `return 0u` is correct: A.2.4 pins "both
live ops band 0"; seal is `lead==2` and never reaches op_band). Confirmed against key_of
below: instantiate/death take the DEFAULT key branch `{1, stratum, band=0, tid, sign, oi}`.

**key_of (:3445-3469): add ONE case for the seal (band 11); instantiate/death fall through
to DEFAULT.**
```
+ if (op.kind == DROpKind::kInstanceSeal) {
+   return Key{2u, max_stratum + 1u, 11u, op_table_id(op), 0, oi};  // trails kStateSeal(10)
+ }
  // instantiate (sign +1) / death (sign -1) → DEFAULT: {1, op_stratum, op_band=0,
  //   op_table_id=pub_table+1, op_sign=table_op_sign, oi}. SAME table_id ⇒ key_less
  //   reaches the sign compare (:3475) ⇒ death(-1) sorts BEFORE instantiate(+1). (OD-2)
```
This IS the minus-before-plus mechanism: death and instantiate share `table_id`
(both = `pub_table->id + 1`), so `key_less` falls through lead/stratum/band/table_id and
hits `a.sign < b.sign` — death's `-1` < instantiate's `+1`. No mint-time DRDep (OD-2: the
mandated explicit edge is circular under emit_waw's key-forcing, DeltaRel.cpp:3540-3556).

**Effect-hazard switch (:3280-3320): add two cases; flip `default`.**
```
+ case EffKind::kInstanceRebuild:
+   flag_accesses.push_back(FlagAccess{oi, e.value_table, /*is_write=*/true});  // WRITE pub
+   break;
+ case EffKind::kInstanceDemand:
+   break;  // frozen read of the demand key: NO hazard (HP-8, kInIReadFrozen precedent)
- default:
-   break;
+ default:
+   ValidatorFail("linearizer: unhandled EffKind in the hazard switch");  // F-OPS-5
```
The collapsed emit/old/sealswap need NO new switch arms — they ride the landed
`kStateEmit`(read `pub`), `kStateOld`(`break`), `kStateFold`(write `pub`) arms.
**P-D1b.3 restated:** with all 12 EffKinds now cased, `default` is unreachable on every
corpus flow (instance ops never minted; the pre-existing 10 all had arms). The `default →
ValidatorFail` is a runtime backstop only. NOTE it trades away `-Wswitch` exhaustiveness on
THIS switch (a `default` suppresses the warning) — accepted because the **`EffKindName`
spelling table (Format.cpp:117-132, no `default`) keeps compile-time exhaustiveness for
the render path**: a future EffKind added without a spelling row fails to compile, the
belt behind the runtime abort.

### 3.5 Validators (DeltaRel.cpp, in `ValidateDROps` / `LinearizeAndValidateDRFlow`)
Five instance validators, all **always-on (survive NDEBUG, fprintf+abort)**, molded on the
landed V-AGG-* family (:2937-3010) and V-READY (:3943-3972).

**Census recount** (in `ValidateDROps`, after the state-seal `expect`, :2882) — the
GROUP_UPDATE recount mold (:2833-2851), but see §3.6 for liveness + the D1.b gate:
```
+ unsigned exp_instance = 0u, exp_death = 0u;
+ std::vector<InstanceKey> exp_inst_keys;   // (pub_table, provenance=kSubgraph,
+                                            //  mode=Rederive, forcing view id) — E-28:
+                                            //  store_id EXCLUDED (mint-order artifact)
+ if (context.demand_instance_enabled) {     // <-- D1.b: ALWAYS FALSE ⇒ recount not entered
+   live = LiveViewPtrSet(impl)
+   for rs in query.RecognizedSubgraphs():
+     if (!live.count(rs.pub_view.UniqueId()) || !live.count(rs.demanded_view.UniqueId())) continue
+     ++exp_instance
+     if (TableIsDifferential(model_table(rs.demanded_view))) ++exp_death
+     exp_inst_keys.push_back({model_table(rs.pub_view), kSubgraph, Rederive,
+                              rs.demanded_view.UniqueId()})
+ }
+ expect(DROpKind::kSubgraphInstantiate, exp_instance, "subgraph instantiates");
+ expect(DROpKind::kInstanceDeath,       exp_death,    "instance deaths");
+ expect(DROpKind::kInstanceSeal,        exp_instance, "instance seals");  // 1:1 w/ inst
+ // + the order-free key-multiset compare vs flow's kSubgraphInstantiate ops (the
+ //   :2911-2935 mold).
```
At D1.b `demand_instance_enabled` is false, so `exp_instance==exp_death==0`,
`expect(...)` demands 0 of each (which holds — nothing minted), and **`RecognizedSubgraphs()`
is never iterated/dereferenced** (§3.6).

**V-INST-EFFECT** (per-op effect-multiset totality — the V-AGG-EFFECT mold, :2937-3010):
```
for op in flow.ops:
  if op.kind == kSubgraphInstantiate:
    DIFF = TableIsDifferential(op.table_op_table)     // regime-matched, both first-class
    require multiset == INSTANTIATE_EFFECTS(DIFF, ...) // §3.3 exactly; a stray kCounter,
                                                       //   a missing kStateEmit, etc. abort
  if op.kind == kInstanceDeath:
    require multiset == DEATH_EFFECTS(...)             // ZERO {emit,counter,crossing,append}
  if op.kind == kInstanceSeal:
    require op.effects.size()==1 && op.effects[0]==kStateFold(sign=0)   // kStateSeal mold
```
**V-INST-SOLE**: `pub_table` has exactly one kSubgraphInstantiate deriver; the summarized
`input_table` is monotone (`!TableIsDifferential`) and never aliases `pub_table` (the
V-AGG-SOLE analogue).

**V-INST-PAIR**: per `instance_store_id`, the op set is the **2-way {instantiate, seal}
under R-MONO / 3-way {death, instantiate, seal} under R-DIFF** — regime is a per-flow fact
(`TableIsDifferential(demand_table)`). **The R-DIFF (3-way) arm ships INERT at D1.b/D2.b
(HP-17)** — it compiles and is exercised only from D3.a.

**V-INST-ORDER** (the OD-2 enforcement — NEW, ALWAYS-ON, in
`LinearizeAndValidateDRFlow` after `pinned_order` is built, ~:3830):
```
for each instance_store_id sid present in flow.ops:
  di = pinned index of the kInstanceDeath with that sid (if any)
  ii = pinned index of the kSubgraphInstantiate with that sid
  if di exists and !(di < ii):
    ValidatorFail("V-INST-ORDER: death must precede instantiate for a store id")
```
Grouped by `instance_store_id` — **never table_id, never forcing_index** (HP-3). Vacuous
under R-MONO (no death op). **HP-17 vacuous-green corpus observability** (§6): the loop
still RUNS on every flow (0 death ops ⇒ 0 iterations of the inner check, no abort); its
having-run is made observable by a debug counter (§6.2).

**V-INST-EMITTED** (§A.5c; enrollment DESIGNED now per HP-1, ENFORCED at D2.b): a
multiset compare of every EMITTED instance region (Stratum.cpp `LowerSubgraphInstance`)
against the flow's enrollment of **ALL THREE kinds** — `kSubgraphInstantiate`,
`kInstanceDeath`, `kInstanceSeal` (the V-INGEST-XCHECK Site-5 mold, Stratum.cpp:2099-2170).
A kind with no lowering loop ABORTS instead of silently dropping the derivation (the
F17/F18-class guard; HP-1's teeth against a minted-but-unlowered seal). **Enrollment list
= {kSubgraphInstantiate, kInstanceDeath, kInstanceSeal}** — the seal is IN (superseding
A.5c's design text that enrolled only instantiate/death). Lands at D2.b with the emitter;
at D1.b there is no emitter, so the enrollment list is recorded here, not wired.

### 3.6 The DANGLING-HANDLE caveat (§19(K), addressed explicitly)
D1.a's Fable review carried LOUD: the stored `RecognizedSubgraph` QueryView handles
(`pub_view`, `demanded_view`) **DANGLE after dead-flow elimination erases a view**. The
census recount is exactly "such a consumer" (§19(K) directive). Verified severity:
`EliminateDeadFlows` (DeadFlowElimination.cpp:69) ends in `RemoveUnusedViews`
(Optimize.cpp:515-551), which calls `selects.RemoveUnused()`/`tuples.RemoveUnused()`/… —
`DefList::RemoveUnused` **destroys** the owned `QueryViewImpl`. So a handle to an erased
view is a **dangling pointer — dereferencing it (even reading `->is_dead`) is
use-after-free**, not merely a stale flag. And `BuildDRInventory` runs at Program::Build
(Stratum.cpp:2081), AFTER Query::Build's `Optimize` (Build.cpp:2599) has run dead-flow
elim — so any recount consuming `RecognizedSubgraphs()` is unavoidably post-Optimize.

**D1.b resolution — the recount NEVER touches the handles.** The recount is gated on
`context.demand_instance_enabled` (§3.5), which is unconditionally false at D1.b, so the
short-circuit `if (enabled) { … iterate RecognizedSubgraphs … }` is never entered. The
dangling-handle hazard is **structurally sidestepped at D1.b** (this is why the recount is
knob-gated, not merely the mint).

**D2.b requirement — LOUD, carried forward.** When the recount DOES iterate (knob on), it
must NOT dereference the stored handles blindly. The safe primitive is a **pointer-identity
set of LIVE views** built at mint time from `impl->ForEachView` — `LiveViewPtrSet(impl)` =
`{ uintptr_t(v) : v in impl->ForEachView }`. The filter tests
`live.count(rs.pub_view.UniqueId())`; `QueryView::UniqueId()` returns the **pointer-derived
id** (a `reinterpret_cast` of the wrapped impl pointer, Node.h:31-33 — the same value
`ForEachView` inserts), read from the QueryView's OWN by-value pointer member — it does
**NOT dereference the (possibly freed) pointee**, so it is UAF-safe even on a dangling
handle. A dead handle is filtered without ever reading through it, and the mint + recount apply
the IDENTICAL filter so their counts agree. In the common case (a live bound query feeding
a published answer) both handles are live by construction; the filter only matters when a
demand subgraph was wholly optimized away. This §3.6 primitive is DESIGNED here and
lands with the D2.b mint — at D1.b it is dead code behind the false gate.

**[ADJ:crit-correctness-1] (MED — INERT at D1.b; a LOUD D2.b HOLE the pointer-identity filter
does NOT close).** The `live.count(UniqueId())` FILTER is UAF-safe (no deref). But the very next
mint line — `pub_table = model_table(rs.pub_view)` (§3.2) — and (post-[ADJ:crit-grammar-1]) the
dump's `col.Name(pub_view.Columns()[pos])` DEREFERENCE the handle. The filter is NOT ABA-safe:
`QueryImpl::Optimize` both FREES views (`RemoveUnused` destroys `QueryViewImpl`,
Optimize.cpp:515-551) AND ALLOCATES new ones (CSE merges, canonicalization tuples). A `pub_view`
wholly optimized away can have its address REUSED by a surviving newly-allocated view; then
`live.count(rs.pub_view.UniqueId())` finds that address in the live set, the filter FALSELY
PASSES, and `model_table(rs.pub_view)` derefs the reused-but-WRONG view → wrong `pub_table` →
**silent miscompile** (NOT UAF — the reused allocation is valid, just misidentified). This is
exactly the "wholly optimized away" window the design admits the filter is for — and pointer
identity is unsound precisely there. The §19(K) resolution closes UAF-on-DEAD but NOT
ABA-on-REUSED. **D2.b MUST NOT ship the pointer-identity liveness set as the mint's identity
scheme.** The ABA-safe primitive is a STABLE, deref-free identity surviving free+alloc — e.g.
key the recount/mint by `forcing_index` and re-resolve the CURRENT live pub/demanded views by a
deterministic view walk keyed on the forcing's message/relation identity (never a raw pointer),
so a freed-and-reused address can never masquerade. Carried LOUD to the D2.b design (and to the
owner, as it reveals the §19(K) directive's resolution is incomplete). INERT at D1.b — the gate
is false, nothing derefs.

---

## §4. DUMP GRAMMAR (Format.cpp) — sequenced AFTER the §1 HP-11 call

All rows land as REAL Format.cpp rows at D1.b with the loud-abort fallback (HP-13 duty,
T2b law). Because the mint is off, **no instance op or DRInstance ever renders at D1.b** —
these rows compile and are `-Wswitch`-covered but are dead until D2.b. What IS exercised at
D1.b: the census line only (§4.3, §5).

### 4.1 DROpKind spelling rows — 3 rows in `DROpKindName` (Format.cpp:111, after kStateSeal)
```
+   case DROpKind::kSubgraphInstantiate: return "kSubgraphInstantiate";
+   case DROpKind::kInstanceDeath:       return "kInstanceDeath";
+   case DROpKind::kInstanceSeal:        return "kInstanceSeal";
```
(No `default` — the `-Wswitch` + trailing fprintf+abort idiom is preserved.)

### 4.2 EffKind spelling rows — 2 rows in `EffKindName` (Format.cpp:128, after kStateOld)
```
+   case EffKind::kInstanceRebuild: return "kInstanceRebuild";
+   case EffKind::kInstanceDemand:  return "kInstanceDemand";
```
And the **arg-shape rows** in `emit_effect` (Format.cpp:411-459, C-8 per the §1 outcome):
```
+   case EffKind::kInstanceRebuild:  // WRITE current, signed
+     os << "(" << tid(e.value_table) << ", " << effect_sign(e.sign) << ")";  // kInstanceRebuild(%table:4, +)
+     break;
+   case EffKind::kInstanceDemand:   // frozen demand-key read
+     os << "(" << tid(e.read_table) << ")";                                  // kInstanceDemand(%table:8)
+     break;
```
The collapsed effects need **no new arg-shape rows** — they render through the landed
`kStateEmit`/`kStateOld` (`(%table:4)`, Format.cpp:455-457) and `kStateFold`
(`(%table:4, ·)`, :452-454) arms. So the instance family's rendered effects are:
`kVecDrain($net-addition…)`, `kInstanceDemand(%table:8)`, `kInstanceRebuild(%table:4, +)`,
`kStateEmit(%table:4)`, `kStateOld(%table:4)`, `kCounter(%table:4, +, NonRecursive)`; seal
= `kStateFold(%table:4, ·)`; `kFlagRead` → `reads:` line.

### 4.3 Census counters — 3 counters appended to `kAllKinds[]` (Format.cpp:787, HP-10)
```
      DROpKind::kIngestFold,  DROpKind::kGroupUpdate,
-     DROpKind::kStateSeal};
+     DROpKind::kStateSeal,
+     DROpKind::kSubgraphInstantiate, DROpKind::kInstanceDeath,
+     DROpKind::kInstanceSeal};
```
In **enum-declaration order** (HP-10 / F6). The census line then renders 18 `Name=count`
pairs; `census_total` still equals `flow.ops.size()` (all kinds covered → the :796
completeness abort stays green). **This is the ONLY byte-visible change on any existing
`.deltarel` carrier at D1.b** (§5).

### 4.4 The `instances:` section + `DRInstance` render (OD-I1/C-6) — NEW section
Placement (p11 + C-6): after `joins:`, before `ops:`. In the neighborhood witness (no
vecs/branches/joins) it leads. **Empty-section law (p11): render nothing when there are no
instances** — guarded exactly like vecs/branches/joins:
```
+   if (!flow.instances.empty()) {
+     os << "\n";
+     os << "instances:\n";
+     for (unsigned i = 0u; i < flow.instances.size(); ++i) {
+       const DRInstance &in = flow.instances[i];
+       os << "  DRInstance i#" << i
+          << " forcing=" << <forcing name/adornment from query.DemandForcings()[in.forcing_index]>
+          << " key=" << tid(in.demand_table)
+          << " pub=" << tid(in.pub_table)
+          << " input=" << tid(in.input_table)
+          << " store=I#" << i                       // ONE id space (§2.4): i# == I#
+          << " key_cols=[" << <col names @ in.key_cols> << "]"
+          << " row_cols=[" << <col names @ in.row_cols> << "]\n";
+     }
+   }
```
Field order per the §B.4 exemplar (C-6): `forcing= key= pub= input= store= key_cols=
row_cols=`. **At D1.b `flow.instances` is always empty ⇒ the guard is false ⇒ the
`instances:` section renders ZERO bytes** (no leading `\n`, no header) — byte-exactly the
p11 empty-section separator law, identical to how an emit with no vecs renders no `vecs:`.
So the section's presence never perturbs any existing golden.

### 4.5 The instantiate op p-rule — HP-6 (F,T)/(T,F) partition + ik:/row: tags (C-12)
The kSubgraphInstantiate op renders under the `default:` op arm (Format.cpp:707) extended
into its own `case`, OR (cleaner) its own case beside kGroupUpdate:
```
+   case DROpKind::kSubgraphInstantiate: {
+     os << " sign=" << SignGlyph(op.table_op_sign) << " ctx=" << CtxName(op.ctx)
+        << " stratum=" << DROpStratum(flow, op)
+        << " i#" << op.instance_store_id << "\n";   // [ADJ:crit-grammar-2] header id-glyph
+                                                     // ONLY; full store=I# on args (below).
+     // second header line (C-12): the published-row partition + binding-source tags
+     os << "    demand=" << tid(op.demand_table) << " pub=" << tid(op.table_op_table)
+        << " input=" << tid(op.input_table)
+        << " pub_row=[" << <for each published position p: (p in DRInstance.key_cols ?
+                            "ik:" : "row:") << col.Name(pub_view.Columns()[p])> << "]"
+        << " nested=<" << <row_cols col names> << ">\n";  // [ADJ:crit-grammar-3] see below
+     emit_reads(op);      // reads: Present(%table:11)     — the kFlagRead(input,...)
+     emit_effects(op);    // effects: {kVecDrain, kInstanceDemand, kInstanceRebuild,
+                          //           kStateEmit, kStateOld, kCounter}
+     emit_spine(op);      // spine: kAccess(%table:11, section-walk) -> kFold(%table:4,+,NonRecursive)
+     os << "    args: demand=" << tid(op.demand_table) << " pub=" << tid(op.table_op_table)
+        << " input=" << tid(op.input_table) << " store=I#" << op.instance_store_id << "\n";
+     break;
+   }
```
**[ADJ:crit-grammar-3] (LOW — D2.b-latent redundant composite; reconcile at the D2.c bless).**
`nested=<row_cols>` re-renders the SAME columns already tagged `row:` inside `pub_row=[...]`
— the p12/R-9 redundant-composite strike. Inherited from the OD-4-banner-amended (explicitly
UNBLESSED) §B.4 exemplar. Never rendered at D1.b (mint off). At the D2.c bless choose ONE: drop
`nested=<...>` OR drop the `row:` positions from `pub_row=[...]` — do not ship both.
**[ADJ:crit-pins-3] (LOW — the loud-abort must be REAL, not a placeholder).** §4 preamble
claims all rows land "with the loud-abort fallback." Make it concrete: the forcing-name source
`query.DemandForcings()[in.forcing_index]` and the col-name source `pub_view.Columns()[pos]`
each fprintf+abort on an out-of-range/unresolvable index (the kGroupUpdate agg-functor precedent
aborts on an unresolvable source, Format.cpp:569-571) — NOT a silent `<...>` guess (t2-dump-spec
p12). Never exercised at D1.b, but the abort must be WRITTEN, not implied by the placeholder.

**HP-6 partition, made contract:** the `pub_row=[ik:..,row:..]` tags are the CONTRACT that
band (b) is the two-scan partition, not a publish-ALL-current lowering. `ik:` positions
are the α (instance-key) columns — sourced from `DRInstance.key_cols` (a position is `ik:`
iff it is in `key_cols`, else `row:`) — **NOT from context_col_sources** (which is D2.b,
§2.3). The p-rule additionally PINS in the spec (t2-dump-spec p15+): `kStateEmit(pub)` is
the **(F,T)-GATED publish** (fires only on current∖frozen) and `kStateOld(pub)` is the
**(T,F)-GATED retract** (provably EMPTY under !DIFF — HP-7). The dump alone cannot force
the partition (HP-6: two reads + a counter are satisfiable by a double-publishing
lowering); the p-rule is where the discipline becomes contract, and V-INST-EFFECT (§3.5)
+ V-INST-FRESH (D2.a runtime) are its teeth.

### 4.6 The kInstanceDeath + kInstanceSeal op p-rules
```
+   case DROpKind::kInstanceDeath: {   // R-DIFF only; never rendered at D1.b/D2.b
+     os << " sign=" << SignGlyph(op.table_op_sign) << " ctx=" << CtxName(op.ctx)
+        << " stratum=" << DROpStratum(flow, op)
+        << " i#" << op.instance_store_id << "\n";   // [ADJ:crit-grammar-2] header i# only
+     emit_reads(op); emit_effects(op);   // {kVecDrain(net-removal), kInstanceDemand,
+                                          //  kStateOld, kInstanceRebuild(-1)}
+     os << "    args: demand=" << tid(op.demand_table) << " pub=" << tid(op.table_op_table)
+        << " store=I#" << op.instance_store_id << "\n";
+     break;
+   }
+   case DROpKind::kInstanceSeal: {
+     os << " sign=" << SignGlyph(0) << " ctx=" << CtxName(op.ctx)
+        << " band=11 i#" << op.instance_store_id << "\n";  // [ADJ:crit-grammar-2] header i# only
+     emit_effects(op);                    // {kStateFold(%table:4, ·)}
+     os << "    args: pub=" << tid(op.table_op_table)
+        << " store=I#" << op.instance_store_id << "\n";
+     break;
+   }
```
(`band=11` mirrors kStateSeal's hard-coded `band=10` render at Format.cpp:622 — a rendered
constant matching key_of's kInstanceSeal band.)

### 4.7 What is EXERCISED at D1.b — the byte-exact pin
- The `instances:` section: **empty on every flow ⇒ zero bytes** (§4.4).
- The three op p-rules (§4.5/§4.6): **never reached** (no instance op in `pinned_order`).
- The three EffKind/DROpKind spelling rows: **never reached** at runtime (compile-covered).
- The census line: **renders `kSubgraphInstantiate=0 kInstanceDeath=0 kInstanceSeal=0`
  appended** on EVERY flow (kAllKinds renders unconditionally). ← the ONE exercised change.

---

## §5. THE ONE CHURN + BLESS (P-D1b.2, HP-14)

**The only golden that changes** is `tests/OptDiff/goldens/demand_tc_witness.deltarel.opt.golden`
(the sole `.deltarel` carrier — the two irgold sidecars are `demand_tc_witness` {h,ir,df,
deltarel} and `symrec_tie_1` {ir,df}; symrec_tie_1 has NO deltarel surface). Its line 16
(the census, verified) goes:
```
  BEFORE (line 16, verified at tip):
census: kCrossover=0 … kIngestFold=2 kGroupUpdate=0 kStateSeal=0
  AFTER (append exactly the three enum-tail counters, kAllKinds order):
census: kCrossover=0 … kIngestFold=2 kGroupUpdate=0 kStateSeal=0 kSubgraphInstantiate=0 kInstanceDeath=0 kInstanceSeal=0
```

**The referee — DIRECT DIFF, three points (HP-14; permcheck struck, EXECUTED-confirmed to
reject the census append):**
1. `diff` shows **exactly one changed line** (line 16). No other golden byte moves.
2. the delta is **exactly the three enum-tail counters** ` kSubgraphInstantiate=0
   kInstanceDeath=0 kInstanceSeal=0` in kAllKinds order (Format.cpp:787), no
   zero-suppression, appended (not inserted).
3. the census-completeness abort (Format.cpp:796) is **green** — `census_total == ops.size()`
   still holds (the three new kinds count 0, total unchanged).

Bless mechanics: `runall.sh --bless <workroot> demand_tc_witness` after reviewing the
diff. **permcheck is NOT the referee here** (its `boundary_re`, permcheck.py:62, classifies
the census line as a structural boundary that must be byte-identical, so it FAILS the
append with "segment 3: boundary line differs", exit=1 — re-executed at §19(J)). permcheck
stays the referee for genuine published-delta STDOUT permutations, its actual domain.

**d1-desired-states §B.3 STAGE-1 nbhd block is PROBE-ONLY** — no golden file exists for
demand_neighborhood_witness at D1.b (it enters cases/ at D2.c). §B.3's flat STAGE-1 block
is the predicted post-D1b bytes for any existing deltarel carrier and is verified by
comparing the demand_tc_witness bless target, not by a separate golden.

---

## §6. NEGATIVE-SPACE TEST for V-INST-ORDER (HP-3)

**Requirement (HP-3):** a deliberately mis-minted plus-before-minus must trip V-INST-ORDER.

### 6.1 Feasibility findings (decided at code)
- **No ctest target links the compiler internals.** MiniDisassembler/PointsTo link the
  GENERATED datalog library + Runtime; runtime_test links Runtime only
  (tests/*/CMakeLists.txt). None link lib/DeltaRel + lib/DataFlow + lib/ControlFlow. A new
  unit linking them is real infra (transitive: DataFlow QueryView, ControlFlow
  ProgramImpl/TABLE, Parse).
- **DrTest has no death-test support** — no `ASSERT_DEATH`, no fork wrapper
  (tests/DrTest/DrTest.h: only `TEST`/`ASSERT_*`). `ValidatorFail` is fprintf+`abort()`,
  which would kill the test process; catching it needs a hand-rolled fork/waitpid.
- **V-INST-ORDER at D1.b is vacuous** — mint off + R-MONO ⇒ 0 death ops ⇒ the inner check
  never runs. A permanent unit would have to hand-build a 2-op `DRFlowGraph` (constructible;
  V-INST-ORDER compares `pinned_order` indices + `instance_store_id`, so no real TABLE/
  Context deref is needed) — feasible **only** if V-INST-ORDER is factored into a pure
  `void CheckInstanceOrder(const DRFlowGraph&)` helper and wrapped in a fork death-test in a
  new CMake target.

### 6.2 DECISION — a TEMPORARY IN-TREE PROBE, run-and-deleted, recorded in the ledger
The permcheck-execution precedent (E-77 "run the referee against the exact change") + the
"no env-gated debug scaffolding — delete before commit" memory. Concretely:
1. Temporarily patch `BuildSubgraphInstanceOps` (a throwaway local edit): force the gate
   ON for `demand_neighborhood_witness`, force the death mint (relax the
   `TableIsDifferential(demand_table)` guard), and **SWAP the two signs** —
   `death.table_op_sign = +1`, `instantiate.table_op_sign = -1`. The pair is otherwise
   WELL-FORMED (passes V-INST-EFFECT/SOLE/PAIR-3way), so the failure isolates to ordering.
2. Compile `demand_neighborhood_witness.dr` (the probe .dr in KeyedInstances.artifacts/)
   under `-demand`. The band key now sorts instantiate(−1) before death(+1) — the WRONG
   order — so `pinned_order` has instantiate before death.
3. **Observe:** V-INST-ORDER fires `ValidatorFail("V-INST-ORDER: death must precede
   instantiate for a store id")` → fprintf + abort, **exit 134 (SIGABRT)**. This proves
   BOTH halves of OD-2: the band key's sign tie-break placed them, and V-INST-ORDER caught
   the wrong placement (the band key alone would silently accept the swapped signs — that
   is why V-INST-ORDER is the guarantor, HP-3).
4. **Record** the exact stderr line + exit code in the KeyedInstances ledger (the permcheck
   precedent).
5. **REVERT** the probe (`git checkout lib/DeltaRel/DeltaRel.cpp`); re-run the suite → SUITE
   PASS (169), confirming no probe residue.

**Deferred stronger form (noted, not built at D1.b):** the permanent `CheckInstanceOrder`
+ fork death-test unit becomes worth its CMake infra at **D3.a**, when the death op mints
for real and V-INST-ORDER earns live golden coverage (HP-17's residual retirement). Building
it at D1.b, where the validator is vacuous, is premature.

**[ADJ:crit-pins-2] (MED — HARD-PIN DEVIATION → OWNER SIGN-OFF REQUIRED).** HP-3
(d1-pinned.md:116-129) is a hard pin: "**D1.b SHIPS a negative-space test: a deliberately
mis-minted plus-before-minus must trip it.**" §6.2 substitutes a run-ONCE-and-`git checkout`
REVERTED probe — which SHIPS NO regression coverage — and defers the permanent test to D3.a.
The feasibility premises are TRUE (verified: no ctest target links lib/DeltaRel + DataFlow +
ControlFlow; DrTest has no `ASSERT_DEATH`/fork, tests/DrTest/DrTest.h), and the E-77/permcheck
precedent the design leans on is about RUNNING A REFEREE, not substituting a throwaway probe
for a shipped test. "ships" ≠ "run once and revert." The adjudicator CANNOT unilaterally
downgrade a hard pin (the fleet applied OWNER ratification, OD-2, to comparable wording
changes). **This section is GATED on owner ruling** — the owner must either (a) accept the
probe-and-defer-to-D3.a reading of HP-3's "ships," OR (b) require the factored
`void CheckInstanceOrder(const DRFlowGraph&)` helper + a minimal fork/waitpid death-test in a
new CMake target AT D1.b (feasible per §6.1 — the check is pure over `pinned_order` +
`instance_store_id`, so a hand-built 2-op flow needs no real TABLE/Context). Recommendation:
(b) is the pin-faithful path and the helper factoring is cheap and reusable at D3.a; (a) is
defensible only with explicit owner ratification. The rest of the design is independent of this
choice.

---

## §7. HP-17 LINES (the runtime-gated residual, carried LOUD)

- **kInstanceDeath is minted ONLY when `TableIsDifferential(demand_table)`** (§3.2) — a
  runtime mint predicate, NOT a compile-time `#if`. The enum member, its `key_of`/
  `DROpStratum`/effect cases, its spelling/census rows, and V-INST-ORDER + V-INST-PAIR's
  3-way arm all **compile and SHIP INERT** at D1.b/D2.b. This is legitimate semantic-
  predicate staging (HP-17), not the env-gated-scaffolding anti-pattern: the predicate is a
  real regime bit read from the model, and the code is default-off production, not a debug
  toggle.
- **NO executing coverage until D3.a** — LOUD residual (§D). At D1.b/D2.b the DR-IR
  kInstanceDeath op and V-INST-ORDER's inner check have zero runtime exercise (R-MONO mints
  no death). The D2.a DrTest covers only the runtime store's Recycle/death half; the §6
  probe gives ONE-SHOT DR-side coverage that is then reverted. Full standing coverage
  arrives at D3.a with `-demand-retract`.
- **Vacuous-green corpus observability** (HP-17 adopted D1.b checklist line): V-INST-ORDER
  RUNS on every corpus flow (it is always-on, in `LinearizeAndValidateDRFlow`), iterating
  the (empty) set of instance store ids and asserting nothing. To make "it ran" OBSERVABLE
  (not merely "it didn't abort"), add a **pre-Program::Build-only debug counter**
  incremented once per V-INST-ORDER pass (mirroring the D1.a pre-Optimize census idiom) —
  a debug `fprintf` under a build-local flag confirming `V-INST-ORDER visited N flows, 0
  death ops, 0 violations` across the suite, then removed before commit (the "delete before
  commit" discipline). This discharges HP-17's "how is it-ran observable?" without shipping
  scaffolding.

### 7.1 Predictions restated with derivations
- **P-D1b.1** — Knob-off (everywhere at D1.b): zero instance ops minted (the
  `demand_instance_enabled` gate is unconditionally false, §3.1) ⇒ SUITE PASS (169) [G1];
  676-row corpus A/B byte-identical vs frozen `e6264b54` [G2]. *Derivation:* no mint ⇒
  `flow.ops` unchanged on every flow ⇒ every emitted region byte-identical ⇒ every
  generated `.h`/`.ir` byte-identical; only the census line (a dump surface, not in the A/B
  hash) moves.
- **P-D1b.2** — THE ONE CHURN: `demand_tc_witness.deltarel.opt.golden` line 16 gains
  ` kSubgraphInstantiate=0 kInstanceDeath=0 kInstanceSeal=0`; no other golden byte changes;
  refereed by the §5 three-point DIRECT DIFF (permcheck struck). *Derivation:* kAllKinds
  renders all kinds unconditionally (Format.cpp:788-794); the three new kinds count 0 on
  the witness; the census is the only dump line that enumerates every kind.
- **P-D1b.3** — the linearizer `default → ValidatorFail("unhandled EffKind")` trips nothing
  on the corpus. *Derivation:* the pre-existing 10 EffKinds all have explicit arms
  (:3282-3317, verified); the 2 new arms (§3.4) cover kInstanceRebuild/kInstanceDemand; no
  instance op is minted so neither new EffKind ever appears — the switch is exhaustive on
  every reached effect, `default` unreachable.

---

## §8. GATES CHECKLIST

- **G1 — SUITE: PASS (169)** with all sidecar arms live (irgold, .batches oracle/monotone,
  .drflags). Only the demand_tc_witness deltarel golden re-blessed (§5).
- **G2 — 676-row corpus A/B byte-identical** vs frozen `e6264b54` (knob-off). *CONFIRMED
  compatible:* the A/B rows hash **exit code / generated `.h` / `.ir`** only; the golden
  census churn is a `.deltarel` DUMP surface, not in the A/B hash — so A/B stays
  byte-identical while the suite golden churns. (The census move is invisible to A/B by
  construction.)
- **G3-amended — the census-line bless** (§5): the single-line DIRECT DIFF, permcheck NOT
  the referee (HP-14). The `-Wswitch`/loud-abort tables are the render-side belt.
- **G5 — 3-run determinism + debug==release on ALL FOUR surfaces of BOTH irgold carriers**
  (demand_tc_witness {h,ir,df,deltarel} and symrec_tie_1 {ir,df}) — mandatory on this
  dump-touching diff. demand_tc_witness's deltarel surface churns identically across runs
  and debug/release (census counts are deterministic); symrec_tie_1 has no deltarel surface
  so its {ir,df} are DeltaRel-independent and untouched.
- **G6 — E-62 tripwire re-grep MANDATORY** (the new kinds flow through `pinned_order`):
  re-grep every `body_ops`/`output_ops`/`pinned_order` consumer for a range-for/sort that a
  new op kind could perturb; confirm the sole standing hit is the Stratum.cpp:1073 comment
  (per §19(J)). The instance ops enter `pinned_order` via the band key — verify no consumer
  assumes a closed kind set.
- **G7 — data/ examples clean; ctest 3/3.** No Runtime edit at D1.b (store is D2.a) ⇒ ctest
  unaffected; data/ compiles unchanged (mint off).
- **ASAN — both surfaces** (per §19(F) standing gate + the per-diff cadence): build/asan
  ctest 3/3 + full suite under `DR=asan`; and the CXX-wrapper surface (ASAN-compiled
  generated code + drivers + Runtime). Zero reports expected (no new Runtime, no new
  allocation; the mint is dead). This is a DeltaRel-touching diff so surface 2 is run.
- **Q5 — ABABAB bench neutral** (progsize@128 release): no emitted-code change (mint off) ⇒
  expect noise-level (~0%).

---

## §9. PRECEDENCE / RATIFICATION TRACE (what each pin bought)
- **OD-2 / HP-3** → §3.4 key_of: death/instantiate share `table_op_table = pub_table`, sign
  ∓1; the sign tie-break (:3475) fires on equal table_id; V-INST-ORDER (§3.5) is the
  always-on guarantor; §6 is its negative test. NO mint-time DRDep.
- **OD-5 / HP-1** → §1.4 seal self-lowered (kStateFold collapse); §3.5 V-INST-EMITTED
  enrolls ALL THREE kinds.
- **OD-6 / HP-11** → §1: three-op family confirmed; kInstanceEmit/Old/SealSwap COLLAPSE,
  kInstanceRebuild/Demand KEEP (2 new EffKinds).
- **HP-6** → §4.5 pub_row ik:/row: p-rule = the (F,T)/(T,F) partition contract.
- **HP-8** → §3.4 kInstanceDemand no-hazard switch case + V-INST-EFFECT expectation.
- **HP-10** → §4.3 kAllKinds pre-registered.
- **HP-13(b)** → §1.4: §B.4's no-seal-edge is corrected at the D2.b desired-state refresh.
- **HP-14** → §5 DIRECT DIFF bless.
- **HP-17** → §3/§7 runtime-gated death, ships inert, vacuous-green observability.
- **§19(K) dangling-handle** → §3.6: recount knob-gated (never derefs at D1.b); pointer-set
  liveness filter designed for D2.b.
- **OD-I2/C-7** → §2.4 ONE id space (i# == I#), the sc#N precedent.
- **OD-R7** → §3.3: OD-7 frontier is a kVecDrain effect (LOWER-time TableDeltaVector), NO
  first-class DRVec.

---

## §10. ADJUDICATION (XHIGH, D1.b design round — tip 4d92d255)

### 10.1 HP-11 RULING — UPHELD (the designer's collapse call stands, code-verified)
The owner lean-collapse nudge (§19(I) OD-6) is the prior; the bar is a DISTINCT hazard target
OR a census distinction. Verified at code:
- `kStateEmit` reads an arbitrary `read_table` (DeltaRel.cpp:3312-3315); keyed on `pub_table`
  it IS the "RAW after rebuild" read A.2.2 asks for — no distinct hazard. `kStateOld` is
  already `break` / no-hazard (:3316-3317) — the exact "reads frozen, no hazard." So
  `kInstanceEmit`/`kInstanceOld` clear NEITHER bar → **COLLAPSE** to `kStateEmit`/`kStateOld`.
- `kInstanceSealSwap` → `kStateFold(sign=0)`: the landed peer seal (`kStateSeal`) ALREADY
  realizes `sealed := working` as exactly `kStateFold(value_table, sign=0)`
  (DeltaRel.cpp:756-758, verified) — the collapse is mold-faithful, no new hazard target.
- `kInstanceRebuild` KEPT — structural ±1 regime discriminant + store-op selector; a named
  kind keeps the lowering/validator unambiguous (HP-11 "justified as-is").
- `kInstanceDemand` KEPT — census-load-bearing: folding into `kInIReadFrozen` blinds the
  census to the F1 silent-drop (HP-11's named bar-clearer).

**FINAL: 2 new EffKinds (`kInstanceRebuild`, `kInstanceDemand`); collapse emit/old/sealswap
into the landed `kStateEmit`/`kStateOld`/`kStateFold`.** Semantically exact for the
InstanceStore transpose (current==working, frozen==sealed). Faithful to the owner nudge and
HP-11's bar. **Designer call UPHELD, not overturned.**

### 10.2 FINDINGS LEDGER (all 10 adjudicated)
| id | sev | ruling | disposition |
|----|-----|--------|-------------|
| crit-grammar-1     | MED | **ACCEPT — MANDATORY (D1.b compile fix)** | §2.4 gains `pub_view` (or precomputed name strings); render was uncompilable from positions+TABLE*. |
| crit-correctness-1 | MED | **ACCEPT — LOUD carry-forward** | §3.6 ABA hole flagged; pointer-identity filter is NOT the D2.b identity scheme. INERT at D1.b. Owner heads-up (touches §19(K)). |
| crit-pins-2        | MED | **ACCEPT — OWNER SIGN-OFF** | HP-3 hard-pin "ships a negative-space test" downgraded to a reverted probe; owner must rule (a) accept or (b) require the factored helper + death-test. |
| crit-pins-1        | MED | **ACCEPT — relabel** | kGroupUpdate returns 0u on miss (:3197), NOT ValidatorFail; the design's fail-loud is a STRENGTHENING, not "the exact precedent." Mechanic is fine. |
| crit-correctness-2 | LOW | **ACCEPT — record intentional** | 4 more `switch(op.kind)` default sites (:2432/:3221/:3497/:3765); moot at D1.b, intended classification at D2.b. |
| crit-grammar-2     | LOW | **ACCEPT** | store id over-rendered 3×; struck header `store=I#` → two renders (GROUP_UPDATE precedent). |
| crit-grammar-3     | LOW | **ACCEPT — D2.c reconcile** | `nested=<row_cols>` duplicates `pub_row` row: tags; drop one at the D2.c bless. |
| crit-grammar-4     | LOW | **ACCEPT — note** | HP-11 collapse restyles §B.4/§A.2.3 effect spellings (kInstance*→kState*); refresh at D2.c bless. |
| crit-pins-3        | LOW | **ACCEPT** | name-source rows must fprintf+abort on unresolvable (Format.cpp:569-571 precedent), not `<...>` placeholders. |
| crit-pins-4        | LOW | **ACCEPT — cite** | append-order determinism sourced to Query.h:1054-1056 + op_table_id tie-break; HP-9 caveat discharged. |

**Rejected: none.** All 10 findings verified at code and accepted (severity-appropriate).

### 10.3 FINAL VERDICT — **GO-WITH-AMENDMENTS**
The architecture is sound and the HP-11 call is correct and code-verified. The gated-off mint
(false `demand_instance_enabled`) is legitimate semantic-predicate staging (HP-17), not
env-gated scaffolding; the census-line churn + the DIRECT-DIFF bless (HP-14, permcheck struck)
is the right referee; the §3.6 knob-gating correctly sidesteps the dangling-handle UAF AT D1.b.
D1.b may LAND once the amendments are folded. Two items rise above the folded amendments:
1. **crit-grammar-1 is MANDATORY before code** — the render pseudocode does not compile from
   the DRInstance struct as originally specified (needs `pub_view` or precomputed strings).
2. **crit-pins-2 is GATED on owner ruling** — the HP-3 hard-pin deviation cannot be blessed by
   the adjudicator. The design lands GREEN on every other axis regardless of the owner's
   choice; only §6 is contingent.
crit-correctness-1 is a LOUD residual for the D2.b design (INERT at D1.b) — it does not block
this landing but reveals the §19(K) resolution is incomplete for the mint's live deref path.

