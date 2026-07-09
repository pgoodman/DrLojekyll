// Copyright 2020, Trail of Bits. All rights reserved.

#pragma once

#include <drlojekyll/DataFlow/Query.h>
#include <drlojekyll/Parse/Parse.h>
#include <drlojekyll/Util/DefUse.h>
#include <drlojekyll/Util/Node.h>

#include <functional>
#include <memory>
#include <optional>
#include <utility>

namespace hyde {

class ProgramImpl;

class ProgramVisitor;

class DataColumn;
class DataIndex;
class DataTable;
class DataVariable;
class DataVector;

class Program;
class ProgramCallRegion;
class ProgramReturnRegion;
class ProgramTestAndSetRegion;
class ProgramGenerateRegion;
class ProgramInductionRegion;
class ProgramLetBindingRegion;
class ProgramParallelRegion;
class ProgramProcedure;
class ProgramPublishRegion;
class ProgramSeriesRegion;
class ProgramVectorAppendRegion;
class ProgramVectorClearRegion;
class ProgramVectorLoopRegion;
class ProgramVectorSwapRegion;
class ProgramVectorUniqueRegion;
class ProgramUpdateCountRegion;
class ProgramChangeRecordRegion;
class ProgramCheckMemberRegion;
class ProgramCheckRecordRegion;
class ProgramCommitSweepRegion;
class ProgramClaimRegion;
class ProgramRetireRegion;
class ProgramNetBatchRegion;
class ProgramTableJoinRegion;
class ProgramTableProductRegion;
class ProgramTableScanRegion;
class ProgramTupleCompareRegion;
class ProgramWorkerIdRegion;

// A generic region of code nested inside of a procedure.
class ProgramRegionImpl;
class ProgramRegion : public Node<ProgramRegion, ProgramRegionImpl> {
 public:
  // Return the region containing `child`, or `std::nullopt` if this is the
  // topmost region in a procedure.
  static std::optional<ProgramRegion> Containing(ProgramRegion &child) noexcept;

  ProgramRegion(const ProgramCallRegion &);
  ProgramRegion(const ProgramReturnRegion &);
  ProgramRegion(const ProgramTestAndSetRegion &);
  ProgramRegion(const ProgramGenerateRegion &);
  ProgramRegion(const ProgramInductionRegion &);
  ProgramRegion(const ProgramLetBindingRegion &);
  ProgramRegion(const ProgramParallelRegion &);
  ProgramRegion(const ProgramPublishRegion &);
  ProgramRegion(const ProgramSeriesRegion &);
  ProgramRegion(const ProgramVectorAppendRegion &);
  ProgramRegion(const ProgramVectorClearRegion &);
  ProgramRegion(const ProgramVectorLoopRegion &);
  ProgramRegion(const ProgramVectorSwapRegion &);
  ProgramRegion(const ProgramVectorUniqueRegion &);
  ProgramRegion(const ProgramUpdateCountRegion &);
  ProgramRegion(const ProgramChangeRecordRegion &);
  ProgramRegion(const ProgramCheckMemberRegion &);
  ProgramRegion(const ProgramCheckRecordRegion &);
  ProgramRegion(const ProgramCommitSweepRegion &);
  ProgramRegion(const ProgramClaimRegion &);
  ProgramRegion(const ProgramRetireRegion &);
  ProgramRegion(const ProgramNetBatchRegion &);
  ProgramRegion(const ProgramTableJoinRegion &);
  ProgramRegion(const ProgramTableProductRegion &);
  ProgramRegion(const ProgramTableScanRegion &);
  ProgramRegion(const ProgramTupleCompareRegion &);
  ProgramRegion(const ProgramWorkerIdRegion &);

  void Accept(ProgramVisitor &visitor) const;

  bool IsCall(void) const noexcept;
  bool IsReturn(void) const noexcept;
  bool IsTestAndSet(void) const noexcept;
  bool IsGenerate(void) const noexcept;
  bool IsInduction(void) const noexcept;
  bool IsVectorLoop(void) const noexcept;
  bool IsVectorAppend(void) const noexcept;
  bool IsVectorClear(void) const noexcept;
  bool IsVectorSwap(void) const noexcept;
  bool IsVectorUnique(void) const noexcept;
  bool IsLetBinding(void) const noexcept;
  bool IsUpdateCount(void) const noexcept;
  bool IsChangeRecord(void) const noexcept;
  bool IsCheckMember(void) const noexcept;
  bool IsCheckRecord(void) const noexcept;
  bool IsCommitSweep(void) const noexcept;
  bool IsClaim(void) const noexcept;
  bool IsRetire(void) const noexcept;
  bool IsNetBatch(void) const noexcept;
  bool IsTableJoin(void) const noexcept;
  bool IsTableProduct(void) const noexcept;
  bool IsTableScan(void) const noexcept;
  bool IsSeries(void) const noexcept;
  bool IsParallel(void) const noexcept;
  bool IsPublish(void) const noexcept;
  bool IsTupleCompare(void) const noexcept;
  bool IsWorkerId(void) const noexcept;

  std::string_view Comment(void) const noexcept;

 private:
  friend class ProgramCallRegion;
  friend class ProgramReturnRegion;
  friend class ProgramTestAndSetRegion;
  friend class ProgramGenerateRegion;
  friend class ProgramInductionRegion;
  friend class ProgramLetBindingRegion;
  friend class ProgramParallelRegion;
  friend class ProgramProcedure;
  friend class ProgramPublishRegion;
  friend class ProgramSeriesRegion;
  friend class ProgramVectorAppendRegion;
  friend class ProgramVectorClearRegion;
  friend class ProgramVectorLoopRegion;
  friend class ProgramVectorSwapRegion;
  friend class ProgramVectorUniqueRegion;
  friend class ProgramUpdateCountRegion;
  friend class ProgramChangeRecordRegion;
  friend class ProgramCheckMemberRegion;
  friend class ProgramCheckRecordRegion;
  friend class ProgramCommitSweepRegion;
  friend class ProgramClaimRegion;
  friend class ProgramRetireRegion;
  friend class ProgramNetBatchRegion;
  friend class ProgramTableJoinRegion;
  friend class ProgramTableProductRegion;
  friend class ProgramTableScanRegion;
  friend class ProgramTupleCompareRegion;
  friend class ProgramWorkerIdRegion;

  using Node<ProgramRegion, ProgramRegionImpl>::Node;
};

// A generic sequencing of regions.
class ProgramSeriesRegionImpl;
class ProgramSeriesRegion : public Node<ProgramSeriesRegion, ProgramSeriesRegionImpl> {
 public:
  static ProgramSeriesRegion From(ProgramRegion) noexcept;

  // The sequence of regions nested inside this series.
  UsedNodeRange<ProgramRegion> Regions(void) const;

 private:
  friend class ProgramRegion;

  using Node<ProgramSeriesRegion, ProgramSeriesRegionImpl>::Node;
};

// A generic representation the two or more regions can execute in parallel.
class ProgramParallelRegionImpl;
class ProgramParallelRegion
    : public Node<ProgramParallelRegion, ProgramParallelRegionImpl> {
 public:
  static ProgramParallelRegion From(ProgramRegion) noexcept;

  // The set of regions nested inside this series.
  UsedNodeRange<ProgramRegion> Regions(void) const;

 private:
  friend class ProgramRegion;

  using Node<ProgramParallelRegion, ProgramParallelRegionImpl>::Node;
};

enum class VariableRole : int {
  kInitGuard,
  kConstant,
  kConstantTag,
  kConstantZero,
  kConstantFalse,
  kConstantTrue,
  kVectorVariable,
  kLetBinding,
  kJoinPivot,
  kJoinNonPivot,
  kProductOutput,
  kScanOutput,
  kFunctorOutput,
  kMessageOutput,
  kParameter,
  kWorkerId,
  kRecordElement
};

// A variable in the program.
class DataVariableImpl;
class DataVariable : public Node<DataVariable, DataVariableImpl> {
 public:
  VariableRole DefiningRole(void) const noexcept;

  // The region which defined this local variable. If this variable has no
  // defining region then it is a global variable.
  std::optional<ProgramRegion> DefiningRegion(void) const noexcept;

  // Unique ID of this variable.
  unsigned Id(void) const noexcept;

  // Name of this variable, if any. There might not be a name.
  Token Name(void) const noexcept;

  // The literal, constant value of this variable.
  std::optional<QueryConstant> Value(void) const noexcept;

  // Type of this variable.
  TypeLoc Type(void) const noexcept;

  // Whether this variable is global.
  bool IsGlobal(void) const noexcept;

  // Whether or not this variable is a constant.
  bool IsConstant(void) const noexcept;

  // Return the number of uses of this variable.
  unsigned NumUses(void) const noexcept;

 private:
  using Node<DataVariable, DataVariableImpl>::Node;
};

enum class VectorKind : unsigned {
  kParameter,
  kInductionInputs,
  kInductionSwaps,
  kInductionOutputs,
  kJoinPivots,
  kInductiveJoinPivots,
  kInductiveJoinPivotSwaps,
  kProductInput,
  kInductiveProductInput,
  kInductiveProductSwaps,
  kTableScan,

  // Vector that collects outputs for messages marked with `@differential`.
  kMessageOutputs,

  // This is a vector created inside of a message procedure and passed down to
  // the primary data flow function. It is guaranteed to be empty.
  kEmpty,

  // Worklists of claimed rows drained by a stratum's delete/insert
  // fixpoint rounds.
  kDeleteQueue,
  kAddQueue,

  // A stratum's accumulated overdeletion/addition row sets for one batch.
  kOverdeleteSet,
  kAdditionSet,

  // A differential message's netted explicit removals/additions.
  kNetRemovals,
  kNetAdditions,

  // A recursive stratum's per-round CLAIMED frontier (`Δ_D`/`Δ_A` in MD
  // §5.2/§5.3): the subset of `kOverdeleteSet`/`kAdditionSet` claimed in
  // the *current* fixpoint round only. CLAIM appends a row here on
  // success (not the drained queue); the OVERDELETE/INSERT fixpoint's
  // firing loop ranges over this vector, the loop's empty-check is its
  // break condition (claim progress, not queue emptiness), and RETIRE
  // clears the round's frontier bit over this same vector. Distinct from
  // `kOverdeleteSet`/`kAdditionSet`, which accumulate across all rounds
  // of the batch.
  kClaimedDeleteFrontier,
  kClaimedAddFrontier,
};

// A column in a table.
class DataColumnImpl;
class DataColumn : public Node<DataColumn, DataColumnImpl> {
 public:
  // Unique ID of this column.
  unsigned Id(void) const noexcept;

  // Index of this column within its table.
  unsigned Index(void) const noexcept;

  // Type of this column.
  TypeLoc Type(void) const noexcept;

  // Possible names that can be associated with this column.
  //
  // NOTE(pag): Multiple columns of the same table might have intersecting
  //            sets of possible names.
  const std::vector<Token> &PossibleNames(void) const noexcept;

 private:
  friend class DataTable;

  using Node<DataColumn, DataColumnImpl>::Node;
};

// An index on a table.
class DataIndexImpl;
class DataIndex : public Node<DataIndex, DataIndexImpl> {
 public:
  // Unique ID of this index.
  unsigned Id(void) const noexcept;

  // Columns from a table that are part of this index.
  UsedNodeRange<DataColumn> KeyColumns(void) const;

  // Columns from a table that are part of this index.
  UsedNodeRange<DataColumn> ValueColumns(void) const;

 private:
  friend class DataTable;

  using Node<DataIndex, DataIndexImpl>::Node;
};

// A persistent table, backed by some kind of data store / database.
class DataTableImpl;
class DataTable : public Node<DataTable, DataTableImpl> {
 public:
  static DataTable Containing(DataColumn col) noexcept;
  static DataTable Backing(DataIndex index) noexcept;

  unsigned Id(void) const noexcept;

  // Columns in this table. The columns may be from different `QueryView`
  // nodes.
  DefinedNodeRange<DataColumn> Columns(void) const;

  // Indices on this table.
  DefinedNodeRange<DataIndex> Indices(void) const;

  // Visit the users of this table.
  void VisitUsers(ProgramVisitor &visitor);

  // Apply a function to each user.
  void ForEachUser(std::function<void(ProgramRegion)> cb);

  // Return the list of views associated with this table.
  const std::vector<QueryView> Views(void) const noexcept;

  // Are there any views that will possibly remove entries from this table?
  bool IsDifferential(void) const noexcept;

  // Whether this table backs a unit relation modeling a zero-arity predicate
  // (a condition): its only possible row is `(true)`.
  bool IsCondition(void) const noexcept;

 private:
  using Node<DataTable, DataTableImpl>::Node;
};

// A record case is a particular instantiation or variant of a record.
// A record might have multiple cases.
class DataRecordCaseImpl;
class DataRecordCase : public Node<DataRecordCase, DataRecordCaseImpl> {
 public:

 private:
  using Node<DataRecordCase, DataRecordCaseImpl>::Node;
};

// A record is an abstraction over a persisted tuple. The storage for the
// record is implemented in terms of one or more cases.
class DataRecordImpl;
class DataRecord : public Node<DataRecord, DataRecordImpl> {
 public:

 private:
  using Node<DataRecord, DataRecordImpl>::Node;
};

// A vector in the program.
class DataVectorImpl;
class DataVector : public Node<DataVector, DataVectorImpl> {
 public:
  VectorKind Kind(void) const noexcept;

  unsigned Id(void) const noexcept;

  // Do we need to shard this vector across workers?
  bool IsSharded(void) const noexcept;

  // Types of the variables/columns stored in this vector.
  const std::vector<TypeLoc> ColumnTypes(void) const noexcept;

  // Visit the users of this vector.
  void VisitUsers(ProgramVisitor &visitor);

  // Apply a function to each user.
  void ForEachUser(std::function<void(ProgramRegion)> cb);

  // If this is a paramter vector to the data flwo entry function, or to
  // an I/O procedure, then these will tell us the corresponding message being
  // added or removed by this vector.
  std::optional<ParsedMessage> AddedMessage(void) const noexcept;
  std::optional<ParsedMessage> RemovedMessage(void) const noexcept;

 private:
  friend class ProgramVectorClearRegion;
  friend class ProgramVectorSwapRegion;
  friend class ProgramVectorUniqueRegion;

  using Node<DataVector, DataVectorImpl>::Node;
};

// A run-once guard: increments `Accumulator` and executes `Body` if and only
// if the incremented value is `1`, i.e. `if ((A += 1) == 1) { ... }`. The
// sole producer is the entry procedure's init guard (a `kInitGuard` global),
// which makes the constant-initialization flows run exactly once even though
// the entry procedure runs on every message batch.
class ProgramTestAndSetRegionImpl;
class ProgramTestAndSetRegion
    : public Node<ProgramTestAndSetRegion, ProgramTestAndSetRegionImpl> {
 public:
  static ProgramTestAndSetRegion From(ProgramRegion) noexcept;

  // The accumulator variable. This is `A` in `(A += 1) == 1`.
  DataVariable Accumulator(void) const;

  // Return the body executed when the incremented accumulator equals `1`.
  std::optional<ProgramRegion> Body(void) const noexcept;

 private:
  friend class ProgramRegion;

  using Node<ProgramTestAndSetRegion, ProgramTestAndSetRegionImpl>::Node;
};

// Apply a functor to one or more inputs, yield zero or more outputs. In the
// zero output case, the output is considered to be a boolean value.
class ProgramGenerateRegionImpl;
class ProgramGenerateRegion
    : public Node<ProgramGenerateRegion, ProgramGenerateRegionImpl> {
 public:
  static ProgramGenerateRegion From(ProgramRegion) noexcept;

  unsigned Id(void) const noexcept;

  // Does this functor application behave like a filter function?
  bool IsFilter(void) const noexcept;

  // Returns `true` if repeated executions of the function given the same
  // inputs generate the same outputs.
  bool IsPure(void) const noexcept;

  // Returns `true` if repeated executions of the function given the same
  // inputs are not guaranteed to generate the same outputs.
  bool IsImpure(void) const noexcept;

  // Returns the functor to be applied.
  ParsedFunctor Functor(void) const noexcept;

  // List of variables to pass at inputs. The Nth input variable corresponds
  // with the Nth `bound`-attributed variable in the parameter list of
  // `Functor()`.
  UsedNodeRange<DataVariable> InputVariables(void) const;

  // List of variables that are generated by applied this functor. The Nth
  // output variable corresponds with the Nth `free`-attributed variable
  // in the parameter list of `Functor()`. This will be empty if `IsFilter()`
  // is `true`.
  DefinedNodeRange<DataVariable> OutputVariables(void) const;

  // Return the body which is conditionally executed if the filter functor
  // returns true (`IsFilter() == true`) or if at least one tuple is generated
  // (as represented by `OutputVariables()`).
  std::optional<ProgramRegion> BodyIfResults(void) const noexcept;

  // Return the body which is conditionally executed if the filter functor
  // returns `false` (`IsFilter() == true`) or if zero tuples are generated.
  // Functor negations use this body.
  std::optional<ProgramRegion> BodyIfEmpty(void) const noexcept;

 private:
  friend class ProgramRegion;

  using Node<ProgramGenerateRegion, ProgramGenerateRegionImpl>::Node;
};

// A let binding is an assignment of variables.
class ProgramLetBindingRegionImpl;
class ProgramLetBindingRegion
    : public Node<ProgramLetBindingRegion, ProgramLetBindingRegionImpl> {
 public:
  static ProgramLetBindingRegion From(ProgramRegion) noexcept;

  DefinedNodeRange<DataVariable> DefinedVariables(void) const;
  UsedNodeRange<DataVariable> UsedVariables(void) const;

  // Return the body to which the lexical scoping of the variables applies.
  std::optional<ProgramRegion> Body(void) const noexcept;

 private:
  friend class ProgramRegion;
  using Node<ProgramLetBindingRegion, ProgramLetBindingRegionImpl>::Node;
};

enum class VectorUsage : unsigned {
  kInvalid,
  kInductionVector,
  kUnionInputVector,
  kJoinPivots,
  kProductInputVector,
  kProcedureInputVector,
  kTableScan,
  kMessageOutputVector,
};

// Loop over a vector.
class ProgramVectorLoopRegionImpl;
class ProgramVectorLoopRegion
    : public Node<ProgramVectorLoopRegion, ProgramVectorLoopRegionImpl> {
 public:
  static ProgramVectorLoopRegion From(ProgramRegion) noexcept;

  // Return the loop body.
  std::optional<ProgramRegion> Body(void) const noexcept;

  VectorUsage Usage(void) const noexcept;
  DataVector Vector(void) const noexcept;

  // Variables extracted from the vector during each iteration of the loop
  DefinedNodeRange<DataVariable> TupleVariables(void) const;

  // Optional worker ID variable.
  std::optional<DataVariable> WorkerId(void) const;

 private:
  friend class ProgramRegion;

  using Node<ProgramVectorLoopRegion, ProgramVectorLoopRegionImpl>::Node;
};

// Append a tuple to a vector.
class ProgramVectorAppendRegionImpl;
class ProgramVectorAppendRegion
    : public Node<ProgramVectorAppendRegion, ProgramVectorAppendRegionImpl> {
 public:
  static ProgramVectorAppendRegion From(ProgramRegion) noexcept;

  VectorUsage Usage(void) const noexcept;
  DataVector Vector(void) const noexcept;
  UsedNodeRange<DataVariable> TupleVariables(void) const;

  // Optional worker ID variable.
  std::optional<DataVariable> WorkerId(void) const;

 private:
  friend class ProgramRegion;

  using Node<ProgramVectorAppendRegion, ProgramVectorAppendRegionImpl>::Node;
};

#define VECTOR_OP(name) \
  class name ## Impl; \
  class name : public Node<name, name ## Impl> { \
   public: \
    static name From(ProgramRegion) noexcept; \
    VectorUsage Usage(void) const noexcept; \
    DataVector Vector(void) const noexcept; \
    std::optional<DataVariable> WorkerId(void) const; \
   private: \
    friend class ProgramRegion; \
    using Node<name, name ## Impl>::Node; \
  }

// Clear a vector.
VECTOR_OP(ProgramVectorClearRegion);

// Sort and unique the elements in a vector.
VECTOR_OP(ProgramVectorUniqueRegion);

#undef VECTOR_OP

class ProgramVectorSwapRegionImpl;
class ProgramVectorSwapRegion
    : public Node<ProgramVectorSwapRegion, ProgramVectorSwapRegionImpl> {
 public:
  static ProgramVectorSwapRegion From(ProgramRegion) noexcept;

  DataVector LHS(void) const noexcept;
  DataVector RHS(void) const noexcept;

 private:
  friend class ProgramRegion;
  using Node<ProgramVectorSwapRegion, ProgramVectorSwapRegionImpl>::Node;
};

// The named membership predicates of a differential table. Every read of a
// differential table by generated joins goes through exactly one of these,
// named explicitly on its CHECKMEMBER region so the frozen-vs-current read
// discipline of the delta schemas is auditable in `-ir-out`.
enum class MembershipPredicate : unsigned {
  kInI,                   // Batch-start state (the frozen "I").
  kInNew,                 // Final-so-far: (kInI && !kDel) || kAdd.
  kSurvivesSoFar,         // kInI && !kDel.
  kAliveAtClaim,          // kInI && (!kDel || kDelNow).
  kInNewWithFrontier,     // (kInI && !kDel) || kAdd.
  kInNewSansFrontier,     // (kInI && !kDel) || (kAdd && !kAddNow).
  kPresent,               // Count-based presence: C_nr + C_r > 0.
  kRecursivelySupported,  // C_r > 0 (the REDERIVE survival test).
  kNetDeleted,            // kDel && !kAdd (net deletion this batch).
  kNetAdded,              // kAdd && !kDel (net addition this batch).
};

// One signed derivation-counter fold on a table: a single +1/-1 of the
// row's `C_nr` or `C_r` counter, applied inline at a rule-firing site
// (multiset discipline: one fold per rule-instance firing). `Body` executes
// only when the fold is a zero-crossing event. On a monotone table the fold
// degenerates to an insert-if-new, whose crossing is "the row is new".
class ProgramUpdateCountRegionImpl;
class ProgramUpdateCountRegion
    : public Node<ProgramUpdateCountRegion, ProgramUpdateCountRegionImpl> {
 public:
  static ProgramUpdateCountRegion From(ProgramRegion) noexcept;

  // The body that conditionally executes on a zero crossing.
  std::optional<ProgramRegion> Body(void) const noexcept;

  unsigned Arity(void) const noexcept;

  UsedNodeRange<DataVariable> TupleVariables(void) const;

  DataTable Table(void) const;

  // Fold sign: `true` adds a derivation, `false` retracts one.
  bool IsAdd(void) const noexcept;

  // Which counter the fold lands on: `kNonRecursive` for seed/init-position
  // firings (C_nr), `kRecursive` for fixpoint back-edge firings (C_r).
  DerivClass DerivationClass(void) const noexcept;

  // Whether the fold is explicit (message) support: the row's `kExplicit`
  // set-semantics bit rather than a multiset derivation count. Only
  // meaningful on differential tables.
  bool IsExplicit(void) const noexcept;

 private:
  friend class ProgramRegion;

  using Node<ProgramUpdateCountRegion, ProgramUpdateCountRegionImpl>::Node;
};

// This is similar to a `ProgramUpdateCountRegion`; however, it also
// creates new definitions for the variables which it is updating. The key
// idea is that this gets us the "record" associated with some tuple data,
// rather than us keeping with the tuple data itself.
class ProgramChangeRecordRegionImpl;
class ProgramChangeRecordRegion
    : public Node<ProgramChangeRecordRegion, ProgramChangeRecordRegionImpl> {
 public:
  static ProgramChangeRecordRegion From(ProgramRegion) noexcept;

  // The body that conditionally executes on a zero crossing.
  std::optional<ProgramRegion> Body(void) const noexcept;

  unsigned Arity(void) const noexcept;

  // Returns a unique ID for this region.
  unsigned Id(void) const noexcept;

  UsedNodeRange<DataVariable> TupleVariables(void) const;

  // The defined record variables.
  DefinedNodeRange<DataVariable> RecordVariables(void) const;

  DataTable Table(void) const;

  // Fold sign: `true` adds a derivation, `false` retracts one.
  bool IsAdd(void) const noexcept;

  // Which counter the fold lands on.
  DerivClass DerivationClass(void) const noexcept;

 private:
  friend class ProgramRegion;

  using Node<ProgramChangeRecordRegion, ProgramChangeRecordRegionImpl>::Node;
};

// A strictly two-way membership gate on a table: `IfPresent` executes when
// the named predicate holds for the tuple, `IfAbsent` when it does not. The
// predicate names one of the differential membership reads; on a monotone
// table the gate is simply row existence.
class ProgramCheckMemberRegionImpl;
class ProgramCheckMemberRegion
    : public Node<ProgramCheckMemberRegion, ProgramCheckMemberRegionImpl> {
 public:
  static ProgramCheckMemberRegion From(ProgramRegion) noexcept;

  std::optional<ProgramRegion> IfPresent(void) const noexcept;
  std::optional<ProgramRegion> IfAbsent(void) const noexcept;

  unsigned Arity(void) const noexcept;

  UsedNodeRange<DataVariable> TupleVariables(void) const;

  DataTable Table(void) const;

  // The membership predicate this gate reads.
  MembershipPredicate Predicate(void) const noexcept;

 private:
  friend class ProgramRegion;

  using Node<ProgramCheckMemberRegion, ProgramCheckMemberRegionImpl>::Node;
};

// This is like `ProgramCheckMemberRegion`, except that it operates on
// records, i.e. it defines new variables for what is being returned.
class ProgramCheckRecordRegionImpl;
class ProgramCheckRecordRegion
    : public Node<ProgramCheckRecordRegion, ProgramCheckRecordRegionImpl> {
 public:
  static ProgramCheckRecordRegion From(ProgramRegion) noexcept;

  std::optional<ProgramRegion> IfPresent(void) const noexcept;
  std::optional<ProgramRegion> IfAbsent(void) const noexcept;

  // Returns a unique ID for this region.
  unsigned Id(void) const noexcept;

  unsigned Arity(void) const noexcept;

  DefinedNodeRange<DataVariable> RecordVariables(void) const;

  UsedNodeRange<DataVariable> TupleVariables(void) const;

  DataTable Table(void) const;

  // The membership predicate this gate reads.
  MembershipPredicate Predicate(void) const noexcept;

 private:
  friend class ProgramRegion;

  using Node<ProgramCheckRecordRegion, ProgramCheckRecordRegionImpl>::Node;
};

// The end-of-batch commit sweep of one table. On a differential table it
// publishes each touched row's net 0/1 presence change (against the
// batch-start snapshot) to `Message()` when the table backs a
// `@differential` transmit view, seals the new snapshot, and clears the
// batch-scratch flags. On a monotone table it advances the sealed row-id
// watermark, so the next epoch's frozen-state reads see this epoch's rows;
// such a sweep never carries a message.
class ProgramCommitSweepRegionImpl;
class ProgramCommitSweepRegion
    : public Node<ProgramCommitSweepRegion, ProgramCommitSweepRegionImpl> {
 public:
  static ProgramCommitSweepRegion From(ProgramRegion) noexcept;

  DataTable Table(void) const;

  // The `@differential` message fed by this table, if any: net presence
  // crossings publish through it.
  std::optional<ParsedMessage> Message(void) const noexcept;

 private:
  friend class ProgramRegion;

  using Node<ProgramCommitSweepRegion, ProgramCommitSweepRegionImpl>::Node;
};

// Claims a row of a differential table into the overdeletion set
// (`IsDelete()`) or the addition set, and into the current frontier round.
// `Body` executes only when the claim succeeds, i.e. on the row's first
// claim this batch; a repeated claim is absorbed. The claim mutates the
// row's batch-scratch flags, so the region is a side effect even without a
// body.
class ProgramClaimRegionImpl;
class ProgramClaimRegion
    : public Node<ProgramClaimRegion, ProgramClaimRegionImpl> {
 public:
  static ProgramClaimRegion From(ProgramRegion) noexcept;

  // The body that conditionally executes on the row's first claim this
  // batch.
  std::optional<ProgramRegion> Body(void) const noexcept;

  unsigned Arity(void) const noexcept;

  UsedNodeRange<DataVariable> TupleVariables(void) const;

  DataTable Table(void) const;

  // Claim target: `true` claims into the overdeletion set (the delete
  // frontier), `false` into the addition set (the insert frontier).
  bool IsDelete(void) const noexcept;

 private:
  friend class ProgramRegion;

  using Node<ProgramClaimRegion, ProgramClaimRegionImpl>::Node;
};

// Retires a row of a differential table from the current frontier round:
// clears the row's current-frontier bit (`kDelNow` when `IsDelete()`,
// `kAddNow` otherwise). Has no body.
class ProgramRetireRegionImpl;
class ProgramRetireRegion
    : public Node<ProgramRetireRegion, ProgramRetireRegionImpl> {
 public:
  static ProgramRetireRegion From(ProgramRegion) noexcept;

  unsigned Arity(void) const noexcept;

  UsedNodeRange<DataVariable> TupleVariables(void) const;

  DataTable Table(void) const;

  // Which frontier bit is cleared: `true` for the delete frontier,
  // `false` for the insert frontier.
  bool IsDelete(void) const noexcept;

 private:
  friend class ProgramRegion;

  using Node<ProgramRetireRegion, ProgramRetireRegionImpl>::Node;
};

// Nets a differential message's explicit adds against its explicit removes
// within one batch: rewrites `AddVector()` to one copy of each row whose
// net occurrence count is positive and `RemoveVector()` to one copy of
// each row whose net is negative. Has no body.
class ProgramNetBatchRegionImpl;
class ProgramNetBatchRegion
    : public Node<ProgramNetBatchRegion, ProgramNetBatchRegionImpl> {
 public:
  static ProgramNetBatchRegion From(ProgramRegion) noexcept;

  DataVector AddVector(void) const noexcept;
  DataVector RemoveVector(void) const noexcept;

 private:
  friend class ProgramRegion;

  using Node<ProgramNetBatchRegion, ProgramNetBatchRegionImpl>::Node;
};

// Perform an equi-join between two or more tables, and iterate over the
// results.
class ProgramTableJoinRegionImpl;
class ProgramTableJoinRegion
    : public Node<ProgramTableJoinRegion, ProgramTableJoinRegionImpl> {
 public:
  static ProgramTableJoinRegion From(ProgramRegion) noexcept;

  // Unique ID for this join.
  unsigned Id(void) const noexcept;

  // The body that conditionally executes for each joined result. Variable
  // bindings are applied.
  std::optional<ProgramRegion> Body(void) const noexcept;

  // Delta sections. Each executes for a joined combination per a named
  // batch-delta discipline that codegen evaluates directly on the scanned
  // row ids; variable bindings are applied as for `Body()`. A joined
  // combination requires every side's scanned key columns to equal the
  // pivot (the index scans are approximate; the sections conjoin the key
  // equality that the body path re-checks through its TUPLECMP).
  //
  // `AddedBody()`: every side is in the batch-final state (InNew) and at
  // least one side is a net addition of this batch — a combination that
  // newly holds. `RemovedBody()`: every side was in the batch-start state
  // (InI) and at least one side is a net deletion of this batch — a
  // combination that stopped holding. The pivot vector is sort-uniqued and
  // the join runs once per batch, so each section fires exactly once per
  // started/stopped rule instance; an instance with a net-added side and a
  // net-deleted side fires neither. Monotone sides answer the reads
  // through their sealed row-id watermark.
  std::optional<ProgramRegion> AddedBody(void) const noexcept;
  std::optional<ProgramRegion> RemovedBody(void) const noexcept;

  // The pivot vector that contains the join pivots. The elements of this
  // pivot vector are in the same order as `OutputPivotVariables()`.
  DataVector PivotVector(void) const noexcept;

  // The tables that are joined together. The same table may appear more than
  // once.
  UsedNodeRange<DataTable> Tables(void) const;

  // The columns used in the scan of the Nth table. These are in the same
  // order as the entries in `PivotVector()` and `OutputPivotVariables()`.
  UsedNodeRange<DataColumn> IndexedColumns(unsigned table_index) const;

  // These are the output columns associated with the Nth table scan. These
  // do NOT include pivot columns.
  UsedNodeRange<DataColumn> SelectedColumns(unsigned table_index) const;

  // The index used by the Nth table scan.
  std::optional<DataIndex> Index(unsigned table_index) const noexcept;

  // These are the output variables for the pivot columns. These are in the same
  // order as the entries in the pivot vector.
  DefinedNodeRange<DataVariable> OutputPivotVariables(void) const;

  // These are the output variables from the Nth table scan. These do NOT include
  // pivot variables.
  DefinedNodeRange<DataVariable> OutputVariables(unsigned table_index) const;

 private:
  friend class ProgramRegion;

  using Node<ProgramTableJoinRegion, ProgramTableJoinRegionImpl>::Node;
};

// Perform an cross-product between two or more tables, and iterate
// over the results.
class ProgramTableProductRegionImpl;
class ProgramTableProductRegion
    : public Node<ProgramTableProductRegion, ProgramTableProductRegionImpl> {
 public:
  static ProgramTableProductRegion From(ProgramRegion) noexcept;

  // Unique ID of this region.
  unsigned Id(void) const noexcept;

  // The body that conditionally executes for each produced result. Variable
  // bindings are applied.
  std::optional<ProgramRegion> Body(void) const noexcept;

  // The tables that are joined together. The same table may appear more than
  // once.
  UsedNodeRange<DataTable> Tables(void) const;

  // The input vectors that need to be merged with all entries of the tables
  // that don't correspond to the input vectors themselves.
  UsedNodeRange<DataVector> Vectors(void) const;

  // The table used by the Nth table scan.
  DataTable Table(unsigned table_index) const noexcept;

  // The index used by the Nth table scan.
  DataVector Vector(unsigned table_index) const noexcept;

  // These are the output variables from the Nth table scan.
  DefinedNodeRange<DataVariable> OutputVariables(unsigned table_index) const;

 private:
  friend class ProgramRegion;

  using Node<ProgramTableProductRegion, ProgramTableProductRegionImpl>::Node;
};

// Perform a scan over a table, possibly using an index. If an index is being
// used the input variables are provided to perform equality matching against
// column values. The results of the scan fill a vector.
class ProgramTableScanRegionImpl;
class ProgramTableScanRegion
    : public Node<ProgramTableScanRegion, ProgramTableScanRegionImpl> {
 public:
  static ProgramTableScanRegion From(ProgramRegion) noexcept;

  // Unique ID of this region.
  unsigned Id(void) const noexcept;

  // The table being scanned.
  DataTable Table(void) const noexcept;

  // Optional index being scanned.
  std::optional<DataIndex> Index(void) const noexcept;

  // The body that conditionally executes for each scanned tuple.
  std::optional<ProgramRegion> Body(void) const noexcept;

  // The columns used to constrain the scan of the table. These are in the same
  // order as the entries in `InputVariables()`. This is empty if an index isn't
  // being used.
  UsedNodeRange<DataColumn> IndexedColumns(void) const;

  // These are the output columns associated with the table scan. These
  // do NOT include any indexed columns.
  //
  // NOTE(pag): These will be ordered, such that the first selected column
  //            is also the earlier appearing column of all selected columns
  //            within the table.
  UsedNodeRange<DataColumn> SelectedColumns(void) const;

  // The variables being provided for each of the `IndexedColumns()`, which are
  // used to constrain the scan of the table. This is empty if an index isn't
  // being used.
  UsedNodeRange<DataVariable> InputVariables(void) const;

  // The variables which are scanned. There is one variable for each column in
  // the table. This does not have a 1:1 correspondence with `SelectedColumns`.
  DefinedNodeRange<DataVariable> OutputVariables(void) const;

 private:
  friend class ProgramRegion;

  using Node<ProgramTableScanRegion, ProgramTableScanRegionImpl>::Node;
};

// An inductive area in a program. An inductive area is split up into three
// regions:
//
//    1)  The initialization region, which is responsible for finding the inputs
//        to kick off the inductive cycle.
//    2)  The cyclic region, which iterates, appending on newly proven tuples
//        to one or more vectors. Iteration continues until a fixpoint is
//        reached. The cyclic region is always entered at least once, and the
//        condition on the back edge of the loop is that the induction vectors
//        are non-empty.
//    2)  The output region, which iterates over all tuples amassed during the
//        initialization and cyclic regions, and operates on those tuples to
//        push to the next region of the data flow.
class ProgramInductionRegionImpl;
class ProgramInductionRegion
    : public Node<ProgramInductionRegion, ProgramInductionRegionImpl> {
 public:
  static ProgramInductionRegion From(ProgramRegion) noexcept;

  // Unique ID for this induction group/region.
  unsigned Id(void) const;

  // Set of induction vectors that are filled with initial data in the
  // `Initializer()` region, then accumulate more data during the
  // `FixpointLoop()` region (and are tested), and then are finally iterated
  // and cleared in the `Output()` region.
  UsedNodeRange<DataVector> Vectors(void) const;

  std::optional<ProgramRegion> Initializer(void) const noexcept;
  ProgramRegion FixpointLoop(void) const noexcept;
  std::optional<ProgramRegion> Output(void) const noexcept;

 private:
  friend class ProgramRegion;

  using Node<ProgramInductionRegion, ProgramInductionRegionImpl>::Node;
};

// A comparison between two tuples.
class ProgramTupleCompareRegionImpl;
class ProgramTupleCompareRegion
    : public Node<ProgramTupleCompareRegion, ProgramTupleCompareRegionImpl> {
 public:
  static ProgramTupleCompareRegion From(ProgramRegion) noexcept;

  ComparisonOperator Operator(void) const noexcept;

  // Variables in the left-hand side tuple.
  UsedNodeRange<DataVariable> LHS(void) const;

  // Variables in the right-hand side tuple.
  UsedNodeRange<DataVariable> RHS(void) const;

  // Code conditionally executed if the comparison is true.
  std::optional<ProgramRegion> BodyIfTrue(void) const noexcept;

  // Code conditionally executed if the comparison is false.
  std::optional<ProgramRegion> BodyIfFalse(void) const noexcept;

 private:
  friend class ProgramRegion;

  using Node<ProgramTupleCompareRegion, ProgramTupleCompareRegionImpl>::Node;
};

// Publishes a message to the pub/sub.
class ProgramPublishRegionImpl;
class ProgramPublishRegion : public Node<ProgramPublishRegion, ProgramPublishRegionImpl> {
 public:
  static ProgramPublishRegion From(ProgramRegion) noexcept;

  // Return a unique ID for this region.
  unsigned Id(void) const noexcept;

  ParsedMessage Message(void) const noexcept;

  // List of variables being published.
  UsedNodeRange<DataVariable> VariableArguments(void) const;

  // Are we publishing the removal of some tuple?
  bool IsRemoval(void) const noexcept;

 private:
  friend class ProgramRegion;

  using Node<ProgramPublishRegion, ProgramPublishRegionImpl>::Node;
};

enum class ProcedureKind : unsigned {

  // Function to initialize all relations. If any relation takes a purely
  // constant tuple as input, then this function initializes those flows.
  kInitializer,

  // This is the initial data flow function, which gets us into the primary
  // data flow function. The purpose of the separation is that the initial
  // data flow function collects the init induction vectors induced by message
  // receipts, whereas the primary dataflow function pushes forward all
  // computations from that point forward.
  kEntryDataFlowFunc,

  // The primary function that executes most data flows.
  kPrimaryDataFlowFunc,

  // Process an input vector of zero-or-more tuples received from the
  // network. This is a kind of bottom-up execution of the dataflow.
  kMessageHandler,

  // A query message forcing function, i.e. a function that internally sends
  // a message given the bound parameters of a query.
  kQueryMessageInjector,
};

// A procedure in the program. All procedures return either `true` or `false`.
class ProgramProcedureImpl;
class ProgramProcedure : public Node<ProgramProcedure, ProgramProcedureImpl> {
 public:
  // Return the procedure containing another region.
  static ProgramProcedure Containing(ProgramRegion region) noexcept;

  // Unique ID of this procedure.
  unsigned Id(void) const noexcept;

  // What type of procedure is this?
  ProcedureKind Kind(void) const noexcept;

  // The message received and handled by this procedure.
  std::optional<ParsedMessage> Message(void) const noexcept;

  // Zero or more input vectors on which this procedure operates.
  DefinedNodeRange<DataVector> VectorParameters(void) const;

  // Zero or more input variables on which this procedure operates.
  DefinedNodeRange<DataVariable> VariableParameters(void) const;

  // Zero or more vectors on which this procedure operates.
  DefinedNodeRange<DataVector> DefinedVectors(void) const;

  // Return the region contained by this procedure.
  ProgramRegion Body(void) const noexcept;

 private:
  friend class ProgramCallRegion;

  using Node<ProgramProcedure, ProgramProcedureImpl>::Node;
};

// Calls another IR procedure. All IR procedures return `true` or `false`. This
// return value can be tested, and if it is, a body can be conditionally
// executed based off of the result of that test.
class ProgramCallRegionImpl;
class ProgramCallRegion : public Node<ProgramCallRegion, ProgramCallRegionImpl> {
 public:
  static ProgramCallRegion From(ProgramRegion) noexcept;

  unsigned Id(void) const noexcept;

  ProgramProcedure CalledProcedure(void) const noexcept;

  // List of variables passed as arguments to the procedure.
  UsedNodeRange<DataVariable> VariableArguments(void) const;

  // List of vectors passed as arguments to the procedure.
  UsedNodeRange<DataVector> VectorArguments(void) const;

  // Conditionally executed body, based on how the return value of the procedure
  // being `true`.
  std::optional<ProgramRegion> BodyIfTrue(void) const noexcept;

  // Conditionally executed body, based on how the return value of the procedure
  // being `false`.
  std::optional<ProgramRegion> BodyIfFalse(void) const noexcept;

 private:
  friend class ProgramRegion;

  using Node<ProgramCallRegion, ProgramCallRegionImpl>::Node;
};

// Returns `true` or `false` from a procedure.
class ProgramReturnRegionImpl;
class ProgramReturnRegion : public Node<ProgramReturnRegion, ProgramReturnRegionImpl> {
 public:
  static ProgramReturnRegion From(ProgramRegion) noexcept;

  bool ReturnsTrue(void) const noexcept;
  bool ReturnsFalse(void) const noexcept;

 private:
  friend class ProgramRegion;

  using Node<ProgramReturnRegion, ProgramReturnRegionImpl>::Node;
};

// Computes a worker ID from a set of variables.
class ProgramWorkerIdRegionImpl;
class ProgramWorkerIdRegion
    : public Node<ProgramWorkerIdRegion, ProgramWorkerIdRegionImpl> {
 public:
  static ProgramWorkerIdRegion From(ProgramRegion) noexcept;

  // Body, executed in a context where the worker ID has been computed.
  std::optional<ProgramRegion> Body(void) const noexcept;

  // List of variables that will be mixed and hashed together to compute
  // the worker id.
  UsedNodeRange<DataVariable> HashedVariables(void) const;

  // Computed output that is a worker ID. These are 16 bit integers.
  DataVariable WorkerId(void) const;

 private:
  friend class ProgramRegion;

  using Node<ProgramWorkerIdRegion, ProgramWorkerIdRegionImpl>::Node;
};

// Specifies a relationship between a `#query` declaration, backing storage and
// various procedures needed to fill or check that storage. This information is
// sufficient for code generators to implement a notion of queries without
// imposing or prescribing a particular implementation in the control-flow IR.
class ProgramQuery {
 public:
  // The specific `#query` declaration associated with this entry point.
  ParsedQuery query;

  // The backing table storing the data that is being queried.
  DataTable table;

  // The index that must be scanned using any `bound`-attributed parameters
  // of the query declaration.
  std::optional<DataIndex> index;

  // If present, a procedure which must be invoked in order to ensure the
  // presence of any backing data. The parameters to this procedure are any
  // `bound`-attributed parameters of the query declaration.
  std::optional<ProgramProcedure> forcing_function;

  inline explicit ProgramQuery(
      ParsedQuery query_, DataTable table_, std::optional<DataIndex> index_,
      std::optional<ProgramProcedure> forcing_function_)
      : query(query_),
        table(table_),
        index(std::move(index_)),
        forcing_function(std::move(forcing_function_)) {}

  ProgramQuery(const ProgramQuery &) = default;
  ProgramQuery(ProgramQuery &&) noexcept = default;
};

// A program in its entirety.
class Program {
 public:
  // Build a program from a query. Data-flow views whose kinds the control-flow
  // builder does not support (aggregates, KV indices, impure functors,
  // differential cross-products) are reported to `log` and yield
  // `std::nullopt`. When `optimize` is `false`, the control-flow IR is built
  // but region-level optimization (flattening, no-op removal, procedure
  // deduplication) is skipped.
  static std::optional<Program> Build(const Query &query, const ErrorLog &log,
                                      unsigned first_id=0, bool optimize=true);

  // All persistent tables needed to store data.
  DefinedNodeRange<DataTable> Tables(void) const;

  // List of all global constants.
  DefinedNodeRange<DataVariable> Constants(void) const;

  // List of all global variables.
  DefinedNodeRange<DataVariable> GlobalVariables(void) const;

  // List of all procedures.
  DefinedNodeRange<ProgramProcedure> Procedures(void) const;

  // List of all table join regions in a program. It is convenient to provide
  // access to these to enable code generators to create specialized, per-join
  // code, e.g. that selects an optimal evaluation order at runtime.
  DefinedNodeRange<ProgramTableJoinRegion> JoinRegions(void) const;

  // List of query entry points.
  const std::vector<ProgramQuery> &Queries(void) const noexcept;

  // Return the query used to build this program.
  ::hyde::Query Query(void) const noexcept;

  // Return the parsed module used to build the query.
  ::hyde::ParsedModule ParsedModule(void) const noexcept;

  ~Program(void);

  Program(const Program &) = default;
  Program(Program &&) noexcept = default;
  Program &operator=(const Program &) = default;
  Program &operator=(Program &&) noexcept = default;

 private:
  Program(std::shared_ptr<ProgramImpl> impl_);

  std::shared_ptr<ProgramImpl> impl;
};

// `ProgramRegion` instances have an `Accept` method that will dispatch to the
// appropriate method in this class.
//
// NOTE(brad): This class only does dispatching, it doesn't do traversal.
class ProgramVisitor {
 public:
  virtual ~ProgramVisitor(void);

  virtual void Visit(ProgramCallRegion val);
  virtual void Visit(ProgramReturnRegion val);
  virtual void Visit(ProgramTestAndSetRegion val);
  virtual void Visit(ProgramGenerateRegion val);
  virtual void Visit(ProgramInductionRegion val);
  virtual void Visit(ProgramLetBindingRegion val);
  virtual void Visit(ProgramParallelRegion val);
  virtual void Visit(ProgramProcedure val);
  virtual void Visit(ProgramPublishRegion val);
  virtual void Visit(ProgramSeriesRegion val);
  virtual void Visit(ProgramVectorAppendRegion val);
  virtual void Visit(ProgramVectorClearRegion val);
  virtual void Visit(ProgramVectorLoopRegion val);
  virtual void Visit(ProgramVectorSwapRegion val);
  virtual void Visit(ProgramVectorUniqueRegion val);
  virtual void Visit(ProgramUpdateCountRegion val);
  virtual void Visit(ProgramChangeRecordRegion val);
  virtual void Visit(ProgramCheckMemberRegion val);
  virtual void Visit(ProgramCheckRecordRegion val);
  virtual void Visit(ProgramCommitSweepRegion val);
  virtual void Visit(ProgramClaimRegion val);
  virtual void Visit(ProgramRetireRegion val);
  virtual void Visit(ProgramNetBatchRegion val);
  virtual void Visit(ProgramTableJoinRegion val);
  virtual void Visit(ProgramTableProductRegion val);
  virtual void Visit(ProgramTableScanRegion val);
  virtual void Visit(ProgramTupleCompareRegion val);
  virtual void Visit(ProgramWorkerIdRegion val);
};

}  // namespace hyde
namespace std {

template <>
struct hash<::hyde::DataVariable> {
  using argument_type = ::hyde::DataVariable;
  using result_type = unsigned;
  inline unsigned operator()(::hyde::DataVariable var) const noexcept {
    return var.Id();
  }
};

template <>
struct hash<::hyde::DataVector> {
  using argument_type = ::hyde::DataVector;
  using result_type = unsigned;
  inline unsigned operator()(::hyde::DataVector vec) const noexcept {
    return vec.Id();
  }
};

template <>
struct hash<::hyde::DataColumn> {
  using argument_type = ::hyde::DataColumn;
  using result_type = unsigned;
  inline unsigned operator()(::hyde::DataColumn col) const noexcept {
    return col.Id();
  }
};

template <>
struct hash<::hyde::DataTable> {
  using argument_type = ::hyde::DataTable;
  using result_type = unsigned;
  inline unsigned operator()(::hyde::DataTable table) const noexcept {
    return table.Id();
  }
};

template <>
struct hash<::hyde::DataIndex> {
  using argument_type = ::hyde::DataIndex;
  using result_type = unsigned;
  inline unsigned operator()(::hyde::DataIndex index) const noexcept {
    return index.Id();
  }
};

}  // namespace std
