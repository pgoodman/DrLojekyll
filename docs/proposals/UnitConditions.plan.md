# Workflow plan: conditions → unit relations, then delete the condition tax

Companion to `docs/proposals/UnitConditions.md` (the design rationale and
complete touchpoint inventory — read it first; this file is the execution
plan). Target: a multi-agent workflow session. Repo state at plan time:
commit `691f8ee` on `main`, all OptDiff findings F1–F14 resolved, ctest 3/3,
suite 4-mode green.

## Mission

1. Desugar every zero-arity condition into an ordinary 1-arity `bool` unit
   relation (`⊥c`, sole possible row `(true)`), flagged `is_condition`.
2. Delete the entire condition-specific universe: `QueryConditionImpl`, the
   three per-view side-channel lists, all preservation choreography, the CF
   counter machinery (`kConditionRefCount`, `kConditionTester`,
   `EvaluateConditionAndNotify`), and codegen counter emission.
3. Then sweep for follow-on simplifications the deletion unlocks (passes that
   exist only to compensate for conditions; guards that guard nothing now).

## Ground rules for every agent

- Build: `cmake --build --preset debug`; test: `cd build/debug && ctest
  --output-on-failure`; differential: `DR=build/debug/bin/drlojekyll
  TIMEOUT=120 tests/OptDiff/diffrun.sh cases/<n>.dr cases/<n>.main.cpp <wk>`
  (macOS bash 3.2: no `declare -A`; clangd diagnostics are noise — only the
  real build counts).
- **No dual code paths.** A change that adds the new path deletes the old one
  in the same change. No compatibility shims, no "keep the old accessor just
  in case", no retrospective comments.
- Every stage ends green: ctest 3/3 AND the full OptDiff suite (~115 cases ×
  4 modes, byte-identical stdout vs the `opt` build; the 5 expected-error
  cases aggregate_1/kvindex_1–4 keep their documented diagnostic behavior).
  Commit per stage.
- Desugaring happens in the **dataflow builder, not the parser**: debug
  builds assert a print→parse→print fixpoint on the AST, so surface syntax
  (zero-arity heads, `!c` tests) must survive parsing untouched.
- Sentinel OptDiff cases per stage are listed inline. A sentinel failing =
  stop and fix before proceeding; do not weaken the case.

## Workflow shape

Five sequential stages. Stages 0 and A have internal parallelism (worktree
isolation if agents edit concurrently); B is one deletion-dominated change
(single agent or tight pair: desugarer + deleter, same worktree); C and D are
single-agent. After each stage: an independent verifier agent that rebuilds
from scratch, runs the gates, and adversarially reviews the diff against this
plan (especially: did the stage delete everything it claimed to delete?
`grep -r` for the names listed in its deletion manifest — surviving mentions
outside docs/history are failures).

    Stage 0 (pin) ──► Stage A (groundwork, 3 packets) ──► Stage B (cutover)
                                                             │
                    Stage D (docs+sweep) ◄── Stage C (fast path) ◄┘

---

## Stage 0 — pin current behavior (parallel case-writing agents)

New OptDiff cases capturing today's condition semantics; these are the oracle
for every later stage. Each case = `tests/OptDiff/cases/<name>.dr` +
`<name>.main.cpp` (copy driver shape from `booleans_diff.main.cpp`), verified
4-mode green *before* any refactoring starts.

| Case | Shape it pins |
|---|---|
| `cond_multi_setter` | `c : a(_). c : b(_).` — two setter clauses; `@differential` messages; retract one setter's support while the other still holds (the F11-for-conditions shape: `c` must stay true), then retract both (must flip false, downstream retracts). |
| `cond_in_induction` | Setter and positive tester inside one recursive cycle: `t(X) : m(X). t(Y) : t(X), step(X,Y), c. c : seed(_).` — seed arrives in the same batch as, before, and after the `t` messages (three drivers-in-one via batching; output must be identical). |
| `cond_in_induction_deep` | Tester in a deeper induction than the setter's cycle (nested/two inductions, cf. `two_inductions.dr`). |
| `cond_both_polarities` | One clause tests `c1` positively and `c2` negatively; another clause tests `c1` both ways across bodies (the F8 shape — AND semantics across multiple positive conditions, and the negative test not being discarded). |
| `cond_diff_flipflop` | `@differential` retraction closing a gate mid-flow, then reopening it: add support, derive, retract (downstream must retract), re-add (downstream must re-derive). |
| `cond_double_test` | One body tests the same condition twice (`q(X) : r(X), c, c.`) — pins CSE behavior for later unit-SELECT merging. |

Existing sentinels already in the suite (do not touch, just know them):
`booleans`, `booleans_diff`, `cf16_6`, `deadflowelimination_4/5`,
`elim-cond-cycle-simple`, `insert_4`, `merge_3`, `negate_3`, `optimize_3/5`,
`prove_constant`, `select_1`, `tuple_2/3`, `view_3/4`.

Risk note (proposal §8.1): `cond_in_induction*` are the most likely to expose
*pre-existing* bugs in today's counter machinery. If a Stage 0 case fails on
unmodified `main`, that is a new finding — record it in
`tests/OptDiff/FINDINGS.md` (F15+), fix or document, and only then pin.

---

## Stage A — generic groundwork (3 packets, each independently green)

### A1. `is_condition` bit (~60 lines)

Why: later stages need "was declared as a condition" observable at every
level without consulting parse trees.

    diff sketch:
    lib/DataFlow/Query.h        QueryRelationImpl { + const bool is_condition; }
    lib/DataFlow/Build.cpp      relation creation: + is_condition =
                                  (decl.Arity() == 0)   // pre-desugar: tag only
                                  (after Stage B: the ⊥c relations it creates)
    include/.../DataFlow/Query.h  QueryRelation { + bool IsCondition() const; }
    lib/ControlFlow/Data.cpp    TABLE::GetOrCreate: thread bit to DataTableImpl
    include/.../ControlFlow/Program.h  DataTable { + bool IsCondition() const; }

### A2. INSERT `attached_columns` (~150 lines)

Why: the one genuinely new IR capability — "presence depends on view B, but
none of B's data is stored." The desugared setter INSERT stores only the
`true` token yet must keep a data edge to the body.

Semantics: `input_columns` = stored; `attached_columns` = read-only witness
edge establishing dataflow dependency. Algorithms affected:

    QueryInsertImpl::Canonicalize:
      for each attached col: if constant/duplicate, may drop it ONLY if
      at least one input-or-attached edge to the incoming view remains
      (keep-last-edge rule, A3 — shared helper)

    ControlFlow Build/Insert.cpp (bottom-up insert emission):
      no new emission: attached vars are already in scope at the insert
      site; CHANGETUPLE persists input_columns only.

    Top-down checker for a relation fed by witness-only INSERTs:
      checker(⊥c, [true]) :=
        for each setter INSERT i of ⊥c:
          scan i.incoming_view's table with EMPTY bound prefix   // full scan
            (BuildMaybeScanPartial already supports this)
          if any row checks out → return present
        return absent

    CSE / Equals: attached_columns participate in structural equality.

### A3. Keep-last-edge rule; delete `CreateDependencyOnView` (~250 lines, net-negative)

Why: optimization-invented *anonymous* conditions exist only because
canonicalization may sever the last data edge to a predecessor (all outputs
became constants). Prevent instead of compensate; `QueryConditionImpl`
cannot be deleted in Stage B while any pass can still mint one.

    rule (new canonicalization invariant):
      when constant-propagation / unused-column removal in {TUPLE, CMP, MAP,
      NEGATE}::Canonicalize or GuardWithOptimizedTuple would drop the FINAL
      input-column edge to the incoming view:
        keep exactly one representative input column (arbitrary), even if
        its output column is otherwise unused
      delete: View.cpp CreateDependencyOnView + its 5 call sites

    also (dead-flow prerequisite, proposal §8.3):
      DeadFlowElimination: + empty-relation folding —
        NEGATE whose negated view is unsat/untainted → forwarding TUPLE
      (today this vacuous-truth case lives only inside the condition
       fixpoint, which Stage B deletes; land the generic rule FIRST)

Sentinels: `elim-cond-cycle-simple`, `tuple_2/3`, `view_3/4`, `optimize_3`,
`deadflowelimination_4`. After A3, `grep -rn CreateDependencyOnView lib/`
must be empty.

---

## Stage B — the cutover (one deletion-dominated change, ~−1800/+500 across ~30 files)

### B1. Desugaring in `BuildClause` (lib/DataFlow/Build.cpp)

    // once per QueryImpl:
    true_const := QueryConstantImpl with no ParsedLiteral, kBoolean
                  (the QueryTagImpl literal-less CONST-stream pattern)
    unit_rel(c) := GetOrCreate relation ⊥c, one bool column,
                   is_condition = true          // A1 bit, now for real

    // (a) condition-defining clause  c : body.
    B := build body view (witness columns w0..wn)
    P := TUPLE [ t = true_const, w = B.w0 ]
    INSERT into ⊥c { input_columns = [P.t], attached_columns = [P.w] }
    // CHANGETUPLE (true) absent|unknown→present; second row fails the
    // transition and stops — ordinary bottom-up idempotence.

    // (b) positive test  head(..) : r(X), c.
    U    := user's body view extended with constant col [t = true_const]
    Sel  := PUSH/SELECT of ⊥c   (its column NOT marked constant in the IR,
                                 so the pivot survives useless-pivot elim)
    JOIN pivot { U.t , Sel.t }  // ONE pivot: column-vs-constant equi-join,
                                // never a cross-product — F14 removal-path
                                // product TODO is never entered
    // multiple conditions: chain one join per condition

    // (c) negative test  head(..) : r(X), !c.
    NEGATE (match: [t] against ⊥c; attached: [X])   // ordinary AND-NOT
    // @never keeps is_never unchanged

    // (d) all-constant setter bodies flow through ALL-CONSTS tuple →
    //     initializer, generically. Zero-arity messages remain unsupported.
    // (e) @differential removal: generic path. Retraction marks (true)
    //     present→unknown; top-down checker (A2) re-proves by scanning
    //     setter tables; absent propagates via pivoted-join remover (joins)
    //     and MaybeReAddToNegatedView (negates). This SUBSUMES
    //     EvaluateConditionAndNotify's scan-everything machinery, and the
    //     global counters: kUnknown + recheck IS the refcount.

### B2. Deletion manifest (same change; grep-verified empty afterward)

| Where | Delete | Why safe now |
|---|---|---|
| `lib/DataFlow/Condition.cpp` | entire file | no COND nodes exist |
| `lib/DataFlow/Query.h` | `QueryConditionImpl`, `COND`, `QueryImpl::conditions`, `decl_to_condition`; `QueryViewImpl::{sets_condition, positive_conditions, negative_conditions}` | desugaring emits only views/columns |
| `lib/DataFlow/View.cpp` | `{Copy,Transfer}TestedConditionsTo`, `DropTestedConditions`, `DropSetConditions`, `DropSetConditionsOfDeadView`, `TransferSetConditionTo`, `OrderConditions`; condition arms of `IsConditional`/depth calc; the `ReplaceUsesWithIf` condition-user filter in `SubstituteAllUsesWith` (the F9 mechanism — delete, don't refine) | no condition edges to preserve; user replacement is uniform again |
| `lib/DataFlow/Build.cpp` | `AddConditionsToInsert`, `add_set_conditon`, zero-arity INCREMENT path, `cond_guard` branches (incl. the F9 guard-tuple workaround), `ExtractConditionsToTuples` call | replaced by B1 |
| `lib/DataFlow/Optimize.cpp` | `ShrinkConditions` + driver call | dead-flow + A3 empty-relation folding subsume it |
| per-node `Equals` (`Select/Join/Merge/Compare/Aggregate.cpp`) | `positive_conditions != ...` guards | conditions are structural operands now |
| `Join.cpp`/`Merge.cpp` | condition asserts, sets-condition transfers, conditional-merge rewrite blocks | nothing to transfer |
| `DeadFlowElimination.cpp` | setter-less-condition fixpoint; `sets_condition` in `IsUsed` | generic taint + A3 folding |
| `Differential.cpp` | "has conditions ⇒ can_produce_deletions" forcing | join/negate operands get differential support from existing rules |
| `Induction.cpp` (DF) | condition-skip in merge clustering | unit relations cluster like any relation |
| `lib/DataFlow/Format.cpp` | COND cells, purple edges, `Conditions()` loop | condition relations print as ordinary RELATION (annotate `unit`) |
| public `Query.h` | `QueryCondition` class, `std::hash`, `QueryView::{SetCondition,PositiveConditions,NegativeConditions}`, `Query::Conditions` | no external consumers (greenfield rule) |
| CF `Build/Build.cpp` | `InCondionalTests` (F5/F8 site), `EvaluateConditionAndNotify`, `BuildEagerUpdateCondAndNotify`, `ConditionVariable`, condition blocks in eager insert/removal walks, checker counter-wrapping, `FillDataModel` condition forcing; `Build.h Context::cond_checker_procs` | ordinary join/negate lowering + data-model rules take over |
| CF `Program.h/.cpp`, `Data.cpp` | `VariableRole::kConditionRefCount`, `ProcedureKind::kConditionTester`, `DataVariableImpl::query_cond`, `cond_ref_counts` | no counters exist |
| CF `Build/Insert.cpp` | `assert(!view.SetCondition())` | API gone |
| `lib/CodeGen/CPlusPlus/Database.cpp` | `^test:` proc-name arm | kind gone. KEEP: TESTANDSET emission + global-var loop (still serve `kInitGuard`) |

### B3. Defense-in-depth gate (proposal §3) — added in the same change

    dataflow build, at the existing @product diagnostic site, and
    CF Build/Product.cpp + CreateBottomUpJoinRemover, as asserts:
      zero-pivot JOIN is legal iff clause has @product
        OR all-but-one joined views are SELECTs of is_condition relations
    // under the pivot encoding case 2 never occurs; the gate exists so an
    // accidental product-producing desugaring aborts loudly instead of
    // entering the unimplemented product-removal path

New invariants asserted: `is_condition` relation ⇒ exactly one bool column,
only desugarer-created INSERTs, at most the row `(true)` (debug assert in CF
build); condition-pivot never removed by canonicalization; CSE never folds a
unit SELECT into a non-unit one.

Gate for Stage B: full suite green in all 4 modes **including all Stage 0
cases**; `grep -rniE 'querycondition|sets_condition|positive_conditions|negative_conditions|kConditionRefCount|kConditionTester|cond_ref_counts|InCondionalTests|EvaluateConditionAndNotify' lib include bin` → zero hits.

---

## Stage C — unit-gate fast path in the CF build (~200 lines)

Why: baseline lowers unit-joins as full `TABLEJOIN` (pivot vectors, index
probes). A unit side never multiplies cardinality — it only gates.

    CF Build/Join.cpp join work-item, per joined view V:
      if V.table.IsCondition():
        do not treat V as a scan source; instead wrap the remaining join
        (or the sole successor body if only one non-unit side remains):
          check-tuple {true} in %⊥c_table     // ProgramCheckTupleRegion
            if-present  <body>
      top-down join checkers: test the same CHECKTUPLE instead of a join
      checker arm for the unit side.
      unit-NEGATE: gate on if-absent; unknown arm calls ⊥c's top-down
      checker (negation lowering already does this for negated tables).
      differential: a unit side contributes NO removal scan — removal is
      driven by ⊥c's own state transition (the generic path from B1(e)).

Location rationale (already decided, proposal §5): CF build, not codegen —
the CF optimizer CSEs/flattens/merges `CHECKTUPLE` regions for free and every
backend sees the cheap form. A codegen tri-state-byte table representation is
explicitly deferred; measure first.

Verification: `-ir-out` before/after on the condition corpus must show gates
replacing join machinery (no `table-join` over a unit table remains); full
suite green; Stage 0 induction cases byte-identical.

---

## Stage D — simplification sweep, docs, corpus

1. **Tax-elimination sweep** (the "then simplify" half of the mission). With
   conditions gone, hunt for now-dead generality — candidates known at plan
   time, verify each before deleting:
   - `ProgramTestAndSetRegion` shrinks to the single `kInitGuard` use — can
     it become a plain guarded init, deleting the region kind + visitor arm +
     codegen emission entirely?
   - `Clause.cpp`'s "should be factored out into a zero-argument predicate"
     error: A2's witness-edge INSERT makes the data-less shape expressible —
     can the restriction be lifted (desugar inline) or should it stay as
     style enforcement? (Owner call; default: keep, note in Language.md.)
   - `GuardWithTuple(force)` call sites: with the F9 branch gone, which
     forced guards remain necessary?
   - Depth/`EstimateDepth` computation: condition arms removed — simplify.
   - Grep for `unsatisfiable`/`is_unsat` consumers: A3+B may leave unsat
     handling uniform enough to fold into dead-flow directly.
2. Re-attempt `data/examples/conditions_to_bools.dr` (feature-gap file, F14
   family): its removal-path cross-products may now compile. If green in all
   4 modes, graduate it into the suite and strike it from CLAUDE.md's gap
   list.
3. Rewrite condition sections: `docs/DataFlowIR.md` (§Conditions → unit
   relations), `docs/ControlFlowIR.md` (drop `kConditionTester`, TESTANDSET
   rows per outcome of D1), `docs/Architecture.md`, `docs/Language.md`
   (semantics note: conditions are sugar for unit relations),
   `docs/proposals/UnitConditions.md` gains nothing (it described the plan;
   the docs describe the present — per repo comment rules, no "previously"
   framing anywhere).
4. Update `tests/OptDiff/FINDINGS.md` (new findings from Stage 0, if any) and
   `CLAUDE.md` (invariants list: swap the condition invariants for the five
   new ones in proposal §6).

Final acceptance: ctest 3/3; full OptDiff suite 4-mode byte-identical
(including 6 new Stage 0 cases and possibly `conditions_to_bools`); deletion
greps empty; net LOC strongly negative outside `tests/` and `docs/`.

## Known risks the executing workflow must respect

1. **Induction × unit relations** (riskiest; proposal §8.1): gate flips
   mid-fixpoint are untested territory. Stage 0's three induction cases are
   non-negotiable and must be written by an agent that has read
   `lib/ControlFlow/Build/Induction.cpp`'s vector discipline (see
   `docs/ControlFlowIR.md`).
2. **Order dependency**: A3's empty-relation folding must land before B, or
   negative tests of never-set conditions get wrongly deleted
   (`deadflowelimination_4` is the tripwire).
3. **Performance regression** (proposal §8.2): register compare → table
   probe. Accept for correctness; if profiling later complains, hoist
   `CHECKTUPLE` above `VECTORLOOP`s in `OptimizeImpl(SERIES)` or revisit the
   tri-state byte. Do not pre-optimize in this workflow.
4. **`-ir-out` goldens shift** (proposal §8.5): gates move from insert-sinks
   to body literals. OptDiff compares program *output*, so this is invisible
   to the suite — do not chase IR-shape diffs, only behavior.
