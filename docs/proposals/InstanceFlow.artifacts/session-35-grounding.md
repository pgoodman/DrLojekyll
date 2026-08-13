<!-- Copyright 2026, Peter Goodman. All rights reserved. -->
# Session-35 grounding — Phase-C step 2: the op-model TABLE*→StateResourceId retype

Status: DRAFT for the refuter panel. Tip `53131835` (branch `keyed-instances`).

## §1 What is true at tip (verified this session)

- **Resolution is TOTAL by construction.** Every non-null base-table `TABLE*` on
  every `DROp`/`DRBranch`/`DRJoin`/`DREffect`/`PlanNode` is sourced from
  `impl->view_to_model[view]->FindAs<DataModel>()->table` or the
  `for (TABLE *table : impl->tables)` loop var, and every `DataModel::table`
  is created at the single choke-point `DataTableImpl::GetOrCreate`
  (`lib/ControlFlow/Data.cpp:194`) which registers it into `impl->tables`.
  `BuildDRInventory` runs strictly AFTER `FillDataModel`. ⇒ every base-table
  field points at a table that has a `DRTable` (hence a resolved `resource`).
- **The `DRInstance` / kSubgraphInstantiate family is DEAD.** `flow.instances`
  is never populated; `DROp::demand_table` / `DROp::input_table` (the subgraph
  family) have NO assignment site (P1 greenfield cut deleted `-demand-instance`;
  not yet rebuilt). Their `TABLE*` fields are always null. The belt SKIPS nulls,
  so no dead shadow is written.
- **View partners exist for real cross-checks** (op-construction pairing teeth):
  `negate_table↔QueryView(*negate)`, `negated_table↔negate->NegatedView()`,
  `pred_table↔*pred_view`, `product_table↔*product_view`,
  `side_tables[j]↔JoinedViews()[j]`, `fire_table↔*fire_join`,
  `gate_table↔gate_negate->NegatedView()`, `agg_table↔*agg_view`,
  `ingest_table↔*ingest_receive`, `branch.source↔path.front()`,
  `branch.target↔path.back()`. The per-table claim/filter/retire/rederive/
  commit `table_op_table` is sourced straight from the `impl->tables` loop var
  (no view field) ⇒ resolution-only.
- **DRTable already carries `resource`** (s34), cross-checked by V-REL-RESOURCE
  (member views share ONE class; class has one authoritative resource).

## §2 The Step-2 design (chosen shape)

### §2.1 A SINGLE central map (not 44 parallel fields)

`DRFlowGraph::table_resource : unordered_map<TABLE*, StateResourceId>`, populated
in `BuildDRInventory` immediately after the `DRTable` loop:

```
for (const DRTable &t : flow.tables)
    flow.table_resource.emplace(t.model, t.resource);
```

WHY the map, not per-field ids: a per-op `StateResourceId` field would be
`table_resource[field]` verbatim — 44 redundant fields + 44 stamp sites for a
SHADOW. The map is the derivable truth, one source, O(tables) build, O(1)
lookup. It is LITERALLY the `TABLE*→StateResourceId` function Step 3's allocation
inversion consumes (invert it: for each resource mint its TABLE up front, then
region builders look up ids instead of `GetOrCreateIndex`/`view_to_model`). So
the map is the Step-3 bridge, not a throwaway.

### §2.2 The belt V-REL-OP-RESOURCE (at the BuildDRInventory tail)

For the FULLY built `flow`, walk every op (its kind-specific base-table fields +
its effects' tables + its arms' spine trees' tables), every branch, every join:

- **resolution**: every non-null base-table field is a key in `table_resource`
  (fires on a table outside `impl->tables` — a dangling/foreign pointer);
- **class cross-check** (teeth): where a semantic view partner exists, assert
  `table->views.front().EquivalenceSetId() == partner_view.EquivalenceSetId()`
  (fires on an op that paired the wrong table with its view).

fprintf+abort, NDEBUG-surviving (the V-REL-RESOURCE idiom).

### §2.3 The render (`-rel-out`)

` resource=sr#K` after each **`args:`-line** base-table token (the canonical
"arguments" restatement — one consistent rule; NOT the header/detail duplicates
that would double-render for kGroupUpdate/kSeedFold). In-scope sites
(`lib/Rel/Format.cpp`): kGroupUpdate args (agg_table 718, input 721), kStateSeal
(731), kIngestFold (742), kSeedFold args (src 784, tgt 796), kClaimDrain/
kFrontierFilter/kCommitSweep (809/819/830), the 8 kEager* markers + kJoinEmit/
kProductEmit (846/857/880/892/910/920/935/945/971, guarded non-null), and the
DRInstance line (665/666 — zero corpus bytes, instances empty). Effects/reads/
spine/branch/join sublines and header duplicates are OUT (belt covers them).

Shape: `args: table=%table:4 resource=sr#0`. Emitted via a `tidr(TABLE*)` helper
= `tid(t) + " resource=sr#" + id`, used only at the args sites.

### §2.4 Goldens

All 10 `.rel.opt.golden` move (every one has op args-lines with base tables) —
an ADDITIVE within-line token, predicted per line then re-blessed. Codegen
goldens (`.h`/`.ir`/`.stdout`/oracle/monotone/behavioral) BYTE-IDENTICAL and the
`.materialization`/`.contract`/`.df`/`.region` dumps untouched (this slice edits
only `Rel.h`/`Rel.cpp`/`Format.cpp`, all downstream of and invisible to codegen).

## §3 Line of sight to Step 3 (the allocation inversion)

`table_resource` IS the inverse Step 3 needs. Today: `FillDataModel` mints tables
+ `GetOrCreateIndex` mints indexes INLINE mid-region-build; `BuildDRInventory`
shadows the result. Step 3: an `AllocateRuntimeResources(materialization)` mints
one TABLE per `StateResource` up front; region builders LOOK UP
`resource→TABLE` (the inverse of `table_resource`) instead of allocating.
Step-2's map + belt PROVE that map is total, single-valued, and class-coherent —
the precondition Step 3 stands on. Step 3 additionally needs the ARRANGEMENT
half (§2.5.3 — no standing index-requirements pass); Step 2 does not.

## §4 The Step-2b decision: DEFER (grounded)

Step 2b reconciles the materialization `support` (`OR CanReceiveDeletions`) with
`TableIsDifferential` (`OR (CanProduceDeletions ∨ agg/kv)`) and cross-checks
`resource.support == DRTable.differential`.

MEASUREMENT (agent C, empirical on the built compiler): the two notions **DO
diverge** — on an aggregate/KV OUTPUT table, `TrackDifferentialUpdates`
(`Differential.cpp:74`) forces `can_produce_deletions=true` but NOT
`can_receive_deletions`, so `support=monotone` (CanReceiveDeletions) while
`DRTable.differential=true` (CanProduceDeletions∨agg∨kv). Confirmed live on
`aggregate_1` (sr#2 monotone vs `%table:5` differential), `average_weight`
(1/8), `agg_distinct_1` (1/8). NEGATE never diverges (`Negate.cpp:14` forces
receive⇒produce lockstep). The 5 committed `.materialization` goldens contain NO
aggregate/KV case ⇒ reconciling moves **0** committed goldens.

WHY DEFER (two concrete risks the measurement surfaced):
1. **Semantics of a landed authority.** Reconciling changes `support_of` in the
   s34 MaterializationPlan — a behavioral change to a landed observer, and it
   makes Materialization `support` DIVERGE from the Regional row-contract
   convention it deliberately mirrors. Bigger blast radius than an additive
   shadow.
2. **Unverified view-set-equality premise (the teeth-killer).** The cross-check
   `resource.support == DRTable.differential` compares an OR over the class's
   views (`ForEachViewKindTagged`, `Materialization.cpp:196` — ALL views in the
   `EquivalenceSetId` class) against an OR over `table->views` (the narrower
   REGISTERED feeders). Those view sets are NOT proven equal; a class-view with
   `CanProduceDeletions` outside `table->views` would fire the cross-check on a
   valid program. This must be PROVEN before the cross-check is safe.

CONCLUSION: Step 2b is genuinely valuable (a real cross-authority invariant) but
NOT trivially clean. Land Step 2 with the §2.2 view-class cross-checks (safe by
construction — the partner view minted the field via `view_to_model[view]`, so
`field_table.class == view.EquivalenceSetId()` holds and PINS the pairing, the
s32-bug category), and defer Step 2b to a slice that (a) proves view-set equality
or reformulates the cross-check table-side, and (b) blesses the agg/KV
`.materialization` goldens so the reconciliation is visible.

## §4.5 Panel folds (refuter 2, opus)

- **(b) SURVIVES on mechanics** (non-vacuous — all 10 goldens have args-line
  tables; deterministic/total/injective — sr#K keys on lc-id/eqset canonical
  order, one TABLE* per eqset per V-MAT-BIJECTION). negate_1 mapping CONFIRMED:
  %table:4→sr#0, %table:11→sr#1, %table:8→sr#2.
- **(b) reviewability finding, FOLDED-as-adequate:** where two resources share
  schema+support (join_1 sr#0/sr#1 both `(B) monotone`), the `.rel resource=sr#K`
  and `.materialization sr#K` share ONE id space (the `.rel` id IS
  `plan.resources[k].id`), so the dumps ARE cross-referenceable by id; per-table
  correctness is certified by the belt (V-REL-RESOURCE class-coherence +
  V-REL-OP-RESOURCE pairing), the golden PINS the deterministic mapping. Belts
  certify, goldens pin — no eqset token needed on the `.rel` line (kept clean).
- **(d) SURVIVES, QUALIFIED — HONEST FRAMING ADOPTED:** the map is a transitional
  re-index of the s34 `DRTable.resource` shadow and the render is observability;
  the GENUINELY non-shadow artifact is the **op-level V-REL-OP-RESOURCE pairing
  belt** (§2.2 view-class cross-checks — the op↔view↔table certification that
  V-REL-RESOURCE cannot make, an F17/F18-class catch, and a real precondition for
  the Step-3 op-field retype). Step 2's value is the CERTIFICATION half of the
  retype + the pinned coverage artifact — stated plainly, not oversold.
- **Both refuter 2 and agent C push to bundle Step 2b.** Their argument targets
  the PREDICATE difference (CanReceiveDeletions vs CanProduceDeletions∨agg/kv);
  it does NOT address the VIEW-SET difference (§4 risk 2) that makes the
  table-vs-plan support cross-check unsound as-formulated. Defer stands; §4
  records the premise to discharge.

## §4.6 Panel folds (refuter 1, opus) + the HONEST verdict

- **(a) resolution-total: SURVIVES.** No counterexample. Correction to §1: there
  is a SECOND `tables.Create` — `BuildEmptyQueryEntryPoint` (`Build.cpp:519`, an
  all-empty table for an all-free `#query` with no backing INSERT) — but it runs
  AFTER `BuildDRInventory` (inventory is inside `BuildEntryProcedure`,
  `Build.cpp:1422`; the empty-query pass is `Build.cpp:1454`), has no member
  views, and is referenced by no `DROp` field. Harmless; `flow.tables` is a
  complete snapshot at inventory time.
- **(c) pure-shadow: SURVIVES.** `EmitDRFlow` is `-rel-out`-only
  (`DumpRelIfEnabled` PRE-guarded on `gRelDumpStream`); codegen never sets it. No
  `Emit*`/`Lower*` reads `.resource`/`table_to_resource`. `DRFlowGraph`/`DRTable`
  have no Hash/Equals/serialization; adding one map field is the s34
  `DRTable::resource` move at container grain (SUITE PASS 227 precedent). The
  belt cannot spuriously abort: every view has a real `EquivalenceSetId`
  (`BuildEquivalenceSets` is unconditional), and `table->views.front()` never
  crashes (s34 belt already guarantees non-empty single-class member views).
- **IMPLEMENTATION obligation (adopted):** run the RESOLUTION check BEFORE the
  class cross-check, so a hypothetical dangling pointer reports "unresolved"
  instead of dereferencing `->views.front()`.

### THE HONEST VERDICT (the crux the charter told me to surface)

Refuter 1's caveat, verified: for every op whose base table is minted from the
SAME `view_to_model[partner_view]->table` expression as the partner view
(crossover negate/negated/pred, product/side, agg, ingest, fire, gate — i.e.
ALL the view-bearing families), the class cross-check is **TAUTOLOGICAL at
construction** (same DataModel ⟺ same EqSetId). The resolution check is likewise
total by construction (§4.6 a). ⇒ **V-REL-OP-RESOURCE has no bug-catching teeth
TODAY.** It is a REGRESSION FENCE that earns teeth exactly when Step 3 begins
storing an op's table and its id/view from INDEPENDENT sources that could
disagree — the same "currently-always-holds invariant" role as V-PROJ-ROLE-
STABLE / V-XOVER-ONE and the rest of the always-on belt family.

So what Step 2 REALLY delivers (stated plainly, not oversold):
1. **A goldened, corpus-wide coverage artifact** — the `.rel` render pins the
   full `TABLE→StateResourceId` mapping as byte-exact truth (the repo's core
   "pin truth with goldens" methodology); any future perturbation of the mapping
   moves a golden and is caught. This is the prompt's "REAL, pinned artifact".
2. **The Step-3 bridge** — `table_to_resource` IS the `TABLE*→id` function the
   allocation inversion inverts (`id→handle`); a transitional but load-bearing
   index.
3. **A regression fence** (the belt) for the op-field retype to come.

It is NOT a live bug detector, and the map/render are ~2/3 re-surfacing of the
s34 `DRTable.resource` shadow. That is real incremental infrastructure of the
same kind s34 step 1 was accepted as — but it is THIN, exactly as the charter
anticipated ("if step 2 is too thin to be real, say so"). Hence the paired
forward deliverable below.

## §6 Recommendation to the owner

**Land Step 2 as the pinned coverage brick + regression fence (honestly framed),
AND scope Step 3's arrangement derivation (§2.5.3 — the named TRUE blocker) as
the grounded forward plan.** Rationale: Step 2 is low-risk, byte-identical-codegen
real infrastructure (goldened coverage + Step-3 bridge), and pairing it with a
scoped arrangement-derivation plan gives the session real code landed + a
ratified path that actually advances toward the codegen payoff — rather than a
thin shadow alone. Step 2b is deferred on the §4 view-set-equality soundness
blocker (NOT the predicate difference the panel focused on).

## §5 Refuter targets (default refuted=true on uncertainty)

- (a) Find a base-table field NOT resolvable via the map (a table ∉ impl->tables;
  an index/arrangement masquerading as a base table).
- (b) Find a witness where an op field's resource "bites" (render non-vacuous)
  AND confirm codegen stays byte-identical.
- (c) Find an `Emit*`/`Lower*` path that would read the new map/field for
  EMISSION (breaking the shadow invariant).
- (d) Argue Step 2 is shadow-for-shadow with no real line to Step 3.
