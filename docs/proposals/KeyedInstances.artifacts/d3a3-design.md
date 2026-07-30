# D3.a.3 (multi-adornment) — STAGE (b)/(c) BINDING DESIGN (d3a3-design.md)
# (COMMITTED; ledger §20(AR); ORCHESTRATOR-RE-VERIFIED pre-commit — the R-DUP
#  mechanism + O1 + the anchor-correction findings re-read at code)

ORCHESTRATOR PRE-COMMIT VERIFICATION (2026-07-30, E-77): the HIGH finding
F1/R-DUP is CONFIRMED at code — RewireConsumer substitutes ONLY `c->view ==
read` (Demand.cpp:236), and its own comment (:230-231) documents the
shared-reader shape, so a second adornment's rewire over an already-rewired
consumer orphans its guard (the union-rewire fix rules at code; stage (c) owns
mechanism + verification per R-9). The O1 target (`inst_per_pub`
`std::unordered_map<uintptr_t,unsigned>` at Rel.cpp:4289; `forcing_index` on the
DROp Rel.h:713 stamped at mint :1117 — no threading), the census recount gate
(`demand_instance_enabled` :3999, `expect(kInstanceSeal, exp_instance)` :4020,
NO exp_seal — LOW-1's correction stands), the file-local `static
LiveRecognition ResolveLiveRecognition` :918 (b3's §3.4 test rightly DELETED),
and the internal-only fold helpers (lib/DataFlow/Query.h:1186/:1188, absent from
the public header — F4) were all re-read verbatim. VERDICT: design SOUND and
implementable; all 17 critique findings confirmed, zero owner-escalation.

The merged whole-slice design, adjudicated from the four stage-(b) design annexes
(b1 g3 pass-loop / b2 g1 fold-arm / b3 g2 dedup+design-1+E2c / b4 g5 O1 + g6
witness) and their four fresh critiques, against the BINDING substrate
`d3a3-substrate.md` (ledger §20(AQ)). Written in the `d3a2-design.md` idiom.
Repo READ-ONLY, tip b65e7668 (code bytes == the D3.a.2 landing bfc068d1); every
load-bearing anchor below was RE-READ at code this session by the adjudicator and
every critique finding was independently re-verified at code (§7). The four
annexes b1–b4 are committed alongside as the sub-lane records: **this design binds
where it amends; a lane edit spec is adopted verbatim where the design is silent.**

The slice = ONE commit, coherent sub-diffs (the d3a2 precedent). SCOPE = OD-15
(N disjoint stores, one per (query, BindingPattern), sharing ONE pub; recursive
demand DEFERRED all-epoch).

===============================================================================
## §0 HEADLINE + RATIFICATIONS

### 0.1 The headline shape

Multi-adornment demand lands by (i) lifting the per-name reject at
`Demand.cpp:457` and looping the pass STEP-1b→10 per adornment (g3/b1); (ii)
installing the kBody-survivor policy so a fold cannot silently drop a body input
(g1/b2, FIRST/BINDING); (iii) relaxing V-INST-SOLE to key `(pub_table,
forcing_index)` so N forcings may share one pub (g5/b4, substrate §6 O1); (iv) the
NEW From-preserving two-adornment success witness `demand_multi_adorn_witness`
(g6/b4); plus the g2 builder dedup + [F] fence + E2c clone + design-1 inline teeth
(b3). All emitted-code surfaces are **[BYTE]** for the whole single-adornment
corpus; the only structural/new golden is the new witness.

### 0.2 THE ONE HIGH FINDING THAT REWRITES THE SLICE (R-DUP)

**b1-critique Finding 1 is CONFIRMED at code and is the central adjudication of
stage (b).** For EVERY two-adornment query the two adornments trace to the SAME
`q_read`/`q_consumer` (the query projection is one clause, one materialization —
`Demand.cpp:477` `q_rel->inserts.Size() != 1u`; the reader is full-width so both
bound columns route to it, `Demand.cpp:530-552`), AND — for the g6 seed
`q(A,B):rel(A,B)` — to the SAME body-guard `(consumer, read)` over `edge_2`. Step 7
(`Demand.cpp:1013/:1025`) and Step 8 (`Demand.cpp:1065`) call
`RewireConsumer(consumer, read, …)`, which substitutes ONLY columns with
`c->view == read` (`Demand.cpp:236` `if (c->view == read)`). So the FIRST
adornment's rewire redirects the shared consumer's uses of `read` to its guard;
the SECOND adornment's rewire then finds NO `read`-use left → substitutes nothing →
its guard is ORPHANED → dead-flow-eliminated → its forcing is skipped at mint
(`ResolveLiveRecognition` off the surviving guards, `Rel.cpp:1053`) → the second
adornment's probe UNDER-answers → HP-5 fires at the g6 witness. **b1's "move
Steps 5-10 verbatim under Loop 2" does NOT lower the witness correctly.** This is
a realization GAP, not a ruling conflict (§8): the correct answer is FIXED (the pub
is the reference-counted UNION of both adornments' demanded rows — substrate §6.2),
so it RULES AT CODE (no owner brief). It is FOLDED as amendment **R-DUP** into g3
(§1.5): group the minted guards by `(consumer, read)`; a singleton group rewires
directly (today's bytes, [BYTE] for |plan|==1); a multi-guard group mints a MERGE
UNION of the guards' restored outputs and rewires the consumer ONCE.

### 0.3 Ratifications (co-land / ownership bindings)

- **R-1 — g1 is FIRST/BINDING and co-lands with g3 in ONE commit** (substrate
  §3.5). g1 = `PromoteSurvivorToBody` (View.cpp defn + INTERNAL `lib/DataFlow/
  Query.h` decl) + the g8 stamp-time census belt + the predicate-justification
  comment rewrite (BOTH the `:571-583` residual AND the `:559-562` sentence, per
  b2-F3) + the pure DataFlowValidators teeth. The g3 fence-lift is the only thing
  that first mints N>1 forcings sharing one pub; landing it without g1 re-opens the
  silent-skip cliff with zero protection.
- **R-2 — g3 (b1) OWNS R-DUP.** The two-phase locate/check/mint loop + the
  `seen_variants` binding-pattern dedup + RE-MESSAGE (not narrow) `:668/:723` + the
  shared-consumer UNION rewire (R-DUP, §1.5). g3's Loop 2 is on the critical path
  for the witness's correctness (b1-critique Finding 2).
- **R-3 — g5/O1 is co-committed, byte-neutral, exercised only once g3 lands.**
  `inst_per_pub` re-keyed `(pub_table, forcing_index)` at `Rel.cpp:4289/4406/4454`
  + `#include <map>` (b4-LOW-2). Abort predicate `!= 1u` UNCHANGED; message
  sharpened.
- **R-4 — `PromoteSurvivorToBody` is b2-OWNED and a HARD co-land dependency**
  (resolves b3-B). b3's tests do NOT depend on extracting it themselves.
- **R-5 — the g1 direction-(a) teeth is b2's PURE DataFlowValidators unit**
  (hand-mutate two `GuardAnnotation` records, assert `surv.role==kBody`).
  **b3's §3.4 `GuardSurvivorPolicyRecoversInput` is DELETED** — it references
  `static` Rel.cpp symbols the test TU cannot see AND is a toothless masked
  negative (the fold arm is DORMANT — substrate §3.2 — so `ri.ok` holds with or
  without the policy). (Resolves b3-A + b3-D.)
- **R-6 — design-1 inline teeth land by PURE EXTRACTION, NOT the production
  hook.** The `gValidateDROpsTestHook` global in Rel.cpp/Stratum.cpp is the exact
  "env-gated debug scaffolding" class the owner memory forbids
  (`no-env-gated-debug-scaffolding`), and b3-C's g5-conflict justification is
  overstated (the extractable ranges `Rel.cpp:4308-4377` / `:4592-4624` are
  separable from O1's edited lines `:4289/:4406/:4454`). Extract
  `CheckInstantiateEffects` / `CheckInstanceInputDrain` (taking `diff`/`input_diff`
  as params) — the `CheckInstanceInputArm` precedent, used 3× already. Design-1 is
  pure test-hardening with NO multi-adornment dependency; if extraction entangles
  with the O1 co-land at implementation, **design-1 DEFERS to a follow-up** (it is
  the lowest-priority sub-diff and must not jeopardize the slice).
- **R-7 — g6 flagship is mono-only** (`demand_multi_adorn_witness`, bare `-demand`
  + `-demand-instance` eqgate). `demand_multi_adorn_diff_witness` DEFERRED. eqgate
  4→5. `demand_multi_adorn_1` STAYS diagnostic (its reject MOVES `:457`→`:717-722`).
- **R-8 — ASAN is a pre-registered slice gate** (b2-F1; standing gate per
  MEMORY.md `asan-standing-gate`): ASAN SUITE + ctest under `build/asan`.
- **R-9 — R-DUP cannot be byte-closed read-only.** Stage (c) selects the mechanism
  (union-rewire PREFERRED; per-adornment consumer-subtree duplication the
  alternative) and MUST verify against the g6 eqgate (flat==nested==golden) + the
  NEW structural gate (each forcing's `ResolveLiveRecognition` yields a non-null
  `input_table`, `Rel.cpp:976-982`) BEFORE blessing the witness golden.

### 0.4 Substrate rulings PRESERVED (no lane/critic overturns them)

§6 O1 (adopted, O2 rejected); §3 g1 (arm DORMANT + survivor policy + KEEP the
predicate); §2 g6 (`demand_multi_adorn_1` STAYS diagnostic + NEW witness); §4 g3
(RE-MESSAGE `:668/:723`, lift `:444-462`, snapshots-in-loop, stray-union,
once-per-module census). No owner escalation (§8).

===============================================================================
## §1 SUB-DIFF g3 — THE PASS LOOP (b1, ADOPTED-WITH-AMENDMENT R-DUP)

Target `lib/DataFlow/Demand.cpp` `QueryImpl::ApplyDemandTransform` (:385-1167).
b1's §1 is ADOPTED VERBATIM except where §1.5 amends. Load-bearing anchors
re-verified: the reject block `:444-462` (`patterns.size() != 1u` at :457); the
single-materialization pin `:477-482`; the trace `:489-565` (full-width reader at
:530-552; the "must share one projection" belt :545-549); Step 3 SIP `:587-759`
(the two position fences `:668-672`/`:723-726`; left-linear `:717-722`; base-atom
`:679-687`; `sites.push_back` :758; only kPushDown/kReadAtTuple push
`pushdown_reads` :754-757 — a kBaseAtom site does NOT); Step 4 stray `:769-791`;
Step 6 snapshots `:983-985`; Step 7 `:990-1027`; Step 8 `:1035-1066`; Step 8b
`:1074-1083`; Step 10 `:1129-1130`; Step 11 census `:1140-1163` +
`MarkDemandFabricated` :1165.

### 1.1 Two-phase split (ADOPT b1 §1.0–§1.3)

- **(M) above both loops, byte-order unchanged** (:393-482): mode gate, G2 reject,
  `reject` lambda, bound-query collection, empty check, the `>1 bound QUERY` reject
  `:435-439` STAYS (different axis: multiple query NAMES; two adornments of one
  name = ONE (name,arity) REL — the belt's own comment :446-448), `q_rel/q_decl`
  :441-442, `q_insert` :477-482.
- **DELETE the `:444-462` per-name multi-adornment belt** (ADOPT b1 §1.2). The loop
  enumerator + the `seen_variants` dedup subsume it.
- **Loop 1 (Phase 1) — Steps 1b+2+3 per adornment, NO minting**, over
  `q_decl.UniqueRedeclarations()` with the `std::unordered_set<std::string>
  seen_variants` binding-pattern dedup (ADOPT b1 §1.1 — mirror `BuildQueryEntryPoint`
  Build.cpp:682-689; REQUIRED, not optional: `UniqueRedeclarations()` can return
  duplicate-pattern redecls, so the old `patterns`-SET belt existed; without the
  dedup the second duplicate fabricates `demand__q_<adorn>` twice → `assert(!io_slot)`
  Demand.cpp:860). `bound_indices`/Step-2 sourced from `redecl`; Loop-1 tail builds
  the `known_consumers` union + asserts one `p_merge` + `plan.push_back` (b1 §1.0
  `PerAdornment`).
- **Step 4 ONCE between the loops** over the pre-mint `known_consumers` union /
  `plan.front().p_merge` (ADOPT b1 §1.5 / substrate §4.3 ADV-3).
- **Loop 2 (Phase 2) — Steps 5-10 per adornment**, AMENDED by §1.5 (R-DUP): the
  MINT/STAMP/REGISTER stays per-adornment; the REWIRE is deferred and grouped.
  `adorn`/`bound_types` off `a.redecl`; snapshots `:983-985` stay per-iteration
  (ADV-2 — the bucket key `forcing_index` at Rel.cpp:953 / Build.cpp:1513); Step 10
  `ParsedQuery::From(a.redecl)`.
- **(below) ONCE after the last adornment**: Step 11 census + `MarkDemandFabricated`
  (ADOPT b1 §1.3 "below").

### 1.2 RE-MESSAGE `:668-672` and `:723-726` (ADOPT b1 §1.4 verbatim)

Condition `Index() != pos` STAYS (it fences a SIP-derived SIDEWAYS adornment — a
recursion-shape artifact, confirmed at code: a swap recursion with ONE declared
`bf` fires `:723`, substrate §4.4). Text → "Sideways (non-From-preserving) demand
propagation is not yet supported under -demand". All OTHER body-walk rejects STAY
verbatim (NEGATE/AGG :625-628, self-join :633-637, left-linear :717-722, malformed
belts). `expect_diagnostic` is exit-code-only (runall.sh:169-181) → text-only, zero
churn.

### 1.3 The `seen_variants` dedup + `p_merge` belt (ADOPT b1 §4.2/§4.3)

Both are folded-in refinements to the substrate's bare loop; b2/b3/b4 assume the
dedup is present.

### 1.4 [BYTE]-neutrality for |plan|==1 (ADOPT b1 §2, EXTENDED by R-DUP)

Every point of b1 §2 holds; R-DUP ADDS the guarantee that for a SINGLETON
`(consumer, read)` group (always the case at |plan|==1) the rewire is DIRECT — no
MERGE node minted — so the id stream and emitted bytes are byte-identical to today.
Surfaces: 176 non-witness stdouts × 4 modes; `demand_tc_witness.{h,ir,df,rel}gold`;
the 4 frozen nested witness dumps + their eqgate arms; config-invariance. All
**[BYTE]**.

### 1.5 AMENDMENT R-DUP — the shared-consumer UNION rewire (BINDING; the Finding-1 fix)

**Problem (CONFIRMED §0.2/§7-F1):** `RewireConsumer` substitutes only `c->view ==
read` (Demand.cpp:236). Two adornments sharing `(q_consumer, q_read)` (Step 8, ALWAYS
for N≥2) and — for the g6 seed — the body `(consumer, read)` over `edge_2` (Step 7)
double-rewire; the second guard orphans.

**Mechanism (grounded at code).** `MintGuardJoin` returns `out_for_read_pos` mapping
each READ position to the join's output column (pivots lead, then pass-throughs;
Demand.cpp:162-210). `MintRestoringTuple` re-presents a guard's output in READ
column order (Demand.cpp:212-226 — the base/pushdown restore). So **every guard over
one `read`, once restored, presents READ's schema** and is union-compatible. The
amendment:

1. In Loop 2, for each site (Step 7) and for the query-projection guard (Step 8),
   MINT the guard JOIN + stamp `guard_annotation_index` + `guard_annotations.push_back`
   + (Step 8b) register the `RecognizedSubgraph` + (Step 10) the forcing —
   **exactly as today, per adornment** — but DO NOT call `RewireConsumer` yet.
   Instead record `(consumer, read) -> [restored_output_j]`, where `restored_output`
   is a read-schema-ordered view of guard_j: for `kReadAtTuple` this is the
   `out_for_read_pos`-indexed JOIN (identity read-pos map); for `kBaseAtom`/
   `kPushDown` it is the `MintRestoringTuple` result (as today).
2. After all adornments mint, for each `(consumer, read)` group:
   - **singleton** (|group|==1, ALWAYS at |plan|==1): `RewireConsumer(consumer,
     read, restored_output_0_as_read_pos_vec, guard_or_restore_0)` — **today's exact
     call, [BYTE]**.
   - **multi-guard** (N≥2 sharing): mint a `MERGE` whose members are the N restored
     outputs (all READ-schema), and `RewireConsumer(consumer, read, merge_cols_as_
     read_pos_identity, merge)` ONCE. The MERGE is the flat-arm realization of the
     reference-counted-union pub (substrate §6.2); in the nested arm each guard's
     `RecognizedSubgraph` lowers to its OWN store and the union is band-(b)'s
     reference-counted publish (Database.cpp:2736/:2763/:2801 — b4 ADV-10).

**Why R-DUP does not re-arm the fold hazard:** guard_bf and guard_fb read DISTINCT
fabricated demand relations (`demand__q_bf` vs `demand__q_fb`) → distinct
`joined_views[0]` → `Join::Equals` fails → never fold (substrate §3.2). The MERGE
is a new sink node, not an annotated guard. The fold arm stays DORMANT under R-DUP.

**Why R-DUP does not disturb Step 4:** the MERGE is minted in Phase 2, AFTER Step 4
(Phase-1/between) has run over the pre-mint graph.

**Stage-(c) obligations (R-9).** (a) Choose union-rewire (PREFERRED) vs
per-adornment consumer-subtree duplication (the b1-critic alternative); (b) verify
`ResolveLiveRecognition` (Rel.cpp:933-1022) re-derives each forcing's (demand,
input, pub) triple correctly with the MERGE on the `consumer→q_insert` path — the
recognition walks from the stored `RecognizedSubgraph{p_merge, p_bound, q_insert,
guard_indices}` (Demand.cpp:1080-1082), and the interaction with an interposed
union MERGE is UNVERIFIED read-only; (c) the NEW structural gate (b1-critique
Finding 2): for the two-adornment witness, EACH forcing's `ResolveLiveRecognition`
yields non-null `input_table` (`Rel.cpp:976` `role == kBody && !ri.input_table`) —
if either is null a guard was orphaned. Gate BEFORE the eqgate bless.

===============================================================================
## §2 SUB-DIFF g1 — THE FOLD-ARM SURVIVOR POLICY + PREDICATE (b2, ADOPTED-WITH-AMENDMENTS)

Target `lib/DataFlow/View.cpp` + INTERNAL `lib/DataFlow/Query.h` + `lib/DataFlow/
Demand.cpp` (g8 belt) + `tests/DataFlowValidators/GuardAnnotationFoldTest.cpp`.
b2's §0–§5 are ADOPTED with the amendments below. Verified at code: the predicate
`GuardAnnotationsCompatible` returns `a.forcing_index == b.forcing_index &&
a.instance_key == b.instance_key` (View.cpp:585-588); the fold-arm survivor fetch
`query->guard_annotations[that->guard_annotation_index]` (View.cpp:676); CSE picks
`that` by depth/det_seq NOT role (Optimize.cpp:365/375); input derived ONLY from
`role == kBody` (Rel.cpp:976-982); the SECOND consumer (recursive-content fence)
`Build.cpp:1528` (b2 V3b — real); struct `GuardAnnotation` is PUBLIC
`include/drlojekyll/DataFlow/Query.h:988`.

### 2.1 `PromoteSurvivorToBody` (ADOPT b2 §1, AMENDED by F2/F4)

ADOPT b2's pure `PromoteSurvivorToBody(GuardAnnotation &surv, const GuardAnnotation
&loser)` (View.cpp defn beside `GuardAnnotationsCompatible`; call site after
`CheckGuardAnnotationFold` and before `++guard_annotation_folded_count`,
View.cpp:677→678). AMENDMENTS:
- **F2 (BINDING):** the DECL goes in the **INTERNAL `lib/DataFlow/Query.h`, beside
  the OWN-3 decls at :1186-1189** (NOT the public `include/drlojekyll/DataFlow/
  Query.h`). Verified: `GuardAnnotationsCompatible`/`CheckGuardAnnotationFold` live
  at internal `lib/DataFlow/Query.h:1186/:1188`; the public header has the struct
  but not these helpers. State the file explicitly to prevent leaking an internal
  fold helper onto the public API.
- **F4 (fold):** add one sentence to the promotion comment — the survivor resolves
  the RIGHT `input_table` because a fold happens ONLY when `Join::Equals` holds,
  forcing `joined_views[1]` pairwise-equality ⇒ same input table; the compat gate
  governs IDENTITY, `Equals` governs input-table SOUNDNESS.

### 2.2 The g8 stamp-time belt (ADOPT b2 §2, AMENDED by F5)

ADOPT b2 §2.1's always-on kBody-presence census belt appended after Demand.cpp:1162
(inside the once-per-module block g3 relocates below the loop). AMENDMENT **F5**:
cite the belt-silence chain in the comment — `Demand.cpp:758` one-site-per-body loop
(every non-tail path `return reject`, tail `sites.push_back`) + Step 7 per-site
`kBody` mint `:1001-1006` ⇒ ≥1 `kBody` per compiling forcing; a zero-`kBody` forcing
would itself `!ri.ok`→silent-skip, so a belt-abort there is CORRECT teeth. **HONESTY
(L15):** the belt is STAMP-TIME (pre-Optimize; Build.cpp:2587 census before :2599
Optimize) → it catches a MINT bug, NOT a fold regression; the fold-time teeth is
§2.4's pure unit. b2 §2.3's Rel.cpp mint-skip sharpening is a RECOMMENDED companion
owned by g5/b1 — ADOPTED as OPTIONAL (it is byte-neutral and dormant; if g5 declines
it, the fold-time teeth reduces to §2.4).

### 2.3 The predicate justification (ADOPT b2 §3, AMENDED by F3)

KEEP `GuardAnnotationsCompatible` unchanged (substrate §3.4). ADOPT b2 §3.2's rewrite
of the LABELED RESIDUAL (View.cpp:571-583). AMENDMENT **F3 (BINDING):** ALSO scope
the UNCONDITIONAL sentence at **View.cpp:559-562** ("`instance_key` … is
fold-invariant: two guards cannot be `Equals` yet carry different stamped pivot
vectors") to JOIN carriers — substrate §3.4 targets exactly that sentence, and the
new `:571-583` proxy-TUPLE justification (proxy TUPLEs CAN be `Equals` with different
keys) directly contradicts it. Rewrite to "two guard JOINs cannot be `Equals` yet
carry different pivots; proxy-TUPLE carriers are handled by the belt below."

### 2.4 The directed witnesses (ADOPT b2 §4, AMENDED by R-5)

ADOPT b2 §4.1's `MakeAnn` role-parameter extension (defaulted `kBody`, keeps the two
existing callers) + §4.2's THREE pure `ASSERT_EQ` policy tests
(`SurvivorPolicyRepairsQueryProjectionSurvivor` + the two no-op arms) + §4.3's
`GuardFoldTripsOnSameForcingDifferentKey` death test, all in
`tests/DataFlowValidators/GuardAnnotationFoldTest.cpp` (ctest **+4**). Per **R-5**,
b2 §4.2's pure repair unit IS the authoritative g1 direction-(a) teeth (it
hand-mutates the exact kBody-loser/qp-survivor pair the DORMANT arm never produces
live) — **b3's §3.4 live-flow variant is DELETED**. b2 §4.2's deferred "ri.ok holds"
end-to-end half is DROPPED (the DataFlowValidators fixture cannot build a Query
graph; the pure unit + the R-9 structural gate cover the same ground).

===============================================================================
## §3 SUB-DIFF g5/g6 — O1 PER-PUB + THE WITNESS (b4, ADOPTED-WITH-AMENDMENTS)

### 3.1 O1 (ADOPT b4 Deliverable 1, AMENDED by LOW-1/LOW-2)

ADOPT the three edits at `Rel.cpp:4289` (decl `std::unordered_map<uintptr_t,
unsigned> inst_per_pub;` → `std::map<std::pair<uintptr_t, unsigned>, unsigned>`),
`:4406` (increment re-keyed to `{ptr, op.forcing_index}` — `forcing_index` is a
first-class DROp field, `Rel.h:713`, stamped at mint `Rel.cpp:1117`, so no
threading), `:4454-4456` (abort `!= 1u` UNCHANGED, message gains `, forcing)`).
Byte-neutral single-adornment (`{pub,0}==1`). AMENDMENTS:
- **LOW-2:** add `#include <map>` to `Rel.cpp` (today it arrives transitively via
  `Rel.h`→`Program.h:17`; the direct include is IWYU hygiene, matching the design's
  own precision bar).
- **LOW-1 (BINDING anchor fix):** the independent count authority is the census
  recount at **`Rel.cpp:3998-4020`** (NOT b4's cited :3964-3974, which is the generic
  `count_kind`/`expect` lambda), **gated on `context.demand_instance_enabled`**
  (:3999), `++exp_instance` :4011, `++exp_death` :4014, `expect(kSubgraphInstantiate,
  exp_instance)` :4018, `expect(kInstanceSeal, exp_instance)` :4020 — **there is no
  `exp_seal`** (seal reuses `exp_instance`). CONSEQUENCE: the instance recount is
  SKIPPED under the FLAT `-demand` arm (gated off) — so the witness's instance
  census is validated only under `-demand-instance` (the eqgate/`.rel`-pin arm).

### 3.2 ADV-10 per-store coupling (ADOPT b4 Deliverable 2 conclusion, AMENDED by MED-2)

The "NO code change; N stores over one pub have no cross-store aliasing" conclusion
STANDS (verified: per-store namespace `Database.cpp:2391-2392` `sname = "instance_"
+ std::to_string(region.StoreId())`; shared input read non-destructively via range-for
:2560; shared pub folded reference-counted). AMENDMENT **MED-2 (BINDING anchor fix):**
DROP b4's fabricated band-(b) API (`NumTouched()`, indexed `Touched(t)`,
`CurrentContains` — none exist) and adopt the SUBSTRATE §6.2 real anchors, verified
at code: `.Touched()` (Database.cpp:2736, `InstanceStore.h:148`), `.Current(iid)`
:2738, `.Frozen(iid)` :2739, `.SubDerivation` :2763, `.TryAdd` :2785,
`.AddDerivation` :2801.

### 3.3 The g6 witness (ADOPT b4 Deliverable 3, AMENDED by MED-1 + R-7)

ADOPT `demand_multi_adorn_witness` (the §2.3 substrate seed `q(A,B):rel(A,B)` with
`#query q(bound A,free B)` + `#query q(free A,bound B)`), mono flagship, `.drflags`
`-demand`, `.eqgate` `-demand -demand-instance`, no `.batches` (monotone), eqgate
4→5. The 7-line sorted golden VALUES are VERIFIED CORRECT (b4-critique ran bf-alone
+ fb-alone against the runtime). AMENDMENT **MED-1 (BINDING):** the §3.4 driver
harness lines are WRONG against the real generated surface — re-author against a
real corpus driver (verified `demand_neighborhood_mono_witness.main.cpp:37-68`):
`const auto allocator = hyde::rt::MallocAllocator();`; `DatabaseLog log;`;
`hyde::rt::Vec<edge_2_input> ev(allocator);` (the message-input alias, NOT
`Vec<uint64_t,uint64_t>`); `edge_2_2(db, log, functors, std::move(ev));` (message
entry is `<name>_<arity>` = `edge_2_2`, the name literally ends `_2`); `ev.Add({a,
b});` (brace-init). The probe/`q_bf`/`q_fb`/`next` half is correct. Per **R-7**, the
diff-multi-adorn witness is DEFERRED.

### 3.4 The census pin (ADOPT b4 §3.6 default, AMENDED — MEASURE, do not bless-from-prediction)

ADOPT the 12th `.rel` census pin `demand_multi_adorn_witness.rel` + `.irgold` under
`-demand -demand-instance` (the only multi-store census witness; `kSubgraphInstantiate=2`
is the slice headline). b4's C.2 census is a PREDICTION (the combined program cannot
compile read-only): `kSubgraphInstantiate=2 kInstanceSeal=2 kInstanceDeath=0
kIngestFold=3 kEagerForward=3 kCommitSweep=3 kSeedFold=0`. **Stage (c) MUST MEASURE
the combined nested `.rel` against this before blessing** (the D3.a.1 under-prediction
lesson). If any N-scaled count differs, bless the MEASURED census, not the prediction.

===============================================================================
## §4 SUB-DIFF g2 — DEDUP + [F] + DESIGN-1 + E2c (b3, ADOPTED-WITH-AMENDMENTS)

### 4.1 The builder dedup D1 + the [F] fence D2 (ADOPT b3 §1/§2 verbatim)

SOUND, byte-identical (b3-critique verified line-by-line; the 6 delta sites, the
3 reachable id-stream cases). ADOPT the ONE `BuildQueryInjectorFromRegistry(…,
bool is_retract)` + ONE `BuildQueryInjectorProcedure` with the always-on [F] fence
(mirror Build.cpp:508-512, hardening the forcer's NDEBUG-compiled-out assert :393-394).
The registry match keeps BOTH conjuncts (`entry.query == query` + BindingPattern) +
the retract `IsDifferential` gate. **[BYTE].**

### 4.2 The E2c clone D4 (ADOPT b3 §4 verbatim)

SOUND, character-identical emission (b3-critique verified: the two arms differ in one
token, `VecName(input_front)` :2561 vs `VecName(*input_removal)` :2668). ADOPT the
`emit_edge_drain(frontier)` lambda called twice — it structurally enforces the binding
R-A2-TRIGGER §7(2) gate-identity invariant. **[BYTE]** on both frozen diff witnesses.

### 4.3 Design-1 inline teeth D3 (ADOPT b3 §3 MECHANISM, AMENDED by R-6 — EXTRACTION, not hook)

The GAP is real (V-INST-EFFECT totality Rel.cpp:4304-4420 + V-INST-DRAIN input arm
:4592-4624 have no death test; a fake flow aborts at the census `:3998-4020` first).
But per **R-6** the fix is PURE EXTRACTION, not the `gValidateDROpsTestHook`
production global:
- **REJECT** the seam (b3 §3.2/§3.3): a permanent test-only global in shipped Rel.cpp/
  Stratum.cpp is the `no-env-gated-debug-scaffolding` class the owner forbids; and
  b3-C shows the g5-conflict justification is overstated (extractable ranges
  :4308-4377/:4592-4624 are separable from O1's :4289/:4406/:4454).
- **ADOPT** extraction: factor the totality-count loop and the input-drain arm into
  pure `CheckInstantiateEffects(op, diff, input_diff)` / `CheckInstanceInputDrain(op,
  input_diff, table_vecs)` helpers that the inline site CALLS (the `diff`/`input_diff`
  booleans passed as params so the helper is fake-table-safe), death-tested in
  `tests/RelValidators/` by the established `CheckInstanceInputArm` mold — drift
  ELIMINATED. b3's mutant catalogue (E1 role-flip / E2 dropped counter / D1 erased
  kNetRemoval) becomes hand-built-op death tests, no real compile.
- **DELETE** b3 §3.4 `GuardSurvivorPolicyRecoversInput` (per R-5; the g1-(a) teeth is
  b2's pure unit §2.4). b3 §3.3's `InlineInstanceEffectAndDrainTeeth` becomes the
  extraction's death tests.
- **DEFER-IF-ENTANGLED:** design-1 is pure test-hardening with NO multi-adornment
  dependency; if the extraction cannot land clean beside O1 in the one commit, design-1
  slips to a follow-up (the slice does not need it).

### 4.4 g4 keying sweep (ADOPT substrate §5 — VERIFICATION, no re-key)

ADV-7 holds. No name-only keying survives once g3 stamps `forcing_index` correctly.
The directed two-adornment probe per site is a stage-(c) test-authoring task (b1's
new structural gate §1.5(c) + the census pin §3.4 cover the load-bearing ones).

===============================================================================
## §5 CROSS-LANE RECONCILIATION

| edge | binding | status |
|---|---|---|
| g1 co-lands with g3, g1 FIRST | R-1 (substrate §3.5) | the ONE commit; g1 diff in place before the g3 loop goes live |
| g3 owns R-DUP | R-2 / §1.5 | g3's Loop 2 is on the witness critical path (b1-critique F2) |
| g5/O1 co-committed, exercised by g3 | R-3 | line-disjoint hunk (Rel.cpp:4289/4406/4454); [BYTE] standalone on pre-b1 tree |
| `PromoteSurvivorToBody` b2-owned, HARD dep | R-4 | resolves b3-B; b3 tests don't extract it |
| g1-(a) teeth = b2 pure unit; b3 §3.4 DELETED | R-5 | resolves b3-A/b3-D |
| design-1 = extraction, not hook | R-6 | resolves b3-C; honors no-scaffolding; DEFER-if-entangled |
| g8 belt rides g3's relocated once-per-module census | b2 §6 / substrate §4.1 | same-region textual merge (Demand.cpp:1140-1163) |
| g6 witness blocked on b1 (compile) + g2 [F] (run under ASAN/release) | b4 §5.1 | witness golden blessed only after g3+R-DUP land |
| `demand_multi_adorn_1` STAYS diagnostic (owned by g3) | R-7 / substrate §2.2 | `:457`→`:717-722`; coexists with the new golden witness |
| E2c D4 independent | b3 §4 | touches only Database.cpp a2/a2'; can co-land or defer |
| O1 orthogonal to design-1 mutants | b3 §5.2 | mutants perturb effects/table_vecs, not instantiate COUNTS |

**No line-level conflict:** g1 → View.cpp:559-588/675-682 + internal Query.h:1189;
g3 → Demand.cpp:444-462/668/723/770/983 + Loop structure; g5 → Rel.cpp:4289/4406/4454;
g2 → Build.cpp:385-559 + Database.cpp:2547-2709 + Rel.cpp extraction (design-1, if
kept, coordinates the co-land ordering with O1 — O1 lands first, extraction refactors
around the untouched :4308-4377/:4592-4624 ranges); g6 → new `tests/OptDiff/cases/`
files. The g1 belt + g3 both edit Demand.cpp:1140-1163 (same-region merge, named).

===============================================================================
## §6 GATE FAMILY (PRE-REGISTERED)

### 6.1 Phase-A red set + golden churn

- **Phase-A red: EXACTLY +1 case** — `demand_multi_adorn_witness` (GOLDEN-MISSING:
  `goldens/demand_multi_adorn_witness.stdout`, and `…​.rel` if §3.4 census pin
  adopted). No EXISTING golden goes red (O1 byte-neutral; g1 dormant-arm byte-neutral;
  g2 dedup/E2c byte-identical; g3 |plan|==1 + R-DUP-singleton byte-identical).
- **Golden churn: NONE existing.** `demand_multi_adorn_1` STAYS diagnostic (no stdout
  golden; `expect_diagnostic` exit-code-only; reject moves `:457`→`:717-722`).
- **NEW files:** `cases/demand_multi_adorn_witness.{dr,main.cpp,drflags,eqgate}` +
  `goldens/demand_multi_adorn_witness.stdout` (+ `.rel`/`.irgold` if §3.4). CLAUDE.md
  eqgate-carrier list + suite count 178→179; runall.sh NO diagnostic-list edit (the
  new case is a GOLDEN case).

### 6.2 [BYTE] / [STRUCT] per surface

| surface | gate | pred |
|---|---|---|
| 176 non-witness stdouts × 4 modes | SUITE | **[BYTE]** |
| 20 pinned `.irgold` (incl. `demand_tc_witness.{h,ir,df,rel}gold`) | pinned .irgold | **[BYTE]** |
| 4 existing nested witness dumps + eqgate carriers | eqgate + frozen dumps | **[BYTE]** |
| 11 existing `.rel` census goldens | .rel .irgold | **[BYTE]** |
| both frozen diff witnesses (E2c) | stdout/`.rel`/eqgate | **[BYTE]** |
| config-invariance (release==debug single hash) | hash | **[BYTE]** |
| `demand_multi_adorn_1` (4 modes) | `expect_diagnostic` | **[BYTE]** (exit 1) |
| `demand_multi_adorn_witness` stdout | SUITE (4 modes) | **[STRUCT]/new** (answer byte-fixed) |
| `demand_multi_adorn_witness.rel` census (nested) | .irgold | **[STRUCT]** (store-id-anchored, counts pinned, MEASURE §3.4) |
| `demand_multi_adorn_witness` eqgate | eqgate | **answer-identical** flat==nested==golden |

### 6.3 ctest delta

- **DataFlowValidators +4** (b2 §2.4: 3 policy `ASSERT_EQ` + 1 same-forcing-diff-key
  death; `MakeAnn` role param defaulted keeps the 2 existing).
- **RelValidators**: **+ O1 pair** (a double-mint-one-forcing death test that STILL
  aborts under the relaxed key + a two-forcings-one-pub POSITIVE that now PASSES —
  b4 §1.4); **+ design-1 extraction** death tests (E1/E2/D1 mutants + positive
  control) IF design-1 lands (R-6; else deferred).
- **eqgate carriers 4→5.**

### 6.4 census (the new witness)

Nested arm `-demand-instance`: `kSubgraphInstantiate=2`, `kInstanceSeal=2`,
`kInstanceDeath=0` (monotone demand), predicted `kIngestFold=3`/`kEagerForward=3`/
`kCommitSweep=3`/`kSeedFold=0` — MEASURE at stage (c) (§3.4). Recount scales to N via
`Rel.cpp:3998-4020` (gated `demand_instance_enabled`).

### 6.5 config-invariance + ASAN (R-8)

Config-invariance single-hash [BYTE] (no emission change on the corpus). **ASAN SUITE
+ ctest under `build/asan`** (standing gate; the g8 belt heap-allocates two
`unordered_set`s per `-demand` compile, the new fork death tests, and the witness run
are exactly what ASAN gates). Prediction: PASS.

### 6.6 The d7 L-table (both fold directions, keying probes, dedup fence, O1 liveness)

- **L-g1-repair (teeth):** hand-build {kBody loser, kQueryProjection survivor},
  `PromoteSurvivorToBody` → assert `surv.role==kBody` + adopted `demand_side`. Removing
  the policy fails the assert (b2 §4.2).
- **L-g1-noop×2:** survivor-already-kBody / both-qp → survivor unchanged (b2 §4.2).
- **L-g1-belt (teeth):** same-forcing/different-key → `CheckGuardAnnotationFold`
  SIGABRT (b2 §4.3); proves the KEPT `instance_key` conjunct is armed under
  multi-adornment.
- **L-g8-mint (teeth, dormant-on-corpus):** a forcing that stamped guards but ZERO
  kBody → the stamp-time census aborts. Silent on the corpus (≥1 kBody per forcing,
  Demand.cpp:758/:1001).
- **L-keying-probe:** for the two-adornment witness, EACH forcing's
  `ResolveLiveRecognition` yields non-null `input_table` (§1.5(c) — the R-DUP orphan
  guard); a double-rewire regression makes one null → the store is skipped → HP-5
  diverges at the eqgate probe. THE tooth on R-DUP.
- **L-dedup-fence ([F], documented no-suite-coverage):** honest belt — teeth only on a
  corrupt build (missing handler); parity with the retract twin (b3 §5.3). Not a masked
  test.
- **L-O1-liveness (both spaces):** a perturbation making `inst_per_pub` see TWO
  forcings of ONE pub → the RELAXED `(pub, forcing)` key does NOT abort (positive), AND
  a same-forcing DOUBLE-mint STILL aborts (negative) — b4 §1.4. THE tooth proving the
  relaxation kept its bite.
- **L-design1 (if kept):** each mutant is a REAL SIGABRT of the REAL check; the
  POSITIVE control proves the death arms fail for the RIGHT reason (L15 — a mutant that
  aborted for a census reason would also abort the positive control).

**L15 support-1 lesson honored:** every belt AND the eqgate has teeth. The DORMANT g1
arm is red-teamed by the pure hand-mutate unit (which forces the exact shape the live
compile never produces); R-DUP is red-teamed by L-keying-probe (a real HP-5 divergence,
not a belt echo); O1 by the positive+negative pair. NO masked negative.

===============================================================================
## §7 ADJUDICATION RECORD (finding → severity → verdict → disposition → verified-at)

| # | finding | sev | verdict | disposition | verified-at |
|---|---|---|---|---|---|
| F1 | b1: shared `q_consumer`/`q_read` (and shared body `(consumer,read)`) double-rewired; 2nd guard orphaned → witness HP-5 | **HIGH** | **CONFIRMED** | **FOLD → R-DUP §1.5** (union the guards, rewire once; singleton=direct=[BYTE]); stage-(c) verify (R-9) | Demand.cpp:236 (`c->view==read`), :477 (one materialization), :530-552 (shared full-width reader), :1013/:1025/:1065 (rewire sites), :162-210 (MintGuardJoin out schema) |
| F2 | b1: only single-adornment gates pre-registered; the two-adornment structural surface b1 owns is untested | MED | CONFIRMED | FOLD → §1.5(c) NEW structural gate (each forcing's `ResolveLiveRecognition` non-null `input_table`) | Rel.cpp:976-982 |
| F3 | b2: ASAN not pre-registered | MED | CONFIRMED | FOLD → R-8 / §6.5 | MEMORY.md asan-standing-gate |
| F4 | b2: `Query.h` ambiguity (internal vs public) | MED→LOW | CONFIRMED | FOLD → §2.1 F2 (decl in INTERNAL lib/DataFlow/Query.h:1189) | lib/DataFlow/Query.h:1186/:1188 vs include/…/Query.h:988 |
| F5 | b2: `:559-562` comment contradicts the new `:571-583` justification | LOW-MED | CONFIRMED | FOLD → §2.3 F3 (scope :559-562 to JOIN carriers; substrate §3.4 targets it) | View.cpp:559-562 + :571-583 |
| F6 | b2: promotion soundness mis-attributed (compat gate vs `Join::Equals`) | LOW | CONFIRMED | FOLD → §2.1 F4 (one sentence) | Rel.cpp:974-975 |
| F7 | b2: belt-silence precondition uncited | LOW | CONFIRMED | FOLD → §2.2 F5 (cite :758/:1001 chain) | Demand.cpp:758/:1001-1006 |
| F8 | b3: §3.4 `GuardSurvivorPolicyRecoversInput` non-compiling (static Rel.cpp symbols) AND toothless (dormant arm) | MED | CONFIRMED | **DELETE §3.4** (R-5); teeth = b2 pure unit | Rel.cpp:918/:929/:933 all `static`/file-local; Rel.h:1022 only ValidateDROps |
| F9 | b3: the teeth-bearing g1-(a) witness contingent on unowned `PromoteGuardSurvivor` | MED | CONFIRMED | RATIFY R-4 (b2 OWNS `PromoteSurvivorToBody`, HARD dep) | b3 §3.4 self-cites the request |
| F10 | b3: production hook justified by overstated g5 conflict; violates no-scaffolding | MED | CONFIRMED (line-disjoint) | **REJECT hook, ADOPT extraction** (R-6 §4.3) | Rel.cpp:4308-4377/:4592-4624 vs :4289/:4406/:4454 |
| F11 | b3: §3.4 over-asserts `ok` for dead forcings | LOW | CONFIRMED | RESOLVED-BY-DELETING §3.4 | Rel.cpp:4008 `continue` on `!ok` |
| F12 | b3: CMake/Stratum path cites imprecise | COSM | CONFIRMED | FOLD (real paths at landing) | tests/RelValidators/CMakeLists.txt |
| F13 | b4: g6 driver won't compile (StdErrorLog/Allocator/Vec/edge_2 vs edge_2_2/.Add) | MED | CONFIRMED | FOLD → §3.3 MED-1 (real surface); golden VALUES stand | demand_neighborhood_mono_witness.main.cpp:37-68 |
| F14 | b4: ADV-10 band-(b) anchors fabricated (NumTouched/Touched(t)/CurrentContains) | MED | CONFIRMED | FOLD → §3.2 MED-2 (substrate §6.2 anchors); conclusion stands | Database.cpp:2736/:2738/:2739/:2763/:2785/:2801; InstanceStore.h:142/:143/:148 |
| F15 | b4: census-recount anchor wrong (×3) + nonexistent `exp_seal`; instance recount flag-gated | LOW | CONFIRMED | FOLD → §3.1 LOW-1 (real :3998-4020, gated, seal reuses exp_instance) | Rel.cpp:3998-4020 |
| F16 | b4: O1 `std::map` relies on transitive `<map>` | LOW | CONFIRMED (not a break) | FOLD → §3.1 LOW-2 (add `#include <map>`) | Rel.h:33→Program.h:17 |
| F17 | b4: path label / "corrects substrate" overstatement | COSM | CONFIRMED | FOLD (path fix; framing noted) | Database.cpp is lib/CodeGen/CPlusPlus/ |

**Substrate rulings — all PRESERVED, none contradicted:** §6 O1 (adopted, O2
rejected); §3 g1 (dormant + survivor + KEEP predicate); §2 g6 (multi_adorn_1
diagnostic + new witness); §4 g3 (RE-MESSAGE :668/:723). Where a lane deviated
(b3 §3.5 "extraction treads on g5"; b4 band-(b) anchors), the SUBSTRATE won.

===============================================================================
## §8 OWNER-ESCALATION

**NONE.** The one HIGH finding (F1/R-DUP) is a REALIZATION GAP in lowering the
OD-15 N-disjoint-stores-over-one-pub answer at the flat DataFlow guard layer — NOT
an OD-15 or ruling conflict. The correct answer is FIXED and observable-unambiguous
(the pub is the reference-counted union of both adornments' demanded rows; the
eqgate + HP-5 probes pin it), so it RULES AT CODE per the d2 / R-A2-TRIGGER
precedent, no owner brief. It IS a stage-(b) incompleteness that cannot be
byte-closed read-only (R-9): stage (c) selects the mechanism (union-rewire preferred)
and verifies against the g6 eqgate + the §1.5(c) structural gate before blessing.

The two contingent owner-brief triggers stay DORMANT: (i) a DEMONSTRATED live
role-divergent both-set fold — none produced (the arm is dormant even under R-DUP,
§1.5; distinct fabricated demand relations ⇒ never Equals); (ii) any body-walk /
recognition WIDENING (R-5/ADV-9 derived-input branch) — the multi-adornment loop
does NOT widen admitted body shapes (each declared adornment is an independent
From-preserving SIP; `:668/:723` are RE-MESSAGED, not lifted), so the OB8(i)
obligation is not triggered.

**Single-pass note for stage (c):** re-verify the R-9 anchors (the `ResolveLiveRecognition`
× interposed-MERGE interaction, Rel.cpp:933-1022 / :976-982) and the MEASURED witness
census (§3.4) before blessing — these are the two load-bearing items this read-only
stage could not close.
