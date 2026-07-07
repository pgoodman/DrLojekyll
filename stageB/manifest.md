# Stage B deletion manifest checklist

Working checklist against UnitConditions.plan.md B2 table plus the B1/B3 work.

## B1 desugarer
- [ ] true-const token (literal-less bool CONST) + shared select column
- [ ] unit relation SELECT + one-pivot equi-join for positive tests
- [ ] ordinary NEGATE for negative tests
- [ ] setter clauses: witness TUPLE + INSERT{input=[t], attached=[w]}
- [ ] ConnectInsertsToSelects: unit relations keep INSERT->REL->SELECT; wire select->inserts
- [ ] QuerySelect::ForEachUse maps select cols to insert INPUT cols (inserts have no out cols)

## B2 deletion manifest
- [ ] lib/DataFlow/Condition.cpp deleted (+ CMakeLists)
- [ ] lib/DataFlow/Query.h: QueryConditionImpl, COND, conditions, decl_to_condition, per-view cond lists
- [ ] lib/DataFlow/View.cpp: Copy/Transfer/Drop*Conditions, OrderConditions, cond depth arms, F9 ReplaceUsesWithIf filter
- [ ] lib/DataFlow/Build.cpp: AddConditionsToInsert, add_set_conditon, INCREMENT path, cond_guard, ExtractConditionsToTuples call, check_conds
- [ ] lib/DataFlow/Optimize.cpp: ShrinkConditions + driver call
- [ ] per-node Equals cond guards (Select/Join/Merge/Compare/Aggregate/Tuple/Insert/KVIndex/Map/Negate)
- [ ] Join.cpp: cond asserts/transfers; RemoveConstants + CMP-sinking deleted (constant pivots stay pivots)
- [ ] Merge.cpp: cond guards in trivial-cycle/pull-through/sink checks
- [ ] DeadFlowElimination.cpp: setter-less-cond fixpoint; sets_condition in IsUsed/IsTrivialCycle
- [ ] Differential.cpp: cond => can_produce_deletions forcing
- [ ] Induction.cpp (DF): cond-skip in merge clustering
- [ ] lib/DataFlow/Format.cpp: COND cells/purple edges/Conditions() loop; unit annotation for RELATION
- [ ] public include DataFlow/Query.h: QueryCondition, std::hash, SetCondition/Pos/NegConditions, Query::Conditions
- [ ] CF Build/Build.cpp: InCondionalTests, EvaluateConditionAndNotify, BuildEagerUpdateCondAndNotify, ConditionVariable, cond blocks in eager insert/removal, checker cond wraps, FillDataModel cond forcing
- [ ] CF Build/Build.h: cond_checker_procs, ConditionVariable decl
- [ ] CF Program.h/.cpp, Data.cpp: kConditionRefCount, kConditionTester, query_cond, cond_ref_counts
- [ ] CF Build/Insert.cpp: assert(!view.SetCondition())
- [ ] CF Format.cpp: ^test: arm
- [ ] CF Analyze.cpp: query_cond copies
- [ ] CodeGen Database.cpp: kConditionTester ProcName arm (KEEP TESTANDSET + global-var loop)
- [ ] Connect.cpp/Link.cpp: TransferSetConditionTo/TransferTestedConditionsTo call sites

## B3 defense gate + invariants
- [ ] CreateProduct: diagnostic if a unit-condition SELECT reaches a product
- [ ] CF Product.cpp + zero-pivot removal branch: assert no unit table in a product
- [ ] unit relation shape assert (1 bool column) at CF table creation
- [ ] condition-pivot survives canonicalization (by construction: RemoveConstants deleted)
- [ ] CF: witness-insert predecessors get tables (top-down re-prove scan needs them)
- [ ] CF: INSERT->SELECT eager handoff binds select columns (insert stored cols)
