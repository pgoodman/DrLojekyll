# Stage B deletion manifest checklist

Working checklist against UnitConditions.plan.md B2 table plus the B1/B3 work.

## B1 desugarer
- [x] true-const token (literal-less bool CONST) + shared select column (`QueryImpl::true_col`, `TrueColumn` in Build.cpp)
- [x] unit relation SELECT + one-pivot equi-join for positive tests (`ApplyPositiveConditionTest`)
- [x] ordinary NEGATE for negative tests (`ApplyNegativeConditionTest`)
- [x] setter clauses: witness TUPLE + INSERT{input=[t], attached=[w]} (BuildClause zero-arity arm)
- [x] ConnectInsertsToSelects: unit relations keep INSERT->REL->SELECT; wire select->inserts
- [x] QuerySelect::ForEachUse maps select cols to insert INPUT cols (inserts have no out cols)
- [x] per-clause/per-polarity dedup keyed by declaration equality (auto-export `Id()`s collide)
- [x] Select/Insert CSE equality by relation pointer / declaration context (same collision)
- [x] literal-less CONST: Select::Hash, QueryConstant::Type -> kBoolean, dot prints TRUE

## B2 deletion manifest
- [x] lib/DataFlow/Condition.cpp deleted (+ CMakeLists)
- [x] lib/DataFlow/Query.h: QueryConditionImpl, COND, conditions, decl_to_condition, per-view cond lists
- [x] lib/DataFlow/View.cpp: Copy/Transfer/Drop*Conditions, OrderConditions, cond depth arms, F9 ReplaceUsesWithIf filter
- [x] lib/DataFlow/Build.cpp: AddConditionsToInsert, add_set_conditon, INCREMENT path, cond_guard, ExtractConditionsToTuples call, check_conds
- [x] lib/DataFlow/Optimize.cpp: ShrinkConditions + driver call
- [x] per-node Equals cond guards (Select/Join/Merge/Compare/Aggregate/Tuple/Insert/KVIndex/Map/Negate)
- [x] Join.cpp: cond asserts/transfers; RemoveConstants + CMP-sinking deleted (pivots are never removed)
- [x] Merge.cpp: cond guards in trivial-cycle/pull-through/sink checks
- [x] DeadFlowElimination.cpp: setter-less-cond fixpoint; sets_condition in IsUsed/IsTrivialCycle
- [x] Differential.cpp: cond => can_produce_deletions forcing (check_conds -> report_message_errors)
- [x] Induction.cpp (DF): cond-skip in merge clustering + COND user-filter in union injection
- [x] lib/DataFlow/Format.cpp: COND cells/purple edges/Conditions() loop; unit annotation for RELATION
- [x] public include DataFlow/Query.h: QueryCondition, std::hash, SetCondition/Pos/NegConditions, Query::Conditions
- [x] CF Build/Build.cpp: InCondionalTests, EvaluateConditionAndNotify, BuildEagerUpdateCondAndNotify, ConditionVariable, cond blocks in eager insert/removal, checker cond wraps, FillDataModel cond forcing
- [x] CF Build/Build.h: cond_checker_procs, ConditionVariable decl
- [x] CF Program.h/.cpp, Data.cpp: kConditionRefCount, kConditionTester, query_cond, cond_ref_counts
- [x] CF Build/Insert.cpp: assert(!view.SetCondition()) + stale condition TODO
- [x] CF Format.cpp: ^test: arm
- [x] CF Analyze.cpp: query_cond copies
- [x] CodeGen Database.cpp: kConditionTester ProcName arm (KEPT: TESTANDSET emission + global-var loop for kInitGuard)
- [x] Connect.cpp/Link.cpp: TransferSetConditionTo/TransferTestedConditionsTo call sites

## B3 defense gate + invariants
- [x] CreateProduct: debug assert that no unit-condition SELECT reaches a product
- [x] CF Product.cpp: assert no unit-relation SELECT among product sides
- [x] CreateBottomUpJoinRemover: zero-pivot never reaches the pivoted remover (assert + comment)
- [x] unit relation shape assert (exactly 1 bool column) at CF table creation (Data.cpp)
- [x] condition-pivot survives canonicalization (by construction: pivot columns are never removed)
- [x] CF: witness-insert predecessors get tables (top-down re-prove scan needs them)
- [x] CF: INSERT->SELECT eager handoff binds select columns (insert stored cols); MapVariablesInEagerRegion assert relaxed for INSERT readers

## Gates
- [x] ctest 3/3
- [x] all 6 Stage 0 cond_* cases + 17 legacy condition sentinels 4-mode green
- [x] deletion grep zero hits in lib include bin
- [x] full suite SUITE: PASS (128 cases, run in 8 name-filtered chunks: 9+17+13+18+25+12+9+25)
