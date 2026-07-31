# Quantity and Type Audit

The current proposal often uses a bare integer or floating expression for
quantities that have different algebra, provenance, and observability.

## Quantity Table

| Quantity | Current representation | Meaning | Valid operations | Risk |
|---|---|---|---|---|
| Query view id | `unsigned`/opaque handle | logical node identity | equality, lookup | confused with column/table ids |
| Table id | `unsigned`, often behind `TABLE *` | physical storage identity | equality, lookup | pointer lifetime and serialization |
| Column index | `unsigned` | position in one schema | bounded lookup | mixed across coordinate systems |
| Stratum/SCC/round | `unsigned` | schedule coordinates | order, successor | distinct coordinates share one type |
| Sign | `int` in `{-1,0,+1}` | removal, neutral marker, addition | branch/multiply by count | invalid values representable |
| Cardinality | proposed number | live distinct rows at a state boundary | add for disjoint union, constrained products | duplicates and time are omitted |
| Frontier size | proposed number | rows/events in one batch/round | add across disjoint events | not interchangeable with live rows |
| Fanout | proposed number | matches per key or distribution summary | multiply with probes | a mean hides skew |
| Selectivity | proposed scalar | surviving fraction | multiply compatible population | estimator, not measured fact |
| Closure size | sidecar number | final SCC rows | compare/add | does not determine per-round frontier trace |
| Operation count | `uint64_t` counter | dynamic calls/events | add in same counter family | counter names are not interchangeable |
| Runtime cost | wall-clock duration | elapsed time under a benchmark protocol | compare distributions | cannot be added to operation counts |

## BenchCounters Is Not One Quantity

The X-macro struct contains multiple counter families:

- exact call counts: `finds`, `idx_first`, `idx_adds`, `sort_calls`;
- data-dependent loop work: `probe_steps`, `idx_hops`, `sort_elems`,
  `netbatch_compares`;
- state-transition outcomes: `claims_*`, `stale_drops_*`,
  `commit_publishes`, `touch_appends`;
- capacity/lifecycle events: rehash and compaction fields;
- logical fold/member events.

They cannot all be predicted from one row cardinality rule. For example:

```text
same live row cardinality
  + different duplicate message rows
  -> same table size, different folds/finds

same key cardinality
  + different insertion order/hash collisions
  -> same finds, different probe_steps

same join output cardinality
  + different key-degree distribution
  -> potentially different probe chains and cache behavior
```

## Required Model Types

Any implementation should avoid returning or accepting raw counter maps and
raw numeric scenario values. A minimal typed shape is:

```text
ViewId, TableId, ColumnIndex, ScenarioId
LiveRowCount
InputEventCount
DistinctKeyCount
MatchCount
FixpointRoundCount
OperationCount

PredictionKind = Exact | Estimated | Unsupported
CounterPrediction = {
  counter: CounterName,
  kind: PredictionKind,
  value/range,
  assumptions,
  evidence rule
}
```

Do not use a `BenchCounters` instance as the predicted domain object. The
runtime struct is compile-time conditional and denotes observations. A
separate `PredictedCounters`/`CostReport` contract should make unsupported and
estimated fields impossible to confuse with exact predictions.

## Exact, Estimated, and Unsupported

The first slice should explicitly classify every field:

- **Exact** only when concrete inputs and prior state determine the event count
  under a proven op rule.
- **Estimated** when selectivity, fanout, closure, or retraction behavior is
  supplied as a summary. Report error, not an equality assertion.
- **Unsupported** when the scenario does not contain enough information. Never
  fill with zero or a default.

This classification is the difference between a useful experimental model and
a numeric document that merely appears precise.
