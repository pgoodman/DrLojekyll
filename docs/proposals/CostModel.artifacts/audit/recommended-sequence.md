# Recommended CostModel Sequence

The sequence deliberately front-loads falsification. Later stages should not
start merely because earlier code compiles.

## Stage 0: Restore Trustworthy Preconditions

1. Remove the release-only `catch (...)` continuation in `Query::Build`.
2. Make the mono calibration reproducible from tracked files.
3. Rename the recognizer-off mono artifacts and generate the current on form.
4. Instrument and record Query canonicalization exit reasons for the witness.
5. Run build and test gates sequentially. Never test a tree while rebuilding it.

Exit criterion: one documented command reproduces the measured mono counter
table from a clean tree, or the old table is explicitly withdrawn.

## Stage 1: Adjudicate the Product Contract

Write a short binding decision that answers:

1. Is Rel intended to become a complete physical plan, remain a partial
   differential scheduler, or be retired after its algorithms are extracted?
2. Is slice 1 a static estimator or an exact concrete-trace cost interpreter?
3. What concrete representation drives epochs, messages, queries, and prior
   state?
4. Which counter fields are exact, estimated, measured-only, or unsupported?
5. How does a generated process return measured counters to the harness?
6. Is the cost analyzer a library, a compiler mode, or a standalone executable?
7. What immutable pointer-free artifact crosses Query/Rel/ControlFlow ownership?

Recommended answer for the first slice:

```text
product:
  library analysis over CostProgramSnapshot + ConcreteScenarioTrace

supported domain:
  monotone, non-recursive identity-join witness

prediction:
  exact idx_adds delta for the redundant intermediate materialization
  other fields unsupported until separately grounded

measurement:
  dedicated generated calibration executable

orchestration:
  test/build harness compares prediction report with measured report
```

Exit criterion: input/output schemas and unsupported behavior are explicit.
No `close()` placeholder remains.

## Stage 2: Create the Cost Snapshot Boundary

Unless Stage 1 deliberately completes the Rel boundary first, do not expose
`DRFlowGraph` or parse the diagnostic dump as the core API. Create a
value-semantic snapshot with domain types:

```text
CostProgramSnapshot:
  views: stable semantic ids + node kinds + edges
  tables: stable ids + index definitions + logical producers
  operations: normalized cost-relevant events
  access_plans: explicit for every supported operation
  mappings: view -> table/op identities
```

Construct it while Query, Rel, and Program objects are alive. Record the source
representation for each fact so mismatches remain visible. Return it as part of
a required build result. If ordinary compilation does not need it, use a
required collector protocol with `NullCostCollector`, not a nullable pointer.

Exit criterion: snapshot tests prove deterministic output and no pointer values
or presentation-only node numbers escape.

## Stage 3: Land One Exact Vertical Slice

Implement only the mono identity-join delta:

```text
input:
  current on/off CostProgramSnapshots
  concrete K probes with F distinct neighbors per key

prediction:
  redundant intermediate receives F new distinct rows per probe
  delta idx_adds = F * K * relevant_index_multiplicity

measurement:
  generated on/off executables run the same trace
  compare exact integer delta
```

Add duplicate, empty, repeated-demand, and index-multiplicity negatives. If the
formula needs more state, extend the domain types rather than adding defaults.

Exit criterion: exact held-out cases pass and the negative witness disproves the
naive one-cardinality rule.

## Stage 4: Decide Whether a General Simulator Is Still Worth Building

Use the slice to estimate implementation cost and coverage. Then choose:

- extend an exact trace interpreter op family by op family;
- build a separate statistical estimator with honest error bars;
- use direct measured counter A/B tests without a static simulator;
- stop because the simulator duplicates too much runtime behavior.

This is a real adjudication point. Continuing is not the default.

## Stage 5: Expand by Evidence, Not Vocabulary

Suggested order if the project continues:

1. monotone ingest and materialization;
2. point probes and section walks;
3. eager joins with explicit index facts;
4. duplicate/set semantics;
5. differential single-table epochs;
6. recursive rounds;
7. aggregate and keyed-instance state;
8. estimated skew/cache models, if still useful.

Each family lands with a hand prediction, a measured positive case, a negative
case, and an unsupported case. Do not specify all op formulas in prose years
ahead of executable witnesses.

## Documentation Cutover

After Stage 1, rewrite `CostModel.md` as a current contract:

- demote the old symbolic design from "Binding" to historical proposal;
- retain structural lessons that remain code-true;
- delete unsupported exactness and universal scenario claims;
- name the implemented slice and its exclusions;
- link every measured law to a tracked reproduction command.

Do not preserve symbolic and numeric architectures as parallel supported modes
unless there is a concrete caller for both.
