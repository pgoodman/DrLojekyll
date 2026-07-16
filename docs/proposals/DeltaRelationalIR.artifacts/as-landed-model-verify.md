# Model verification (opus)

VERDICT: The AS-LANDED MODEL is substantially accurate on the R2/DR-IR structural spine, the Stratify/CSE/pragma (PART A) claims, the B0 feature-gap pre-pass, DeriveDRStrata's ported lift, and the dead-but-alive/decorative-payload characterizations — all verified against code. HOWEVER its most consequential framing is WRONG by exactly one commit: it is written against ecfe674 and treats V-PRED-XCHECK as a PENDING working-tree-only diff, but the real committed HEAD (e3559592, docs-recorded by 8fec79d; working tree clean) has V-PRED-XCHECK fully landed. This falsifies a cluster of the model's central [AUTHORITY]/[STRENGTH] annotations: (1) EmitChainStep's negate gate is cross-checked against DR NegateGatePred (Stratum.cpp:399), not 'SOLE authority / DR copy UNREAD' — and DELTA 4's 'SITE 1 not wired' is wrong; (2) the EmitJoinFire fixpoint matrix is cross-checked against the stored DR arm spine (Stratum.cpp:897-925), not 'exists twice, no cross-check'; (3) V-CLAIM-GATE's ClaimGate datum is consumed/cross-checked by EmitClaimDrain (Stratum.cpp:553-557), not 'unconsumed at HEAD'. Net: the F1 duplication that the model presents as an open committed-state hazard is, at the real HEAD, a checked equality at all three primary sites. Secondary defects: an imprecise crossover read-set enumeration in B.V.a, systematic ~7-150-line cite drift throughout PART B (structural claims sound, anchors stale), and two minor doc-path slips. The R3c-ii/R3d and DIFF-OPEN-QUESTIONS sections are forward-looking proposals whose verifiable cite-groundings (StateCell API, Build.cpp/runall.sh anchors, corpus files) I spot-checked and found accurate; they need updating only insofar as they inherit the ecfe674 baseline and the (now-false) 'V-PRED-XCHECK is pending, SITE 1 unwired' premises.

## [CRITICAL] The entire committed subject is mis-dated one epoch behind. The model and PART C assert HEAD=ecfe674 with the V-PRED-XCHECK diff PENDING/working-tree-only ('Confirmed by grep: HEAD:Stratum.cpp has ZERO V-PRED-XCHECK; the strings exist ONLY in the working-tree M lib/ControlFlow/Build/Stratum.cpp'). This is false against the actual repo.

EVIDENCE: Actual HEAD is 8fec79d (docs-only) atop e3559592, whose commit message is 'predicate cross-check: V-PRED-XCHECK ties the DR-IR model to the surviving Emit* templates (big-review finding 1)'. ecfe674 is its PARENT. `git show HEAD:lib/ControlFlow/Build/Stratum.cpp | grep -c V-PRED-XCHECK` = 8 (model claims 0). `git status --short` = clean: there is NO modified Stratum.cpp in the working tree. The PENDING framing is inverted — V-PRED-XCHECK is COMMITTED, not in-flight.

RECOMMENDATION: Re-anchor the AS-LANDED subject to e3559592/8fec79d. The three V-PRED-XCHECK deltas (helpers, EmitClaimDrain gate check, EmitJoinFire matrix check) are committed code, not a pending diff. Every downstream annotation that assumes an un-cross-checked emitter must be re-evaluated (see findings below).

## [CRITICAL] B.VI claim that EmitChainStep 'OWNS the F18 negate-gate rule at :388 ... Duplicated in DR by NegateGatePred (DR.cpp:346) — DR copy UNREAD. If someone reintroduces the sign-keyed read HERE, every DR validator stays green (F1)' is false at real HEAD. Correspondingly, DELTA 4's claim that 'SITE 1 (EmitChainStep negate gate) is NOT wired ... remains the emitter's SOLE authority' is also false.

EVIDENCE: Stratum.cpp:399-401: `PredXCheck(NegateGatePred(in_fixpoint ? Ctx::kFixpoint : Ctx::kSeed, NegateHint::kNormal), negate_pred, "EmitChainStep negate gate")`. The DR model's NegateGatePred IS read and cross-checked against the emitter's `in_fixpoint ? kInNew : kInI` choice; a reintroduced sign-keyed read here diverges and aborts. Comment at :392-398 documents this as 'V-PRED-XCHECK (finding 1) — Site 1'.

RECOMMENDATION: Correct B.VI: EmitChainStep is cross-checked. Caveat worth preserving (the code comment states it, :395-398): the check is against the model FUNCTION NegateGatePred, not a threaded per-arm STORED gate node, because EmitChainStep runs deep inside EmitSeedLoop with no correlated DR arm. So it is not a stored-payload cross-check, but it is emphatically not 'UNREAD / SOLE authority'.

## [HIGH] B.VI claim 'THE EXACTLY-ONCE MATRIX EXISTS TWICE, no cross-check (at committed HEAD)' for the EmitJoinFire fixpoint matrix is false. DELTA 3 correctly describes this wiring but files it as PENDING.

EVIDENCE: Stratum.cpp:897-925: when `fire_op != nullptr`, EmitJoinFire finds the matching DR arm by (delta_pos, sign), collects the stored per-table Pred from the arm's plan-spine kAccess nodes, and for each scanned side calls `PredXCheck(it->second, emitted_pred, "EmitJoinFire matrix")`, aborting on divergence or a missing side. It is threaded LIVE from LowerRoundBody at :1636 (`EmitJoinFire(..., op)`). The FixpointSamePred/kInNew-populated DR matrix and the emitter matrix (Stratum.cpp:793-800) are now cross-checked for FIXPOINT_FIRE.

RECOMMENDATION: Correct B.VI and the FIXPOINT_FIRE row of B.II: the matrix duplication is a CHECKED equality at HEAD, not an unchecked twice-existing datum.

## [HIGH] The V-CLAIM-GATE [STRENGTH: self-satisfied] justification 'F3/F2: gate unconsumed at HEAD' is false. The ClaimGate IS consumed by the emitter at HEAD.

EVIDENCE: EmitClaimDrain (Stratum.cpp:546-557) takes `drain_op` and, when non-null, computes `want = is_del ? kDelGateCnrNonPositive : kAddGateTotalPositive` and calls `PredXCheckFail` if `drain_op->claim_gate != want`. Threaded live from the acyclic path (:1510-1512 region) and the in-round path (:1607-1617). The DR.cpp V-CLAIM-GATE validator (DR.cpp:1974-1982) remains self-satisfied on its own, but the claim-gate DATUM is no longer unconsumed — a reintroduced sign-flipped gate aborts at the emitter.

RECOMMENDATION: The validator body's tautology characterization is fair, but drop the 'gate unconsumed at HEAD' rationale — the F2 ClaimGate now has a real emitter-side consumer/cross-check.

## [MEDIUM] B.V.a's per-family 'reads' summary for crossovers is imprecise: '(3) CROSSOVERS ... [reads crossover_sign, negate only]'. LowerCrossoverArm reads several stored table pointers off the op.

EVIDENCE: Stratum.cpp LowerCrossoverArm (from ~:1097): reads op.negate, op.negate_table, op.negated_table, op.pred_table, op.pred_view, plus is_add (sign). Not just crossover_sign+negate. (These are Query-recoverable, so DUPLICATED(F1) still holds in spirit, but the enumerated read-set is wrong.)

RECOMMENDATION: Amend the B.V.a crossover read-set to include negate_table/negated_table/pred_table/pred_view. The DUPLICATED(F1) label is defensible (payload is convenience-cached, re-derivable), but the specific 'negate only' enumeration is inaccurate.

## [LOW] Systematic line-number drift throughout PART B. Every DR.cpp validator and Stratum.cpp BuildStratumPhases anchor is cited ~7-150 lines low because the model was written against ecfe674 (pre V-PRED-XCHECK insertion).

EVIDENCE: BuildStratumPhases cited :1642, actual :1794. ComputeRecursiveSCCs :1647 vs :1799. BuildDRInventory :1674 vs :1826. DeriveDRStrata :1681 vs :1833; ValidateDRInventory :1688 vs :1840; ValidateDROps :1689 vs :1841; LinearizeAndValidateDRFlow :1690 vs :1842; LowerDRFlow :1799 vs :1951; LowerDRRounds :1811 vs :1963. V-CLAIM-GATE :1967 vs :1974. The structural ordering claims are all correct; only the cites are stale.

RECOMMENDATION: Re-cite against e3559592. Structural/ordering claims need no change; only the file:line anchors.

## [LOW] Two document-path citation slips (in the prompt's ground-truth pointers, not the model body).

EVIDENCE: The lens is at docs/proposals/DeltaRelationalIR.artifacts/big-review-2026-07-16.md (prompt gave 'DeltaRelationalIR.artifacts/big-review-2026-07-16.md' without the docs/proposals/ prefix). Format.cpp round-trip cited :132-144; actual @invertible.. @idempotent block is :133-145. Both are trivially locatable.

RECOMMENDATION: Prefix the artifacts path with docs/proposals/; bump the Format.cpp range by one line.

## [LOW] Verified-correct claims (recorded so the review is not read as wholesale rejection). The following load-bearing claims check out exactly.

EVIDENCE: Stratify A1/A2 rejects (Stratify.cpp:270-341, agg at :327, kv at :334, one-diag-per-agg via break). CSE color-refinement: InitColor:121, StepColor:141, Refine:241, F6 assert :154-155 (can-fail, asserts JOIN/SELECT/MERGE carry empty col-lists), co-bucket assert :320 (can-fail). Pragmas parse-and-store-only: Functor.cpp:315-360 conflict diag, Parse.cpp:1211-1229 accessors, Format.cpp round-trip, and ZERO consumers in DataFlow/ControlFlow/CodeGen (grep empty) — the [inert] label holds. B0 pre-pass: agg/kv/impure/on-cycle-@product via `!NumPivotColumns && CanReceiveDeletions && ViewSelfReachable` (Build.cpp:1040-1041, NOT InductionGroupId, F22 correct). B.III DeriveDRStrata sole-writer of *_stratum maps, owner_stratum ports TableOwnerStratum, product strict-ready_after acyclic fence (:1817-1826), WART operator[] pollution (:1780) all present. flow.tables written (DR.cpp:561) never read (F8e correct). PivotAssemble consumed by no lowering (dead, correct). V-LOOP witness genuinely implemented (DR.cpp:3030-3050, F3(b), correctly credited). StateCell R3c-ii cite-groundings accurate: Touched() const contract :333-336 (F7 fix landed), Invertible :85, Recompute :141, StateCellStore<Key,Algebra> typedefs :199-201; kGroupUpdate/kStateSeal/BuildGroupUpdateOps absent as expected for a forward diff; EffKind kStateFold/kStateEmit/kStateOld reserved (DR.h:81-83).

RECOMMENDATION: Retain these claims unchanged.

