# Proposal: Desugar Zero-Arity Conditions into Unit Relations

## Summary

Zero-arity predicates ("conditions") are a parallel universe in the IR:
`QueryConditionImpl` is not a view, views carry three side-channel lists
(`sets_condition`, `positive_conditions`, `negative_conditions`), every
dataflow pass must remember to preserve/compare/transfer them, the
control-flow build has dedicated counter machinery (`TESTANDSET`,
`kConditionRefCount` globals, `kConditionTester` procedures), and the C++
backend emits global counters. This proposal desugars every condition into an
ordinary **unit relation**: a 1-arity, `bool`-typed relation whose only
possible row is `(true)`. Setting a condition is an INSERT of `(true)`;
testing it positively is a one-pivot equi-JOIN on the `bool` column; testing
it negatively is an ordinary NEGATE. All condition-specific bookkeeping in
dataflow optimization, control-flow build, and codegen is deleted wholesale.

The recommended test encoding is a **constant-pivot join, not a
cross-product**, so requirement 4's product concerns are sidestepped: no
implicit product is ever created, and the unimplemented product-removal path
(F14) is never entered. A control-flow-build fast path lowers unit-joins and
unit-negations as `CHECKTUPLE` existence gates instead of full `TABLEJOIN`
machinery.

## 1. Conditions today: complete touchpoint inventory

Fates: **D** = deleted, **G** = replaced by generic machinery, **S** = new
special case, **K** = kept.

### Parser (`lib/Parse`)

| Site | What it does | Fate |
|---|---|---|
| `Parser.cpp`, clause-head resolution ("If it's a zero-arity clause head...") | Auto-declares a `ParsedExport` for a zero-arity clause head | **K** (surface syntax unchanged; the export becomes the source of the `is_condition` bit) |
| `Clause.cpp`, error "should be factored out into a zero-argument predicate" | Rejects body groups that share no variables with the head, forcing users to introduce conditions | **K** initially; candidate for deletion once the keep-last-edge rule (§4) makes the shape expressible |
| `Parse/Format.cpp`, `if (decl.Arity())` | Prints zero-arity declarations without parens | **K** |

### Dataflow public API (`include/drlojekyll/DataFlow/Query.h`)

| Site | What it does | Fate |
|---|---|---|
| `QueryCondition` class (`Predicate`, `PositiveUsers`, `NegativeUsers`, `Setters`, `Depth`), `std::hash<QueryCondition>` | Public handle for `QueryConditionImpl` | **D** |
| `QueryView::SetCondition` / `PositiveConditions` / `NegativeConditions` | Per-view condition accessors | **D** |
| `Query::Conditions()` | Iterates all conditions | **D** |

### Dataflow impl (`lib/DataFlow/Query.h` + per-file)

| Site | What it does | Fate |
|---|---|---|
| `QueryConditionImpl` (weak `setters`/`positive_users`/`negative_users` lists, `IsTrivial`, consistency checks), `COND` alias, `QueryImpl::conditions` DefList, `QueryImpl::decl_to_condition` | The condition node itself | **D** |
| `QueryViewImpl::positive_conditions`/`negative_conditions` UseLists, `sets_condition` WeakUseRef | Per-view condition edges | **D** |
| `QueryViewImpl::{Copy,Transfer}TestedConditionsTo`, `DropTestedConditions`, `DropSetConditions`, `DropSetConditionsOfDeadView`, `TransferSetConditionTo`, `OrderConditions`, `IsConditional`, `EstimateDepth`/`GetDepth` over cond lists (`View.cpp`, ~70 sites) | Condition preservation choreography every rewrite must invoke | **D** |
| `View.cpp` `SubstituteAllUsesWith`: `ReplaceUsesWithIf` filter excluding `QueryConditionImpl` users | The F9 mechanism: user replacement must special-case condition weak lists, and the filter's precision determines whether `QueryIOImpl::receives`/`QueryRelationImpl::selects` get corrupted | **D** (no condition user lists ⇒ no filter) |
| `View.cpp` `CreateDependencyOnView`; call sites in `Tuple.cpp`, `Compare.cpp`, `Map.cpp`, `Negate.cpp`, `GuardWithOptimizedTuple` | Invents an *anonymous* condition when canonicalization severs the last data edge to a predecessor | **G** — replaced by the keep-last-edge rule (§4) |
| `Condition.cpp` (entire file) | COND destructor unlinking INCREMENT inserts, `IsTrivial`, `ExtractConditionsToTuples` | **D** |
| `Build.cpp` (dataflow): `AddConditionsToInsert`, the `cond_guard` pass-through TUPLE in `BuildClause` (both the arity>0 and zero-arity branches), `add_set_conditon`, the zero-arity INSERT (`INCREMENT`) path, `ExtractConditionsToTuples` call | Builds condition edges from parsed clauses | **G** — replaced by the desugaring in §2 |
| `Optimize.cpp`: `ShrinkConditions` + its call in the optimize driver; CSE/canonicalize interplay | Collapses chains of constant-input condition-setting TUPLEs | **D** — dead-flow + empty-relation folding subsume it |
| Per-node `Equals` guards comparing `positive_conditions != that->positive_conditions ...` (`Select.cpp`, `Join.cpp`, `Merge.cpp`, `Compare.cpp`, `Aggregate.cpp`) | CSE must refuse to merge views with differing conditions | **D** — condition relations are ordinary JOIN/NEGATE operands, compared structurally |
| `Join.cpp`: asserts joins carry no conditions; sets-condition transfer to guard tuples during join canonicalization | Condition preservation during join rewrites | **D**; add the condition-pivot preservation rule (§5, invariant 3) |
| `Merge.cpp`: condition guards in trivial-cycle / pull-through checks (plus unreachable sinking code) | Blocks rewrites across conditional merges | **D** |
| `DeadFlowElimination.cpp`: setter-less condition fixpoint (negative tests vacuously true, positive testers unsat); `sets_condition` liveness in `IsUsed` | Condition-aware dead-flow | **G** — empty-relation folding: a SELECT of a relation with no live INSERTs is unsat; a NEGATE whose negated view is unsat forwards unconditionally (see §7 risk 3) |
| `Differential.cpp`: views with conditions forced `can_produce_deletions` | Condition flips retract tuples | **G** — NEGATE/JOIN operands get differential support from the existing negation/join rules |
| `Induction.cpp`: skips tuples with conditions during merge clustering | Conditions opt out of induction grouping | **D** |
| `Insert.cpp`: `INCREMENT` kind naming | Zero-arity INSERT | **D** |
| `Format.cpp` (dot): `COND` cell rendering, purple condition edges, "relation modeled by a CONDition" skip, `query.Conditions()` loop | Dump formatting | **D** — condition relations print as ordinary `RELATION` nodes (annotate with `unit` from the `is_condition` bit) |
| `Query.cpp`: public accessor implementations | API plumbing | **D** |

### Control-flow IR (`include/drlojekyll/ControlFlow/Program.h`, `lib/ControlFlow`)

| Site | What it does | Fate |
|---|---|---|
| `VariableRole::kConditionRefCount` | Global counter role | **D** |
| `ProcedureKind::kConditionTester` (`^test:` in `Format.cpp`) | Procedures testing counter conjunctions | **D** |
| `ProgramTestAndSetRegion` (public class; impl in `Operation.cpp`; visitor arm) | `(A += D) == C` on globals | **K** — still used for `kInitGuard` (`Build/Procedure.cpp`); shrinks to that single use |
| `lib/ControlFlow/Program.h`: `DataVariableImpl::query_cond`, `ProgramImpl::cond_ref_counts` | Counter-variable bookkeeping | **D** |
| `Program.cpp`, `Data.cpp`: `kConditionRefCount` switch arms, condition-derived variable naming | Naming/roles | **D** |
| `Build/Build.cpp` `FillDataModel`: forces a table for any view with `SetCondition()`/`PositiveConditions()`/`NegativeConditions()` | Condition participants must be persisted | **G** — join/negation operands get tables from the ordinary data-model rules |
| `Build/Build.cpp` `InCondionalTests`: builds `kConditionTester` procedures as chained `TUPLECMP`s over counters (the F5/F8 site) | Positive/negative counter conjunctions | **D** |
| `Build/Build.cpp` top-down-checker wrapping: re-wraps a checker body in counter `TUPLECMP`s | Condition tests in `^find:` procedures | **D** — generic join/negate checkers do this structurally |
| `Build/Build.cpp` `ConditionVariable`, `Context::cond_checker_procs` (`Build.h`) | Counter allocation | **D** |
| `Build/Build.cpp` `EvaluateConditionAndNotify` + `BuildEagerUpdateCondAndNotify`: on a 0↔1 counter flip, scans every positive/negative user's table and pushes additions/retractions | The bespoke "unleash/rip back" machinery | **G** — inserting/retracting the `(true)` row drives the ordinary join and negation differential paths |
| `Build/Build.cpp` `BuildEagerInsertionRegionsImpl` / `BuildEagerRemovalRegionsImpl` condition blocks; "needs availability" predicate including conditions | Wraps successors in condition calls | **D** |
| `Build/Insert.cpp` `assert(!view.SetCondition())` | Guard | **D** |
| `Build/Join.cpp` (CF) | Lowers JOIN to `TABLEJOIN` | **S** — add the unit-join `CHECKTUPLE` fast path (§5) |

### Codegen (`lib/CodeGen/CPlusPlus/Database.cpp`)

| Site | What it does | Fate |
|---|---|---|
| `kConditionTester` proc-name arm | Names `^test:` procs | **D** |
| TESTANDSET emission (`(acc += disp) == cmp`), two sites | Counter updates | **K** for init guards |
| Global-variable emission loop | Emits `uint64_t` counter globals | **K** — condition counters vanish because no `kConditionRefCount` variables exist |

### Docs and tests

`docs/DataFlowIR.md` §Conditions, `docs/ControlFlowIR.md` (`kConditionTester`,
TESTANDSET rows), `docs/Architecture.md` — rewrite. OptDiff cases exercising
conditions today (17): `booleans`, `booleans_diff`, `cf16_6`,
`deadflowelimination_4`, `deadflowelimination_5`, `elim-cond-cycle-simple`,
`insert_4`, `merge_3`, `negate_3`, `optimize_3`, `optimize_5`,
`prove_constant`, `select_1`, `tuple_2`, `tuple_3`, `view_3`, `view_4`; plus
`data/examples/conditions_to_bools.dr` (currently a feature-gap file, F14
family).

## 2. The desugaring specification

For each condition declaration `c` (a zero-arity `ParsedExport`), the
dataflow builder creates one relation `⊥c` with a single `bool` column and
`is_condition = true`. Desugaring happens in `BuildClause`
(`lib/DataFlow/Build.cpp`), **not** in the parser: the AST keeps zero-arity
syntax (preserving the debug-build print→parse→print fixpoint and
diagnostics), and the parser's auto-export already identifies conditions
(`Arity() == 0 && IsExport()`).

The token value is a compiler-synthesized boolean-true constant column,
`QueryImpl::true_const`, a `QueryConstantImpl` with no `ParsedLiteral`
(the `QueryTagImpl` pattern: a literal-less CONST stream), typed
`TypeKind::kBoolean`.

### (a) Condition-defining clause `c : body.`

Build the body as usual into view `B` (columns `w0..wn`, the *witnesses*).
Emit a proxy TUPLE `P` forwarding `[true_const, B.w0]` and an INSERT into
`⊥c` whose `input_columns = [P.t]` and `attached_columns = [P.w]`:

    Before:                             After:

    RECEIVE ping[X]                     RECEIVE ping[X]   CONST true
       |                                     \             /
    TUPLE S[X] --sets--> COND c             TUPLE P [t=true, X]
                                                 |
                                            INSERT ⊥c (store: t; attached: X)
                                                 |
                                            RELATION ⊥c(bool)

`attached_columns` already exists on the `QueryViewImpl` base; INSERT simply
starts using it, with the semantics "read these columns to establish the
incoming-view data edge, store only `input_columns`" — a generic
projection-insert, not a condition special case. This is the one genuinely
new IR capability the design needs: the dataflow column model cannot
otherwise express "presence depends on B, but no data from B is stored"
(which is exactly the hole conditions currently fill — see the
`Clause.cpp` "should be factored out" error, which exists to forbid the
data-less shape everywhere else). Insertion of any body row does
`CHANGETUPLE (true) absent|unknown → present`; second and later rows fail
the transition and stop — the same idempotence that makes all bottom-up
addition terminate.

### (b) Positive test `head(..) : r(X), c.`

Attach the token to the user's flow and equi-join on it — **one pivot, not a
product**:

    Before:                             After:

    RECEIVE r[X]                        RECEIVE r[X]  CONST true
       |                                     \         /
    TUPLE U[X] --tests(+)--> COND c        TUPLE [X, t=true]      PUSH ⊥c[t]
       |                                          \                /
    MATERIALIZE head[X]                        JOIN (pivot: t) [t, X]
                                                    |
                                             MATERIALIZE head[X]

The pivot merges the user's constant-ref `true` column with `⊥c`'s stored
column. `⊥c`'s SELECT column is *not* marked constant in the IR, so the
pivot is an ordinary column-vs-constant join (like joining on `X = 5`) and
is not subject to useless-pivot elimination. Multiple conditions in one body
chain one join per condition (or one join with several unit sides).

### (c) Negative test `head(..) : r(X), !c.`

    TUPLE [X, t=true]  →  NEGATE (match: t against ⊥c; attached: X)  →  head

An ordinary `AND-NOT` with one matched column. `@never` tests map to the
existing `is_never` flag unchanged. `⊥c` gets `is_used_by_negation` and full
differential support from the existing negation rules.

### (d) Messages vs. locals/exports

Conditions are always exports (the parser auto-exports zero-arity heads), so
`⊥c` is always an ordinary internal relation. Setter bodies rooted at
messages flow through `^receive:`/`^entry:` as any INSERT does; setter bodies
that are all-constant (e.g. `c : X=1.` shapes) are constant-fed flows and run
in the initializer, generically. Zero-arity messages remain unsupported, as
today.

### (e) `@differential` removal

Retracting a body row runs the generic removal path: the setter INSERT's view
marks `(true)` `present → unknown`; the top-down checker for `⊥c` re-proves
by scanning the setter views' tables (`BuildMaybeScanPartial` with an empty
bound prefix — a full scan; multiple setters go through the ordinary
relation/union checker over all INSERTs). If re-proved, `(true)` returns to
`present` and nothing downstream moves; if disproved, `absent` propagates:
join users retract via the pivoted-join remover (implemented today), negate
users re-derive via `MaybeReAddToNegatedView`. `EvaluateConditionAndNotify`'s
bespoke scan-everything machinery is subsumed entirely. Today's global proof
counters (counter = number of present rows across setter tables) are
subsumed by the tuple state model itself: `kUnknown` + top-down recheck *is*
the refcount, expressed the same way as for every other relation. (Note: the
runtime does not physically refcount rows; the state model plus re-proving is
the mechanism, and it is already exercised by all differential flows.)

## 3. The cross-product gate (requirement 4)

Under the constant-pivot encoding, **no condition test ever creates a
cross-product**, so no implicit blessing of products is needed and the
F14-family removal-region product TODO is never reached. This is the primary
reason to prefer the pivot encoding over the literal zero-pivot product
encoding.

Specify the gate anyway, as validation defense-in-depth:

> A JOIN with `num_pivots == 0` is legal iff (a) the originating clause has
> `@product` (`ParsedClause::CrossProductsArePermitted`), or (b) every joined
> view except at most one is a SELECT of an `is_condition` relation.

Enforce (b) in two places: the dataflow build, at the existing site that
emits the `@product` diagnostic (so an accidental future product-producing
desugaring is caught at build time), and as an assert in the control-flow
build's product path (`Build/Product.cpp` / `CreateBottomUpJoinRemover`), so
that a product that survives to lowering without `@product` and without
unit-only sides aborts loudly rather than silently entering the
unimplemented removal path. Because case (b) never arises under this
proposal's encoding, the gate should never fire; it exists to keep the
product-removal gap from being widened by accident.

## 4. Anonymous conditions: the keep-last-edge rule

Optimization-invented conditions (`CreateDependencyOnView`) exist because
canonicalization sometimes rewrites a view's outputs to all-constants,
severing the last data edge to its predecessor while the presence dependency
remains. Replace the compensation with prevention:

> **Canonicalization never severs the last input-column edge to a view's
> incoming view.** When constant propagation or unused-column removal would
> drop the final edge, keep exactly one input column (an arbitrary
> representative), even if its output is otherwise unused.

CMP, MAP, NEGATE, and TUPLE call sites of `CreateDependencyOnView` all become
"retain one column" instead of "invent a condition". The cost is one
dead-weight column in an occasional table versus a global counter, a
`kConditionTester` procedure, and a purple side-channel edge. This deletes
anonymous conditions entirely, which is a prerequisite for deleting
`QueryConditionImpl` (user conditions alone could be desugared, but the class
survives as long as any pass can mint one).

## 5. Lowering design

### Generic path (correctness baseline)

Nothing new: the unit-join lowers as an ordinary `TABLEJOIN` (pivot vector of
`true`, index on `⊥c[bool]` — which is *covering*, so per
`docs/RuntimeAndCodegen.md` no index is materialized and the table's own hash
lookup serves it), the unit-negate as the ordinary negation existence check,
and removal as the pivoted-join remover. This is what Stage B ships with.

### Fast path (recommended): `CHECKTUPLE` gate in the control-flow build

In the CF build's join lowering (`Build/Join.cpp` and the join work-item),
when a joined view's backing `DataTable` has `is_condition` set, do not treat
it as a scan source; instead wrap the remaining join (or the plain successor
body, when the join has only one non-unit side) in:

    check-tuple {true} in %⊥c_table
      if-present
        <body>

i.e. `ProgramCheckTupleRegion` — the IR's existing existence test. Top-down
checkers test the same `CHECKTUPLE` instead of recursing into a join checker
arm for the unit side. Unit-negations similarly gate on `if-absent`
(with the `unknown` arm calling `⊥c`'s top-down checker, as negation lowering
already does for negated tables).

**Where the special case lands — CF build, not codegen.** Trade-offs:

- *CF build (recommended).* The CF optimizer sees small `CHECKTUPLE` regions
  it already knows how to CSE, flatten, and prune; identical gates across
  parallel arms merge for free. Any backend (and `-ir-out` readers) sees the
  cheap form. The build already dispatches per dataflow shape and has the
  `DataTable` bit in hand. Cost: one more shape in the join work-item, and
  the differential path must know a unit side contributes no removal scan
  (it is gated by the `⊥c` state instead).
- *Codegen.* Zero IR churn, but the special case must pattern-match
  `TABLEJOIN`-over-unit-table *after* the CF optimizer has rewritten regions
  — fragile — and every consumer of the IR still pays the full join
  machinery (pivot vectors, per-table select arms) conceptually. The only
  codegen-level special case worth keeping as an option is representation:
  emitting a condition table as a tri-state byte (`absent/present/unknown`)
  instead of a full `Table<Row>`. Defer it; measure first — a 1-row
  `Table<Row>` is already a single hash probe.

## 6. Bugs eliminated by construction, and the new invariants

- **F8 (OR-instead-of-AND condition testing).** The bug lived in hand-rolled
  boolean logic: `InCondionalTests` built one `TUPLECMP` over all positive
  counters (OR semantics) instead of a chain, and the top-down wrap dropped
  the negative test when both polarities were present. Under the refactor
  there is no hand-rolled conjunction: each condition is its own JOIN or
  NEGATE node, and conjunction is node composition — the same composition
  every multi-literal body already uses. The bug class cannot recur because
  the code that could host it does not exist.
- **F5 (zero-pair `TUPLECMP` fold assert in CF optimize).** Condition
  counters are the chief producer of variable-vs-constant `TUPLECMP`s, whose
  simplification can degenerate to zero pairs. With counter tests gone, the
  remaining `TUPLECMP`s compare real tuple variables (scan re-checks,
  comparisons), which do not degenerate this way.
- **F9 (zero-arity heads corrupting `QueryIOImpl::receives` /
  `QueryRelationImpl::selects`).** The root shape — a condition-defining
  clause whose head *is* the body's SELECT, patched around by
  `GuardWithTuple`'s user-replacement filter that excludes only
  `QueryConditionImpl` users — disappears: desugared condition heads are
  ordinary 1-arity heads with ordinary INSERT proxies, and with no condition
  user lists the `ReplaceUsesWithIf` filter in `SubstituteAllUsesWith` is
  deleted rather than kept precise.
- **The forgotten-guard class.** Every per-pass "remember the conditions"
  site in §1 (CSE `Equals` guards, transfer/copy choreography, merge/join
  canonicalization checks) is deleted. A pass that forgets about conditions
  is no longer expressible because conditions are not a separate thing.

New invariants replacing the old ones:

1. `QueryImpl` owns no conditions; every inter-view dependency is a column
   edge. (Replaces: condition user/setter consistency checks.)
2. Canonicalization never severs the last input-column edge to an incoming
   view (§4). (Replaces: `CreateDependencyOnView` compensation.)
3. A JOIN pivot whose non-user side is an `is_condition` relation is never
   removed, and CSE never folds a unit SELECT into a non-unit one.
   (Replaces: condition-aware CSE guards.)
4. An `is_condition` relation contains at most the row `(true)`: only the
   desugaring creates its INSERTs, and they insert only the token. Assert at
   CF build (`DataTable::is_condition` ⇒ one bool column) and in debug
   codegen.
5. Zero-pivot JOINs appear only under `@product` (§3) — now with no implicit
   exceptions at all.

## 7. Staged migration plan

Each stage leaves `ctest` and the 112-case OptDiff suite green; no stage
introduces a dual code path — a stage that adds the new path deletes the old
one in the same change.

**Stage 0 — pin behavior.** Add OptDiff cases capturing today's semantics:
multi-setter condition; condition set inside a recursive cycle; condition
tested both positively and negatively in one clause (F8 shape);
`@differential` message retraction flipping a condition both ways; condition
guarding an INSERT whose relation is also SELECTed. Files:
`tests/OptDiff/cases/*.{dr,main.cpp}` (~5 cases, ~400 lines). These are the
regression oracle for every later stage.

**Stage A — generic groundwork (three independent, individually green
changes).**
1. `is_condition` bit: `QueryRelationImpl` (set where relations are created
   in dataflow `Build.cpp`), threaded to `DataTable` in
   `lib/ControlFlow/Data.cpp` / `TABLE::GetOrCreate`, exposed on
   `QueryRelation` and `DataTable` public APIs. (~60 lines)
2. INSERT `attached_columns` support: dataflow `Insert.cpp` canonicalization,
   `ProxyInsertsWithTuples` alignment, CF `Build/Insert.cpp` (attached vars
   are in scope; nothing extra to emit), top-down checker scan with empty
   bound prefix (`BuildMaybeScanPartial`). (~150 lines)
3. Keep-last-edge rule replacing `CreateDependencyOnView` (§4): rewrite the
   four call sites in `Tuple.cpp`, `Compare.cpp`, `Map.cpp`, `Negate.cpp`,
   plus `GuardWithOptimizedTuple`; delete `CreateDependencyOnView`. Also add
   empty-relation folding to dead-flow (`DeadFlowElimination.cpp`): NEGATE
   whose negated view is untainted/unsat becomes a forwarding TUPLE (today
   this exists only in the condition fixpoint). Anonymous conditions are gone
   after this stage; `elim-cond-cycle-simple`, `tuple_2/3`, `view_3/4`,
   `optimize_3` are the sentinels. (~250 lines net-negative)

**Stage B — the cutover (one large, deletion-dominated change).** Desugar
user conditions in `BuildClause` per §2 and delete the entire condition
universe in the same change: `Condition.cpp`; `QueryConditionImpl` and all
`lib/DataFlow/Query.h` members/methods in §1; `AddConditionsToInsert`,
`ExtractConditionsToTuples`, `ShrinkConditions`; per-node `Equals`/
canonicalization guards; dot-format condition rendering; public
`QueryCondition` API; CF `InCondionalTests`, `EvaluateConditionAndNotify`,
`BuildEagerUpdateCondAndNotify`, `ConditionVariable`,
`Context::cond_checker_procs`, `kConditionTester`, `kConditionRefCount`,
`DataVariableImpl::query_cond`, `cond_ref_counts`; codegen `^test:` naming.
Lowering uses the generic path (§5 baseline). Estimated −1,800/+500 lines
across ~30 files (`lib/DataFlow/*`, `lib/ControlFlow/{Build/Build.cpp,
Build/Build.h, Build/Insert.cpp, Program.cpp, Data.cpp, Format.cpp,
Operation.cpp}`, both `Query.h`s, `Program.h`, `Database.cpp`). This stage is
only green if Stage 0's cases pass in all four opt modes — run the full
OptDiff suite, not just ctest.

**Stage C — unit-gate fast path.** The `CHECKTUPLE` lowering in CF
`Build/Join.cpp` and negation lowering (§5), plus invariant-4 asserts, plus
the §3 gate asserts. Compare `-ir-out` before/after on the condition corpus
to confirm gates replace join machinery. (~200 lines)

**Stage D — docs and corpus.** Rewrite the condition sections of
`docs/DataFlowIR.md`, `docs/ControlFlowIR.md`, `docs/Architecture.md`;
re-attempt `data/examples/conditions_to_bools.dr` (its cross-products in
removal paths may now compile — if so it graduates from the feature-gap
list); update `tests/OptDiff/FINDINGS.md` open-bug list.

## 8. Open questions and risks

1. **Induction interactions (the riskiest item).** A condition set inside a
   recursive cycle becomes a unit relation participating in an induction
   set. The termination invariant holds by construction — the back-edge is
   dominated by `CHANGETUPLE (true)`, which fires at most once per epoch —
   but gate flips *during* a fixpoint change which iteration first sees the
   gate open, and the generic induction machinery has never executed over
   unit relations (today counters are updated outside `INDUCTION` regions).
   Stage 0's cond-in-recursion cases must cover: setter and tester in the
   same cycle, tester in a deeper induction depth, and a `@differential`
   retraction that closes a gate mid-flow.
2. **Performance: table probe vs. register compare.** Today a positive test
   is a compare on a `uint64_t` global; after, it is a hash probe of a 1-row
   table (baseline) or a `CHECKTUPLE` (fast path) per driven row. `PARALLEL`
   CSE merges identical gates across arms, but there is no loop-invariant
   hoisting in the CF optimizer. If profiles regress, options are: hoist
   `CHECKTUPLE` gates above `VECTORLOOP`s in `OptimizeImpl(SERIES)` (sound —
   tables mutate only through dominated transitions), or the codegen
   tri-state-byte representation (§5).
3. **Dead-flow NEGATE rule.** The current taint rule marks a NEGATE live only
   when *both* predecessor and negated view are tainted; setter-less
   conditions relied on the condition fixpoint for the vacuous-truth case.
   Stage A.3's empty-relation folding must land first or negative tests of
   never-set conditions get wrongly deleted (`deadflowelimination_4` is the
   sentinel).
4. **Multiple setters.** Handled by the ordinary multi-INSERT relation
   machinery (union checker over insert views); but re-proving `(true)`
   scans *all* setter tables, where today one counter decrement sufficed.
   Acceptable: retraction of conditions is rare; correctness first.
5. **Insert-guard vs. select-guard asymmetry.** Today conditions attach to
   the pre-INSERT guard tuple (`AddConditionsToInsert`) — the whole clause is
   gated at its sink. Desugared tests are body literals, so the join
   scheduler may place the gate anywhere in the join tree. Semantics are
   identical (set semantics; the gate filters everything either way), but
   generated-code shape and `-ir-out` goldens shift; OptDiff compares program
   output, so the suite is insensitive to this.
6. **CSE over-merging unit SELECTs.** Two SELECTs of the same `⊥c` merging is
   sound (same unit data; no diagonal hazard since the encoding never
   products them) and desirable. Group-ID propagation handles joins over the
   shared SELECT generically; no exception needed — verify with a case where
   one clause tests `c` twice.
7. **Column type of the witness edge.** INSERT `attached_columns` carries one
   arbitrary body column; for bodies whose terminal view is itself a unit
   SELECT (`c2 : c1.`), the witness is the `bool` column — fine. Bodies with
   zero columns cannot occur (every SELECT/TUPLE has ≥1 column; all-constant
   bodies flow through the ALL-CONSTS tuple, whose constant columns are
   themselves legal witnesses since constant-fed INSERTs run in the
   initializer).
