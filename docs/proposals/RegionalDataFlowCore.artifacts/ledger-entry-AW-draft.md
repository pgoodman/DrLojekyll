# §20(AW) — DRAFT ledger entry

DRAFT for KeyedInstances.md §20(AW). The owner decides whether/where it lands;
KeyedInstances.md is UNTOUCHED (its ledger stops at (AV)). House idiom per (AU)/
(AV): dense, past-tense, every claim anchored, a NEXT line at the tail. Baselines
are session-local — re-snapshot from the tip at next session open.

---

(AW) REGIONAL-DATAFLOW-CORE EPOCH OPENED — A DESIGN-GROUNDING TURN
    (2026-08-02, docs-only, tip f0c913e0). The owner's RegionalDataFlowCore.md
    (§20(AV)'s successor candidate: request edges + typed identity replacing the
    `-demand`/keyed-instance machinery — F1 two-demand-authorities, F2 physical-
    row-equality-as-identity, F3 demand-vs-cursor two-lifetimes, F4 no-request-
    owner) was GROUNDED, DIFFED, CRITIQUED, and TEST-DESIGNED across a six-phase
    fleet session; NO production code, NO goldens touched, NO commits. All
    artifacts in docs/proposals/RegionalDataFlowCore.artifacts/ (INDEX.md is the
    map). WHAT LANDED (as design artifacts): (1) THE PSEUDOCODE re-verified +
    EXTENDED (regional-arch-pseudocode.md, nine-extractor fleet): §4b the runtime
    epoch path (entry → ingest folds → eager web → stratum phases → publication
    tail → commit/compaction, placing LowerSubgraphInstances OUTSIDE all strata at
    the epoch tail) and §4c the generated cursor lifetime were newly derived,
    exposing F3 at emission grain (the RootRequestLease vs raw `uint32_t pos` are
    STRUCTURALLY UNCONNECTED APIs; the CLAUDE.md cursor contract rules out the
    CompactDead()-renumber hazard by CONVENTION, never by type). (2) FIVE STAGE
    DIFFS authored with golden-master exit gates (A typed identity + explicit
    projections; I0 the reference relational interpreter INSERTED per the review's
    Concern 1; B the canonical planning regional program; C request edges replace
    forcing — THE cutover, deleting ApplyDemandTransform/FabricateDemand*/
    Guard-Annotation/RecognizedSubgraph/ResolveLiveRecognition/BuildSubgraphInstance-
    Ops/BuildQueryInjector*/the three flags; D deep forest + instance-qualified
    local recursion). (3) ADVERSARIALLY CRITIQUED (opus panel, ≥4 lenses/stage +
    a refuter): 59 findings survived / 23 refuted; FIVE BLOCKING across four stages
    (A: T-conf-1/T-conf-2 cyclic-graph contract soundness — InferConservativeRow-
    Contracts is a single depth-ordered pass over a genuinely-cyclic contract graph,
    hard-rejecting the recursive corpus; I0: A-corr-1 the bound-query probe-
    enumeration contract — the flagship demand gate compares EMPTY content; C:
    corr-1 the force.dr `@first` byte-identity over-claim; D: T-oracle-4 the
    §0.1-abort-vs-E3-silent-full-materialize contradiction — the SAME decision as
    the review's Concern 2). Stage B is the ONLY clean stage. (4) COVERAGE AUDITED
    (phase3-coverage-audit.md): 5 §11 ORPHAN validators (3/5/6/9/10, concentrated
    at Stage C's H-I where the purity/port-agreement/sequestered-key/permanent-
    root/effects surface Stage B deferred under-delivers — X1/X2), 2 §12.3 UNHOMED
    witnesses (Permanent root, Effects), 3 §15 caveated invariants, X3
    (V-DEMAND-SUPPORT-DERIVED named-in-C-undefined-in-A), X4 (bound-query count
    drift), ZERO §14 ordering violations (every replacement lands ≤ its deletion).
    (5) FOUR DESIRED-STATE SURFACES authored + critiqued (the .df/.contract after A,
    the new -region-out grammar after B, the .rel lifecycle ops after C, the
    generated header after C), yielding owner-facing ADJUDICATION INPUTS AI-1..AI-8
    (the in-.df-vs-separate-sink role rendering; the G1/G2/G3 grammar; the 29→36
    census re-bless; tc's .rel under Variant A/B; the differential= flag; the
    PIVOTAL owner-identity-vs-refcount request-edge schema; A2-A5; the Stage-B
    ADJ-2/ADJ-3). (6) THE TEST MATRIX designed AND EMPIRICALLY VERIFIED at tip
    (test-matrix-proposal.md): a gate×mode COVERING ARRAY (13 configs, complete
    strength-2 CA over the 8 PassPolicy gates — the 4 golden modes flip gates in
    correlated blocks, leaving S-always-on and intra-df-body mixed pairs uncovered)
    + a FEATURE-MIXING corpus (14 crossings). VERIFIED: kvindex_1's canon-off
    compile/reject split (carve-out A, the sole compilation mode-split); the
    ident-off × -demand probe SAFE (recognizer tolerates raw_seed un-folded); the
    S=0 space SAFE on witnesses. PREDICTION-FAILED (four): PF-1/PF-2 the df.dfe
    SIGABRT — `-opt-disable=df.dfe` (canon/cse ON) aborts Stratify.cpp:420 on
    deadflowelimination_1/2/4 + recursion, root-caused to an UNSTATED, UNOWNED
    pipeline postcondition (canonicalization DESTROYS the merge/io-seam structures,
    DFE's underivable-cycle collection is the janitor that DELETES the residue;
    canon-ON+dfe-OFF = demolition without the janitor; the dead cycles are
    USER-AUTHORED LEGAL programs denoting empty relations, NOT corruption — the F1
    disease class recurring at the OPTIMIZATION layer, mis-binned on the optional
    side of the Optimize.cpp:884 required-hygiene line); PF-3 the demand-body reject
    vocabulary splits THREE ways (demand-SINK / R-MAT / R-BODYWALK, `!`↔`@never`
    indistinguishable by message); PF-4 `@barrier`/`:-` IS a demand fence today
    (the SIP walk does not traverse a barrier-staged join chain). (7) THE NECESSITY
    AUDIT returned a SMALLER-NOT-LARGER verdict: NO legacy-two-authority and NO
    zero-consumer mechanism survived the lens; all 5 simplification candidates are
    deferrals/reductions folding into stage diffs (top: strip the POPULATED
    RowContract.derivation_support, keep the F4 static_assert domain types; defer
    V-OWNERSHIP-ACYCLIC to its Stage-C birth). THE OWNER-DECISION QUEUE
    (owner-adjudication-brief.md, three tiers): T1 ratify A→I0→B→C→D +
    interpreter-before-cutover + tagged-binary oracle, the 5 blocking findings, and
    Concern 2's inadmissible-extraction semantics (Variant A uniform full-
    materialization vs Variant B retain-recursion-rejects — the MASTER decision the
    §6-vs-§11 bound-query-routing conflict blocks, binding Stage C H-J, Stage D
    T-oracle-4, AI-4, and the four clean diagnostics' fate); T2 the Stage-C H-I
    validator cluster + AI-1..8 + O-A1/2/3 + E-A2 + the R-DIFF oracle case + V-PI/
    V-CW; T3 the df.dfe hygiene/optimization split + Stratify-validator promotion
    (a standalone pre-Stage-A cleanup, effort S, with a FINDINGS.md F-record + an
    audit prompt to re-ask "whose absence breaks whom" of every gate), the
    PassPolicy.h stale 4-vs-5-gate prose fix, the 9-case pre-Stage-A landing set,
    and the necessity deferrals. SIX SESSION ERRATA recorded (the demand_diff_pub_1
    /_witness name drift; the residual bound-query count beyond X4; pseudocode §6's
    stale distinct-nodes Stage-A hunk; the df-stage-a §4.2 views=19→20 census error;
    the X1/X2/X3 validator-handoff mismatch; the PassPolicy.h prose). §19 acceptance
    for the design-grounding charter MET (verified pseudocode + diffs + critiques +
    desired states + test matrix + this draft + the brief + the updated prompt).
    NEXT: owner walks owner-adjudication-brief.md; then EITHER the pre-Stage-A
    cleanup slice (df.dfe split + PassPolicy prose + the 9 matrix cases + apply the
    Phase-3 amendments to the blocked stage docs) OR — if the owner ratifies the
    fast path — directly the Stage A implementation slice (typed identity +
    Member/Distinct enum-in-identity + SCC-aware InferConservativeRowContracts +
    explicit aggregate input key + lint→contract-validation), per next-session-
    prompt.md. STANDING RULE: the blocked stage docs MUST have their Phase-3
    amendments applied before any implementation of that stage begins.
</content>

---

# §20(AX) — DRAFT ledger entry (session 2, 2026-08-02/03)

(AX) ADJUDICATION + BRANCH A + STAGE A LANDED — THE FIRST PRODUCTION-CODE
    TURN OF THE EPOCH (2026-08-02/03, working-tree only, base f0c913e0,
    NOTHING COMMITTED). All 23 brief decisions adjudicated tier-by-tier
    (owner-adjudication-record.md is the authority): fast path ratified
    with D2.6 DEFERRED (the concurrency requirement recorded as the
    deciding fact; Stage-C header authoring paused), D1.4 = drop force.dr's
    query-time forcing, D3.4 candidate-3 = FLAT-KEY ratified. BRANCH A:
    F26 landed (the df.dfe hygiene/optimization split — CollectDeadCycles
    REQUIRED beside RemoveUnusedViews, TaintDerivedFromInput factored,
    Stratify's assert promoted to always-on V-SCC-SEAM; dfe-off full-corpus
    sweep 190/190 clean, covering-array carve-out B GONE, the 4 SIGABRT
    cases now directed witnesses); the whose-absence-breaks-whom audit
    (8 gates x 190 compile-only) leaves carve-out A as the ONLY coupling;
    F23 promoted (exit-139 no longer reproduces; JOIN-taint null guard;
    pinned as product_in_scc_diff_1); PassPolicy.h 4->5 prose fixed
    (Errata-6); Errata-2 resolved (49/181 bound-query cases at HEAD);
    Errata-5 applied (V-PORT-AGREE/V-PURE-REGION named at stage-c H-I,
    lines-3/6 softening stated, X3 miscite reconciled). THE 8-CASE D3.3
    LANDING SET LANDED (suite 181->190) and caught TWO REAL BUGS on day
    one: F27 FIXED (dataflow-opt phantom re-publish — 1-arm-MERGE
    elimination detached a monotone tap from its table, eager descent
    published UNGATED; fix = dedup table per table-less monotone stream
    insert in FillDataModel; zero golden fallout) and F28 FIXED (oracle
    modeled @invertible KV merges last-writer; now folds by declared
    algebra; zero existing-golden churn), plus F29 RECORDED (idx_43 scope
    bug, bound-query-over-KV codegen, repro in the record). demand_diff_pub_1
    proved the R-DIFF pub arm LIVE (answers retract through the
    @differential tap under standing demand; nested==flat all 4 modes) —
    the stage-c E1 oracle gap is closed. STAGE A LANDED under the standing
    method (anchors fleet-re-verified — BROKEN-2: IdentifyInductions
    membership is PARTIAL, resolved by STRATUM-based cycle membership
    post-Stratify; BROKEN-3: the two-site stamping story extended to a
    ~48-mint-site audit with kMember default; amendments applied as dated
    diffs; 5-lens refute-verified panel: 18 findings -> 4 surviving, all
    applied with fresh-dump grounding): lib/DataFlow/Identity.h typed ids
    + the F4 static_assert battery (ctest IdentityTypes), ProjectionRole
    folded into Equals ONLY (the Hash fold empirically perturbs hash-derived
    Rel/CF tie-breaks for zero CSE benefit — CSE buckets by cse_color,
    Equals decides; the T-oracle-1 "Hash differs" unit is unrealizable),
    InferConservativeRowContracts (two-phase pure graph function: Phase-1
    AllFields on multi-view strata, Phase-2 acyclic flat-key transfer,
    QueryImpl::row_contracts, post-Stratify slot), V-CONTRACT-CENSUS/
    V-MEMBERKEY-REALIZED/V-AGG-INPUT-KEY always-on + V-NO-COLLAPSE
    BELT-ONLY (the amended hard-abort false-fired on valid programs —
    AllFields keys make benign drops indistinguishable; E-A2's fallback
    taken), LintAggregateProjection DELETED (agg_distinct_1 zero warnings,
    stdout untouched), -contract-out sink + .contract irgold surface with
    THREE blessed goldens (agg_distinct_1, demand_tc_witness, join_1).
    All three H-A9 witnesses proven UNCONSTRUCTIBLE with evidence (role-OFF
    full-corpus sweep = zero .df diffs; refusal load-bearing only on
    dead-cycle shapes; AllFields floor makes V-AGG-INPUT-KEY unfireable
    from surface programs) — the stage doc's own fallback branches, no
    fabricated witnesses. PREDICT-THEN-VERIFY: keys/input-keys/census
    matched the desired states EXACTLY; the role= tally diverged (the
    structural stamp does not survive proxying — sole surviving kDistinct
    = a renaming clause head feeding a JOIN) and was adjudicated WRONG-
    PREDICTION, docs reconciled, produced dumps blessed. Owner directives
    landed mid-session: DOT digraph twins (dataflow -dot-out now renders
    role=/KEY(...) + cluster_stratum_<id> per multi-view stratum; Stage-B
    -region-out gains a cluster_region DOT twin; advisory, never goldened);
    two design conversations recorded as re-brief inputs (Mobius/DD: the
    counter split = DD's trip axis quotiented to a 2-point order, the
    claim matrices = hand-rolled inclusion-exclusion; shared arrangements:
    D2.6 IS the reader-handle question, compaction needs frontiers once
    leases outlive epochs, logical-origin provenance on models = Stage-B
    planning input). Exit state: SUITE: PASS (190), ctest 7/7, zero
    existing-golden churn all session, goldens changed ONLY by reviewed
    bless of NEW surfaces. NEXT: owner commits (or splits) the tree;
    I0 (the ranked-#2 interpreter, .probes contract ratified) opens; the
    Stage-C re-brief queue = D2.6 concurrency requirement + the §6-vs-§11
    routing rule + the arrangement/provenance direction.
