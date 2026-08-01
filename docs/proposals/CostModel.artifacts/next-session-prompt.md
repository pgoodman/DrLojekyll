# Next Session: CostModel — the predictor fork, or extend measurement

Continue work in the Dr. Lojekyll repository. The governing discipline is
reality grounding through independent tests, differential execution, reviewed
goldens, explicit negative witnesses, and reproducible measurements. A numeric
result is not evidence merely because it is numeric.

The prior session (2026-07-31) adjudicated the product boundary and landed the
MEASUREMENT half of the identity-join slice. Do not re-litigate what it settled;
read its evidence first and continue from the one unresolved fork.

## Start here (mandatory reading, in order)

1. `audit/README.md` — the living audit index (now carries a session-decisions
   section at the top).
2. `audit/rel-endstate-decision.md` — Work Order 2 result: the op-family
   authority table and the A/B/C fork. **This is where the one owner decision
   lives.**
3. `audit/costmodel-contract-decision.md` — Work Orders 3 + 6: the binding
   slice-1 contract and the general-simulator reassessment.
4. `audit/calibration-slice.md` — the landed measurement slice.
5. `audit/verification-record-2.md` — what ran, what did not.
6. The rest of `audit/` for anything you touch.

Then re-verify anchors against current code; the audit is a log, not an
authority by assertion.

## Operating rules (unchanged)

1. Record `git status`, branch, and `git log -1` at start.
2. Greenfield: no compatibility shims, parallel old/new modes, fallback paths,
   warning-and-continue branches, or test-only production behavior.
3. Injected writers/collectors/policies are required protocols with no-op
   implementations, never nullable.
4. Named domain types for view/table/column identities, counts, signs, strata,
   and prediction classifications — never a raw `unsigned`/`float`/pointer as a
   new architectural boundary.
5. Never bless a golden to make a test pass. Derive the expectation independently
   first.
6. Build and test a tree sequentially; never test a tree while it rebuilds.
7. Do not commit or push unless the owner asks. Identity
   `Peter Goodman <peter.goodman@gmail.com>`; keep tool/model/provider details
   out of commits and artifacts.

## Settled (do not redo without new evidence)

- Trust prerequisites are repaired: the `Query::Build` catch-all is deleted; the
  canonicalization iteration cap is a loud abort (measured 0/519 activations);
  the mono artifacts are config-labelled with a tracked generator; the
  identity-join on/off delta is verified in all four modes; full OptDiff and
  Debug/Release CTest pass.
- Slice-1 product = the **runtime measurement runner** (`calib/`), reproducing
  `idx_adds(OFF) − idx_adds(ON) == F·min(N,K)` by exact integer equality, with
  the duplicate-row, empty-tail, held-out, and idempotence negatives.
- Rel is the sole authority for the differential scheduler; it MODELS +
  cross-checks the eager/join-emission families. Recommended CostModel boundary
  is direction **B** (snapshot from Query+Rel+Program while alive), not A, unless
  the owner intends Rel to become the universal IR.

## Design inputs to reconcile (added since the audit)

Two owner-authored proposals landed on `keyed-instances` and bear on the Rel
fork below; read them before touching the snapshot:

- `docs/proposals/BoundedObservation.md` — bounded-consumption operators
  (`ONLY`/`CHOOSE_ONE`/`EXISTS`). Read + spot-verified this session; it requires
  Rel to author a query-read physical descriptor (a slice of direction A) and
  restates the exact cost/correctness boundary the CostModel contract uses. See
  the cross-reference in `audit/rel-endstate-decision.md`.
- `docs/proposals/InstanceFlow.md` — NOT yet read; a further Rel/keyed-instance
  design input. Read it before ratifying the Rel fork.

## The one decision that needs the owner

Ratify the Rel end state for CostModel purposes:

- **B (recommended):** proceed with a multi-representation `CostProgramSnapshot`
  built at the owner boundary; label it a partial-scheduler snapshot.
- **A (the fork):** if Rel is to become a value-semantic one-way physical plan
  on its own schedule, design the snapshot to converge on that plan type instead.

Stop and present the tradeoff if this is not already answered; do not pick A
because prose calls Rel the sole authority, nor B because it is the current shape.

## Candidate work, once the fork is answered

Pick ONE; keep it a falsifiable vertical slice.

1. **Static-predictor half of the mono law (needs B ratified).** Build a
   pointer-free `CostProgramSnapshot` while Query/Rel/Program are alive, with
   named domain types and per-fact provenance and a required `NullCostCollector`.
   Predict `idx_adds` for the mono witness from the snapshot + trace and compare
   to the measured runner on held-out (N,K,F). Add the `PredictionKind =
   Exact | Estimated | Unsupported` report type; prove an unsupported field is a
   distinct value, never `0` (Stage-3 #8). Abandon the static predictor — do not
   tolerance-fudge — if the field turns out to depend on hash occupancy /
   insertion order / prior state not in the trace, or needs an analyst-supplied
   selectivity/fanout number (abandon-evidence in the contract decision).
2. **Extend the measurement runner (needs nothing).** Add the index-multiplicity
   case (a witness whose queried relation carries ≥2 indexes, so `idx_adds`
   multiplicity > 1), then a second op family (monotone ingest/materialization).
   Each lands with a hand-derived law, a measured positive, a negative, and a
   stated boundary.

## Broader reality gates (independent of the fork; do not fold into the slice)

- Repair the inert parser round-trip fuzz target; delete or implement the missing
  `BackendFuzzer.cpp` before claiming fuzz coverage.
- Add a unit truth-table for pass selection and a pairwise covering array over
  live passes; keep the four coarse OptDiff modes as the readable baseline.
- Strengthen PointsTo from a nonempty smoke test to an exact/independently-checked
  semantic test.
- The `df.sink` dead pass registration and the duplicate `Compare.cpp` CMake
  entry remain (comment-code-drift.md targets 7, 8); delete when convenient.
- The investigation-coordinator flagship (`agent-harness-reality.md`) is the
  feature-composition test; keep external effects outside Datalog evaluation.

## Stop conditions (unchanged; write an adjudication finding instead of improvising)

- the scenario summary cannot drive the actual generated program;
- a supposedly-exact field depends on concrete values/order/prior-state/hash/
  capacity not in the trace;
- the snapshot would need to retain Query/Rel/ControlFlow pointers after build;
- a Rel fact has two production authors and no clear authority;
- a finite scenario family is used to claim universal/asymptotic behavior;
- the only way to pass is a default/nullable collaborator, warning-and-continue,
  broad exception handler, or unexplained tolerance;
- a golden expectation comes solely from the implementation under test;
- the Rel end-state fork still needs the owner.

## Deliverables

Updated audit files with evidence and dispositions; the Rel decision (or the
owner-adjudication brief if still open); the chosen vertical slice with tests
IF prerequisites were met; a verification record of commands run and gates not
run; and an updated version of this prompt pointing at the next unresolved
decision. Success is a smaller set of claims with stronger independent evidence,
plus one more exact supported behavior if the architecture can honestly provide
it.
