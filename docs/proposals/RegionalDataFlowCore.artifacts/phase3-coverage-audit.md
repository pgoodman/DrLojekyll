# Phase-3 cross-stage coverage audit

Auditor pass, 2026-08-02, tip f0c913e0 (branch keyed-instances). Sources:
`RegionalDataFlowCore.md` (normative §11/§12.3/§14/§15) + the five stage
artifacts (`stage-a-diff.md`, `stage-i0-interpreter.md`, `stage-b-diff.md`,
`stage-c-diff.md`, `stage-d-diff.md`) + `fable-review-2026-08-01.md`.

Legend: a **row is clean** if some stage/hunk lands it; **ORPHAN / GAP /
UNHOMED** = no stage lands it; **soft** = landed only implicitly (carried-forward
mechanism or admissibility gate, no dedicated named artifact).

---

## Table 1 — §11 required validators (17) → landing stage + hunk, or ORPHAN

| # | §11 validator | Lands | Notes |
|---|---|---|---|
| 1 | unrealized semantic member identity | **A / H-A7** `V-MEMBERKEY-REALIZED` | clean |
| 2 | accidental collapse outside DistinctProjection | **A / H-A7** `V-NO-COLLAPSE` | clean; user-facing surface at-risk (E-A2 constructibility) |
| 3 | symbolic parameter escape | **ORPHAN** | only a clause of the Stage-C/D §8.1 admissibility gate ("no parameter escape"); no named validator, and admissibility *declines extraction* rather than *failing compilation*, so the §11 "compilation fails" semantics is unhomed |
| 4 | open port reaching FrozenRegionalProgram | **B / H9** `V-FROZEN-NO-OPEN-PORT` (asserts at **C / H-I** `V-PORT-CLOSED`; deepened **D / Part 5** `V-PORT-POSTORDER-CLOSED`) | clean |
| 5 | parent/child frozen-port disagreement | **ORPHAN** | Stage B H9 explicitly defers it to "ALL STAGE C"; Stage C H-I validator list omits it; Stage D `V-PORT-POSTORDER-CLOSED` checks port *openness/order*, not schema *disagreement* between a `FrozenChildCall` mapping and the child's frozen ports. **Inconsistency (see X1).** |
| 6 | mutation of a sealed external ABI | **ORPHAN** | admissibility-gate clause only ("sealed ABIs untouched", Stage C H-E); no named validator; a would-be mutation is declined into full materialization, never a compile failure |
| 7 | region call not targeting a direct child | **B / H9** `V-OWNERSHIP-ACYCLIC` (scaffold) → **C / H-I** `V-REGION-ACYCLIC` → **D / Part 5** `V-NEST-DEPTH-FINITE` | clean |
| 8 | cycle in the region ownership/call graph | **B / H9** `V-OWNERSHIP-ACYCLIC` → **C / H-I** `V-REGION-ACYCLIC` → **D** `V-NEST-DEPTH-FINITE` | clean |
| 9 | effectful operator inside a region | **ORPHAN** | Stage B H9 defers `V-PURE-REGION` to "STAGE C"; Stage C H-I added-validator list does **not** contain `V-PURE-REGION`. Purity *is* checked inside Stage C H-E admissibility ("all operators pure") for extracted children, but the named §11 validator never lands. **Inconsistency (see X2); undermines §15.13.** |
| 10 | sequestered key absent from InstancePath qualification | **ORPHAN** | Stage B H9 defers "sequestered-key-in-InstancePath" to Stage C; Stage C H-I omits it; Stage D adds `V-INSTANCE-KEY-ALIAS` / `V-INSTANCEKEY-NOT-DERIVED` (alias-distinctness, not-derived) but neither checks "sequestered key present in InstancePath until proven removable". **Undermines §15.12.** |
| 11 | ambiguous RequestOwnerId or RequestEdgeId | **C / H-I** `V-OWNER-EXACT` | clean |
| 12 | request edge without caller-qualified result maintenance | **C / H-I** `V-EDGE-BALANCE` + `V-ROUTED-FANOUT` | clean |
| 13 | result removal without exact routed-result identity | **C / H-I** `V-ROUTE-EXACT` | clean |
| 14 | inactive state cleared before routed removals | **C / H-I** `V-INACTIVE-AFTER` | clean |
| 15 | non-converged local relational fixpoint | **soft — carried-forward** rel-arch `V-LOOP`/`V-READY` (Stage B carry; Stage D Part 3-4 termination) | no *dedicated new* validator is named for this §11 line; convergence rests on the carried-forward Rel fixpoint machinery |
| 16 | FrozenRegionalProgram/Rel operation census mismatch | **B / H9** `V-REGION-CENSUS-IDENTITY` (degenerate) → **C / H-I** `V-LIFECYCLE-CENSUS` | clean |
| 17 | Rel/ControlFlow lifecycle census mismatch | **C / H-I** `V-LIFECYCLE-CENSUS` | clean |

**§11 tally:** 11 clean, 1 soft (15), **5 ORPHAN (3, 5, 6, 9, 10).**

---

## Table 2 — §15 acceptance invariants (16) → establishing stage/hunk, or GAP

| # | §15 invariant | Established | Notes |
|---|---|---|---|
| 1 | one `FrozenRegionalProgram` per compilation | **B / H1+H3** | clean |
| 2 | Rel consumes only frozen contracts, never discovers regions | **B / H4** (consumes frozen) + **C / H-D** (deletes `BuildSubgraphInstanceOps`/discovery fork) | clean |
| 3 | ownership + calls are one acyclic forest | **C / H-E** + `V-REGION-ACYCLIC`; deepened **D / Part 1** | clean |
| 4 | recursive relation eval stays local to one region instance | **D / Part 3** (instance-qualified fixpoint) | clean |
| 5 | `InstancePath` follows lexical ownership, not call stack | **C** `ChildInstanceId` (one level) → **D** depth-2 `InstancePath` | clean |
| 6 | `RequestEdgeRelation` is the sole demand authority | **C / H-F** | clean |
| 7 | every requester member + root lease has exact `RequestOwnerId` | **C / H-F** + `V-OWNER-EXACT` | clean |
| 8 | every request edge participates in caller-qualified routed results | **C / H-G** + `V-ROUTED-FANOUT` | clean |
| 9 | second owner receives existing results; one removal isolates | **C / H-G** (`RoutedResultRelation`) | clean; concrete permutation witnesses only in **D / Part 6** |
| 10 | child state cleared only after last edge dies + removals visible | **C / H-G** `kRetireInactive` + `V-INACTIVE-AFTER` | clean |
| 11 | member vs distinct projection are different operators | **A / H-A2** | clean |
| 12 | sequestered key stays in `InstancePath` until proof removes all needs | **D / Part 3+5** (InstancePath qualification) | **caveat:** the guarding §11 validator (line 10) is ORPHAN, so the invariant's *enforcement* is unhomed |
| 13 | pure regions contain no observable effect | **C / H-E** (admissibility purity check); ESC-1 makes it a Stage-C-onward invariant | **caveat:** enforcing validator `V-PURE-REGION` (§11 line 9) is ORPHAN |
| 14 | same-epoch request/data order can't change membership/publication | **C / H-G** (epoch netting) + I0/`permcheck`; witnessed **C/D Part 6** | clean; closes the I0 `negation_flap` order-dependence carve-out (H9) |
| 15 | no demand mode, alternate lowering, recognizer, or fallback | **C / H-A** + H-B/H-C/H-D + grep gates EG.6 | clean |
| 16 | every artifact deterministic and validator-complete | **B** (deterministic dumps) + **C** (lifecycle census) + **D** (matrix) | **caveat:** "validator-complete" is undermined by the 5 §11 ORPHANs (esp. 5, 9, 10) |

**§15 tally:** 0 hard GAP. **3 caveated (12, 13, 16)** — each rests on an
orphaned §11 validator.

---

## Table 3 — §12.3 directed witnesses (13) → stage where it FIRST passes, or UNHOMED

| # | Area | First passes | Notes |
|---|---|---|---|
| 1 | Identity (equal payload, distinct members) | **A / H-A9** (contract goldens; agg_distinct_1) | clean |
| 2 | Projection (hidden preserve vs distinct collapse) | **A / H-A2+H-A9** (agg_distinct_1 + `member_collapse_1`) | collapse-witness constructibility at-risk (E-A2) |
| 3 | Aggregate (equal contributions + exact retraction) | **A / H-A5** (existing agg corpus + `bin/Oracle`) | clean |
| 4 | Multiple owners (retract either first) | **C / H-G** capability | concrete permutation witness only enumerated in **D / Part 6**; I0 has **no pre-cutover referee** (E3) |
| 5 | Multiple requester members (one key, two members) | **C / H-F** | enumerated **D / Part 6** ("Multi-owner ATTACH") |
| 6 | Late subscriber (second edge attaches maintained results) | **C / H-G** | partly exercised by rewritten `demand_neighborhood_witness`; matrix **D / Part 6** |
| 7 | Detachment (one edge retracts only its results) | **C / H-G** + `V-ROUTE-EXACT` | matrix **D / Part 6** |
| 8 | Request/data order (flaps) | **C / H-G** epoch netting | I0 `negation_flap` carve-out (H9); matrix **D / Part 6** |
| 9 | Nested regions (two depths, lexical key aliases) | **D / Part 6** | clean |
| 10 | Local recursion (linear/nonlinear/mutual) | **D / Part 6** | referee = I0 only (E2, no legacy binary) |
| 11 | Permanent root (published obs. live without a cursor lease) | **UNHOMED** | capability exists (§6 `PermanentOutputRoot`, Stage C H-F/H-H introduces leases + permanent-root owners) but **no stage authors a directed permanent-root witness** — absent from Stage C EG and Stage D Part 6 matrices |
| 12 | Effects (effect stays in ProgramRoot; region consumes stored result) | **UNHOMED** | §7.2 placement rule; **no stage authors a directed effects witness** — absent from every stage's witness enumeration (tied to the orphaned `V-PURE-REGION`) |
| 13 | Rejection (key-changing recursive region request fails) | **C / H-J** (Variant B: `demand_cyclic_1` rewritten) | **OWNER-GATED** (E2): under Variant A this becomes a NEW witness owed; Stage D E3 confirms permanent exclusion |

**§12.3 tally:** 11 homed, **2 UNHOMED (11 Permanent root, 12 Effects).**

---

## Table 4 — §14 deletions (9) → deletion stage / replacement stage (ordering check)

All nine demand-layer deletions land at **Stage C**; every replacement lands at
Stage C or earlier. **No replacement lands later than its deletion.**

| # | §14 deleted | Del. stage | Replacement | Repl. stage | Order |
|---|---|---|---|---|---|
| 1 | `-demand` / `-demand-retract` / `-demand-instance` | C (H-A / D9) | unconditional regional path / planner | shell **B**, full **C** (H-E) | OK |
| 2 | parser-object fabrication by DataFlow opt | C (H-C / D2) | `RequestEdgeRelation` (compiler-owned edges) | **C** (H-F) | OK |
| 3 | generated demand-message entry suppression | C (H-C / D3) | absence — seed is a lease/edge | **C** | OK |
| 4 | query forcing + separate demand-retract procs | C (H-H / D6) | move-only `RootRequestLease` | **C** (H-H) | OK |
| 5 | demand guard annotations + shape recognizers | C (H-C / D4) | `RequestEdgeRelation` + `FrozenRegionTemplate` | records **B**, wiring **C** | OK |
| 6 | `RecognizedSubgraph` + `DRInstance` | C (D4/D5/D7) | `FrozenRegionTemplate` + `LowerFrozenRegion` | **B** + **C** (H-G) | OK |
| 7 | flat-vs-instance lowering selection | C (D5 fork) | single `LowerFrozenRegion` path | **C** (H-G) | OK |
| 8 | special `InstanceStore` semantics from global graph | C (D8) | `RequestEdgeRelation` + ordinary pub `DiffTable` | **C** (H-G) | OK — physical per-instance store is a Stage-**D** *addition* (E5), **not** a deferred replacement |
| 9 | tests/comments whose only subject is those paths | C (§12.4/EG.4/EG.6) | rewritten witnesses vs edges/leases (datasets kept) | **C** | OK |

**§14 tally:** 0 ordering violations. (Stage A also self-contained-deletes
`LintAggregateProjection` with same-stage replacement `V-NO-COLLAPSE`, H-A6/H-A7 —
not a §14 item.)

---

## Table 5 — Review Concerns 2 & 3

| Concern | Resolved by | Status |
|---|---|---|
| **Concern 2** — inadmissible-extraction semantics (bound query when extraction is inadmissible) | **Stage C / H-J** (Variant A uniform full-materialization vs Variant B hybrid retaining recursion feature-gap rejects) | **ESCALATED-TO-OWNER — blocking (Stage C ESC E2).** Owner decision owed; exit gate written to bind under either variant. Stage D **E3** re-confirms it load-bearing (permanent-exclusion classes become silent full-materialization). |
| **Concern 3** — "legality-driven" extraction is a cost policy | **Stage C / H-E** — `ExtractionPolicy(candidate)->bool`, a NAMED PROVISIONAL SEAM (Stage-C impl `return true`, the declared cost-model attach point) | Resolved-by-hunk + **ESC E3 (non-blocking).** Named failure mode: message-rooted trivial shapes extracted uselessly (answer-correct). |

(For context: Concern 1 → the **I0** stage; Concern 4 → Stage C **H-G** shared-pub
`RoutedResult` realization, ratified ESC E4.)

---

## Cross-stage consistency findings

- **X1 (inconsistency + ORPHAN).** Stage B H9 states parent/child frozen-port
  disagreement is "ALL STAGE C"; Stage C H-I never lands it. → §11 line 5 ORPHAN.
- **X2 (inconsistency + ORPHAN).** Stage B H9 states `V-PURE-REGION` ("effectful
  operator inside a region") "co-arrives with real extraction (Stage C)"; Stage C
  H-I's added-validator list omits it (only the H-E admissibility purity clause
  survives). → §11 line 9 ORPHAN; undermines §15.13.
- **X3 (naming inconsistency).** Stage C H-F cites `V-DEMAND-SUPPORT-DERIVED` as a
  Stage-A validator forbidding `DemandSupportCount`-as-owner; Stage A never defines
  that named validator — Stage A H-A1 reserves the domain via a `static_assert`
  battery (IdentityTypes unit), not a named `V-*`. Reconcile the name or add the
  validator.
- **X4 (soft numeric drift).** Bound-query corpus counts differ across docs: Stage
  C "~38 flag-off bound-#query cases"; Stage D "~53 bound-query cases (~127
  non-demand)"; CLAUDE.md "38/165 at the demand-seeds sweep." Roughly reconcilable
  (38 flag-off + ~15 demand-active ≈ 53 of 180) but should be stated once.
- **Consistent (verified, no finding):** InstanceStore semantic-delete (C/D8) vs
  physical-may-survive (D/E5); StateCellStore survives C untouched (agg/KV);
  `ExtractPrimaryProcedure` PRESERVED (B/H8) rides into C (E6)/D; planner shell
  (B) → extend (C/H-E) → deepen (D/Part 1); `local_fixpoint` unstratified
  epoch-tail at C (H-G.4) → stratified at D (Part 3.3), handoff explicit;
  `demand_diff_pub_witness` authored at I0 (H8/OG3) is the oracle Stage C EG.7/E1
  depends on.

---

## Compact summary

- **§11 validators (17):** 11 clean, 1 soft (15 non-converged fixpoint — carried
  `V-LOOP`/`V-READY`, no dedicated validator), **5 ORPHAN: 3 symbolic-parameter-
  escape, 5 parent/child frozen-port disagreement, 6 sealed-ABI mutation, 9
  effectful-operator (`V-PURE-REGION` missing), 10 sequestered-key-in-InstancePath.**
- **§15 invariants (16):** 0 hard GAP; **3 caveated (12, 13, 16)** — each depends on
  an orphaned §11 validator (10, 9, and 5/9/10 respectively).
- **§12.3 witnesses (13):** 11 homed (A: 1-3; C: 4-8,13; D: 9-10), **2 UNHOMED: 11
  Permanent root, 12 Effects** (no stage authors a directed witness).
- **§14 deletions (9):** all land Stage C; **0 ordering violations** (every
  replacement ≤ its deletion; InstanceStore physical store is a Stage-D addition,
  not a late replacement).
- **Concerns:** Concern 2 → Stage C H-J, **escalated-to-owner blocking (E2)**;
  Concern 3 → Stage C H-E `ExtractionPolicy` seam (E3 non-blocking).
- **Cross-stage findings:** X1 (frozen-port validator B→C promise unfulfilled),
  X2 (`V-PURE-REGION` B→C promise unfulfilled), X3 (`V-DEMAND-SUPPORT-DERIVED`
  named in C, undefined in A), X4 (soft bound-query count drift).

**Total blocking items for the owner:** the 5 §11 ORPHAN validators + 2 UNHOMED
§12.3 witnesses (Permanent root, Effects) + Concern-2 variant decision (E2). The
two UNHOMED witnesses and 3 of the ORPHANs (5, 9, 10) share one root cause:
Stage C's H-I validator list and Stage C/D witness matrices under-deliver the
purity / port-agreement / sequestered-key / permanent-root / effects surface that
Stage B explicitly deferred to Stage C.

---

## X4 resolution (brief Errata-2) — 2026-08-02

One authoritative count, measured at tip f0c913e0 over HEAD-tracked corpus
cases (`git show HEAD:` content, pattern = a `#query` declaration carrying a
`bound` parameter): **49 of 181 cases carry at least one bound query**
(27.1%). The drifted values are all stale or differently-scoped snapshots:
38/165 was the demand-seeds-epoch corpus, ~41/180 and ~53/~127 were
intermediate corpus sizes with differing inclusion rules. Method note: the
count grows with the D3.3 landing set (which adds bound-query cases by
design); re-measure with the same one-liner at any future reference point
rather than propagating this constant.
