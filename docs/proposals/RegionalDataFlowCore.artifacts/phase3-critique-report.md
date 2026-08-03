# Phase-3 critique report — RegionalDataFlowCore staged cutover

Synthesis pass, 2026-08-02, branch `keyed-instances`. Inputs: the 47 refute-verified
findings that survived (23 killed by refuters), the cross-stage coverage audit
(`phase3-coverage-audit.md`), and the five stage artifacts + `regional-arch-pseudocode.md`
+ `RegionalDataFlowCore.md`.

**Severity discipline.** Each finding's severity below is the *refuter-adjusted* severity,
not the finder's original. A refuter that WEAKENED a "blocking" finding to a doc-sharpening
is recorded as such; a CONFIRMED finding keeps its calibrated severity. A stage is
**BLOCKED-ON** only for findings that survive refutation *at blocking severity*.

Verdict legend per finding: **CONFIRMED** = refuter verified every load-bearing claim
against code/doc; **WEAKENED** = factual core survives but severity/disposition was trimmed.

---

## Stage A — identity typing + row contracts (`stage-a-diff.md`)

### Surviving findings

| id | lens | adj. severity | verdict | one-line |
|---|---|---|---|---|
| **T-conf-1** | termination | **blocking** | CONFIRMED | `InferConservativeRowContracts` is a single depth-ordered pass, but the contract-dependency graph of any recursive program is genuinely cyclic (recursion realized through kMember proxy TUPLEs, not a SELECT leaf), so a back-edge input's contract is read empty/default → member-key inference is a fixpoint modelled as one pass. |
| **T-conf-2** | termination | **blocking** | CONFIRMED | T-conf-1 propagates into the hard clean-diagnostic validators: `V-MEMBERKEY-REALIZED` fires on a recursive kMember TUPLE that inherited an empty back-edge key → a recursive corpus program (`demand_tc_witness`, `d5_recursive_negate`, `fixpoint_stress_1`, `reconverge_1`) hard-rejects, breaking the "zero of 180" exit gate; and no escalation covers cyclic-graph contract soundness. |
| A-corr-3 | correctness | major | CONFIRMED | `DeterminedBy(key, eqsets, consts)` and the JOIN-arm `Minimize(candidate, eqsets)` are wired to `impl->equivalence_sets`, which does not exist — equivalence data is per-view `view->equivalence_set` computing *shared-backing-storage* colocation, not column-value equalities. |
| T-oracle-1 | testability | major | CONFIRMED | The H-A2 exit gate ("zero dumps change") is observationally identical whether role-in-`Equals` refuses a real fold or is a total no-op; the discriminant's active branch is unexercised by all 180 cases — the gate certifies inertness, not correctness. |
| T-oracle-3 | testability | major | CONFIRMED | `member_collapse_1` cannot be an "all-4-modes-diagnostic": kMember facade TUPLEs are minted only inside `::Canonicalize`, skipped under `-disable-dataflow-opt`, so V-NO-COLLAPSE has nothing to fire on in `nodf`/`none` — the case is mode-split at best. |
| A-corr-1 | correctness | minor (was blocking) | WEAKENED | H-A2 should state the two projection-role stamping bullets are an *exhaustive partition* over tuple mint sites and confirm V-PROJ-ROLE-STABLE's structural classifier is total; the "unstamped role → nondeterministic Equals" scenario is caught by V-PROJ-ROLE-STABLE / V-CONTRACT-CENSUS. |
| A-corr-2 | correctness | minor (was blocking) | WEAKENED | The "V-NO-COLLAPSE forbids payload hiding" mechanism is a relational-theory error (a key determines all attributes); residual symptom is that the diff's `DeterminedBy` is *narrow* (const/eqset/functor-output only) so it cannot prove a genuine FD — self-caught by the all-180-compile gate; the real question is A-corr-3. |
| T-oracle-2 | testability | minor (was blocking) | WEAKENED | Every referee for the produced `member_key` derives from the implementation under test; the contract is inert (Rel/codegen don't read it) so no behavioral oracle contradicts a wrong key. Fix: downgrade the headline "PROVABLE" to "recorded, proof deferred"; a wrong key is caught downstream at Stage B/C gates. |
| T-oracle-4 | testability | minor (was major) | WEAKENED | The lint deletion silently drops `agg_distinct_1`'s 5 advisory warnings with no coverage; the multiplicity trap is independently refereed by `bin/Oracle`, so the risk is a lost courtesy warning (UX), not a semantic regression. |

### Amendments (amend-diff)

**T-conf-1 (blocking).** `InferConservativeRowContracts` must not assume depth order is topological — `Depth()` is a cycle-*cut*, and because `ConnectInsertsToSelects`/`ProxySelects` replace every internal-relation SELECT with a kMember proxy TUPLE, the contract-dependency graph of any recursive program is cyclic *with no contract-leaf inside the cycle*. Pick one of two defined rules and write it into the hunk: **(i)** make the pass a real fixpoint over the SCC condensation with a monotone lattice — contracts only grow toward `AllFields(columns)`, bounded by `|columns|`, with an explicit termination measure; or **(ii)** give every view on a cycle (from `IdentifyInductions`' SCC membership, *not* from visitation order) a conservative `member_key = AllFields(columns)`, computed as a pure function of SCC structure. Also correct the H-A3 comment: "Depth order guarantees inputs precede users" is false — state it is a cycle-cut estimate.

**T-conf-2 (blocking).** Add an escalation (call it E-A4) for cyclic-graph contract soundness, and gate Stage A's "zero corpus impact / 180 byte-identical" claim on *actually running* the 180-case suite with `V-MEMBERKEY-REALIZED`/`V-NO-COLLAPSE` live over the recursive subset (`demand_tc_witness`, `d5_recursive_negate`, `fixpoint_stress_1`, `reconverge_1`) *before* blessing any `.contract` golden. Until T-conf-1's SCC/fixpoint rule lands, the "fires on ZERO of the 180" claim for both validators is unproven on cyclic input. Note that the H-A8 `.contract.opt.golden` referee (byte-compare, permcheck N/A) pins a run-order artifact for any recursive-view contract unless T-conf-1 makes the contract a pure graph function.

**A-corr-3 (escalate-owner, major).** Reword the DeterminedBy source. There is no `impl->equivalence_sets`. Name the correct backing: JOIN pivot and CMP equalities persist in the final graph as live `QueryJoinImpl`/`QueryCompareImpl` nodes and as shared column IDs assigned at `FinalizeColumnIDs` (`all_cols_match` compares `.Id()`); DeterminedBy's "proven-equal to a key column" clause and the JOIN Minimize must read *column-ID identity + persisted JOIN/CMP structure*, not the data-model storage union-find. If no persisted column-equality relation exists at the H-A4 slot, one must be produced/persisted before DeterminedBy is implementable — escalate which.

**T-oracle-1 (major).** Add a directed positive witness that exercises the role-in-`Equals` refusal branch: a hand-built micro-graph (or a `.dr`) whose `df.opt` dump would share one TUPLE line pre-refinement and provably splits into two post-refinement, pinned as a golden — plus a unit test asserting `Hash` differs and `Equals` returns false for a pair identical except `projection_role`. Without it the load-bearing "CSE-cannot-fold-kMember-into-kDistinct" property is asserted-unreachable, not tested.

**T-oracle-3 (major).** Correct the exit-gate row (line ~570) and H-A9 (line ~545): `member_collapse_1` is **mode-split** (rejects under `opt`/`nocf`, compiles under `nodf`/`none`), like `kvindex_1` — not "all-4-modes-diagnostic". Reconcile H-A7's promise of a directed reject witness per clean diagnostic with E-A2's admission that the kMember-collapse shape may be unconstructible from surface Datalog: either exhibit one surface-constructible reject (possibly a build-time, non-canon aggregate-input shape for `V-AGG-INPUT-KEY` that *could* be 4-mode) or state plainly that the lint's user-facing half is deleted with only belt-level replacement and reconcile with §13.5.

**T-oracle-4 (minor).** Add a stderr / expected-diagnostics assertion pinning that `agg_distinct_1` now emits **zero** warnings; do not treat "stderr is not golden-compared" as coverage. Under the new typed model there is no ambiguous-collapse class to diagnose (kDistinct collapse = intended; kMember unprovable collapse = hard error), so drop the "genuinely-ambiguous collapse still produces a diagnostic" half of the original ask.

**A-corr-1 / A-corr-2 (minor).** Doc-sharpening only: state the projection-role stamping bullets are an exhaustive source-head→kDistinct / internal-mint→kMember partition; note `DeterminedBy` is deliberately narrow (const/eqset/functor-output) and that the in-key guard is *not* the fix (A-corr-3 is).

**T-oracle-2 (minor).** Downgrade the headline "identity typed and PROVABLE" to "identity typed; contract correctness recorded, independent proof deferred to I0 (ranked #2)". Do not co-gate Stage A with the whole I0 interpreter.

**A-nec-1 (deletion-hunk, folded from necessity-audit candidate 1).** Strip the *populated* `RowContract.derivation_support` field, the merge-arm `DeltaSign` add/remove fold, and the join-arm support product — no Stage-A validator/analysis reads the value (the only consumer is the H-A8 dump that exists to pin it). **Keep** the domain types (`DeltaSign`/`DerivationSupportCount`/`DemandSupportCount`/`SupportAlgebra`): they earn their place via the H-A1 static_assert cross-domain-disjointness battery (the F4 deliverable). This is E-A3's "transfer the shape, not the counts" recommendation made concrete; the real support algebra reappears at Stage C where `RequestEdgeRelation` is its first genuine consumer.

### Coverage-audit results bearing on Stage A
- §11 lines 1–2 (`V-MEMBERKEY-REALIZED`, `V-NO-COLLAPSE`) land here — **clean**, but line 2's user-facing surface is at risk (E-A2 constructibility; see T-oracle-3).
- §15 invariant 11 (member vs distinct are different operators) established at H-A2 — clean.
- §12.3 witnesses 1–3 (Identity, Projection, Aggregate) home here — clean, witness-2 constructibility at risk.
- **X3 (naming inconsistency):** Stage C H-F cites `V-DEMAND-SUPPORT-DERIVED` as a Stage-A *named* validator, but Stage A defines it only as a `static_assert` battery (IdentityTypes unit), not a `V-*`. Reconcile the name or add the validator.

### Stage A verdict
**BLOCKED-ON [T-conf-1, T-conf-2]** — cyclic-graph contract soundness. Both are CONFIRMED blocking and share one fix (T-conf-1's SCC/fixpoint rule; T-conf-2's live-validator gate on the recursive subset). All other Stage-A findings are SOUND-WITH-AMENDMENTS.

---

## I0 — reference interpreter (`stage-i0-interpreter.md`)

### Surviving findings

| id | lens | adj. severity | verdict | one-line |
|---|---|---|---|---|
| **A-corr-1 (I0)** | correctness | **blocking** | CONFIRMED | For the flagship demand corpus, the I0==behavioral gate refereess empty-or-undefined content: `demand_tc_witness` publishes nothing (empty FINAL/epoch blocks), leaving only the QUERY block; H3 makes I0 emit the full definitional answer while the demand ABI exposes only per-key `_bf` cursors with no probe-enumeration contract, so the two QUERY blocks cannot be equal by construction. Hits all 19 bound-query `.batches` cases. |
| A-test-3 | testability | major | CONFIRMED | Stage C flips `demand_cyclic_1`/`demand_recursive_content_1`/`demand_multi_adorn_allfree_1` from reject to silent full materialization; these have no `.batches` and no pre-cutover binary, so post-flip I0 becomes their sole oracle with zero cross-check — strictly worse than the counted hole-1 cases. |
| A-conf-3 | confluence | minor | CONFIRMED | The `negation_flap` OQ3 carve-out is scoped by symptom, not mechanism: `NetBatch` nets order-free, so same-batch annihilation cannot be the order-dependent step; under the harness feeding, I0==behavioral=={1,3} and the carve-out appears **vacuous** — the "removal wins the dequeue race" text is stale pre-NetBatch lore. |
| A-corr-3 (I0) | correctness | minor (was major) | WEAKENED | CBF is the one spec both producers implement, so separators/prefixes are pinned; residual: the grammar says "SORTED" without pinning numeric-vs-lexical key (diverges on multi-digit values) and the behavioral binary can't call Oracle's `PrintValue`. |
| A-conf-1 | confluence | minor (was major) | WEAKENED | I0 as diffed never exercises request/data permutation; but I0 *is* the right (order-blind) confluence oracle — the missing permutation *generator* is a Stage-C/D deliverable already scheduled (Stage-D exit gate). Sharpen §2's holes list. |
| A-det-2 | confluence | minor (was major) | WEAKENED | CBF's byte-identity rests on two independently-implemented sorts sharing only `PrintValue`; a non-total sort key risks a spurious diff, and §3's taxonomy has no format/comparator-bug arm — fail-safe (surfaces at first bless), so mandate one total-order shared sort + block order + a §3 format arm. |
| A-test-1 | testability | minor (was blocking) | WEAKENED | I0's novel per-epoch delta surface is only cross-checked against the behavioral binary (compiler-under-test); genuine under OG1-parsed (independent second implementation) + membership-diff-vs-publish-capture. Nice-to-have: anchor intermediate-epoch membership to the Oracle's per-batch recompute. |
| A-test-2 | testability | minor (was blocking) | WEAKENED | I0's answer-only CBF does not subsume eqgate's HP-5 teeth for *purely-internal* over-materialization — but that escapes every behavioral oracle; the published-delta discriminator IS reproduced by CBF. Reframe §3.3's HP-5 claim; nothing deleted in Stage I0. |
| A-test-4 | testability | minor (was major) | WEAKENED | `permcheck.py`'s default `delta_re` matches only parenthesized `+(...)` tokens, not CBF's `+<pubmsg> <val>` grammar → inoperative on CBF; but sorted-both-sides makes byte-compare already multiset-exact, so the fallback is inert-by-design. Drop it or ship a CBF `--spec`. |
| A-test-6 | testability | minor | WEAKENED | Only the "state the shared `PrintValue` routine requirement" amendment survives; the decisive gate (I0 CBF vs behavioral golden) has no normalizer on it. |
| A-nec-5 | necessity | minor (was major) | WEAKENED | CBF is order-destroying (set oracle), so I0 referees final membership + netted deltas, not ordering-invariance (acceptance-14); the doc already says so (§0 table, holes #4/#5). Sharpen H9's "I0 proves the closure" to "referees the canonical netted value", label `negation_flap` a representative of the ordering class. |

### Amendments (amend-diff)

**A-corr-1 / I0 (blocking).** H4 must define a bound-query probe-enumeration contract so I0 emits answers for exactly the keys the demand binary can serve. Options: a per-case `.probes` sidecar, or thread the bespoke driver's hand-picked probe list (e.g. `demand_tc_witness.main.cpp`'s `{1,3,7,5}`) into *both* the emitted harness and a matching restriction of I0's QUERY block. Escalate the open decision this forces: does I0 emit *full-relation* answers (needs a non-demand enumeration path the demand ABI lacks) or *probe-restricted* answers (needs the probe source)? Until resolved, the QUERY block — the sole non-empty CBF content for the 5 pure demand witnesses — refereess nothing.

**A-test-3 (major).** Add `demand_cyclic_1`, `demand_recursive_content_1`, `demand_multi_adorn_allfree_1` to the exit-gate ledger explicitly, and author I0 demand-blind answer goldens for them *when Concern-2's reject→full-materialization flip is ratified* (the flip is an open owner decision; any golden authored now is provisional). I0 is demand-blind and evaluates the plain recursive program by semi-naive fixpoint, so a definitional answer for the post-flip behavior exists pre-cutover and can be pinned once the semantics are decided.

**A-conf-3 (minor).** Drop the `negation_flap` carve-out (H9/OG4) or, if a genuine order-dependence is asserted, actually construct the cross-batch feeding that exhibits it — the current `.batches` (one block = one entry-point call, NetBatch on the message handler) nets `blocker(2)` add+remove to zero and all three referees agree `{1,3}`, so as written the carve-out is vacuous. Remove the stale "removal wins the dequeue race" comments from `negation_flap.{dr,batches,main.cpp}` and `Oracle/Main.cpp`. Reconcile the "I0 is the closure referee" sentence with A-conf-1 (I0 can only referee this once a permutation generator exists).

**A-corr-3 (I0) / A-det-2 (minor).** Pin **one** normative total-order comparator over full typed tuples (sign excluded from the key or explicitly placed; numeric-by-column, not lexical) and a normative relation/query block emission order, mechanized as a single shared post-processor both producers pipe through — CBF byte-identity becomes a construction guarantee. Require the behavioral harness to render `<val>` through the same `PrintValue` lexeme routine I0 uses (the harness is a tool the I0 authors write). Add a "format/comparator bug" arm to §3's adjudication taxonomy so a harness artifact is not misattributed to the compiler.

**A-conf-1 / A-nec-5 (minor).** One-line clarifications: the permutation *generator* that lets I0 referee acceptance-14 lands with Stage C/D (regional-arch §6 Stage-D exit gate), not the I0 stage; §0/H9 should distinguish "refereeing the canonical netted value a fixed compiler must produce" from "refereeing invariance under input permutation (Stage D harness)".

**A-test-1 (minor).** Extend H7 to byte-tie I0's EPOCH blocks to an independent per-epoch dump: have `bin/Oracle` emit a per-batch `DumpRelations` snapshot (it already recomputes per batch in `CheckAgainstScratch`) so intermediate-epoch membership has an independent authority, not only FINAL.

**A-test-4 (minor).** Either drop the `permcheck.py` fallback (sorted-both-sides makes it dead weight) or ship a CBF-specific permcheck spec (`delta_re`/`boundary_re` matching the H2 grammar) and prove it on a real CBF transcript before citing it.

**A-test-2 / A-test-6 (minor).** Reframe §3.3's HP-5 claim: CBF's published-delta surface reproduces the dead-key discriminator (`if (!log.rows.empty()) abort()`), so I0+behavioral referees answer-visible over-materialization; only *purely-internal* over-materialization escapes, and it escapes every behavioral oracle including the eqgate driver. State the H4 harness `<val>` renderer shares I0's `PrintValue`.

### Coverage-audit results bearing on I0
- Concern 1 (HP-5 oracle class) homes at the I0 stage — reframed per A-test-2.
- §12.3 witnesses 4 (Multiple owners) and 10 (Local recursion) list "I0 has no pre-cutover referee (E2/E3)" — A-test-3 supplies the demand-blind fixpoint path.
- **X4 (numeric drift):** bound-query corpus counts differ (Stage C "~38 flag-off"; Stage D "~53 / ~127 non-demand"; CLAUDE.md "38/165") — state once.

### I0 verdict
**BLOCKED-ON [A-corr-1 (I0)]** — the bound-query probe-enumeration contract; without it the flagship demand gate compares empty content. All other I0 findings are SOUND-WITH-AMENDMENTS.

---

## Stage B — planning regional program (`stage-b-diff.md`)

### Surviving findings

| id | lens | adj. severity | verdict | one-line |
|---|---|---|---|---|
| T-det-1 | termination | minor (was major) | WEAKENED | The `-region-out` dump's determinism is asserted; H7 names the HP-9 rule but never pins the port/ABI/row-contract enumeration key. Deterministic order is the codebase default (`IOs()` is an id-ordered DefList) so risk is low — pin it explicitly. |
| B-test-1 | testability | minor (was blocking) | WEAKENED | The `-region-out` golden is self-blessed from the formatter under test; ports are a copy of `ParsedDeclaration` ABIs (human-bless-checkable) and row-contracts are Stage-A-validated pass-throughs, so the correctness oracle is Stage A's gate, not a Stage-B hole. Soften "No missing oracle at Stage B"; add a ports==declared-ABI assert. |
| B-test-2 | testability | minor (was major) | WEAKENED | The three freeze validators land as abort belts no corpus input trips; but `V-FROZEN-NO-OPEN-PORT` belts the new `FreezeAndValidate` transition (§12.1 "immutable frozen ports" is a genuine Stage-B surface). Add one Stage-B freeze-port abort-path unit test; the cyclic/census abort tests belong to Stage C. |
| B-test-4 | testability | minor (was major) | WEAKENED | The H4 exit-gate "bisect re-cut at same op count" is not exercised (zero `.drflags` mention bisect); the guarded failure is near-nil (the wrapper leaves the `gPassPolicy` thread untouched) but the obligation is unoperationalized. |
| A-corr-1 (B) | correctness | minor (was major) | WEAKENED | H9's §11 certification omits items 3 (symbolic-parameter-escape) and 6 (sealed-ABI mutation); item 3 is vacuous (`parameters=(none)`) and item 6's immutability is enforced structurally (const `FrozenRegionalProgram&`, parse-level ABIs by reference, disjoint from codegen TableId space). Name both in the deferred list. |
| A-corr-2 (B) | correctness | minor | WEAKENED | Textual inconsistency: H3 makes `FreezeAndValidate` a clean-diagnostic path, H9 makes the three validators fprintf+abort. They coexist (abort belts for internal invariants + inert clean-diagnostic scaffolding for Stage C) per the demand-file precedent — clarify. |
| A-corr-3 (B) | correctness | minor | WEAKENED | H2 presupposes a bound `#query` for `request_port`; the query-less majority (~77% of corpus) has no skeleton. The grammar already shows the `<none>` idiom and the witness subset spans both shapes — one-word clarification (`request_port = <none>` when no `#query`). |
| A-corr-4 (B) | correctness | minor | WEAKENED | The H2 byte-identity cell rests on an unstated purity assumption for `BuildPlanningRegionalProgram`/`BuildProgramRootShell`/`FreezeAndValidate`; ESC-2 + H3 already commit no-renumber/no-reopt, and the 180-golden gate backstops it. State the read-only-over-QueryImpl / by-reference-ABI invariant so the cell is derived, not asserted. |

(Necessity-lens Stage-B findings A-nec-1/3/4 are in `necessity-audit.md`; A-nec-3's deletion of `V-OWNERSHIP-ACYCLIC` is folded into the amendment below.)

### Amendments (amend-diff)

**T-det-1 (minor).** In H7, pin the dump's port/ABI/row-contract enumeration explicitly to the id-ordered `IOs()` DefList walk (Query.h:1093), and mint `PortId`/`EdgeId` in that same walk order, stating the sort key exactly as HP-9 requires. Fold this into the already owner-gated grammar decision (ESC-5); no separate referee-strengthening change is needed (single blessed byte-goldens are the house norm, matching `symrec_tie_1`).

**B-test-1 (minor).** Strike or soften the summary "No missing oracle at Stage B. Every hunk has a byte-compare gate" — it conflates determinism-pinning with correctness. Add a cheap cross-check: assert the dump's port set == the parsed-module declared-ABI set. Row-contract correctness is Stage A's oracle (lands A-before-B under the sanctioned order); do not re-home it as a Stage-B hole.

**B-test-2 (minor).** Land, with the validators, one hand-built malformed `PlanningRegionalProgram` unit test that trips `V-FROZEN-NO-OPEN-PORT` (inject a surviving `OpenRequestPort` past freeze) — the §12.1 "immutable frozen ports" surface. Defer the self/cyclic-ownership and census-mismatch abort-path tests to Stage C, where the multi-region machinery that can construct them arrives.

**B-test-4 (minor).** Add a bisect witness to the Stage-B gate: run one existing case at two `-opt-bisect-limit` values before/after the wrapper and byte-diff the cut point. Cheap, and it operationalizes the H4 "bisect thread survives" obligation the standard harness cannot exercise.

**A-corr-1 (B) (minor).** In H9's certification, name §11 item 3 as vacuous-at-Stage-B (`parameters=(none)`) and item 6 as immutability-enforced-by-the-frozen-type/const-seam (runtime belt deferred to Stage C when a mutation-capable path first exists); note item 15 rests on carried-forward `V-LOOP`/`V-READY`. (See coverage ORPHANs 3, 6 below — this is the doc half of closing them.)

**A-corr-2 (B) (minor).** State in H3 that the three Stage-B freeze validators are fprintf+abort internal-invariant belts and the `log.Append`/`num_errors` clean-diagnostic path is inert scaffolding for Stage C's freeze validators — the two coexist, matching the demand-transform file's precedent (clean rejects + abort belts in one surface). Do not delete one contract.

**A-corr-3 (B) (minor).** Specify the query-less region skeleton in H2: `request_port = <none>`/absent when no `#query`; ports carry only input/result ABIs. Add a worked query-less rendering to each H7 grammar illustration (the pinned witnesses `merge_2`, `join_1` are query-less and bless it by construction).

**A-corr-4 (B) (minor).** State as an H2/H-EXIT exit condition that `BuildPlanningRegionalProgram`/`BuildProgramRootShell`/`FreezeAndValidate` are read-only over `QueryImpl` and consume parse-level `ParsedMessage`/`ParsedQuery` ABIs by reference only (no id/order assignment), so the byte-identity cell is derived from a checked invariant, not an assumption.

**A-nec-3 (B) (deletion-hunk, folded from necessity-audit candidate 2).** Remove `V-OWNERSHIP-ACYCLIC` from the Stage-B freeze-validator scaffold: it is provably unfireable over a one-node, zero-edge ownership forest. Keep `V-FROZEN-NO-OPEN-PORT` and `V-REGION-CENSUS-IDENTITY` (they belt Stage-B-new `FreezeAndValidate`/seam code). `V-OWNERSHIP-ACYCLIC` lands at Stage C with its first `FrozenChildCall` and a directed self/cyclic-ownership reject witness — born with its failure mode, testable at birth.

### Coverage-audit results bearing on Stage B
- §11 line 4 (`V-FROZEN-NO-OPEN-PORT`), 7/8 (`V-OWNERSHIP-ACYCLIC`), 16 (`V-REGION-CENSUS-IDENTITY`) land here as scaffolds — clean.
- §15 invariant 1 (one FrozenRegionalProgram) established at H1+H3 — clean.
- **X1/X2 (B→C promises unfulfilled):** Stage B H9 defers the parent/child frozen-port validator (§11 line 5) and `V-PURE-REGION` (§11 line 9) to Stage C; Stage C H-I lands neither. These become ORPHANs at Stage C, not Stage B — but Stage B must stop asserting they are "covered at Stage C" until Stage C actually lists them.

### Stage B verdict
**SOUND-WITH-AMENDMENTS [T-det-1, B-test-1, B-test-2, B-test-4, A-corr-1(B), A-corr-2(B), A-corr-3(B), A-corr-4(B); + defer V-OWNERSHIP-ACYCLIC per necessity-audit A-nec-3(B)]** — no surviving blocking finding; every finding was refuter-trimmed to minor.

---

## Stage C — request-edge lifecycle (`stage-c-diff.md`)

### Surviving findings

| id | lens | adj. severity | verdict | one-line |
|---|---|---|---|---|
| **corr-1** | correctness | **blocking** | CONFIRMED | `force.dr` (shipped as both `data/examples/` and `tests/OptDiff/cases/`, the sole `@first`-body corpus file) proves query-*time* message injection; §10/§14 delete query-body `@first` forcing with no regional replacement, so its round-1 output materially changes — yet EG.1 asserts all `.stdout` stay byte-identical and names only the six demand witnesses as rewritten. A concrete byte-identity over-claim on a shipped example whose source uses deleted syntax. |
| **corr-3** | correctness | major | CONFIRMED | `PermanentRoot` is a first-class `RequestOwnerId` (§5.2) that Stage C owns (§13 step 2), and its published observation needs an add-only request edge born at init — but H-G.2's edge-birth mapping sources only `RootLease`/`RegionalMember`, H-H covers only the cursor lease, and an add-only edge collides with `V-EDGE-BALANCE`. The §12.3 permanent-root witness has no lowering path. |
| corr-2 | correctness | minor (was major) | WEAKENED | H-G.2's shared-pub realization drops the per-instance rescan spine and moves the demand gate to output-only `RoutedResult` membership, so the region body over-materializes derivation; but demanded region bodies are non-recursive in Stage C (fenced), so inv #4/§7.1 are trivially satisfied and there is no divergence witness. State Stage C over-materializes region-body derivation; qualify "answer-neutral" to terminating programs. |
| T-conf-1 (C) | termination | non-blocking (was blocking) | WEAKENED | Same-epoch request/data permutation confluence (§15.14) is asserted but no Stage-C referee tests it; §11/H-I validators are structural, permcheck permutes only output tokens. But §12.2 (a normative diff target) already requires the permutation test with I0 as oracle. Operationalize it in the exit gate. |
| T-oracle-4 (C) | testability | minor (was major) | WEAKENED | EG.3's lifecycle `.rel` goldens are re-blessed from the implementation; `V-LIFECYCLE-CENSUS` is a three-layer agreement check anchored to independently-derived frozen-structure counts, and seven H-I validators catch the finding's own examples. Hand-predict one lifecycle `.rel` census; state the census is an agreement check, not the oracle. |
| T-oracle-3 (C) | testability | minor (was major) | WEAKENED | `permcheck` compares the per-epoch delta *multiset* with no owner attribution; but it is a fallback, not the referee, and the §12.3 multi-owner witnesses are defined by per-owner behavior + in-driver asserts. Require multi-owner witnesses to print per-owner-partitioned output. |
| T-oracle-2 (C) | testability | minor (was blocking) | WEAKENED | EG.4 deletes the eqgate family; the shared-pub cursor-filter structurally eliminates the answer-leaking over-materialization the eqgate caught, so deleting the detector is correct — but the answer-visible mis-routing sibling remains. Mandate rewritten drivers retain per-owner exact-membership asserts (to catch mis-routing, not over-materialization). |
| T-oracle-1 (C) | testability | minor (was blocking) | WEAKENED | I0 cannot compute 3 of §12.2's 5 comparison targets (RequestEdgeId/ChildInstanceId/routed sets); but the diff never tasks I0 with them, and those sets are modelled by always-on H-I validators + directed witnesses. Clarify §12.2's list (only membership+deltas from I0). |
| T-oracle-5 (C) | testability | minor (was major) | WEAKENED | Under Variant A the four demand diagnostics flip to compiling and the new reject is "Stage-D-facing" (possibly unreachable one-level → vacuous 4-mode gate); but the diff already recommends Variant B (reachable via `demand_cyclic_1`'s recursive dataset) and E2 escalates the choice. Explicit per-case 4-mode pinning in EG.5. |
| T-oracle-6 (C) | testability | minor | WEAKENED | The six rewritten witness drivers differ pre/post-cutover, so tagged-binary equivalence is over the semantic projection (machine-decisive), not raw stdout; add an explicit dropped-lease-drain negative witness (mostly covered by §12.1 + the Detachment witness). |
| T-zero-1 | termination | minor | WEAKENED | The "differential JOIN vs band-b queue decrement" dichotomy is false — band-b *rides* the ordinary pub split-counter commit sweep, so double-retraction nets correctly (fixpoint_stress_1 machinery). Residual: E1's differential-pub witness should exercise the same-epoch edge∧child double-retraction crossing. |
| N-nec-2 | necessity | escalate (was major) | WEAKENED | `ExtractionPolicy=always-true` + `-demand` flag deletion ships extraction as the default for the ~38 flag-off cases the opt-in gate existed to exclude; but this is E3 (already escalated) and perf never gates correctness. Disposition (defer all extraction to D) refuted — ChildResult/RoutedResult are defined over ChildInstanceId, so zero extraction leaves the lifecycle with nothing to operate on. |

### Amendments (amend-diff)

**corr-1 (blocking).** Name `force.dr` explicitly. Move `force.stdout` out of EG.1's byte-identical set into a **deleted-behavior** bucket: re-bless it to the reduced output (round 1 collapses to `{5,6}`) or delete the case, accepting the lost `@first` example coverage. Note that §13 Stage-C step 7 removes the `@first` body-forcing syntax, so `force.dr` will not parse — the source must also be removed or rewritten. **Escalate-owner:** does the regional model offer *any* query-time-forcing replacement (a request edge cannot inject a message — §7.2 pins the effect in ProgramRoot), or is `force.dr`'s generative behavior intentionally dropped per §10? EG.1's "every case's `.stdout` stays byte-identical" is false as written; correct it and enumerate `force.dr` alongside the six demand witnesses as a changed case.

**corr-3 (major).** Add `PermanentRoot` as a `kRequestEdgeAdd` source (edge born at program init, never removed), and either carve add-only permanent edges out of `V-EDGE-BALANCE`'s "every request edge has a balanced add/remove path" or define a program-teardown removal. Add a lowering hunk emitting the permanent-root edge and the §12.3 permanent-root directed witness (published observation stays live without a cursor lease). Without this, an extracted permanent-output child — legal under always-true `ExtractionPolicy` — publishes nothing (`RoutedResult = RequestEdge ⋈ ChildResult` with no standing edge), or the census validator aborts the compile. (Closes coverage UNHOMED §12.3 row 11.)

**corr-2 (minor).** State in H-G.2 that Stage C over-materializes region-body derivation (full input crossing, output-only demand filter) and qualify the "extraction is answer-neutral" claim to *terminating* programs. Note that inv #4/§7.1 hold trivially because demanded region bodies are non-recursive in Stage C (Build.cpp cyclic/recursive-content fences), and the instance-qualified-fixpoint validator is deferred *with its feature* to Stage D (H-G.4), not orphaned.

**T-conf-1 (C) (non-blocking).** Operationalize §12.2's request-vs-input permutation test in the Stage-C exit gate (it is already a normative diff target): permute independent request and input updates within one epoch and compare final relation membership + net published deltas + live RequestEdgeId set via I0, and add the §12.3 "Request/data order: request-before-data, data-before-request, same-epoch flaps" directed witness as a Stage-C obligation. Name which oracle discharges §7's "validation proves the schedule computes this transition" — I0, not the structural H-I validators.

**T-oracle-4 (C) (minor).** Hand-predict at least one directed lifecycle `.rel` op census as an independent expected count (the house predict-then-verify-IR methodology), and state in EG.3 that `V-LIFECYCLE-CENSUS` is a three-layer *agreement* check, not admissible as the sole correctness oracle for the census.

**T-oracle-3 (C) / T-oracle-2 (C) (minor).** Require the multi-owner/detachment witness drivers to print deltas partitioned by `RequestOwnerId` (so the permutation fallback never collapses cross-owner tokens) and to retain per-owner exact-membership `ASSERT`s over the demanded key (catching answer-visible mis-routing, the real correctness class the shared-pub cursor-filter leaves). Restate the §12.3 obligation that these witnesses are per-owner by construction.

**T-oracle-1 (C) (minor).** Clarify §12.2's 5-target list: only *final membership + sorted published deltas* come from I0; the live RequestEdgeId/ChildInstanceId sets and routed results are checked by the always-on H-I validators (`V-EDGE-BALANCE`, `V-OWNER-EXACT`, `V-ROUTE-EXACT`, `V-INACTIVE-AFTER`) and the §12.3 directed witnesses, not the interpreter.

**T-oracle-5 (C) (minor).** Adopt Variant B (E2's recommendation): `demand_cyclic_1`'s already-recursive dataset carries a reject genuinely reachable in the one-level Stage-C envelope. Pin every retained/new reject as an explicit 4-mode expected-diagnostic in `runall.sh` (EG.5's "updated accordingly" must spell out the per-case CLAUDE.md pinning). If Variant A is chosen, the new reject must be shown to actually trigger one-level, not merely named.

**T-oracle-6 (C) / T-zero-1 (minor).** Add a directed witness that a dropped lease-removal drain produces an observable, asserted divergence (mostly covered by §12.1's move-only-lease test + the Detachment witness). Sharpen E1: the required differential-pub witness must exercise the same-epoch edge-remove ∧ child-result-remove double-retraction crossing (the ≥0-per-class commit-sweep boundary).

**N-nec-2 (escalate-owner).** The `ExtractionPolicy=always-true` default perf-regresses the ~38 flag-off cases the `-demand` opt-in excluded (message-rooted trivial shapes never pay). Reclassify E3 from "non-blocking, shape/perf only" to an **accepted-regression** with an explicit list of pessimized cases, OR gate `ExtractionPolicy` false unless the slice is demand-rooted. Do not defer all extraction to Stage D (refuted: the request-edge/routed-result lifecycle is defined over ChildInstanceId — zero extraction leaves it inoperative, and Stage C would no longer replace forcing).

### Coverage-audit results bearing on Stage C
- **ORPHAN §11 line 5** (parent/child frozen-port disagreement): Stage B deferred it "ALL STAGE C"; Stage C H-I omits it. Add it to H-I — a `FrozenChildCall` mapping must match the child's frozen port schema.
- **ORPHAN §11 line 9** (`V-PURE-REGION`, effectful operator inside a region): Stage B deferred it to Stage C; Stage C H-I lists only the H-E admissibility purity clause. Add the named validator (undermines §15.13).
- **ORPHAN §11 line 6** (sealed-ABI mutation) and **line 3** (symbolic-parameter-escape): admissibility-gate clauses only, no named validator; a would-be mutation is *declined into full materialization*, never a compile failure. Either add validators or state the "compilation fails" §11 semantics is intentionally softened to "declines extraction".
- **UNHOMED §12.3 row 11** (Permanent root): closed by corr-3's amendment.
- **UNHOMED §12.3 row 12** (Effects): §7.2 placement rule, no directed effects witness in any stage (tied to the orphaned `V-PURE-REGION`) — author one at Stage C.
- **Concern 2 (E2):** owner variant decision owed (blocking-to-owner); exit gate written to bind under either variant.

### Stage C verdict
**BLOCKED-ON [corr-1]** — the `force.dr` `@first` byte-identity over-claim and its escalate-owner query-time-forcing question. **corr-3 (major, CONFIRMED)** is a required amendment (permanent-root edge + witness). Concern-2 (E2) is an owner decision the stage is written to bind under either arm. All other Stage-C findings are SOUND-WITH-AMENDMENTS.

---

## Stage D — deep forest + local recursion (`stage-d-diff.md`)

### Surviving findings

| id | lens | adj. severity | verdict | one-line |
|---|---|---|---|---|
| **T-oracle-4 (D)** | testability | **blocking** | CONFIRMED | §0.1 promotes `cyclic_demand` to a permanent planning-time abort/reject (bound to `V-NEST-DEPTH-FINITE`), while E3 says a recursive region call "stays inadmissible → silent full-materialization FOREVER, not a compile error". A recursive region call cannot be both; until resolved the §12.3-mandated rejection witness has no defined referee (4-mode diagnostic vs 4-mode silent-compile golden), Part 6 carries zero negative rows, and `V-SCC-INSTANCE-CLOSED` may be a latent (unfireable-from-source) validator. |
| **A-corr-3 (D)** | correctness | major | CONFIRMED | The composition {local recursion} × {last-edge detach / inactive-retirement} — the highest-risk new Stage-D surface — is neither witnessed nor argued: recursion rows never retract an edge, the DETACH row is non-recursive, Part 3.3 folds `retire_inactive` into `DeriveDRStrata` without arguing a request-edge net-removal correctly *seeds* the recursive OVERDELETE, and no Stage-D validator ties a dead key's recursive-closure row count to zero after seal. |
| A-corr-4 (D) | correctness | minor (was major) | WEAKENED | The recommended V-CW realization satisfies §15 invariant 4 only via the disjoint-union lemma the diff itself flags as undischarged; but Part 4 already states the lemma + directed witness + `V-INSTANCEKEY-NOT-DERIVED`, and V-PI is offered as the safer option. One-sentence cross-ref: clause (b)/`V-INSTANCEKEY-NOT-DERIVED` is what makes V-CW satisfy inv 4, not only termination. |
| A-corr-2 (D) | correctness | escalate (was major) | WEAKENED | Nothing gates extraction to request-targeted slices, so a non-demand program with a pure admissible slice could extract under Stage-C one-level extraction and break the ~127-case byte-identity gate (the first place a byte-identity gate coexists with active extraction). Contingent, not proven. Correct the case counts (~53/~127 vs measured ~41/139). |
| A-corr-1 (D) | correctness | clarity (was blocking) | WEAKENED | Hunk 1.1 adds `worklist.push(child)` with no explicit `worklist.pop()` drain; but the implementer note requires the order "remain total, deterministic AS THE FOREST DEEPENS" (a growing order), so push feeds the outer iteration and descent happens. Make the drain explicit / state push feeds the outer loop. |
| A-term-1 | termination | minor (was major) | WEAKENED | "The diff never gives a compile-time measure" is false — Hunk 2.2 states the monotone-shrinking-open-slice argument and Measure 1 bounds the worklist by the finite acyclic ownership forest. Fold Measure 1 + Hunk 2.2 + §8 into one explicitly-stated global measure. |
| A-term-2 | termination | minor (was blocking) | WEAKENED | `V-INSTANCEKEY-NOT-DERIVED` worded "no InstanceKey column APPEARS on a recursive back-edge" would reject the V-CW carry; but it is a *frozen-compile* validator (upstream of Rel key-widening), so it never sees the widened rows, and the intent is provenance ("BOUND, never DERIVED"). Restate as "verbatim carry of the bound key, never a derived value"; note the pre-widening timing. |
| A-conf-3 (D) | confluence | minor | WEAKENED | Parent reopt against open-vs-frozen child confluence is asserted; but `ReplaceSliceWithOpenChildCall` runs before `ReoptimizeParent`, so the parent optimizes against a call node in both passes — the child internals are never exposed. E4 already names the post-freeze reopt purity obligation. Optional open-vs-frozen byte-compare witness. |
| T-oracle-1 (D) | testability | non-blocking (was blocking) | WEAKENED | All three Stage-D semantic referees are answer/state observers; `RoutedResult = RequestEdge ⋈ ChildResult` filters answer-correct-but-wasteful child rows out of every observer, so answer-neutral over-materialization is untested. But that is a materialization/perf property (cost/bench track, never gates correctness). Add a work-observing *regression anchor* + note the old driver-level tripwire is retired. |
| T-oracle-5 (D) | testability | minor (was major) | WEAKENED | The ~127-case no-bless byte-identity gate justifies itself with "never extracts", which is unproven/likely-false (nothing gates non-demand extraction off); but the suite byte-compares *stdout* and extraction is answer-neutral, so stdout goldens need no bless. Name and validate the extraction-gating precondition (or prove stdout-neutrality). |
| T-oracle-6 (D) | testability | minor (was major) | WEAKENED | The Late-subscriber witness's no-recomputation *work* claim (§5.3 fanout) is delta-invisible under order-free permutation invariance, so its named referee cannot decide it; give that specific witness a maintained-vs-derived regression counter. (Self-blessed `.rel` pins are already scoped as anchors, not oracles.) |
| N-nec-3 (D) | necessity | clarity (was major) | WEAKENED | `V-REGION-STRATUM-ORDER` is the region-boundary sibling of the carried-forward Kahn `V-LINEAR`/`V-READY` belts, not pure legacy; region calls are port-mediated bidirectional couplings, so the ordering does not fall out of def/use for free unless `DeriveDRStrata` treats cross-region port crossings as edges (which Hunk 3.3 does). State whether the "ADDITIONAL" constraint is derived-from-port-edges (validator-only belt) or an independent tie-break (justify). |

### Amendments (amend-diff)

**T-oracle-4 (D) (blocking, escalate-owner).** Resolve reject-vs-silent-full-materialize per permanently-excluded class, then make §0.1 and E3 consistent: §0.1 currently says a recursive region call is *promoted to a permanent abort*; E3 says it becomes *silent full-materialization*. Decide which, and:
- if reject: fix E3, and prove `V-SCC-INSTANCE-CLOSED` / the `cyclic_demand` abort is source-fireable (author the 4-mode diagnostic witness that positively asserts it fires);
- if silent full-mat: fix §0.1's abort-validator language (the belt fires only on a planner *bug*, user recursion routes to full-mat), and author a 4-mode *golden* asserting the program silently compiles + full-materializes.

Carry §12.3's Rejection row into Part 6's matrix (it has zero negative rows today), and decide whether `V-SCC-INSTANCE-CLOSED` is source-fireable or a bug-injection-only belt. (Latent-validator F-class hazard: an always-pass validator with no source witness is undetectable.)

**A-corr-3 (D) (major).** Add two directed witnesses: **(1)** a recursive local SCC inside one instance whose *last* request edge is retracted, asserting via I0 that every recursively-derived row for that key is gone and no other key's closure is disturbed (exercises OVERDELETE-and-retire of a widened, InstanceKey-partitioned SCC in one epoch); **(2)** a fanout-over-recursion witness attaching a late second edge to a recursive child. Add a retirement-completeness validator tying a dead key's recursive-closure row count to zero after seal. In Part 3.3, argue explicitly that a request-edge net-removal correctly *seeds* the recursive OVERDELETE (the design-completeness half, distinct from the witness coverage).

**A-corr-2 (D) (escalate-owner).** Escalate: does an extraction *candidate* require an incoming request edge with exact identity (candidates = request targets only)? If yes, name that gate in Hunk 1.2 and the byte-identity "never extracts" claim holds; if no, the ~127-case no-bless gate's justification is false and must be dropped or the shape re-blessed. Also correct the case counts to CLAUDE.md's measured values (~41 bound of 180, not ~53).

**A-corr-4 (D) (minor).** In E1/E2, add one sentence: V-CW's satisfaction of §15 invariant 4 *is* the disjoint-union lemma, discharged by clause (b) / `V-INSTANCEKEY-NOT-DERIVED` + the nonlinear-recursion-under-two-keys witness — not only termination. Keep the V-CW-vs-V-PI choice owner-gated (do not silently promote V-PI to default).

**A-corr-1 (D) / A-term-1 (clarity/minor).** Make the descent drain explicit: `while worklist: region = worklist.pop(); OptimizeLocalGraphToFixpoint(region); SolveBackwardRequirements(region); <inner while>`; re-anchor the E4 determinism witness on the pop order — or, if descent rides a live outer iteration, delete `worklist` and state that `stable ownership order` re-yields new children with a deterministic append-vs-view-walk tie-break. Fold Measure 1 + Hunk 2.2's monotone-open-slice argument + §8's no-clone clause into one explicitly-stated compile-time global measure (finite total logical nodes, strictly partitioned by clone-free extraction, monotone under reopt) and state that post-freeze reopt exposes no new candidate.

**A-term-2 (minor).** Restate the property and `V-INSTANCEKEY-NOT-DERIVED` as "the InstanceKey column on every recursive back-edge is a verbatim carry of the instance-bound key, never a derived/transformed value" (a provenance check, not an occurrence check), and note it runs on the FrozenRegionalProgram *before* V-CW's Rel key-widening. Reconcile the three co-referenced sites (Hunk 1.2(b), Part 4 Measure 2, Part 5).

**T-oracle-1 (D) / T-oracle-6 (D) (non-blocking).** Add a work-observing *regression anchor* (a maintained-vs-derived row counter) for the recursion/nesting/late-subscriber witnesses, and note in the diff that the new `RoutedResult` semantics *retires* the old driver-level over-materialization tripwire (a real observability loss worth stating). Do not add a work-bounding referee to the *correctness* exit gate — answer-neutral over-materialization is the cost/bench track's concern (`ExtractionPolicy` seam), which never gates correctness.

**T-oracle-5 (D) (minor).** Replace the exit gate's "never extracts" justification with the actual extraction-gating precondition (`ExtractionPolicy` false unless demand-rooted, or a proof that extraction is stdout-neutral), and make it a validated invariant. If extraction ever changes an answer or diverges a mode, the suite goes RED and the correct response is to gate extraction — not a 127-golden mass re-bless.

**A-conf-3 (D) (minor).** State that `ReplaceSliceWithOpenChildCall` runs before `ReoptimizeParent` (parent optimizes against a call node, child internals never exposed), so parent-reopt confluence is the existing optimizer's canonical-fixpoint property, not a new Church-Rosser obligation; optionally add a byte-compare witness running parent reopt against an open vs frozen child.

**N-nec-3 (D) (clarity).** State whether the "ADDITIONAL" region-ownership-postorder constraint on `DeriveDRStrata` is derived from cross-region port def/use edges (then `V-REGION-STRATUM-ORDER` is a validator-only belt, the region-boundary sibling of `V-LINEAR`/`V-READY`) or an independent tie-break (then justify why the port edges do not capture it). Do not delete the validator.

### Coverage-audit results bearing on Stage D
- §11 line 4 deepens to `V-PORT-POSTORDER-CLOSED`; lines 7/8 to `V-NEST-DEPTH-FINITE` — clean.
- **ORPHAN §11 line 10** (sequestered-key-in-InstancePath): Stage D adds `V-INSTANCE-KEY-ALIAS`/`V-INSTANCEKEY-NOT-DERIVED` (alias-distinctness, not-derived) but neither checks "sequestered key *present* in InstancePath until proven removable" — undermines §15.12. Add the presence check or re-home the invariant.
- §15 invariant 4 established at Part 3 (instance-qualified fixpoint) — clean but caveated (A-corr-4).
- §12.3 witnesses 9 (Nested) and 10 (Local recursion) home at Part 6 — but the recursion × detachment *composition* (A-corr-3) and the Rejection row (T-oracle-4) are missing.

### Stage D verdict
**BLOCKED-ON [T-oracle-4 (D)]** — the §0.1-vs-E3 reject-vs-silent-full-materialize contradiction, which blocks authoring the sole §12.3 rejection witness. **A-corr-3 (D) (major, CONFIRMED)** is a required amendment (recursion × last-edge-detach witness + retirement-completeness validator + OVERDELETE-seeding argument). All other Stage-D findings are SOUND-WITH-AMENDMENTS.

---

## Cross-stage roll-up

| stage | verdict | blocking |
|---|---|---|
| A | BLOCKED-ON | T-conf-1, T-conf-2 |
| I0 | BLOCKED-ON | A-corr-1 (I0) |
| B | SOUND-WITH-AMENDMENTS | — |
| C | BLOCKED-ON | corr-1 (force.dr) |
| D | BLOCKED-ON | T-oracle-4 (D) |

**Five blocking findings** across four stages. Stage B is the only clean stage.
Common thread across the non-blocking amendments: the coverage ORPHANs (§11 lines 3,
5, 6, 9, 10) and UNHOMED §12.3 witnesses (11 Permanent root, 12 Effects) concentrate
at Stage C's H-I validator list and the Stage C/D witness matrices — the purity /
port-agreement / sequestered-key / permanent-root / effects surface Stage B deferred to
Stage C but Stage C under-delivers (X1, X2).
