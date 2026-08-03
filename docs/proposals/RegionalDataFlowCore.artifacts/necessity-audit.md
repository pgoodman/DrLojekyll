# Necessity / complexity audit — RegionalDataFlowCore staged cutover

Synthesis pass, 2026-08-02. Every mechanism the necessity lens attacked, with a
verdict, then the ranked simplification candidates. Deletion-hunk / simplification
dispositions are ranked here and folded as amendments into
`phase3-critique-report.md`.

Verdict vocabulary:
- **inherent** — the mechanism earns its complexity in the stage it lands (or the
  earliest stage where it can be pinned); the attack's disposition is refuted.
- **legacy-two-authority** — the mechanism duplicates or re-encodes an authority the
  cutover elsewhere deletes; a residue of the thing being replaced.
- **bad-factoring** — real complexity pulled into the wrong stage / bundled with an
  unrelated deliverable; separable at no correctness cost.
- **unnecessary** — pure speculative generality with no consumer anywhere in the plan.

---

## Mechanism verdicts

### Stage A

| mechanism | finding | verdict | rationale |
|---|---|---|---|
| Support-domain **types** (`DeltaSign`, `DerivationSupportCount`, `DemandSupportCount`, `SupportAlgebra`) | A-nec-1 | **inherent** | They have a Stage-A consumer: the H-A1 static_assert battery proving cross-domain operator disjointness (`!requires { DemandSupportCount{} + DerivationSupportCount{}; }`). That negative-space typing *is* the F4 deliverable (§1.4). "Producer with no consumer" misreads a compile-time guard as needing a runtime reader. |
| Support-domain **populated field** `RowContract.derivation_support` + merge `DeltaSign` fold + join support product | A-nec-1 | **bad-factoring** | No Stage-A validator/analysis reads the *value*; the only consumer is the H-A8 dump that exists to pin it (a closed loop). First real consumer is Stage C's `RequestEdgeRelation`. E-A3 already recommends the more-conservative "transfer the shape, not the counts" over the finding's delete. |
| `candidate_member_keys` (Antichain) + JOIN-arm `Minimize` + FieldExpression-as-equivalence-class | A-nec-2 | **bad-factoring** | No Stage-A consumer (the three H-A7 validators + H-A5 read only the flat `member_key`); Minimize only *shrinks* keys — an optimization (§8.2 open-port minimization) in an explicitly conservative stage where the full pivot-key union is always sound. Caveat: dropping `candidate_member_keys` reopens §4.2's canonical-RowContract data-model choice (identity *as* the antichain) — an owner call. |
| Stage-A as a standalone landing (H-A1/H-A3/H-A4 infra) | A-nec-3 | **bad-factoring (sequencing)** | The types+inference+accessor are non-behavioral infra bundled into a stage whose only shipped delta is `agg_distinct_1`'s −5 advisory warnings; "should these fold into Stage B against a structural consumer?" is a fair sequencing escalation. But the F2 correctness-model fix (typed impossibility of member→distinct collapse) is real value the byte-identical gate *evidences*, not disproves. |

### Stage B

| mechanism | finding | verdict | rationale |
|---|---|---|---|
| Planning tier: `PlanningRegionalProgram` / `Open*Template` family + `FreezeAndValidate` freeze-relabel | A-nec-1 (B) | **inherent** | Degenerate at Stage B (build-then-freeze, conceded "DEGENERATE FORM" / ESC-2), but normatively specified (pseudocode §6), and debuting the inert type wall under the *byte-identity* gate is strictly lower-risk than introducing it in Stage C alongside port negotiation under the weaker reference-interpreter oracle. Disposition (defer to Stage C) is inverted composability. §3's split is justified by port negotiation broadly, not `OptimizeLocalGraphToFixpoint` alone. |
| `V-FROZEN-NO-OPEN-PORT`, `V-REGION-CENSUS-IDENTITY` | A-nec-3 (B) | **inherent** | fprintf+abort internal-invariant belts guard *implementation* bugs, not corpus variety. `V-FROZEN-NO-OPEN-PORT` belts the new `FreezeAndValidate` transition; `V-REGION-CENSUS-IDENTITY` is the Stage-B instance of the model↔emission census family (`V-PRED-XCHECK`/`V-JOIN-EMIT-XCHECK`) — a bug in the wrap/seam op-attribution trips it. |
| `V-OWNERSHIP-ACYCLIC` | A-nec-3 (B) | **bad-factoring** | Genuinely unfireable at Stage B (one node, zero edges); born with its failure mode at Stage C's first `FrozenChildCall`. The one validator worth deferring. |
| `OpenRegionTemplate` fields / `logical_origins` / `LocalNodeId`↔`LogicalNodeId` split / `PortId`/`EdgeId` newtypes | A-nec-4 (B) | **inherent** | `logical_origins` + the Local/Logical split *are* identity at Stage B (conceded in-diff), but the typed-id prophylaxis (§4.1: ids must not be aliases permitting free cross-domain arithmetic) is a MEMORY-blessed methodology; stripping the newtype wall re-couples the id-type-shape change into Stage C's weaker gate. `row_contracts` are NOT identity under the canonical A→B order. |

### Stage C

| mechanism | finding | verdict | rationale |
|---|---|---|---|
| `ExtractionPolicy` cost seam (provisional `return true`) | N-nec-2 | **inherent (seam)** | §8 is legality-driven by design; profitability is not a semantic input; cost is deferred to this named seam + Stage D. The seam itself is right. |
| `ExtractionPolicy=always-true` shipped as the *default* + `-demand`/`-demand-instance`/`-demand-retract` flag deletion | N-nec-2 | **bad-factoring (default)** | Deletes the opt-in *answer* (the gate) along with the cost complexity, mandating extraction for the ~38 flag-off cases the gate existed to exclude — the feature pessimizing its own opt-out cases with no recourse until Stage D. Not a two-authority legacy. Disposition "defer all extraction to Stage D" refuted: `ChildResult`/`RoutedResult` are defined over `ChildInstanceId`, so zero extraction leaves the lifecycle inoperative and Stage C no longer replaces forcing. Escalate: reclassify E3 as accepted-regression-with-list, or gate `ExtractionPolicy` demand-rooted. |

### Stage D

| mechanism | finding | verdict | rationale |
|---|---|---|---|
| The "ADDITIONAL" region-ownership-postorder constraint on `DeriveDRStrata` + `V-REGION-STRATUM-ORDER` | N-nec-3 | **inherent (framing at-risk)** | Recast as the instance-qualified region-boundary sibling of the carried-forward Kahn belts (`V-LINEAR`/`V-LOOP`/`V-READY`/`V-BAND-HAZARD`), not a restatement of the deleted unstratified `flow.instance_stratum`. Region calls are port-mediated *bidirectional* couplings, so the ordering does not fall out of def/use for free unless `DeriveDRStrata` treats cross-region port crossings as dependence edges — which Hunk 3.3 does. Deletion (the finding's disposition) refuted; a clarity ask survives (derived-belt vs independent-tie-break). |

**No `legacy-two-authority` mechanism survived.** The one candidate (N-nec-3, framed as
"legacy of the just-deleted epoch-tail asymmetry") is refuted — the validator is a new
cross-region-port belt, not a re-encoding of the deleted `flow.instance_stratum[sid]`.
**No `unnecessary` (zero-consumer-anywhere) mechanism survived** either — the closest,
A-nec-1's support types, have the F4 static_assert consumer at Stage A.

---

## Ranked simplification candidates

Ranked by **(composability win × confidence) / effort**. Effort: S ≤ ~½ day of diff
edit; M = a hunk rewrite touching multiple sites; L = a data-model change.

### 1. Strip the *populated* support field + support-transfer arms (keep the types) — A-nec-1 (A)
- **Deletes/simplifies:** `RowContract.derivation_support` (the populated field), the merge-arm `DeltaSign` add/remove fold, the join-arm "product of contributor supports". Keeps `DeltaSign`/`DerivationSupportCount`/`DemandSupportCount`/`SupportAlgebra` (F4 static_assert guard).
- **Composability win:** RowContract becomes a pure member-identity object with a single consumer story — Stage B can freeze it without an inert Rel-owned field riding along; the real support algebra reappears in Stage C where `RequestEdgeRelation` is its first genuine consumer. **HIGH.**
- **Confidence:** **HIGH** — E-A3 already recommends exactly this "shape-only, transfer not the counts" variant; the finding's own refuter endorses it over the harder delete.
- **Land where:** folded into the **Stage A** diff (E-A3's recommendation, made concrete). Not a standalone pre-Stage-A cleanup — the field does not exist in landed code yet.
- **Effort:** **S.**
- **Pinned by:** Stage-A 180+20 byte-identity gate (field drops out of the H-A8 `.contract` dump on re-render) + the H-A1 IdentityTypes static_assert unit (types retained).

### 2. Defer `V-OWNERSHIP-ACYCLIC` from Stage B to Stage C — A-nec-3 (B)
- **Deletes/simplifies:** removes one of the three Stage-B freeze validators (the one that is provably unfireable over a one-node, zero-edge ownership forest). Keeps `V-FROZEN-NO-OPEN-PORT` and `V-REGION-CENSUS-IDENTITY` (they belt Stage-B-new code).
- **Composability win:** the validator is born with its failure mode — it lands at Stage C's first `FrozenChildCall` that could form a cycle, testable at birth with a directed negative witness, instead of shipping as a no-op pinning a green nothing. **LOW–MEDIUM** (one validator).
- **Confidence:** **HIGH** — refuter concedes this is the single genuinely-unfireable belt of the three.
- **Land where:** **Stage B** diff (remove the scaffold); its directed negative witness lands with the Stage C ownership machinery.
- **Effort:** **S.**
- **Pinned by:** the Stage-C directed reject witness (a self/cyclic-ownership region call) — the deferral is safe because that witness is the first thing that could trip it.

### 3. Reduce Stage-A RowContract to `{visible_fields, member_key}`; defer antichain + Minimize + FieldExpression classes to Stage B — A-nec-2 (A)
- **Deletes/simplifies:** `candidate_member_keys` (Antichain), the JOIN-arm `Minimize(candidate, eqsets)`, and the FieldExpression-as-equivalence-class machinery. `member_key` (the flat proven key; join = full unminimized pivot-key union) already exists in the diff and stays.
- **Composability win:** Stage-A `member_key` becomes a flat set both the freeze step and the aggregate wiring consume without an antichain lattice; key *minimization* lands once, next to the §8.2 port solver that is its only client. **MEDIUM–HIGH.**
- **Confidence:** **MEDIUM** — the surplus is real and no Stage-A consumer reads the antichain, but dropping `candidate_member_keys` reopens §4.2's canonical-RowContract data-model choice (identity modelled *as* the antichain with no single `member_key`). That is an **owner call**, not a clear defect — escalate before landing.
- **Land where:** folded into the **Stage A** diff, contingent on the owner ratifying the flat-key data model over §4.2's antichain.
- **Effort:** **M** (touches the RowContract type, the JOIN transfer arm, and the §8.2 forward-reference).
- **Pinned by:** the three H-A7 validators (read only `member_key`) + H-A5 aggregate wiring still passing; a §8.2 port-minimizer unit test lands `Minimize` at Stage B. Note the join `member_key` still needs the existing `input_columns`/`columns` contributor→output mapping — do not drop that with the antichain.

### 4. Defer `logical_origins` + the `LocalNodeId`↔`LogicalNodeId` split to Stage C — A-nec-4 (B)
- **Deletes/simplifies:** the (RegionId, LocalNodeId)→LogicalNodeId identity indirection and the two-newtype distinction at Stage B, where they are provably identity over a single region.
- **Composability win:** the id types arrive when free cross-domain arithmetic is actually possible (clones/extraction make them non-identity), guarding a real confusion instead of an impossible one. **LOW.**
- **Confidence:** **LOW** — refuter argues *against* landing: the typed-id prophylaxis (§4.1, MEMORY-blessed) wants the newtype wall to debut under the byte-identity gate, and deferral re-couples the id-type-shape change into Stage C's weaker oracle. Already-conceded-inert, but keeping it is the defensible call.
- **Land where:** **do not land** unless the owner explicitly de-prioritizes the id prophylaxis. Listed for completeness.
- **Effort:** **S.**
- **Pinned by:** a typed-id newtype unit (free-arithmetic-across-domains rejected) — which is the reason to keep, not defer.

### 5. Defer the whole Planning tier / Open* types to Stage C — A-nec-1 (B)
- **Deletes/simplifies:** at Stage B build a `FrozenRegionalProgram` directly (no `PlanningRegionalProgram`, no `Open*` templates, no freeze relabel); introduce them in Stage C with the first real pre-freeze mutator.
- **Composability win:** **NEGATIVE** — refuted. Moves the inert type-wall debut off the byte-identity gate (the strongest referee in the pipeline) onto Stage C's reference-interpreter oracle over a rewritten corpus, and bundles `Open*`/`Frozen*` + port negotiation together under the weaker gate.
- **Confidence:** **LOW** (disposition refuted).
- **Land where:** **reject.** Listed only to record that the attack was considered and the scaffold is kept deliberately.
- **Effort:** M. **Pinned by:** the Stage-B byte-identity gate — which is the argument for *keeping* it.

---

## Pre-Stage-A standalone cleanups

**None.** Every candidate above simplifies a *proposed* stage diff, not landed code — the
support field, antichain, `logical_origins`, and Planning tier do not exist in the tree
yet. There is no pre-existing two-authority artifact to excise ahead of Stage A; all
simplifications fold into their stage diffs (candidates 1–3 worth landing, 4–5 recorded-
and-kept). The only *deletion* the plan already carries — `LintAggregateProjection`
(H-A6) — is a Stage-A change with a same-stage replacement (`V-NO-COLLAPSE`), not a
pre-stage cleanup.

## Folding into the per-stage amendments (a)

- **Candidate 1** (A-nec-1 A, deletion-hunk) → Stage A amendments: strip the populated
  `derivation_support` field + support-transfer arms per E-A3; keep the domain types.
- **Candidate 2** (A-nec-3 B, deletion-hunk) → Stage B amendments: remove
  `V-OWNERSHIP-ACYCLIC` from the Stage-B scaffold; it lands with its Stage-C witness.
- **Candidate 3** (A-nec-2 A, simplification-refactor) → Stage A amendments, contingent on
  the §4.2 data-model owner call.
