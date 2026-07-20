// Copyright 2026, Peter Goodman. All rights reserved.
// Copyright 2020, Trail of Bits. All rights reserved.

#pragma once

#include <drlojekyll/Util/PassPolicy.h>
#include <drlojekyll/ControlFlow/Program.h>
#include <drlojekyll/DataFlow/Query.h>
#include <drlojekyll/Lex/Token.h>
#include <drlojekyll/Parse/Parse.h>
#include <drlojekyll/Parse/Type.h>
#include <drlojekyll/Util/DefUse.h>
#include <drlojekyll/Util/DisjointSet.h>
#include <drlojekyll/Util/EqualitySet.h>

#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#define NOTE(msg) std::cerr << msg << "\n"

#define COMMENT(...) __VA_ARGS__

namespace std {

template <>
struct hash<std::pair<::hyde::QueryView, ::hyde::QueryView>> {
  using argument_type = std::pair<::hyde::QueryView, ::hyde::QueryView>;
  using result_type = uint64_t;
  inline uint64_t operator()(argument_type views) const noexcept {
    return (views.first.UniqueId() * views.second.Hash()) ^
           views.second.UniqueId();
  }
};

}  // namespace std
namespace hyde {

// Deterministic, pointer-free ordering for view-keyed containers whose
// iteration order reaches emitted output (the (F) determinism fix).
// CAUTION: `QueryView`'s own `operator<`, `UniqueId()`, and wrapper
// `Hash()` are all IMPL-POINTER-derived (Util/Node.h) — a plain
// `std::map<QueryView, ...>` or `std::set<QueryView>` is pointer-ordered
// and varies run to run. Key emission-visible maps with this comparator.
struct OrderQueryViews {
  inline bool operator()(QueryView a, QueryView b) const noexcept {
    return a.DeterministicOrder() < b.DeterministicOrder();
  }
};

template <typename V>
using OrderedViewMap = std::map<QueryView, V, OrderQueryViews>;

class ProgramInductionRegion;
class ProgramOperationRegion;
class ProgramVisitor;

class DataColumnImpl;
class DataIndexImpl;
class DataTableImpl;
class DataRecordImpl;
class DataRecordCaseImpl;

class ProgramImpl;

// A column within a table.
class DataColumnImpl final : public Def<DataColumnImpl>, public User {
 public:
  virtual ~DataColumnImpl(void);

  DataColumnImpl(unsigned id_, const TypeLoc &type_, DataTableImpl *table_);

  void Accept(ProgramVisitor &visitor);

  const unsigned id;
  const unsigned index;
  const TypeLoc type;

  std::vector<Token> names;

  WeakUseRef<DataTableImpl> table;
};

using TABLECOLUMN = DataColumnImpl;

// Represents an index on some subset of columns in a table.
class DataIndexImpl final : public Def<DataIndexImpl>, public User {
 public:
  virtual ~DataIndexImpl(void);

  DataIndexImpl(unsigned id_, DataTableImpl *table_, std::string column_spec_);

  void Accept(ProgramVisitor &visitor);

  const unsigned id;
  const std::string column_spec;

  UseList<TABLECOLUMN> columns;
  UseList<TABLECOLUMN> mapped_columns;

  WeakUseRef<DataTableImpl> table;
};

using TABLEINDEX = DataIndexImpl;

class Context;

// Represents a table of data.
//
// NOTE(pag): By default all tables already have a UNIQUE index on them.
class DataTableImpl final : public Def<DataTableImpl>, public User {
 public:
  virtual ~DataTableImpl(void);

  DataTableImpl(unsigned id_);

  void Accept(ProgramVisitor &visitor);

  // Get or create a table in the program.
  static DataTableImpl *GetOrCreate(ProgramImpl *impl, Context &context,
                                    QueryView view);

  // Get or create an index on the table.
  TABLEINDEX *GetOrCreateIndex(ProgramImpl *impl, std::vector<unsigned> cols);

  const unsigned id;

  // List of defined columns.
  DefList<TABLECOLUMN> columns;

  // Indexes that should be created on this table. By default, all tables have
  // a UNIQUE index.
  DefList<TABLEINDEX> indices;

  // Records associated with this table.
  DefList<DataRecordImpl> records;

  // All views sharing this table.
  std::vector<QueryView> views;

  // Whether this table backs a unit relation modeling a zero-arity predicate
  // (a condition): its only possible row is `(true)`.
  bool is_condition{false};
};

using TABLE = DataTableImpl;

struct DataModel : public DisjointSet {
 public:
  TABLE *table{nullptr};
};

// A vector of tuples in the program.
class DataVectorImpl final : public Def<DataVectorImpl> {
 public:
  DataVectorImpl(unsigned id_, VectorKind kind_,
                 const std::vector<TypeLoc> &col_types_, int)
      : Def<DataVectorImpl>(this),
        id(id_),
        kind(kind_),
        col_types(col_types_) {}

  template <typename ColList>
  DataVectorImpl(unsigned id_, VectorKind kind_, ColList &&cols)
      : Def<DataVectorImpl>(this),
        id(id_),
        kind(kind_) {

    for (QueryColumn col : cols) {
      col_types.push_back(col.Type());
    }
  }

  DataVectorImpl(DataVectorImpl *that_)
      : Def<DataVectorImpl>(this),
        id(that_->id),
        kind(that_->kind),
        col_types(that_->col_types) {}

  bool IsRead(void) const;

  void Accept(ProgramVisitor &visitor);

  const unsigned id;
  const VectorKind kind;
  std::vector<TypeLoc> col_types;

  std::optional<ParsedMessage> added_message;
  std::optional<ParsedMessage> removed_message;

  // `true` if this vector must have variants of itself sharded across workers.
  bool is_sharded{false};
};

using VECTOR = DataVectorImpl;

// A variable in the program. This could be a procedure parameter or a local
// variable.
class DataVariableImpl final : public Def<DataVariableImpl> {
 public:
  explicit DataVariableImpl(unsigned id_, VariableRole role_);

  void Accept(ProgramVisitor &visitor);

  const VariableRole role;

  // Unique ID for this variable. Unrelated to column IDs.
  const unsigned id;

  // NOTE(pag): Only valid after optimization when building the control flow
  //            IR is complete.
  ProgramRegionImpl *defining_region{nullptr};

  inline unsigned Sort(void) const noexcept {
    return id;
  }

  bool IsGlobal(void) const noexcept;

  bool IsConstant(void) const noexcept;

  TypeLoc Type(void) const noexcept;

  std::optional<QueryConstant> query_const;
  std::optional<QueryColumn> query_column;
  std::optional<ParsedParameter> parsed_param;
};

using VAR = DataVariableImpl;

struct RecordColumn {
  RecordColumn(void) = default;

  RecordColumn &operator=(RecordColumn &&that) noexcept {
    derived_index = that.derived_index;
    derived_offset = that.derived_offset;
    column.Swap(that.column);
    var.Swap(that.var);
    return *this;
  }

  RecordColumn(RecordColumn &&that) noexcept
      : derived_index(that.derived_index),
        derived_offset(that.derived_offset) {
    column.Swap(that.column);
    var.Swap(that.var);
  }

  // If this column is derived from another record, then what index in
  // `derived_from` is it from, and then what offset (column) within that
  // record.
  unsigned derived_index{~0u};
  unsigned derived_offset{~0u};

  UseRef<TABLECOLUMN> column;
  UseRef<VAR> var;
};

// A record case is a particular instantiation or variant of a record.
// A record might have multiple cases.
class DataRecordCaseImpl : public Def<DataRecordCaseImpl>, public User {
 public:
  DataRecordCaseImpl(unsigned id_);

  const unsigned id;

  UseList<DataRecordImpl> derived_from;

  std::vector<RecordColumn> columns;

  WeakUseRef<DataRecordImpl> record;

 private:
  DataRecordCaseImpl(void) = delete;
};

using DATARECORDCASE = DataRecordCaseImpl;

// A record is an abstraction over a persisted tuple. The storage for the
// record is implemented in terms of one or more cases.
class DataRecordImpl : public Def<DataRecordImpl>, public User {
 public:
  explicit DataRecordImpl(unsigned id_, TABLE *table_);

  const unsigned id;
  UseList<DATARECORDCASE> cases;
  WeakUseRef<TABLE> table;

 private:
  DataRecordImpl(void) = delete;
};

using DATARECORD = DataRecordImpl;

class ProgramOperationRegionImpl;

// A lexically scoped region in the program.
class ProgramRegionImpl : public Def<ProgramRegionImpl>, public User {
 public:
  virtual ~ProgramRegionImpl(void);
  explicit ProgramRegionImpl(ProgramProcedureImpl *containing_procedure_, bool);
  explicit ProgramRegionImpl(ProgramRegionImpl *parent_);

  virtual void Accept(ProgramVisitor &visitor) = 0;
  virtual uint64_t Hash(uint32_t depth) const = 0;

  virtual ProgramProcedureImpl *AsProcedure(void) noexcept;
  virtual ProgramOperationRegionImpl *AsOperation(void) noexcept;
  virtual ProgramSeriesRegionImpl *AsSeries(void) noexcept;
  virtual ProgramParallelRegionImpl *AsParallel(void) noexcept;
  virtual ProgramInductionRegionImpl *AsInduction(void) noexcept;

  // Returns `true` if all paths through `this` ends with a `return` region.
  virtual bool EndsWithReturn(void) const noexcept = 0;

  inline void ReplaceAllUsesWith(ProgramRegionImpl *that) {
    this->Def<ProgramRegionImpl>::ReplaceAllUsesWith(that);
    if (!this->AsProcedure()) {
      assert(!that->AsProcedure());
      that->parent = this->parent;
      this->parent = nullptr;
    }
  }

  // Returns 'true' if 'this' was able to merge all of the regions in 'merges'.
  // Merging takes the form of a new PARALLEL region to execute the bodies of
  // 'Equals' regions.
  // This method assumes that all elements in 'merges' are 'Equals' at depth 0.
  virtual const bool MergeEqual(ProgramImpl *prog,
                                std::vector<ProgramRegionImpl *> &merges);

  // Gets or creates a local variable in the procedure.
  DataVariableImpl *VariableFor(ProgramImpl *impl, QueryColumn col);
  DataVariableImpl *VariableForRec(QueryColumn col);

  // Returns the lexical level of this node.
  unsigned Depth(void) const noexcept;

  // Returns the lexical level of this node.
  unsigned CachedDepth(void) noexcept;

  // Returns `true` if this region is a no-op.
  virtual bool IsNoOp(void) const noexcept;

  // Returns `true` if `this` and `that` are structurally equivalent (after
  // variable renaming) after searching down `depth` levels or until leaf,
  // whichever is first, and where `depth` is 0, compare `this` to `that.
  virtual bool Equals(EqualitySet &eq, ProgramRegionImpl *that,
                      uint32_t depth) const noexcept;

  // Return the farthest ancestor of this region, in terms of linkage. Often this
  // just returns a `PROC *` if this region is linked in to its procedure.
  ProgramRegionImpl *Ancestor(void) noexcept;

  // Return the nearest enclosing region that is itself enclosed by an
  // induction.
  ProgramRegionImpl *NearestRegionEnclosedByInduction(void) noexcept;

  // Find an ancestor node that's both shared by `this` and `that`.
  ProgramRegionImpl *FindCommonAncestor(ProgramRegionImpl *that) noexcept;

  // Make sure that `this` will execute before `that`.
  void ExecuteBefore(ProgramImpl *program, ProgramRegionImpl *that) noexcept;

  // Make sure that `this` will execute after `that`.
  void ExecuteAfter(ProgramImpl *program, ProgramRegionImpl *that) noexcept;

  // Make sure that `this` will execute alongside `that`.
  void ExecuteAlongside(ProgramImpl *program,
                        ProgramRegionImpl *that) noexcept;

  // Every child REGION of a procedure will have easy access to create new
  // variables.
  ProgramProcedureImpl *containing_procedure;
  ProgramRegionImpl *parent{nullptr};

  // Maps `QueryColumn::Id()` values to variables. Used to provide lexical
  // scoping of variables.
  //
  // NOTE(pag): Only valid before optimization, during the building of the
  //            control flow IR.
  std::unordered_map<unsigned, VAR *> col_id_to_var;

  // A comment about the creation of this node.
  std::string comment;

  unsigned cached_depth{0};
};

using REGION = ProgramRegionImpl;

struct RegionRef : public UseRef<REGION> {
#ifndef NDEBUG
 private:
  REGION *const self;

 public:
  RegionRef(REGION *parent_) : self(parent_) {}
#else
  RegionRef(REGION *) {}
#endif

  void Emplace(REGION *parent, REGION *child) {
    assert(child->parent == self);
    this->UseRef<REGION>::Emplace(parent, child);
  }
};

enum class ProgramOperation {
  kInvalid,

  kClearVectorBeforePrimaryFlowFunction,

  // Used for queries that force the injection of an internal message to
  // "unlock" computation that the query can produce results for the query
  // to observe / report on.
  kAppendQueryParamsToMessageInjectVector,

  // One signed derivation-counter fold on a table (a +1/-1 of `C_nr` or
  // `C_r`); execution descends into `body` on a zero crossing. On a
  // monotone table this is an insert-if-new.
  kUpdateCount,
  kUpdateCountRecord,

  // Two-way membership gate on a table, naming one membership predicate:
  // `body` executes if the predicate holds, `absent_body` if it does not.
  kCheckMember,
  kCheckMemberRecord,

  // End-of-batch commit sweep of one differential table: publish net 0/1
  // presence changes, seal the batch-start snapshot, clear scratch flags.
  kCommitSweep,

  // R3 GROUP_UPDATE: fold one aggregate/KV view's input net frontiers into its
  // StateCell store, then emit the occupancy-generalized one-net-pair into the
  // aggregate's own differential table (v3-spec-statecell.md §2.2).
  kGroupUpdate,

  // When dealing with MERGE/UNION nodes with an inductive cycle.
  kAppendToInductionVector,
  kLoopOverInductionVector,
  kClearInductionVector,
  kSwapInductionVector,
  kSortAndUniqueInductionVector,

  // When dealing with a MERGE/UNION node that isn't part of an inductive
  // cycle.
  kAppendUnionInputToVector,
  kLoopOverUnionInputVector,
  kClearUnionInputVector,

  // Check if a row exists in a view. The tuple values are found in
  // `operands` and the table being used is `views[0]`.
  kCheckTupleIsPresentInTable,
  kCheckTupleIsNotPresentInTable,

  // Pairwise comparison between variables in two tuples.
  kCompareTuples,

  // Used by operations related to equi-joins over one or more tables.
  kJoinTables,
  kAppendJoinPivotsToVector,
  kSortAndUniquePivotVector,
  kClearJoinPivotVector,

  // Used to implement the cross-product of some tables.
  kCrossProduct,
  kAppendToProductInputVector,
  kSortAndUniqueProductInputVector,
  kClearProductInputVector,

  // Used to implement table/index scanning.
  kScanTable,
  kLoopOverScanVector,
  kClearScanVector,

  // Used to implement publication of messages that can be published with
  // additions or removals.
  kAppendToMessageOutputVector,
  kSortAndUniqueMessageOutputVector,
  kClearMessageOutputVector,
  kLoopOverMessageOutputVector,

  // Loop over a vector of inputs. The format of the vector is based off of
  // the variables in `variables`. The region `body` is executed for each
  // loop iteration.
  kLoopOverInputVector,

  // Call a filter functor, stored in `functor`, that is a functor where all
  // parameters are `bound`-attributed. These functors are interpreted as
  // predicates returning `true` or `false`. If `true` is returned, then
  // descend into `body`.
  kCallFilterFunctor,

  // Call a normal functor, stored in `functor`, where there is at least one
  // free parameter to the functor that it must generate. If anything is
  // generated, then descend into `body`.
  kCallFunctor,

  // Publish a message.
  kPublishMessage,
  kPublishMessageRemoval,

  // Creates a let binding, which assigns uses of variables to definitions of
  // variables. In practice, let bindings are eliminated during the process
  // of optimization.
  kLetBinding,

  // Computes a worker ID by hashing a bunch of variables.
  kWorkerId,

  // Used to test-and-update the init-guard variable that ensures the
  // initialization flow runs exactly once.
  kTestAndAdd,

  // Call another procedure.
  kCallProcedure,

  // Return from a procedure.
  kReturnTrueFromProcedure,
  kReturnFalseFromProcedure,

  // Claim a row into the overdeletion/addition set and the current
  // frontier round; `body` executes on the row's first claim this batch.
  kClaim,

  // Retire a row from the current frontier round (clear its kDelNow or
  // kAddNow bit).
  kRetire,

  // Net a differential message's explicit adds against its explicit
  // removes within one batch, rewriting both vectors.
  kNetBatchVectors,

  // TODO: use in future.

  // Test/set a global boolean variable to `true`. The variable is
  // `variables[0]`. The usage is for cross products. If we've ever executed a
  // lazy cross-product, then we must always visit it.
  kTestGlobalVariableIsTrue,
  kSetGlobalVariableToTrue,

  // Merge two values into an updated value when using `mutable`-attributed
  // parameters.
  kCallMergeFunctor,
};

// A generic operation.
class ProgramOperationRegionImpl : public REGION {
 public:
  virtual ~ProgramOperationRegionImpl(void);
  explicit ProgramOperationRegionImpl(REGION *parent_, ProgramOperation op_);

  virtual ProgramCallRegionImpl *AsCall(void) noexcept;
  virtual ProgramReturnRegionImpl *AsReturn(void) noexcept;
  virtual ProgramTestAndSetRegionImpl *AsTestAndSet(void) noexcept;
  virtual ProgramGenerateRegionImpl *AsGenerate(void) noexcept;
  virtual ProgramLetBindingRegionImpl *AsLetBinding(void) noexcept;
  virtual ProgramPublishRegionImpl *AsPublish(void) noexcept;
  virtual ProgramUpdateCountRegionImpl *AsUpdateCount(void) noexcept;
  virtual ProgramChangeRecordRegionImpl *AsChangeRecord(void) noexcept;
  virtual ProgramCheckMemberRegionImpl *AsCheckMember(void) noexcept;
  virtual ProgramCheckRecordRegionImpl *AsCheckRecord(void) noexcept;
  virtual ProgramCommitSweepRegionImpl *AsCommitSweep(void) noexcept;
  virtual ProgramGroupUpdateRegionImpl *AsGroupUpdate(void) noexcept;  // R3
  virtual ProgramClaimRegionImpl *AsClaim(void) noexcept;
  virtual ProgramRetireRegionImpl *AsRetire(void) noexcept;
  virtual ProgramNetBatchRegionImpl *AsNetBatch(void) noexcept;
  virtual ProgramTableJoinRegionImpl *AsTableJoin(void) noexcept;
  virtual ProgramTableProductRegionImpl *AsTableProduct(void) noexcept;
  virtual ProgramTableScanRegionImpl *AsTableScan(void) noexcept;
  virtual ProgramTupleCompareRegionImpl *AsTupleCompare(void) noexcept;
  virtual ProgramVectorLoopRegionImpl *AsVectorLoop(void) noexcept;
  virtual ProgramVectorAppendRegionImpl *AsVectorAppend(void) noexcept;
  virtual ProgramVectorClearRegionImpl *AsVectorClear(void) noexcept;
  virtual ProgramVectorSwapRegionImpl *AsVectorSwap(void) noexcept;
  virtual ProgramVectorUniqueRegionImpl *AsVectorUnique(void) noexcept;
  virtual ProgramWorkerIdRegionImpl *AsWorkerId(void) noexcept;

  ProgramOperationRegionImpl *AsOperation(void) noexcept override;

  // Returns `true` if all paths through `this` ends with a `return` region.
  bool EndsWithReturn(void) const noexcept override;

  ProgramOperation op;

  // If this operation does something conditional then this is the body it
  // executes.
  RegionRef body;
};

using OP = ProgramOperationRegionImpl;

// A let binding, i.e. an assignment of zero or more variables. Variables
// are assigned pairwise from `used_vars` into `defined_vars`.
class ProgramLetBindingRegionImpl final : public OP {
 public:
  virtual ~ProgramLetBindingRegionImpl(void);

  void Accept(ProgramVisitor &visitor) override;
  uint64_t Hash(uint32_t depth) const override;
  bool IsNoOp(void) const noexcept override;

  // Returns `true` if `this` and `that` are structurally equivalent (after
  // variable renaming).
  bool Equals(EqualitySet &eq, REGION *that,
              uint32_t depth) const noexcept override;

  const bool MergeEqual(ProgramImpl *prog,
                        std::vector<REGION *> &merges) override;

  inline ProgramLetBindingRegionImpl(REGION *parent_)
      : OP(parent_, ProgramOperation::kLetBinding),
        defined_vars(this),
        used_vars(this) {}

  ProgramLetBindingRegionImpl *AsLetBinding(void) noexcept override;

  // Local variables that are defined/used in the body of this procedure.
  DefList<VAR> defined_vars;
  UseList<VAR> used_vars;

  std::optional<QueryView> view;
};

using LET = ProgramLetBindingRegionImpl;

// Computes a worker ID by hashing one or more variables.
class ProgramWorkerIdRegionImpl final : public OP {
 public:
  virtual ~ProgramWorkerIdRegionImpl(void);

  void Accept(ProgramVisitor &visitor) override;
  uint64_t Hash(uint32_t depth) const override;
  bool IsNoOp(void) const noexcept override;

  // Returns `true` if `this` and `that` are structurally equivalent (after
  // variable renaming).
  bool Equals(EqualitySet &eq, REGION *that,
              uint32_t depth) const noexcept override;

  const bool MergeEqual(ProgramImpl *prog,
                        std::vector<REGION *> &merges) override;

  inline ProgramWorkerIdRegionImpl(REGION *parent_)
      : OP(parent_, ProgramOperation::kWorkerId),
        hashed_vars(this) {}

  ProgramWorkerIdRegionImpl *AsWorkerId(void) noexcept override;

  // Local variables that are hashed together to compute `worker_id`.
  UseList<VAR> hashed_vars;

  // Variable storing the hashed result.
  std::unique_ptr<VAR> worker_id;
};

using WORKERID = ProgramWorkerIdRegionImpl;

// Loop over the vector `vector` and bind the extracted tuple elements into
// the variables specified in `defined_vars`.
class ProgramVectorLoopRegionImpl final : public OP {
 public:
  virtual ~ProgramVectorLoopRegionImpl(void);

  void Accept(ProgramVisitor &visitor) override;

  inline ProgramVectorLoopRegionImpl(
      unsigned id_, REGION *parent_, ProgramOperation op_)
      : OP(parent_, op_),
        id(id_),
        defined_vars(this) {}

  uint64_t Hash(uint32_t depth) const override;
  bool IsNoOp(void) const noexcept override;

  // Returns `true` if `this` and `that` are structurally equivalent (after
  // variable renaming).
  bool Equals(EqualitySet &eq, REGION *that,
              uint32_t depth) const noexcept override;

  const bool MergeEqual(ProgramImpl *prog,
                        std::vector<REGION *> &merges) override;

  ProgramVectorLoopRegionImpl *AsVectorLoop(void) noexcept override;

  // ID of this region.
  const unsigned id;

  // Local variables bound to the vector being looped.
  DefList<VAR> defined_vars;

  // Vector being looped.
  UseRef<VECTOR> vector;

  // Optional ID of the target worker thread.
  UseRef<VAR> worker_id;

  // If this is a loop over a table associated with an induction vector.
  UseRef<TABLE> induction_table;
};

using VECTORLOOP = ProgramVectorLoopRegionImpl;

// Append a tuple into a vector. The elements in the tuple must match the
// element/column types of the vector.
class ProgramVectorAppendRegionImpl final : public OP {
 public:
  virtual ~ProgramVectorAppendRegionImpl(void);

  void Accept(ProgramVisitor &visitor) override;

  inline ProgramVectorAppendRegionImpl(REGION *parent_, ProgramOperation op_)
      : OP(parent_, op_),
        tuple_vars(this) {}

  uint64_t Hash(uint32_t depth) const override;

  // Returns `true` if `this` and `that` are structurally equivalent (after
  // variable renaming).
  bool Equals(EqualitySet &eq, REGION *that,
              uint32_t depth) const noexcept override;

  const bool MergeEqual(ProgramImpl *prog,
                        std::vector<REGION *> &merges) override;

  ProgramVectorAppendRegionImpl *AsVectorAppend(void) noexcept override;

  UseList<VAR> tuple_vars;
  UseRef<VECTOR> vector;

  // Optional ID of the target worker thread.
  UseRef<VAR> worker_id;
};

using VECTORAPPEND = ProgramVectorAppendRegionImpl;

// Clear a vector.
class ProgramVectorClearRegionImpl final : public OP {
 public:
  virtual ~ProgramVectorClearRegionImpl(void);

  using OP::ProgramOperationRegionImpl;

  void Accept(ProgramVisitor &visitor) override;

  uint64_t Hash(uint32_t depth) const override;

  // Returns `true` if `this` and `that` are structurally equivalent (after
  // variable renaming).
  bool Equals(EqualitySet &eq, REGION *that,
              uint32_t depth) const noexcept override;

  const bool MergeEqual(ProgramImpl *prog,
                        std::vector<REGION *> &merges) override;

  ProgramVectorClearRegionImpl *AsVectorClear(void) noexcept override;

  UseRef<VECTOR> vector;

  // Optional ID of the target worker thread.
  UseRef<VAR> worker_id;
};

using VECTORCLEAR = ProgramVectorClearRegionImpl;

// Swap the contents of two vectors.
class ProgramVectorSwapRegionImpl final : public OP {
 public:
  virtual ~ProgramVectorSwapRegionImpl(void);
  using OP::ProgramOperationRegionImpl;

  void Accept(ProgramVisitor &visitor) override;

  uint64_t Hash(uint32_t depth) const override;

  // Returns `true` if `this` and `that` are structurally equivalent (after
  // variable renaming).
  bool Equals(EqualitySet &eq, REGION *that,
              uint32_t depth) const noexcept override;

  const bool MergeEqual(ProgramImpl *prog,
                        std::vector<REGION *> &merges) override;
  ProgramVectorSwapRegionImpl *AsVectorSwap(void) noexcept override;

  UseRef<VECTOR> lhs;
  UseRef<VECTOR> rhs;
};

using VECTORSWAP = ProgramVectorSwapRegionImpl;

// Sort and unique a vector.
class ProgramVectorUniqueRegionImpl final : public OP {
 public:
  virtual ~ProgramVectorUniqueRegionImpl(void);
  using OP::ProgramOperationRegionImpl;

  void Accept(ProgramVisitor &visitor) override;

  uint64_t Hash(uint32_t depth) const override;

  // Returns `true` if `this` and `that` are structurally equivalent (after
  // variable renaming).
  bool Equals(EqualitySet &eq, REGION *that,
              uint32_t depth) const noexcept override;

  const bool MergeEqual(ProgramImpl *prog,
                        std::vector<REGION *> &merges) override;

  ProgramVectorUniqueRegionImpl *AsVectorUnique(void) noexcept override;

  UseRef<VECTOR> vector;

  // Optional ID of the target worker thread.
  UseRef<VAR> worker_id;
};

using VECTORUNIQUE = ProgramVectorUniqueRegionImpl;

// One signed derivation-counter fold on a table: a single +1/-1 of the
// row's `C_nr` or `C_r` counter, applied inline at a rule-firing site
// (multiset discipline: one fold per rule-instance firing). `body` executes
// only on a zero-crossing event. On a monotone table the fold degenerates
// to an insert-if-new whose crossing is "the row is new".
class ProgramUpdateCountRegionImpl final : public OP {
 public:
  virtual ~ProgramUpdateCountRegionImpl(void);

  inline ProgramUpdateCountRegionImpl(REGION *parent_, bool is_add_,
                                      DerivClass deriv_class_,
                                      bool is_explicit_)
      : OP(parent_, ProgramOperation::kUpdateCount),
        col_values(this),
        is_add(is_add_),
        deriv_class(deriv_class_),
        is_explicit(is_explicit_) {}

  void Accept(ProgramVisitor &visitor) override;

  ProgramUpdateCountRegionImpl *AsUpdateCount(void) noexcept override;

  uint64_t Hash(uint32_t depth) const override;
  bool IsNoOp(void) const noexcept override;

  // Returns `true` if `this` and `that` are structurally equivalent (after
  // variable renaming).
  bool Equals(EqualitySet &eq, REGION *that,
              uint32_t depth) const noexcept override;

  const bool MergeEqual(ProgramImpl *prog,
                        std::vector<REGION *> &merges) override;

  // Variables that make up the tuple.
  UseList<VAR> col_values;

  // Table whose counter is being folded.
  UseRef<TABLE> table;

  // Fold sign: `true` adds a derivation, `false` retracts one.
  const bool is_add;

  // Which counter the fold lands on: `kNonRecursive` (C_nr) for
  // seed/init-position firings, `kRecursive` (C_r) for fixpoint back-edge
  // firings.
  const DerivClass deriv_class;

  // Whether the fold is explicit (message) support: the row's `kExplicit`
  // set-semantics bit rather than a multiset derivation count. Only
  // meaningful on differential tables.
  const bool is_explicit;
};

using UPDATECOUNT = ProgramUpdateCountRegionImpl;

// This is similar to a `ProgramUpdateCountRegion`; however, it also
// creates new definitions for the variables which it is updating. The key
// idea is that this gets us the "record" associated with some tuple data,
// rather than us keeping with the tuple data itself.
class ProgramChangeRecordRegionImpl final : public OP {
 public:
  virtual ~ProgramChangeRecordRegionImpl(void);

  inline ProgramChangeRecordRegionImpl(unsigned id_, REGION *parent_,
                                       bool is_add_, DerivClass deriv_class_)
      : OP(parent_, ProgramOperation::kUpdateCountRecord),
        id(id_),
        col_values(this),
        record_vars(this),
        is_add(is_add_),
        deriv_class(deriv_class_) {}

  void Accept(ProgramVisitor &visitor) override;

  ProgramChangeRecordRegionImpl *AsChangeRecord(void) noexcept override;

  uint64_t Hash(uint32_t depth) const override;
  bool IsNoOp(void) const noexcept override;

  // Returns `true` if `this` and `that` are structurally equivalent (after
  // variable renaming).
  bool Equals(EqualitySet &eq, REGION *that,
              uint32_t depth) const noexcept override;

  const bool MergeEqual(ProgramImpl *prog,
                        std::vector<REGION *> &merges) override;

  const unsigned id;

  // Variables that make up the tuple.
  UseList<VAR> col_values;

  // Output record variables from this fold.
  DefList<VAR> record_vars;

  // Table whose counter is being folded.
  UseRef<TABLE> table;

  // Fold sign: `true` adds a derivation, `false` retracts one.
  const bool is_add;

  // Which counter the fold lands on.
  const DerivClass deriv_class;
};

using CHANGERECORD = ProgramChangeRecordRegionImpl;

// A strictly two-way membership gate on a table, naming one membership
// predicate explicitly: `body` executes if the predicate holds for the
// tuple, `absent_body` if it does not. On a monotone table the gate is
// simply row existence.
class ProgramCheckMemberRegionImpl final : public OP {
 public:
  virtual ~ProgramCheckMemberRegionImpl(void);

  inline ProgramCheckMemberRegionImpl(REGION *parent_,
                                      MembershipPredicate predicate_)
      : OP(parent_, ProgramOperation::kCheckMember),
        col_values(this),
        absent_body(this),
        predicate(predicate_) {}

  void Accept(ProgramVisitor &visitor) override;
  uint64_t Hash(uint32_t depth) const override;
  bool IsNoOp(void) const noexcept override;

  ProgramCheckMemberRegionImpl *AsCheckMember(void) noexcept override;

  // Returns `true` if all paths through `this` ends with a `return` region.
  bool EndsWithReturn(void) const noexcept override;

  // Returns `true` if `this` and `that` are structurally equivalent (after
  // variable renaming).
  bool Equals(EqualitySet &eq, REGION *that,
              uint32_t depth) const noexcept override;

  const bool MergeEqual(ProgramImpl *prog,
                        std::vector<REGION *> &merges) override;

  // Variables that make up the tuple.
  UseList<VAR> col_values;

  // Table whose membership is being read.
  UseRef<TABLE> table;

  // Region that is conditionally executed if the predicate does not hold.
  RegionRef absent_body;

  // The membership predicate this gate reads.
  const MembershipPredicate predicate;
};

using CHECKMEMBER = ProgramCheckMemberRegionImpl;

// This is like `ProgramCheckMemberRegion`, except that it operates on
// records, i.e. it defines new variables for what is being returned.
class ProgramCheckRecordRegionImpl final : public OP {
 public:
  virtual ~ProgramCheckRecordRegionImpl(void);

  inline ProgramCheckRecordRegionImpl(unsigned id_, REGION *parent_,
                                      MembershipPredicate predicate_)
      : OP(parent_, ProgramOperation::kCheckMemberRecord),
        id(id_),
        col_values(this),
        record_vars(this),
        absent_body(this),
        predicate(predicate_) {}

  void Accept(ProgramVisitor &visitor) override;
  uint64_t Hash(uint32_t depth) const override;
  bool IsNoOp(void) const noexcept override;

  ProgramCheckRecordRegionImpl *AsCheckRecord(void) noexcept override;

  // Returns `true` if all paths through `this` ends with a `return` region.
  bool EndsWithReturn(void) const noexcept override;

  // Returns `true` if `this` and `that` are structurally equivalent (after
  // variable renaming).
  bool Equals(EqualitySet &eq, REGION *that,
              uint32_t depth) const noexcept override;

  const bool MergeEqual(ProgramImpl *prog,
                        std::vector<REGION *> &merges) override;

  const unsigned id;

  // Variables that make up the tuple.
  UseList<VAR> col_values;

  // Defined variables from the record.
  DefList<VAR> record_vars;

  // Table whose membership is being read.
  UseRef<TABLE> table;

  // Region that is conditionally executed if the predicate does not hold.
  RegionRef absent_body;

  // The membership predicate this gate reads.
  const MembershipPredicate predicate;
};

using CHECKRECORD = ProgramCheckRecordRegionImpl;

// The end-of-batch commit sweep of one differential table: publishes each
// touched row's net 0/1 presence change (against the batch-start snapshot)
// through `message` when the table backs a `@differential` transmit view,
// seals the new snapshot, and clears the batch-scratch flags.
class ProgramCommitSweepRegionImpl final : public OP {
 public:
  virtual ~ProgramCommitSweepRegionImpl(void);

  inline ProgramCommitSweepRegionImpl(REGION *parent_)
      : OP(parent_, ProgramOperation::kCommitSweep) {}

  void Accept(ProgramVisitor &visitor) override;
  uint64_t Hash(uint32_t depth) const override;
  bool IsNoOp(void) const noexcept override;

  ProgramCommitSweepRegionImpl *AsCommitSweep(void) noexcept override;

  // Returns `true` if `this` and `that` are structurally equivalent (after
  // variable renaming).
  bool Equals(EqualitySet &eq, REGION *that,
              uint32_t depth) const noexcept override;

  const bool MergeEqual(ProgramImpl *prog,
                        std::vector<REGION *> &merges) override;

  // Table being committed.
  UseRef<TABLE> table;

  // The `@differential` message fed by this table, if any: net presence
  // crossings publish through it.
  std::optional<ParsedMessage> message;

  // R3: if this table is an aggregate's own DiffTable (V-AGG-SOLE), the id of
  // the StateCell store to Seal at the commit tail — sealed := Emit(working)
  // for touched groups (STATE_SEAL, spec §2.3 E5). std::nullopt otherwise.
  std::optional<unsigned> seal_statecell_id;
};

using COMMITSWEEP = ProgramCommitSweepRegionImpl;

// R3 GROUP_UPDATE (v3-spec-statecell.md §2.2): the emitted lowering of ONE
// QueryAggregate / desugared QueryKVIndex view. It is the SOLE deriver of its
// aggregate's differential DiffTable (V-AGG-SOLE). The region is self-contained
// (no membership-check partner read) and its codegen writes the whole two-band
// body directly: BAND (a) frontier_in — two VECTORLOOPs over the input's net
// removal / net addition frontier vecs, each folding the projected (group,
// summary) into the StateCell store (`statecell_id`, `algebra`); BAND (b)
// emit_touched — a loop over the store's sort-unique touched set applying the
// occupancy-generalized one-net-pair guard (spec §C-1) and folding ± into the
// agg table's derivation counters + appending to its delete/add queues.
//
// The per-arm projections (which delta-row columns are the group key vs the
// summary value) are carried as parallel var lists; the emit_touched counter
// folds reuse the ordinary UPDATECOUNT machinery, so the agg table's whole
// downstream claim/frontier/commit tail is identity to every acyclic table.
class ProgramGroupUpdateRegionImpl final : public OP {
 public:
  virtual ~ProgramGroupUpdateRegionImpl(void);

  inline ProgramGroupUpdateRegionImpl(REGION *parent_, unsigned statecell_id_,
                                      bool invertible_)
      : OP(parent_, ProgramOperation::kGroupUpdate),
        statecell_id(statecell_id_),
        invertible(invertible_) {}

  void Accept(ProgramVisitor &visitor) override;
  uint64_t Hash(uint32_t depth) const override;
  bool IsNoOp(void) const noexcept override;

  ProgramGroupUpdateRegionImpl *AsGroupUpdate(void) noexcept override;

  bool Equals(EqualitySet &eq, REGION *that,
              uint32_t depth) const noexcept override;

  // Input net-removal / net-addition frontier vectors (BAND (a) drains). Both
  // arms drain the summarized input's frontier and fold the projected
  // (group, summary) into the StateCell store.
  UseRef<VECTOR> neg_frontier;
  UseRef<VECTOR> pos_frontier;

  // The agg table's delete / add queues (emit_touched appends, BAND (b)).
  UseRef<VECTOR> del_queue;
  UseRef<VECTOR> add_queue;

  // The aggregate's OWN differential table (sole-deriver).
  UseRef<TABLE> agg_table;

  // The fold-arm projection: positions into the frontier row (== the input
  // view's column order) that are the group-key columns and the summary
  // columns. Both arms share the same shape (the input is one view). Codegen
  // destructures the drained row into positional locals and folds
  // (group_positions -> key, summary_positions -> value).
  std::vector<unsigned> group_positions;
  std::vector<unsigned> summary_positions;

  // P2c: how many of the trailing `group_positions` are CONFIGURATION columns
  // (the tail beyond the true group-by columns; `group_positions = group ++
  // config`). For an @invertible cell EmitGroupUpdate passes these config
  // frontier locals as leading args to `Fold`; 0 for a config-free aggregate
  // or a KV index.
  unsigned num_config_positions{0u};

  // Index into the program's state-cell descriptor list (codegen names the
  // matching `statecell_<id>` store member and its `Reduce_*`/`Key_*` types).
  const unsigned statecell_id;

  // `true` iff @invertible (O(1) fold/unfold); `false` iff @recompute.
  const bool invertible;
};

using GROUPUPDATE = ProgramGroupUpdateRegionImpl;

// A claim of one row of a differential table into the overdeletion set
// (`is_del`) or the addition set, and into the current frontier round.
// `body` executes only when the claim succeeds, i.e. on the row's first
// claim this batch. The claim mutates the row's batch-scratch flags, so
// the region is a side effect even when the body is gone.
class ProgramClaimRegionImpl final : public OP {
 public:
  virtual ~ProgramClaimRegionImpl(void);

  inline ProgramClaimRegionImpl(REGION *parent_, bool is_del_)
      : OP(parent_, ProgramOperation::kClaim),
        col_values(this),
        is_del(is_del_) {}

  void Accept(ProgramVisitor &visitor) override;

  ProgramClaimRegionImpl *AsClaim(void) noexcept override;

  uint64_t Hash(uint32_t depth) const override;
  bool IsNoOp(void) const noexcept override;

  // Returns `true` if `this` and `that` are structurally equivalent (after
  // variable renaming).
  bool Equals(EqualitySet &eq, REGION *that,
              uint32_t depth) const noexcept override;

  const bool MergeEqual(ProgramImpl *prog,
                        std::vector<REGION *> &merges) override;

  // Variables that make up the tuple.
  UseList<VAR> col_values;

  // Table whose row is being claimed.
  UseRef<TABLE> table;

  // Claim target: `true` claims into the overdeletion set (the delete
  // frontier), `false` into the addition set (the insert frontier).
  const bool is_del;
};

using CLAIM = ProgramClaimRegionImpl;

// A retirement of one row of a differential table from the current
// frontier round: clears the row's current-frontier bit (`kDelNow` when
// `is_del`, `kAddNow` otherwise). Has no body.
class ProgramRetireRegionImpl final : public OP {
 public:
  virtual ~ProgramRetireRegionImpl(void);

  inline ProgramRetireRegionImpl(REGION *parent_, bool is_del_)
      : OP(parent_, ProgramOperation::kRetire),
        col_values(this),
        is_del(is_del_) {}

  void Accept(ProgramVisitor &visitor) override;

  ProgramRetireRegionImpl *AsRetire(void) noexcept override;

  uint64_t Hash(uint32_t depth) const override;
  bool IsNoOp(void) const noexcept override;

  // Returns `true` if `this` and `that` are structurally equivalent (after
  // variable renaming).
  bool Equals(EqualitySet &eq, REGION *that,
              uint32_t depth) const noexcept override;

  const bool MergeEqual(ProgramImpl *prog,
                        std::vector<REGION *> &merges) override;

  // Variables that make up the tuple.
  UseList<VAR> col_values;

  // Table whose row is being retired.
  UseRef<TABLE> table;

  // Which frontier bit is cleared: `true` for the delete frontier,
  // `false` for the insert frontier.
  const bool is_del;
};

using RETIRE = ProgramRetireRegionImpl;

// Nets a differential message's explicit adds against its explicit removes
// within one batch: rewrites `add_vector` to one copy of each row whose
// net occurrence count is positive and `remove_vector` to one copy of each
// row whose net is negative. Has no body.
class ProgramNetBatchRegionImpl final : public OP {
 public:
  virtual ~ProgramNetBatchRegionImpl(void);

  inline ProgramNetBatchRegionImpl(REGION *parent_)
      : OP(parent_, ProgramOperation::kNetBatchVectors) {}

  void Accept(ProgramVisitor &visitor) override;

  ProgramNetBatchRegionImpl *AsNetBatch(void) noexcept override;

  uint64_t Hash(uint32_t depth) const override;
  bool IsNoOp(void) const noexcept override;

  // Returns `true` if `this` and `that` are structurally equivalent (after
  // variable renaming).
  bool Equals(EqualitySet &eq, REGION *that,
              uint32_t depth) const noexcept override;

  const bool MergeEqual(ProgramImpl *prog,
                        std::vector<REGION *> &merges) override;

  // The message's explicit-addition and explicit-removal vectors, both
  // read and rewritten in place.
  UseRef<VECTOR> add_vector;
  UseRef<VECTOR> remove_vector;
};

using NETBATCH = ProgramNetBatchRegionImpl;

// Calls another IR procedure. All IR procedures return `true` or `false`. This
// return value can be tested, and if it is, a body can be conditionally
// executed based off of the result of that test.
class ProgramCallRegionImpl final : public OP {
 public:
  virtual ~ProgramCallRegionImpl(void);

  ProgramCallRegionImpl(
      unsigned id_, REGION *parent_, ProgramProcedureImpl *called_proc_,
      ProgramOperation op_ = ProgramOperation::kCallProcedure);

  void Accept(ProgramVisitor &visitor) override;
  uint64_t Hash(uint32_t depth) const override;
  bool IsNoOp(void) const noexcept override;

  // Returns `true` if `this` and `that` are structurally equivalent (after
  // variable renaming).
  bool Equals(EqualitySet &eq, REGION *that,
              uint32_t depth) const noexcept override;

  const bool MergeEqual(ProgramImpl *prog,
                        std::vector<REGION *> &merges) override;

  ProgramCallRegionImpl *AsCall(void) noexcept override;

  // Returns `true` if all paths through `this` ends with a `return` region.
  bool EndsWithReturn(void) const noexcept override;

  // Procedure being called.
  UseRef<ProgramProcedureImpl, REGION> called_proc;

  // Variables passed as arguments.
  UseList<VAR> arg_vars;

  // Vectors passed as arguments.
  UseList<VECTOR> arg_vecs;

  // If the `call` returns `true`, then `body` is executed, otherwise if it
  // returns `false` then `false_body` is executed.
  RegionRef false_body;

  const unsigned id;
};

using CALL = ProgramCallRegionImpl;

// Returns true/false from a procedure.
class ProgramReturnRegionImpl final : public OP {
 public:
  virtual ~ProgramReturnRegionImpl(void);

  ProgramReturnRegionImpl(REGION *parent_, ProgramOperation op_)
      : OP(parent_, op_) {}

  void Accept(ProgramVisitor &visitor) override;
  uint64_t Hash(uint32_t depth) const override;
  bool IsNoOp(void) const noexcept override;
  bool Equals(EqualitySet &eq, REGION *that,
              uint32_t depth) const noexcept override;

  const bool MergeEqual(ProgramImpl *prog,
                        std::vector<REGION *> &merges) override;

  ProgramReturnRegionImpl *AsReturn(void) noexcept override;

  // Returns `true` if all paths through `this` ends with a `return` region.
  bool EndsWithReturn(void) const noexcept override;
};

using RETURN = ProgramReturnRegionImpl;

// Publishes a message to the pub/sub.
class ProgramPublishRegionImpl final : public OP {
 public:
  virtual ~ProgramPublishRegionImpl(void);

  ProgramPublishRegionImpl(
      REGION *parent_, ParsedMessage message_, unsigned id_,
      ProgramOperation op_ = ProgramOperation::kPublishMessage)
      : OP(parent_, op_),
        message(message_),
        id(id_),
        arg_vars(this) {}

  void Accept(ProgramVisitor &visitor) override;
  uint64_t Hash(uint32_t depth) const override;

  // Returns `true` if `this` and `that` are structurally equivalent (after
  // variable renaming).
  bool Equals(EqualitySet &eq, REGION *that,
              uint32_t depth) const noexcept override;

  const bool MergeEqual(ProgramImpl *prog,
                        std::vector<REGION *> &merges) override;

  ProgramPublishRegionImpl *AsPublish(void) noexcept override;

  // Message being published.
  const ParsedMessage message;

  // ID of this node.
  const unsigned id;

  // Variables passed as arguments.
  UseList<VAR> arg_vars;
};

using PUBLISH = ProgramPublishRegionImpl;

// A run-once guard: increments `accumulator` and executes `body` if and only
// if the incremented value is `1`. The sole producer is the entry procedure's
// init guard (a `kInitGuard` global), which makes the constant-initialization
// flows run exactly once even though the entry procedure runs on every
// message batch.
class ProgramTestAndSetRegionImpl final : public OP {
 public:
  virtual ~ProgramTestAndSetRegionImpl(void);

  inline ProgramTestAndSetRegionImpl(REGION *parent_, ProgramOperation op_)
      : OP(parent_, op_) {}

  void Accept(ProgramVisitor &visitor) override;

  uint64_t Hash(uint32_t depth) const override;
  bool IsNoOp(void) const noexcept override;

  // Returns `true` if `this` and `that` are structurally equivalent (after
  // variable renaming).
  bool Equals(EqualitySet &eq, REGION *that,
              uint32_t depth) const noexcept override;

  const bool MergeEqual(ProgramImpl *prog,
                        std::vector<REGION *> &merges) override;

  ProgramTestAndSetRegionImpl *AsTestAndSet(void) noexcept override;

  // The counter incremented by `1`; `body` executes when the result is `1`.
  UseRef<VAR> accumulator;
};

using TESTANDSET = ProgramTestAndSetRegionImpl;

// An equi-join between two or more tables.
class ProgramTableJoinRegionImpl final : public OP {
 public:
  virtual ~ProgramTableJoinRegionImpl(void);
  inline ProgramTableJoinRegionImpl(
      REGION *parent_, QueryJoin query_join_, unsigned id_)
      : OP(parent_, ProgramOperation::kJoinTables),
        query_join(query_join_),
        id(id_),
        tables(this),
        indices(this),
        pivot_vars(this),
        pivot_cols(),
        added_body(this),
        removed_body(this) {}

  void Accept(ProgramVisitor &visitor) override;
  uint64_t Hash(uint32_t depth) const override;
  bool IsNoOp(void) const noexcept override;

  // Returns `true` if `this` and `that` are structurally equivalent (after
  // variable renaming).
  bool Equals(EqualitySet &eq, REGION *that,
              uint32_t depth) const noexcept override;

  const bool MergeEqual(ProgramImpl *prog,
                        std::vector<REGION *> &merges) override;

  ProgramTableJoinRegionImpl *AsTableJoin(void) noexcept override;

  const QueryJoin query_join;
  const unsigned id;

  UseList<TABLE> tables;

  // NOTE(pag): There might be fewer indices than tables. If the Nth table's
  //            index is not present then `index_of_index[N]` will have a value
  //            of `0`, otherwise its index can be found as:
  //
  //                    indices[index_of_index[N] - 1u]
  //
  //            The only case where an index is absent is when it covers all
  //            columns of a table.
  std::vector<unsigned> index_of_index;
  UseList<TABLEINDEX> indices;

  UseRef<VECTOR> pivot_vec;

  // There is a `1:N` correspondence between `pivot_vars` and `pivot_cols`.
  DefList<VAR> pivot_vars;
  std::vector<UseList<TABLECOLUMN>> pivot_cols;

  // There is a 1:1 correspondence between `output_vars` and columns in the
  // selected tables. Not all of those columns will necessarily be used.
  std::vector<DefList<VAR>> output_vars;
  std::vector<UseList<TABLECOLUMN>> output_cols;

  // Delta sections, alongside the plain `body` (which runs per joined
  // combination of currently stored rows). A joined combination requires
  // every side's scanned key columns to equal the pivot: the index scans
  // are approximate, and where the body path re-checks through its
  // TUPLECMP, the sections conjoin the key equality into their emitted
  // predicates. Each section runs per joined combination under a named
  // batch-delta discipline that codegen evaluates directly on the scanned
  // row ids:
  //
  //   added_body:   every side is in the batch-final state (InNew) and at
  //                 least one side is a net addition of this batch — the
  //                 combination newly holds.
  //   removed_body: every side was in the batch-start state (InI) and at
  //                 least one side is a net deletion of this batch — the
  //                 combination stopped holding.
  //
  // A combination is enumerated once per batch (the pivot vector is
  // sort-uniqued and the join runs once), so each section fires exactly
  // once per started/stopped rule instance: an instance changed at several
  // positions still fires once, and an instance with a net-added side and
  // a net-deleted side fires neither section (it is in neither the old nor
  // the new state). Monotone sides answer the reads through their sealed
  // row-id watermark; a net deletion is impossible there.
  RegionRef added_body;
  RegionRef removed_body;
};

using TABLEJOIN = ProgramTableJoinRegionImpl;

// A cross product between two or more tables.
class ProgramTableProductRegionImpl final : public OP {
 public:
  virtual ~ProgramTableProductRegionImpl(void);
  inline ProgramTableProductRegionImpl(
      REGION *parent_, QueryJoin query_join_, unsigned id_)
      : OP(parent_, ProgramOperation::kCrossProduct),
        query_join(query_join_),
        tables(this),
        input_vecs(this),
        id(id_) {}

  void Accept(ProgramVisitor &visitor) override;
  uint64_t Hash(uint32_t depth) const override;
  bool IsNoOp(void) const noexcept override;

  // Returns `true` if `this` and `that` are structurally equivalent (after
  // variable renaming).
  bool Equals(EqualitySet &eq, REGION *that,
              uint32_t depth) const noexcept override;

  const bool MergeEqual(ProgramImpl *prog,
                        std::vector<REGION *> &merges) override;

  ProgramTableProductRegionImpl *AsTableProduct(void) noexcept override;

  const QueryJoin query_join;

  UseList<TABLE> tables;
  UseList<VECTOR> input_vecs;
  std::vector<DefList<VAR>> output_vars;
  const unsigned id;
};

using TABLEPRODUCT = ProgramTableProductRegionImpl;

// Perform a scan over a table, possibly using an index. If an index is being
// used the input variables are provided to perform equality matching against
// column values. The results of the scan fill a vector.
class ProgramTableScanRegionImpl final : public OP {
 public:
  virtual ~ProgramTableScanRegionImpl(void);

  inline ProgramTableScanRegionImpl(unsigned id_, REGION *parent_)
      : OP(parent_, ProgramOperation::kScanTable),
        id(id_),
        out_cols(this),
        in_cols(this),
        in_vars(this),
        out_vars(this) {}

  void Accept(ProgramVisitor &visitor) override;
  uint64_t Hash(uint32_t depth) const override;
  bool IsNoOp(void) const noexcept override;

  // Returns `true` if `this` and `that` are structurally equivalent (after
  // variable renaming).
  bool Equals(EqualitySet &eq, REGION *that,
              uint32_t depth) const noexcept override;

  const bool MergeEqual(ProgramImpl *prog,
                        std::vector<REGION *> &merges) override;

  ProgramTableScanRegionImpl *AsTableScan(void) noexcept override;

  const unsigned id;

  UseRef<TABLE> table;
  UseList<TABLECOLUMN> out_cols;

  UseRef<TABLEINDEX> index;
  UseList<TABLECOLUMN> in_cols;

  // One variable for each column in `in_cols`.
  UseList<VAR> in_vars;

  // Output variables, one per column in the table!
  DefList<VAR> out_vars;
};

using TABLESCAN = ProgramTableScanRegionImpl;

// Comparison between two tuples.
class ProgramTupleCompareRegionImpl final : public OP {
 public:
  virtual ~ProgramTupleCompareRegionImpl(void);
  inline ProgramTupleCompareRegionImpl(REGION *parent_, ComparisonOperator op_)
      : OP(parent_, ProgramOperation::kCompareTuples),
        cmp_op(op_),
        lhs_vars(this),
        rhs_vars(this),
        false_body(this) {
    assert(cmp_op != ComparisonOperator::kNotEqual);
  }

  void Accept(ProgramVisitor &visitor) override;
  uint64_t Hash(uint32_t depth) const override;
  bool IsNoOp(void) const noexcept override;

  // Returns `true` if `this` and `that` are structurally equivalent (after
  // variable renaming).
  bool Equals(EqualitySet &eq, REGION *that,
              uint32_t depth) const noexcept override;

  const bool MergeEqual(ProgramImpl *prog,
                        std::vector<REGION *> &merges) override;

  ProgramTupleCompareRegionImpl *AsTupleCompare(void) noexcept override;

  // Returns `true` if all paths through `this` ends with a `return` region.
  bool EndsWithReturn(void) const noexcept override;

  const ComparisonOperator cmp_op;
  UseList<VAR> lhs_vars;
  UseList<VAR> rhs_vars;

  // Optional body executed if the comparison fails.
  RegionRef false_body;
};

using TUPLECMP = ProgramTupleCompareRegionImpl;

// Calling a functor.
class ProgramGenerateRegionImpl final : public OP {
 public:
  virtual ~ProgramGenerateRegionImpl(void);
  inline ProgramGenerateRegionImpl(REGION *parent_, ParsedFunctor functor_,
                                   unsigned id_)
      : OP(
            parent_, functor_.IsFilter() ? ProgramOperation::kCallFilterFunctor
                                         : ProgramOperation::kCallFunctor),
        functor(functor_),
        id(id_),
        defined_vars(this),
        used_vars(this),
        empty_body(this) {}

  void Accept(ProgramVisitor &visitor) override;
  uint64_t Hash(uint32_t depth) const override;
  bool IsNoOp(void) const noexcept override;

  // Returns `true` if `this` and `that` are structurally equivalent (after
  // variable renaming).
  bool Equals(EqualitySet &eq, REGION *that,
              uint32_t depth) const noexcept override;

  const bool MergeEqual(ProgramImpl *prog,
                        std::vector<REGION *> &merges) override;

  ProgramGenerateRegionImpl *AsGenerate(void) noexcept override;

  // Returns `true` if all paths through `this` ends with a `return` region.
  bool EndsWithReturn(void) const noexcept override;

  const ParsedFunctor functor;

  // Unique ID of this node. Useful during codegen to ensure we can count the
  // results of one generate without it interfering with the count of a nested
  // generate.
  const unsigned id;

  // Free variables that are generated from the application of the functor.
  DefList<VAR> defined_vars;

  // Bound variables passed in as arguments to the functor.
  UseList<VAR> used_vars;

  // If the `functor` produces results, then `body` is executed, otherwise if it
  // doesn't produce results then `empty_body` is executed.
  RegionRef empty_body;
};

using GENERATOR = ProgramGenerateRegionImpl;

// A procedure region. This represents some entrypoint of data into the program.
class ProgramProcedureImpl : public REGION {
 public:
  virtual ~ProgramProcedureImpl(void);

  ProgramProcedureImpl(unsigned id_, ProcedureKind kind_);

  void Accept(ProgramVisitor &visitor) override;
  uint64_t Hash(uint32_t depth) const override;
  bool IsNoOp(void) const noexcept override;
  bool Equals(EqualitySet &eq, REGION *that,
              uint32_t depth) const noexcept override;

  const bool MergeEqual(ProgramImpl *prog,
                        std::vector<REGION *> &merges) override;

  ProgramProcedureImpl *AsProcedure(void) noexcept override;

  // Returns `true` if all paths through `this` ends with a `return` region.
  bool EndsWithReturn(void) const noexcept override;

  // Create a new vector in this procedure for a list of columns.
  VECTOR *VectorFor(ProgramImpl *impl, VectorKind kind,
                    DefinedNodeRange<QueryColumn> cols);

  const unsigned id;
  const ProcedureKind kind;

  std::optional<QueryIO> io;

  // Temporary tables within this procedure.
  DefList<TABLE> tables;

  // Body of this procedure. Initially starts with a loop over an implicit
  // input vector.
  RegionRef body;

  // Input vectors and variables.
  DefList<VECTOR> input_vecs;
  DefList<VAR> input_vars;

  // Vectors defined in this procedure. If this is a vector procedure then
  // the first vector is the input vector.
  DefList<VECTOR> vectors;

  // Are we currently checking if this procedure is being used? This is to
  // avoid infinite recursion when doing a procedure call NoOp checks.
  mutable bool checking_if_nop{false};

  // Do we need to keep this procedure around? This happens if we're holding
  // a raw, non-`UseRef`/`UseList` pointer to this procedure, such as inside
  // of `ProgramQuery` specifications.
  bool has_raw_use{false};
};

using PROC = ProgramProcedureImpl;

// A series region is where the `regions[N]` must finish before `regions[N+1]`
// begins.
class ProgramSeriesRegionImpl final : public REGION {
 public:
  ProgramSeriesRegionImpl(REGION *parent_);

  virtual ~ProgramSeriesRegionImpl(void);

  void Accept(ProgramVisitor &visitor) override;
  uint64_t Hash(uint32_t depth) const override;
  bool IsNoOp(void) const noexcept override;

  // Returns `true` if all paths through `this` ends with a `return` region.
  bool EndsWithReturn(void) const noexcept override;

  // Returns `true` if `this` and `that` are structurally equivalent (after
  // variable renaming).
  bool Equals(EqualitySet &eq, REGION *that,
              uint32_t depth) const noexcept override;

  const bool MergeEqual(ProgramImpl *prog,
                        std::vector<REGION *> &merges) override;

  inline void AddRegion(REGION *child) {
    assert(child->parent == this);
    regions.AddUse(child);
  }

  ProgramSeriesRegionImpl *AsSeries(void) noexcept override;

  UseList<REGION> regions;
};

using SERIES = ProgramSeriesRegionImpl;

// A region where multiple things can happen in parallel.
class ProgramParallelRegionImpl final : public REGION {
 public:
  ProgramParallelRegionImpl(REGION *parent_);

  virtual ~ProgramParallelRegionImpl(void);

  void Accept(ProgramVisitor &visitor) override;

  // Returns `true` if `this` and `that` are structurally equivalent (after
  // variable renaming).
  bool Equals(EqualitySet &eq, REGION *that,
              uint32_t depth) const noexcept override;

  uint64_t Hash(uint32_t depth) const override;
  bool IsNoOp(void) const noexcept override;

  // Returns `true` if all paths through `this` ends with a `return` region.
  bool EndsWithReturn(void) const noexcept override;

  const bool MergeEqual(ProgramImpl *prog,
                        std::vector<REGION *> &merges) override;

  inline void AddRegion(REGION *child) {
    assert(child->parent == this);
    regions.AddUse(child);
  }

  ProgramParallelRegionImpl *AsParallel(void) noexcept override;

  UseList<REGION> regions;
};

using PARALLEL = ProgramParallelRegionImpl;

// An induction is a loop centred on a `QueryMerge` node. Some of the views
// incoming to that `QueryMerge` are treated as "inputs", as they bring initial
// data into the `QueryMerge`. Other nodes are treated as "inductions" as they
// cycle back to the `QueryMerge`.
class ProgramInductionRegionImpl final : public REGION {
 public:
  virtual ~ProgramInductionRegionImpl(void);

  void Accept(ProgramVisitor &visitor) override;

  explicit ProgramInductionRegionImpl(ProgramImpl *impl, REGION *parent_);
  uint64_t Hash(uint32_t depth) const override;

  // Returns `true` if `this` and `that` are structurally equivalent (after
  // variable renaming).
  bool Equals(EqualitySet &eq, REGION *that,
              uint32_t depth) const noexcept override;

  const bool MergeEqual(ProgramImpl *prog,
                        std::vector<REGION *> &merges) override;

  ProgramInductionRegionImpl *AsInduction(void) noexcept override;

  // Returns `true` if all paths through `this` ends with a `return` region.
  bool EndsWithReturn(void) const noexcept override;

  // Initial regions that fill up one or more of the inductive vectors.
  RegionRef init_region;

  // The cyclic regions of this induction. This is a PARALLEL region.
  RegionRef cyclic_region;

  // The output regions of this induction. This is a PARALLEL region.
  RegionRef output_region;

  // Vectors built up by this induction.
  UseList<VECTOR> vectors;

  // It could be the case that a when going through the induction we end up
  // going into a co-mingled induction, as is the case in
  // `transitive_closure2.dr` and `transitive_closure3.dr`. Thus, we have
  // multiple vectors which must be maintained during an induction.

  // This is the cycle vector, i.e. each iteration of the induction's fixpoint
  // loop operates on this vector. The init region of an induction fills this
  // vector.
  OrderedViewMap<VECTOR *> view_to_add_vec;

  // This is the swap vector. Inside of a fixpoint loop, we swap this with the
  // normal vector, so that the current iteration of the loop can append to
  // the normal vector, while still allowing us to iterate over what was added
  // from the prior iteration of the fixpoint loop.
  OrderedViewMap<VECTOR *> view_to_swap_vec;

  // We try to share swap vectors as much as possible.
  std::unordered_map<std::string, VECTOR *> col_types_to_swap_vec;

  // This is the output vector; it accumulates everything from all iterations
  // of the fixpoint loop.
  OrderedViewMap<VECTOR *> view_to_output_vec;

  // List of append to vector regions inside this induction.
  std::vector<REGION *> init_appends;
  std::vector<OP *> cycle_appends;

  OrderedViewMap<PARALLEL *> output_cycles;

  OrderedViewMap<PARALLEL *> fixpoint_cycles;

  const unsigned id;

  enum State {
    kAccumulatingInputRegions,
    kAccumulatingCycleRegions,
    kBuildingOutputRegions
  } state = kAccumulatingInputRegions;

  // All of the UNIONs, JOINs, and NEGATEs of the induction.
  std::vector<QueryView> views;
};

using INDUCTION = ProgramInductionRegionImpl;

// R3 codegen descriptor for one StateCell store (one GROUP_UPDATE view). Names
// the generated `statecell_<id>` member, its `Key_<id>` / `Reduce_<id>` types,
// and the driver-supplied reduction functor (C-5 free-function ABI). Populated
// from the DR flow's statecells at stratum-phase build time.
struct ProgramStateCell {
  unsigned id{0u};
  bool invertible{false};
  std::vector<TypeLoc> key_types;      // group ++ config column types
  std::vector<TypeLoc> summary_types;  // the folded value column type(s)
  ParsedFunctor functor;               // the reduction (agg summary / kv merge)
  // P2c: how many of the trailing `key_types` are CONFIGURATION column types
  // (the tail beyond the group-by columns). Codegen emits these as leading
  // params on the reduction ABI (`Combine`/`Uncombine` for @invertible,
  // `ReduceLive` for @recompute) and as the `Reduce_<id>::ConfigTuple`. 0 for a
  // config-free aggregate or a KV index.
  unsigned num_config_types{0u};

  ProgramStateCell(unsigned id_, bool invertible_, ParsedFunctor functor_)
      : id(id_), invertible(invertible_), functor(functor_) {}
};

class ProgramImpl : public User {
 public:
  ~ProgramImpl(void);

  explicit ProgramImpl(Query query_, unsigned next_id_);

  void Optimize(const PassPolicy &policy);

  // Analyze the control-flow IR and table usage, looking for strategies that
  // can be used to eliminate redundancies in the data storage model. We do this
  // after optimizing the control-flow IR so that we can observe the effects
  // of copy propagation, which gives us the ability to "hop backward" to the
  // provenance of some data, as opposed to having to jump one `QueryView` at
  // a time.
  void Analyze(void);

  // The data flow representation from which this was created.
  const Query query;

  // Globally numbers things like procedures, variables, vectors, etc.
  unsigned next_id;

  // List of query entry points.
  std::vector<ProgramQuery> queries;

  DefList<PROC> procedure_regions;
  DefList<SERIES> series_regions;
  DefList<PARALLEL> parallel_regions;
  DefList<INDUCTION> induction_regions;
  DefList<OP> operation_regions;
  DefList<TABLEJOIN> join_regions;
  DefList<TABLE> tables;
  DefList<DATARECORDCASE> record_cases;

  // List of variables associated with globals (e.g. the init guard).
  DefList<VAR> global_vars;

  // List of variables associated with constants.
  DefList<VAR> const_vars;

  VAR *const zero;
  VAR *const false_;
  VAR *const true_;

  // Maps constants to their global variables.
  std::unordered_map<QueryConstant, VAR *> const_to_var;

  // Maps views whose outputs are all constants or constant references to
  // condition variables that tracker whether or not we need to actually re-
  // execute the successors.
  std::unordered_map<QueryView, VAR *> const_view_to_var;

  // We build up "data models" of views that can share the same backing storage.
  std::vector<std::unique_ptr<DataModel>> models;
  std::unordered_map<QueryView, DataModel *> view_to_model;

  // R3: one StateCell store descriptor per GROUP_UPDATE view (codegen emits a
  // `statecell_<id>` member, its Key/Reduce types, and the commit-tail Seal).
  std::vector<ProgramStateCell> state_cells;
};

}  // namespace hyde
