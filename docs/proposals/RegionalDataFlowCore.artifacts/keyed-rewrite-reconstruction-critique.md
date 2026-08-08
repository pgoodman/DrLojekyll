# Keyed-instance rewrite — adversarial re-critique of the corrected reconstruction diffs (ranked)

Session 13 (2026-08-06, tip `46a404d4`). An independent opus refuter panel (7 refuters:
one per phase cluster + a cross-phase completeness critic) attacked the session-13 corrected
diff set `keyed-rewrite-reconstruction-diffs.md` — the diffs that already folded the s12
critique's 33 findings. Every CONFIRMED finding was verified by opening the cited
`file:line`; REFUTED = the diff (or a refuter's own claim) did not survive verification.

**Tally: 23 surviving (5 blocking, 6 high, 8 medium, 4 low; 20 CONFIRMED, 3 PLAUSIBLE) + 1
refuter-claim REFUTED-BY-VERIFICATION + 20 certifications** (diffs that held under attack).

## Owner headline

The DIRECTION holds — the strongest mechanism claims are CERTIFIED against real code
(ProgramTableScanRegion reuse genuinely emits the honest key-filter scan with index=nullopt;
codegen is label-blind so D4 Option-1 is right; the landed OVERDELETE→REDERIVE machinery
exists at the cited anchors; F13 render is byte-identical; F8/F21 folds are sound). But the
corrected P1 inventory is STILL not compile-clean (it missed the ControlFlow injector +
Regional census consumers of `QueryDemandForcing`/`DemandForcings()`), and three
reconstruction phases carry real soundness/silent-pass gaps: P3 doesn't root permanent-root
programs (73% of the corpus publishes nothing in the abstract model); P4's exit gate is
non-discriminating (a no-op passes it — the P1 baseline cursor already emits a key-filter
scan); and D3/P6.5's DRed design both mis-attributes the counter model (per-row-global, not
per-BindingState) and drops cross-component transitive retraction. All are addressable
amendments, folded into `keyed-rewrite-reconstruction-diffs.md` §5.

---

## BLOCKING

### B1 · CONFIRMED · P1 · missed-deletion — the atomic cut STILL won't compile (injector + census consumers of QueryDemandForcing)

Two refuters (P1-compile-clean + cross-phase) independently confirmed the corrected §1
inventory omits live consumers of the P1-deleted `QueryDemandForcing` struct /
`Query::DemandForcings()` accessor / `context.demand_forcings` field:
- **ControlFlow injector:** `BuildQueryInjectorFromRegistry` (Build.cpp:413-503) + the
  registry branch of `BuildQueryInjectorProcedure` (Build.cpp:507-519, `for (const
  QueryDemandForcing &entry : *context.demand_forcings)` at :511, `const QueryDemandForcing
  &entry` at :415); `Context::demand_forcings` field decl at **Build.h:127** (P1.3 deleted
  only the `:1560` assignment). `BuildQueryInjectorProcedure` must SURVIVE (its
  `ForcingMessage` fall-through at :522-527 serves user `@first` forcing, still called at
  :551/:556) — so the registry helper + branch must be *surgically* excised, not blanket-deleted.
- **Regional census:** `DeriveRegionalCensus` (Planning.cpp:388, `request_ports =
  query.DemandForcings().size()`) — which §1/P2 EXPLICITLY keeps as a function of `query`;
  the request-port loop `const std::vector<QueryDemandForcing> &forcings =
  query.DemandForcings();` (Planning.cpp:445); `NumForcingsOfName` helper
  (Planning.cpp:142, takes `const std::vector<QueryDemandForcing>&`).
- **The struct/accessor themselves:** `QueryDemandForcing` (Query.h:985), `DemandForcings()`
  decl (Query.h:1093), `demand_forcings` member (Query.h:1222).

**Fix (folded, §5).** Add to the P1 atomic inventory: excise `BuildQueryInjectorFromRegistry`
+ the registry branch (preserve the `ForcingMessage` fall-through), delete `Context::
demand_forcings` (Build.h:127); retarget `DeriveRegionalCensus` so `request_ports` no longer
calls `DemandForcings()` (literal 0); delete the Planning.cpp:445-468 request-port loop +
`NumForcingsOfName` (:142); delete `QueryDemandForcing`/`DemandForcings()`/`demand_forcings`.

### B2 · CONFIRMED · P3 · reintro-gap — permanent roots don't root the empty state; 73% of the corpus publishes nothing

`BuildRequestPorts` routes permanent roots to a separate list (`permanent_roots.push(...)`),
NOT via `AddRequestEdge`. But `RootedReachability` (seed:266-272) seeds `live` ONLY from
`request_edges` with `RootAlive`, and P3 `EvaluateEpoch` gates ALL derivation on `for st in
{EmptyBindingState(R)} ∩ live`. A program with no bound `#query` (≈73% of the corpus per
CLAUDE.md's "~27% carry bound queries") has no request edge → `live={}` → nothing derives →
nothing publishes. Violates the retained "every root/permanent/parent-member has an exact
owner" (next-session-prompt:639) and the `RequestOwnerId::PermanentRoot` arm. The key_*
exit battery (all bound-query cases) never catches it.

**Fix (folded, §5).** In `BuildRequestPorts` also `AddRequestEdge(PermanentRoot(id),
CallSiteId(redecl), EmptyBindingState(ri))` with `RootAlive(PermanentRoot)≡true`; add a P3
exit-gate probe that a no-bound-query program still publishes its full answer.

### B3 · CONFIRMED · P4 · silent-pass-test — all four exit-gate probes are non-discriminating

A no-op P4 that leaves bound queries on the plain post-P1 cursor passes every probe:
- Probe (1) greps for a `NumRows` scan + key-equality filter — but the POST-P1 baseline
  cursor ALREADY emits exactly that (Database.cpp:1768-1799: `while (pos < db.<member>.
  NumRows()) { ... if (row.<field_i> != <param_i>) continue; }` when no covering index).
- Probe (2) ("2nd requester adds a RoutedResult, ZERO new fact Tables") is doubly dead:
  `RoutedResult`/`RequestEdge` have NO codegen surface at P4 (Identity.h:80,93 say "no
  producer until Stage C"), and "zero new Tables" is trivially true for any impl (tables are
  minted at data-model time, never per requester).
- Probe (4) ("Database.cpp:2339-2341 caveat gone") is vacuous — that caveat lives inside
  `EmitSubgraphInstance`, deleted at P1.5, not P4.
- Probe (3) is the default one-table-per-relation.

**Fix (folded, §5).** Adopt D1(i) so the keyed scan carries the terminal BindingStateId
VALUES; assert the emitted filter constant equals the bound value threaded through a
`ProgramTableScanRegion` (distinguishable by the region-cursor `s<id>` naming vs the
query-cursor `pos`/`_cursor` shape at Database.cpp:1768); drop probe (2)'s datalog.h clause
(no runtime surface — move to a `-region-out` compile-time assertion); re-anchor probe (4)
to a line P4 actually changes.

### B4 · CONFIRMED · D3/P6.5 · retained-invariant-violation — "per-BindingState C_nr/C_r" makes a binding state a second fact owner

The landed `C_nr`/`C_r` are PER-ROW-of-a-GLOBAL-TABLE membership counters (Table.h:20-32;
`EmitRederive` Stratum.cpp:684-687 reads `kRecursivelySupported` via CHECKMEMBER over the
whole `table`). But the reconstruction's membership authority is per-FACT
(`AddDerivation` aggregates `d = derivations_of(fact_id); d.support += delta` — single
`RegionalFactRelation`, seed:283-288). Keying `C_nr`/`C_r` "per BindingState" either
contradicts the per-fact aggregation OR gives each binding state its own presence-counter
store = a per-state membership copy — exactly the "do not let a binding state become a second
owner of relation facts" false-start (next-session-prompt:830). The landed counter model IS
the global membership store; it cannot be "re-keyed per BindingState" by mirroring.

**Fix (folded, §5).** Drop "mirror the landed per-row C_nr/C_r keyed per BindingState."
Specify DRed over the per-FACT `FactDerivation.support` in the single `RegionalFactRelation`
(the recursive-vs-non-recursive support split lives ON the FactDerivation, aggregated per
`fact_id`) — a real REBUILD of the counter model, stated as an obligation, not a reuse of
Stratum.cpp/Table.h.

### B5 · CONFIRMED · D3/P6.5 · soundness-gap — the (A)/(B) split drops cross-component transitive retraction (DELETION CONTRACT)

Pass (A) is scoped to `AffectedRecursiveComponents(input_deltas)` — components hit by a
DIRECT input delta. A component C2 affected only TRANSITIVELY (via a fact retracted in C1,
not a direct input) is out of scope. Pass (B) is monotone add-only for reachability + a
semi-naive worklist that only pushes FORWARD on NEW facts — it names no producer of negative
deltas. So a support-loss whose consequence lands in C2 is retracted by neither pass, and
there is no fixpoint back to (A): C2's fact stays Present though fresh-from-committed drops
it. The landed engine avoids this by running OVERDELETE→REDERIVE→INSERT across ALL strata in
stratum order (Stratum.cpp:2447→:2490) with each stratum's signed net-removal frontier
(Stratum.cpp:1856-1869) feeding downstream. "(A)-once-then-(B)" drops that cross-component flow.

**Fix (folded, §5).** Make (A) the transitive closure over all affected SCCs run to a joint
fixpoint interleaved with (B) — one worklist carrying signed frontiers across component
boundaries, mirroring the per-stratum frontier flow — OR wrap (A)+(B) in an outer
repeat-to-fixpoint (the seed §3 single "repeat to a JOINT least fixpoint"). Related: M-medium
P6.5-ordering (the same outer-fixpoint gap) and B5 are the same defect at two grains.

---

## HIGH

### H1 · CONFIRMED · P2 · missed-deletion — retyping frozen_census orphans its sole reader (and is unmotivated)

The P2 diff retypes `Context::frozen_census` (Build.h:230) to `const RegionTemplate
*frozen_regions`, but its ONLY reader (Rel.cpp:4638-4662, V-REGION-CENSUS) reads a
`RegionalCensus` (7 `census_field` members), and `RegionTemplate` has no census member → P2
won't compile. The retype is ALSO unmotivated: the same doc's F12 keeps the census
query-derived (`out.census = DeriveRegionalCensus(query)`). **Fix (folded):** do NOT retype;
keep `const RegionalCensus *frozen_census`; add a SEPARATE `frozen_regions` field only if a
later phase needs the typed record. (This corrects an over-eager retype I introduced folding
s12-F31; F31's real requirement was just to LIST the field, not retype it.)

### H2 · CONFIRMED · P2 · silent-pass-test — V-REGION-CENSUS is fully tautological; the real anti-stub teeth (internal recount over the built vectors) are being deleted

V-REGION-CENSUS compares `DeriveRegionalCensus(query)` to a stored census that IS
`DeriveRegionalCensus(query)` — `f(query)==f(query)`, tautological on ALL seven fields (not
just request_ports). The ACTUAL teeth today are the in-`Build` recount at Planning.cpp:663-687
(`built_request`/`built_input`/`built_result`/`contracts.size()` vs `out.census`) — which
reads the BUILT render vectors P2 DELETES. My P2 exit-gate item 3 ("a recount that NEVER
reads R still aborts a hollow R") is self-contradictory: a recount blind to R cannot detect a
hollow R. **Fix (folded):** P2 must re-provide the internal recount reading the NEW typed
`RegionTemplate` (`R.relation_schemas.size()`, `R.request_ports.size()`, …) against
`DeriveRegionalCensus(query)`, replacing the deleted Planning.cpp:663-687 check; re-scope the
exit-gate wording (the Rel-tail belt is tautological; the anti-stub belts are the region
goldens + this new internal recount).

### H3 · CONFIRMED · P3 · soundness-gap — F29 ProjectRow(row, member_key) is a value-id↔ordinal domain mismatch

`member_key` is a `vector<FieldId>` of view-relative column VALUE ids (RowContract.h:34-46:
"not an array index"), not runtime-tuple positions. `AddDerivation` passing raw `member_key`
to `ProjectRow(row, …)` indexes the wrong columns / out of bounds. The ONLY defined
value-id→position mapping is exactly the `visible_fields` membership loop P2 precomputes as
`declared_key_positions` (F13) — but the F13 bridge was applied only to rendering, not the
derivation path. **Fix (folded):** thread `declared_key_positions` (or a positional member-key
derived from visible_fields) into `ProjectRow`; never index a runtime tuple by raw FieldId
value-ids. (F29 and F13 must share ONE positional projection.)

### H4 · CONFIRMED · D1 · soundness-gap — the "minimal, no P5 machinery" pull-forward is incoherent

`BindingStateId` embeds a `BindingStateSchemaId` whose interner (`BindingStateSchema =
intern(schema_table,…)`) is FIRST defined in P5; the value-bearing id is "runtime-only,
minted in EvaluateEpoch" — yet D1 re-seeds it into the compile-time request port at
BuildRequestPorts (where values are unknown). So D1 cannot carry values at compile time and
must pull `BindingStateSchema` interning (P5) forward, contradicting "does NOT require P5's
machinery." **Fix (folded):** P4's request port scopes to a compile-time
`BindingStateSchemaId` (which bound fields); the value-bearing `BindingStateId` is minted at
runtime in EvaluateEpoch; state P4 pulls `BindingStateSchema` interning (NOT the prefix
DAG/BindingEdge) forward — or adopt D1 option (ii). See also M-medium D1/P4/P5 (single
schema-interner owner).

### H5 · CONFIRMED · P1.6/P5 · missed-deletion — deleting the declared-key renderer reds two live .contract goldens with no scheduled re-bless

P1.6 deletes the `-contract-out` declared-key renderer (correctly re-anchored to
DataFlow/Format.cpp:1745-1798 — see H8), but `key_tc_witness.contract.opt.golden` (real, not
symlink) and `key_multi_adorn_witness.contract.opt.golden` pin its `declared-key … inferred=…`
lines. Deleting the renderer reds both in all 4 modes; §1.3 never schedules the re-bless.
**Fix (folded):** at the P1 tail, re-bless both to drop the declared-key line(s) (the SIP
`inferred=` half is unreconstructable once @key is inert); re-add a `declared=`-only line at
P5 and re-bless again. State both blesses explicitly.

### H6 · CONFIRMED · P5/Parser · ordering-hazard — the order-significant model is unreachable until the parser flip lands in a phase

The order-FREE parser dup rejects fire at PARSE (Parser.cpp:974-995 same-decl + 1477-1488
cross-redecl, both sort), upstream of the freeze-time `InternDeclaredPaths` the doc says
"replaces" them. So `@key(A,B) @key(B,A)` never reaches P5; exit gate (2) is unexercisable.
**EMPIRICALLY CONFIRMED this session:** the `order_converge` probe rejects at parse with
"Duplicate instance key … same column set" (ir-desired-states §7B). **Fix (folded):** move
the order-free→order-significant flip of BOTH parser sites into Phase-0 item 4 (an explicit
prerequisite of P5), re-bless/retire `reject_key_double_1`'s order-free claim, record the
RP-10 set-of-sets semantic reversal.

### H7 · CONFIRMED · P6.6 · exit-gate-weak — retirement neither cascade-retracts interior facts nor survives its own guard

`RetireUnreachableSCCs(drained)` does `assert scc.facts ⊆ drained` where `drained` is ROUTED
removals — but `RouteResults` only routes facts on a request-edge destination; an interior
helper (q in the `{p,q}` co-recursive cycle, never bound by a `#query`) is never routed → not
in `drained` → the assert ABORTS on exactly the exit-gate scenario. Separately,
`FreeBindingStateStorage` only frees storage — it does not decrement per-fact support, so
`CurrentCommittedOutputs()` still reports the cycle Present and Publish emits no removal → the
published surface is not driven empty. **Fix (folded):** retiring an unreachable SCC must
cascade-retract every `FactDerivation` sourced from its states (dropping per-fact support so
membership dies and Publish emits removals) BEFORE freeing storage; re-type the guard to
compare against the retracted-fact set, not routed removals.

---

## MEDIUM

- **M1 · CONFIRMED · P2 · soundness-gap.** `query.PublicRowContracts()` can't retire the
  friend leak without a WIDER leak: `RowContractMap`/`RowContract`/`QueryViewImpl` are
  private lib types (lib/DataFlow/RowContract.h, no public header); returning them on the
  public `Query` API forces the private headers public, and `BuildRelationSchema` still pokes
  `view.impl`. **Fix (folded):** keep the friend-class access (contained to lib/Regional,
  which already includes the private lib/DataFlow/Query.h) OR have DataFlow return an opaque
  pre-materialized `{decl, visible_fields, member_key}` vector by value; drop the "retires the
  friend leak" claim (the leak is contained, not removable).
- **M2 · CONFIRMED · P2 · exit-gate-weak.** The "~24 goldens re-derive with no --bless" gate
  double-counts: P1 deletes the demand twins (8 real goldens) and leaves
  key_tc_witness.region.* DANGLING symlinks → MANDATORY re-bless. Honest surviving set = 12
  real (join_1/merge_2/tc_nonlinear_diff ×4) + key_tc_witness's 4 re-blessed. **Fix (folded):**
  restate the count.
- **M3 · CONFIRMED · P3 · exit-gate-weak.** P3 `EvaluateEpoch`'s `topo_order(rules_of(region))`
  is undefined for recursive relations, but the post-P1 baseline includes them (tc). Seed
  Phase-3 is "over full materialization" — implying the induction fixpoint backend is
  RETAINED. **Fix (folded):** state P3 EvaluateEpoch layers request/activation TRACKING over
  the retained full-materialization backend (does not replace codegen); gate the abstract
  evaluator to the acyclic/keyed slice; add a recursive-baseline probe.
- **M4 · CONFIRMED · P6.1/D2 · anchor-wrong.** `MessageSeamsOf` can't be built from
  `ForEachInsertToSelectSeam` as sketched: both seam endpoints carry the SAME message decl
  (Differential.cpp:18-41), so `RelationOf(pub_decl)`/`RelationOf(recv_decl)` degenerate to a
  self-edge on the message (not a relation). Building p→(m)→q requires walking the m-insert
  back to its producing relation and the m-select forward to its consuming relation.
  `RelationOf`/`MessageSeamsOf` don't exist yet. **Fix (folded):** define `MessageSeamsOf` to
  resolve each endpoint to its producing/consuming RELATION, or run the SCC over the DataFlow
  VIEW graph and project Stratify's condensation (the safer alternative). NDEBUG builds
  otherwise risk mis-partitioning message-mediated recursion (the DEBUG belt catches it only
  in debug).
- **M5 · PLAUSIBLE · D1/P4/P5 · soundness-gap.** No single declared owner for
  `BindingStateSchema` interning across P3/P4/P5; if P4's D1 pull-forward mints its terminal
  schema id by a construction not backed by P5's `schema_table`, the ids diverge and P5's
  convergence gate fails. **Fix (folded):** name ONE owner keyed on `(region, sorted
  field-set)`, available from P3's empty-schema site; P4 and P5 both call it.
- **M6 · PLAUSIBLE · P6.5 · ordering-hazard.** (A)-once-then-(B)-to-fixpoint with no outer
  joint loop can settle on a fixed point differing from the seed's single "repeat to a JOINT
  least fixpoint." Same root as B5. **Fix (folded with B5):** outer repeat, or a directed
  witness proving (A)-once-then-(B) == joint for all delta shapes.
- **M7 · PLAUSIBLE · P5 · exit-gate-weak (F8).** No mechanism separates declared-prefix
  (compile) from visited (runtime) schemas — both intern into one order-free schema table
  with no origin flag — so the "non-prefix subsets absent" gate can false-fail when a
  legitimate join order visits {A,C} at runtime. **Fix (folded):** assert over
  `MaterializePrefixChain`'s declared output BEFORE any EvaluateEpoch, or tag schemas with a
  declared-vs-visited origin.
- **M8 · REFUTED-BY-VERIFICATION · D4/P4 · anchor-wrong (the refuter erred).** The
  cross-phase refuter claimed Program.h:1096-1140 is `ProgramGroupUpdateRegionImpl` and the
  reuse anchor is wrong. VERIFIED FALSE: the public handle `class ProgramTableScanRegion` IS
  at Program.h:1097 (fwd-decl :1096, closes :1140); `ProgramGroupUpdateRegion` is at :808;
  `ProgramTableScanRegionImpl` is at lib/ControlFlow/Program.h:1601. My §0.1/P4 anchor
  (Program.h:1096-1140 for the public handle) is CORRECT. Recorded as a certification of the
  anchor + a caution that even a high-effort refuter mis-read a class range — always verify.
  (Add the Impl anchor :1601 to the doc for completeness.)

---

## LOW

- **L1 · CONFIRMED · P1.6.** `NumForcingsOfName` (Planning.cpp:142) — folded into B1.
- **L2 · CONFIRMED · P1.6 · anchor-wrong.** The declared-key renderer block is Format.cpp:
  **1745-1798** and is `-contract-out` (not `.df`); deleting only 1747-1765 orphans 1766-1798
  (dangling `decl`/`dset`/`*inferred`/unmatched braces). **Fix (folded):** delete the whole
  1745-1798 block, relabel it `-contract-out declared-key render`. (This mislabel is what hid
  the H5 contract-golden fallout.)
- **L3 · CONFIRMED · P1.6 · anchor-wrong.** `QueryView::GuardAnnotationIndex()` DEFINITION is
  at Query.cpp:353 (dereferences the deleted field), not the cited :298-316 (which are the
  Demand.cpp Query:: accessors, already covered by the P1.1 TU deletion). **Fix (folded):** add
  Query.cpp:353 to the P1.6 deletion list; the surviving live reader is `IsCutSuccessorDR`
  (see L4).
- **L4 · CONFIRMED · P1.3/P1.4 · missed-deletion.** `IsCutSuccessorDR` (Rel.cpp:1571-1576)
  references BOTH deleted symbols: `context.demand_instance_enabled &&
  succ.GuardAnnotationIndex() != kNoGuardAnnotation`. "Reduces to flag-off branch" cannot
  compile once the field + accessor are deleted. **Fix (folded):** give it an explicit P1
  hunk — drop the whole conjunct so it returns only `succ.CanReceiveDeletions() ||
  IsAggregate() || IsKVIndex()`.
- **L5 · PLAUSIBLE · P3 · soundness-gap.** `RouteResults` routes every canonical fact in a
  binding state to every RequestEdge on that state regardless of the requested relation; in
  the single-empty-state P3 world a `#query` on p would also receive edge/q facts. **Fix
  (folded):** filter `RouteResults` by the request edge's requested relation (from
  CallSiteId/owner), or state consumers filter downstream.
- **L6 · CONFIRMED · P6.3 · soundness-gap.** `CommonPreservedPrefix`/`route.dest_prefix` are
  ordering constructs over order-FREE inputs (RuleRoutingProjection is a SET of pairs;
  promotion yields order-free classes) — no order source, so the "one frontier" structural
  assert can false-fire. **Fix (folded):** define the ordering source (the declared @key path
  order on the destination relation); make the structural check advisory (fall back to Joint)
  until the ordering is well-founded.

---

## Certifications (diffs / mechanisms that HELD under attack)

1. **D4 Option-1 / P4 mechanism** — `ProgramTableScanRegion` reuse with `index=nullopt`
   genuinely emits the honest full-scan + key-equality filter: EmitScan (Database.cpp:3374-
   3418) with `maybe_index` absent falls to the full-scan arm (:3393-3396) then emits the key
   filter `r.<field> == input_vars[k]` (:3403-3418) with `assert(indexed_cols.size()==
   input_vars.size())`; dispatch is the existing `IsTableScan` arm (:1942→EmitScan). No new
   dispatch arm needed.
2. **Codegen is label-blind** — `grep -c -i lowering Database.cpp = 0` re-verified; EmitScan
   selects scan-vs-index from `region.Index()`/`InputVariables()`, never a Rel `Lowering`
   label. D4's "the DR-tail belt cannot enforce label==emission" frame is correct.
3. **A ProgramTableScanRegion at a keyed-read root needs no ProgramProcedure** — its ctor
   (Program.h:1605) needs only a parent REGION (Join.cpp:254-256 constructs it under an
   arbitrary parent).
4. **The landed OVERDELETE→REDERIVE→INSERT machinery exists** at the cited anchors:
   Stratum.cpp:1799-1841 (round-shell pairing), :660-691 (`EmitRederive` = a `C_r` CHECKMEMBER
   counter read, not a search), Table.h:20-32 (split C_nr/C_r). (The B4/B5 findings are about
   how to RE-KEY it per-fact, not about its existence.)
5. **F13 render is byte-identical** — `declared_key_positions[i] = (i < visible_fields.size()
   && visible_fields[i] in member_key)` reproduces the Planning.cpp:589-609 loop's
   selected-index set in the same order (incl. the arity break and the unit-relation `()` case).
6. **F12-literal** — `out.census` is a function of `query`, never RegionTemplate (this
   independence is real; H2 is about it being TOO independent to catch a hollow R, not about a
   hidden R-dependency).
7. **F8 fold is sound** — {A,B} IS a declared prefix of @key(A,B,C) (present, not absent); the
   true non-prefix subsets are {A,C},{B},{C}.
8. **F21 render target correct** — DataFlow/Format.cpp:1745-1798 IS the `-contract-out`
   declared-key renderer (the §0.1 anchor is right; L2 is only that P1.6's delete-RANGE and
   `.df` LABEL were wrong).
9. **reject_key_double_1 exact-dup reject survives** the order-free→order-significant flip —
   `InternDeclaredPaths` rejects `canon in seen` with `canon = tuple(p)` (no sort), so an
   exact-tuple repeat (`@key(A) @key(A)`) still rejects; only the order-permuted-dup guarantee
   is (intentionally) removed.
10. **D2** — reserving `recursive_components` empty at P2 breaks no consumer (grep: zero
    existing readers); P6.1 sole populator is viable.
11. **MultiViewStrata absent** — the s12 P2 hunk calling it would not compile; the correction
    holds.
12. **ForEachInsertToSelectSeam spans message seams** (Differential.cpp:18-41) — the F7 seam
    input exists (M4 is only that its endpoints need resolving to relations).
13. **P1.4 keep-enum (F4/F15)** — `kSectionWalk` is load-bearing for join pivots
    (Rel.cpp:2432-2433, Ctx::kFixpoint); retiring the value would break ordinary joins.
14. **§1.2 census vacuity** — request-ports genuinely degenerates to 0==0 (both operands are
    `DeriveRegionalCensus(query)`); the pivot to the .rel census multiset as the real P1 belt
    is correct.
15. **P1.2 three IsDemandMessage callers** correctly enumerated (Database.cpp:1522/:3692,
    Planning.cpp:164).
16. **§1.6 Build.cpp:2677** — must be deleted outright (iterates the deleted
    recognized_subgraphs), not stubbed no-op — correct.
17. **F18 Fix(a)** — with no intra-state activation edges, RouteResults still reaches
    multi-hop facts for an acyclic bound-query program (the rule DAG order within the shared
    state carries derivation order). Holds in the single-state slice.
18. **Caller-qualified retract (R5 retained)** holds under P3 and D1's re-seed — RemoveRequestEdge
    erases only the retiring edge's RoutedResults; a 2nd requester adds a distinct edge + its
    own results with zero new FactDerivations.
19. **P4 respects "no second fact owner"** — facts live solely in RegionalFactRelation; binding
    states own frontiers/derivation ids, not fact copies.
20. **The 24-golden tip count** (6 cases × 4 modes) is accurate (the s12 "180+" was wrong).

---

## Net verdict

Direction sound; every survivor is an addressable amendment. The five blocking findings and
six high findings are folded into `keyed-rewrite-reconstruction-diffs.md` §5. No finding
invalidates the four-authority target, the phase sequence, or the retained invariants. The
strongest structural belts remain: the `.rel` census multiset (anti-silent-pass for P1) and
the reject→compile transitions of the three no-baseline carriers (ir-desired-states §6/§7).
