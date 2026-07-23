// Copyright 2026, Peter Goodman. All rights reserved.
//
// The `-deltarel-out <PATH>` DeltaRel (DR-IR) textual dump (T2b). Prints the
// `DRFlowGraph` in the pinned byte grammar (t2b-grammar.md, spec §2.3 + pins
// p10-p12): a header, the vecs in mint order, first-class branch/join sections,
// the ops in `pinned_order`, the rounds substrate, the canonically-sorted deps,
// and the per-kind census. ZERO comments (p10); single blank line between
// sections, none within (p11); no-source fields never render (p12).
//
// The emitter carries no name functions in the model — the eighteen enum
// spelling tables below are the sole spelling authority (grammar §2.0). It is
// wired through a lib/DeltaRel-owned pre-guarded sink (SetDeltaRelDumpStream);
// the drain fires from Stratum.cpp on every Program::Build, so the guard is a
// PRE-guard (format only when a stream is set).

#include "DeltaRel.h"

#include <drlojekyll/DataFlow/Query.h>
#include <drlojekyll/Display/Format.h>
#include <drlojekyll/Parse/Format.h>
#include <drlojekyll/Parse/Parse.h>

#include <algorithm>
#include <tuple>
#include <cstdint>
#include <cstdio>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

#include "Program.h"  // TABLE (DataTableImpl), ->id

namespace hyde {
namespace {

// ---------------------------------------------------------------------------
// The eighteen enum spelling tables (grammar §2.0). The code has NO name
// functions; these ARE the spelling authority. Each is a bare switch.
// ---------------------------------------------------------------------------

// VecRole -> hyphenated-lowercase sigil (grammar §2.1, no `k`, no code source).
static const char *VecRoleSigil(VecRole r) {
  switch (r) {
    case VecRole::kParam: return "param";
    case VecRole::kNetRemoval: return "net-removal";
    case VecRole::kNetAddition: return "net-addition";
    case VecRole::kDeleteQueue: return "delete-queue";
    case VecRole::kAddQueue: return "add-queue";
    case VecRole::kOverdeleteSet: return "overdelete-set";
    case VecRole::kAdditionSet: return "addition-set";
    case VecRole::kClaimedDel: return "claimed-del";
    case VecRole::kClaimedAdd: return "claimed-add";
    case VecRole::kJoinPivots: return "join-pivots";
    case VecRole::kProductInput: return "product-input";
    case VecRole::kTableScan: return "table-scan";
    case VecRole::kMessageOutput: return "message-output";
    case VecRole::kEmpty: return "empty";
  }
  fprintf(stderr, "DELTAREL-DUMP: unhandled enum value in a spelling table\n");
  abort();
}

// VecRole -> the k-PREFIXED identifier form used INSIDE effect args (grammar
// §2.5.b: effects render `kNetAddition`, vec lines render `$net-addition`).
static const char *VecRoleIdent(VecRole r) {
  switch (r) {
    case VecRole::kParam: return "kParam";
    case VecRole::kNetRemoval: return "kNetRemoval";
    case VecRole::kNetAddition: return "kNetAddition";
    case VecRole::kDeleteQueue: return "kDeleteQueue";
    case VecRole::kAddQueue: return "kAddQueue";
    case VecRole::kOverdeleteSet: return "kOverdeleteSet";
    case VecRole::kAdditionSet: return "kAdditionSet";
    case VecRole::kClaimedDel: return "kClaimedDel";
    case VecRole::kClaimedAdd: return "kClaimedAdd";
    case VecRole::kJoinPivots: return "kJoinPivots";
    case VecRole::kProductInput: return "kProductInput";
    case VecRole::kTableScan: return "kTableScan";
    case VecRole::kMessageOutput: return "kMessageOutput";
    case VecRole::kEmpty: return "kEmpty";
  }
  fprintf(stderr, "DELTAREL-DUMP: unhandled enum value in a spelling table\n");
  abort();
}

static const char *UniqueContractSigil(UniqueContract u) {
  switch (u) {
    case UniqueContract::kMultiset: return "multiset";
    case UniqueContract::kSortUniqueAtDrain: return "sort-unique-at-drain";
  }
  fprintf(stderr, "DELTAREL-DUMP: unhandled enum value in a spelling table\n");
  abort();
}

static const char *DROpKindName(DROpKind k) {
  switch (k) {
    case DROpKind::kCrossover: return "kCrossover";
    case DROpKind::kProductArm: return "kProductArm";
    case DROpKind::kSeedFold: return "kSeedFold";
    case DROpKind::kFixpointFire: return "kFixpointFire";
    case DROpKind::kChainFold: return "kChainFold";
    case DROpKind::kClaimDrain: return "kClaimDrain";
    case DROpKind::kRetire: return "kRetire";
    case DROpKind::kRederive: return "kRederive";
    case DROpKind::kFrontierFilter: return "kFrontierFilter";
    case DROpKind::kCommitSweep: return "kCommitSweep";
    case DROpKind::kNegateGate: return "kNegateGate";
    case DROpKind::kPivotAssemble: return "kPivotAssemble";
    case DROpKind::kIngestFold: return "kIngestFold";
    case DROpKind::kGroupUpdate: return "kGroupUpdate";
    case DROpKind::kStateSeal: return "kStateSeal";
    case DROpKind::kSubgraphInstantiate: return "kSubgraphInstantiate";
    case DROpKind::kInstanceDeath: return "kInstanceDeath";
    case DROpKind::kInstanceSeal: return "kInstanceSeal";
    case DROpKind::kEagerForward: return "kEagerForward";
    case DROpKind::kEagerInsert: return "kEagerInsert";
    case DROpKind::kEagerCompare: return "kEagerCompare";
    case DROpKind::kEagerGenerate: return "kEagerGenerate";
    case DROpKind::kEagerUnion: return "kEagerUnion";
    case DROpKind::kEagerSelect: return "kEagerSelect";
  }
  fprintf(stderr, "DELTAREL-DUMP: unhandled enum value in a spelling table\n");
  abort();
}

// R1: the terminal-INSERT sink spelling table (design §C.1, k-stripped
// lowercase-hyphen per the ClaimForm/Deferral house convention). kNone never
// reaches a kEagerInsert (a forward carries no sink), so it loud-aborts.
static const char *EagerSinkName(EagerSink s) {
  switch (s) {
    case EagerSink::kRelation: return "relation";
    case EagerSink::kPublishNow: return "publish-now";
    case EagerSink::kPublishVec: return "publish-vec";
    case EagerSink::kCommitPublished: return "commit-published";
    case EagerSink::kNone: break;  // never on a kEagerInsert — loud-abort
  }
  fprintf(stderr, "DELTAREL-DUMP: unhandled EagerSink in a spelling table\n");
  abort();
}

// R2 (ADJ-R2-1): the CMP operator spelling table, reusing the .df house
// spelling (lib/DataFlow/Format.cpp's eq/neq/lt/gt). Total by construction
// (4 cases = the 4 ComparisonOperator members) + the loud-abort tail (the
// T2b law; -Wswitch is warning-only in the presets, so the tail is the
// enforcement). Fable-review R2 [3], accepted-deferred: this is the THIRD
// hand copy of the spelling (two inline switches in lib/DataFlow/Format.cpp)
// — unify in a dedicated hygiene diff, not mid-slice (it touches the .df
// emitter, outside the R2 scope).
static const char *ComparisonOperatorName(ComparisonOperator op) {
  switch (op) {
    case ComparisonOperator::kEqual: return "eq";
    case ComparisonOperator::kNotEqual: return "neq";
    case ComparisonOperator::kLessThan: return "lt";
    case ComparisonOperator::kGreaterThan: return "gt";
  }
  fprintf(stderr,
          "DELTAREL-DUMP: unhandled ComparisonOperator in a spelling table\n");
  abort();
}

static const char *EffKindName(EffKind k) {
  switch (k) {
    case EffKind::kVecAppend: return "kVecAppend";
    case EffKind::kVecDrain: return "kVecDrain";
    case EffKind::kVecClear: return "kVecClear";
    case EffKind::kCounter: return "kCounter";
    case EffKind::kFlagRead: return "kFlagRead";
    case EffKind::kFlagWrite: return "kFlagWrite";
    case EffKind::kInIReadFrozen: return "kInIReadFrozen";
    case EffKind::kStateFold: return "kStateFold";
    case EffKind::kStateEmit: return "kStateEmit";
    case EffKind::kStateOld: return "kStateOld";
    case EffKind::kInstanceRebuild: return "kInstanceRebuild";
    case EffKind::kInstanceDemand: return "kInstanceDemand";
  }
  fprintf(stderr, "DELTAREL-DUMP: unhandled enum value in a spelling table\n");
  abort();
}

// Pred -> k-STRIPPED spelling (grammar §2.5.a).
static const char *PredName(Pred p) {
  switch (p) {
    case Pred::kPresent: return "Present";
    case Pred::kInI: return "InI";
    case Pred::kInNew: return "InNew";
    case Pred::kSurvivesSoFar: return "SurvivesSoFar";
    case Pred::kAliveAtClaim: return "AliveAtClaim";
    case Pred::kInNewWithFrontier: return "InNewWithFrontier";
    case Pred::kInNewSansFrontier: return "InNewSansFrontier";
    case Pred::kRecursivelySupported: return "RecursivelySupported";
    case Pred::kNetDeleted: return "NetDeleted";
    case Pred::kNetAdded: return "NetAdded";
  }
  fprintf(stderr, "DELTAREL-DUMP: unhandled enum value in a spelling table\n");
  abort();
}

// Ctx -> k-stripped lowercase.
static const char *CtxName(Ctx c) {
  switch (c) {
    case Ctx::kEager: return "eager";
    case Ctx::kSeed: return "seed";
    case Ctx::kFixpoint: return "fixpoint";
  }
  fprintf(stderr, "DELTAREL-DUMP: unhandled enum value in a spelling table\n");
  abort();
}

static const char *ClaimFormName(ClaimForm f) {
  switch (f) {
    case ClaimForm::kSinglePass: return "single-pass";
    case ClaimForm::kInRound: return "in-round";
  }
  fprintf(stderr, "DELTAREL-DUMP: unhandled enum value in a spelling table\n");
  abort();
}

static const char *DeferralName(Deferral d) {
  switch (d) {
    case Deferral::kImmediate: return "immediate";
    case Deferral::kAddLoopOutput: return "add-loop-output";
  }
  fprintf(stderr, "DELTAREL-DUMP: unhandled enum value in a spelling table\n");
  abort();
}

static const char *SweepFlavorName(SweepFlavor f) {
  switch (f) {
    case SweepFlavor::kDifferential: return "differential";
    case SweepFlavor::kMonotone: return "monotone";
  }
  fprintf(stderr, "DELTAREL-DUMP: unhandled enum value in a spelling table\n");
  abort();
}

// ClaimGate -> k-PREFIXED verbatim identifier (grammar §2.4.c).
static const char *ClaimGateName(ClaimGate g) {
  switch (g) {
    case ClaimGate::kDelGateCnrNonPositive: return "kDelGateCnrNonPositive";
    case ClaimGate::kAddGateTotalPositive: return "kAddGateTotalPositive";
  }
  fprintf(stderr, "DELTAREL-DUMP: unhandled enum value in a spelling table\n");
  abort();
}

// DerivClass -> k-STRIPPED (used inside kCounter / kFold / seed class).
static const char *DerivClassName(DerivClass c) {
  switch (c) {
    case DerivClass::kNonRecursive: return "NonRecursive";
    case DerivClass::kRecursive: return "Recursive";
  }
  fprintf(stderr, "DELTAREL-DUMP: unhandled enum value in a spelling table\n");
  abort();
}

static const char *DepKindName(DepKind k) {
  switch (k) {
    case DepKind::kRAW: return "RAW";
    case DepKind::kWAR: return "WAR";
    case DepKind::kWAW: return "WAW";
  }
  fprintf(stderr, "DELTAREL-DUMP: unhandled enum value in a spelling table\n");
  abort();
}

static const char *DepScopeName(DepScope s) {
  switch (s) {
    case DepScope::kEpoch: return "epoch";
    case DepScope::kRound: return "round";
  }
  fprintf(stderr, "DELTAREL-DUMP: unhandled enum value in a spelling table\n");
  abort();
}

static const char *AggProvenanceName(AggProvenance p) {
  switch (p) {
    case AggProvenance::kOver: return "kOver";
    case AggProvenance::kKv: return "kKv";
  }
  fprintf(stderr, "DELTAREL-DUMP: unhandled enum value in a spelling table\n");
  abort();
}

static const char *AlgebraName(Algebra a) {
  switch (a) {
    case Algebra::kInvertible: return "kInvertible";
    case Algebra::kRecompute: return "kRecompute";
  }
  fprintf(stderr, "DELTAREL-DUMP: unhandled enum value in a spelling table\n");
  abort();
}

static const char *PlanKindName(PlanKind k) {
  switch (k) {
    case PlanKind::kAccess: return "kAccess";
    case PlanKind::kGate: return "kGate";
    case PlanKind::kFold: return "kFold";
  }
  fprintf(stderr, "DELTAREL-DUMP: unhandled enum value in a spelling table\n");
  abort();
}

static const char *LoweringName(Lowering l) {
  switch (l) {
    case Lowering::kPointTest: return "point-test";
    case Lowering::kSectionWalk: return "section-walk";
    case Lowering::kFullScan: return "full-scan";
    case Lowering::kSeek: return "seek";
  }
  fprintf(stderr, "DELTAREL-DUMP: unhandled enum value in a spelling table\n");
  abort();
}

// Header `sign=` glyph (grammar §2.4 S-1): `·` is UTF-8 0xC2 0xB7.
static const char *SignGlyph(int s) {
  if (s < 0) return "-";
  if (s > 0) return "+";
  return "\xC2\xB7";  // `·`
}

// ElementShape shape token (grammar §2.1). kIds -> `<ids %table:N>`;
// kIdCols -> `<id-cols>`; kValues -> `<values>`.
static std::string ShapeTok(const DRVec &v) {
  switch (v.shape) {
    case ElementShape::kIds:
      if (v.debug_table) {
        return "<ids %table:" + std::to_string(v.debug_table->id) + ">";
      }
      return "<ids>";
    case ElementShape::kIdCols: return "<id-cols>";
    case ElementShape::kValues: return "<values>";
  }
  return "<ids>";
}

// ---------------------------------------------------------------------------
// The `agg=<name>` field (grammar GU-1). kOver -> the aggregating functor's
// name; kKv -> the value merge functor's name (see the adjudication note in
// the implementer report: no relation-name accessor is reachable from the
// DR-IR agg_view, so the KV renders its merge functor, not a relation name).
// ---------------------------------------------------------------------------

// Column-token helpers, mirroring the .df emitter (Format.cpp name_tok /
// typed_tok): `<var-or-cN>:<type>`.
struct ColRender {
  OutputStream &os;
  std::ostringstream &buf;

  std::string take() {
    auto s = buf.str();
    buf.str("");
    return s;
  }
  std::string Name(QueryColumn c) {
    if (!c.IsConstantOrConstantRef()) {
      if (auto var = c.Variable()) {
        os << *var;
        return take();
      }
    }
    os << 'c' << c.Id();
    return take();
  }
  std::string Typed(QueryColumn c) {
    auto s = Name(c);
    os << c.Type();
    return s + ":" + take();
  }
};

// ---------------------------------------------------------------------------
// The emitter.
// ---------------------------------------------------------------------------
static void EmitDRFlow(OutputStream &os, const DRFlowGraph &flow) {
  std::ostringstream sbuf;
  OutputStream sos(os.display_manager, sbuf);
  ColRender col{sos, sbuf};

  const auto tid = [](TABLE *t) -> std::string {
    return "%table:" + std::to_string(t->id);
  };

  // ---- header ----
  os << "deltarel\n";

  // ---- vecs (mint order; separator only when the section is non-empty,
  // pin p11 — an empty section must not leave a double blank) ----
  if (!flow.vecs.empty()) os << "\n";
  for (unsigned vi = 0u; vi < flow.vecs.size(); ++vi) {
    const DRVec &v = flow.vecs[vi];
    os << "vec $" << VecRoleSigil(v.role) << "." << vi << " " << ShapeTok(v)
       << " uniq=" << UniqueContractSigil(v.uniq) << " def=[";
    for (unsigned i = 0u; i < v.defs.size(); ++i) {
      if (i) os << ",";
      os << "op." << v.defs[i];
    }
    os << "] use=[";
    for (unsigned i = 0u; i < v.uses.size(); ++i) {
      if (i) os << ",";
      os << "op." << v.uses[i];
    }
    os << "]\n";
  }

  // Resolve a join-terminal branch's join index by matching join_view ==
  // path.back() (grammar B-1).
  const auto join_index_of = [&](const DRBranch &b) -> int {
    if (b.path.empty()) return -1;
    const QueryView terminal = b.path.back();
    for (unsigned ji = 0u; ji < flow.joins.size(); ++ji) {
      if (flow.joins[ji].join_view == terminal) {
        return static_cast<int>(ji);
      }
    }
    return -1;
  };

  // ---- branches ----
  if (!flow.branches.empty()) {
    os << "\n";
    for (unsigned bi = 0u; bi < flow.branches.size(); ++bi) {
      const DRBranch &b = flow.branches[bi];
      os << "branch." << bi << " src=" << tid(b.source) << " -> ";
      if (b.ends_at_join) {
        os << "join." << join_index_of(b);
      } else {
        os << "tgt=" << tid(b.target);
      }
      os << " ends_at_join=" << (b.ends_at_join ? "true" : "false") << "\n";
    }
  }

  // ---- joins ----
  if (!flow.joins.empty()) {
    os << "\n";
    for (unsigned ji = 0u; ji < flow.joins.size(); ++ji) {
      const DRJoin &j = flow.joins[ji];
      os << "join." << ji << " view=<";
      bool first = true;
      for (auto c : j.join_view.Columns()) {
        if (!first) os << ",";
        first = false;
        os << col.Name(c);
      }
      os << "> pivot_vec=$" << VecRoleSigil(flow.vecs[j.pivot_vec].role) << "."
         << j.pivot_vec << " targets=[";
      for (unsigned i = 0u; i < j.targets.size(); ++i) {
        if (i) os << ",";
        os << tid(j.targets[i]);
      }
      os << "]\n";
    }
  }

  // Render one effect's live tagged-union fields (grammar §2.5.b). Inside
  // effects the sign renders `+`/`-`, and `sign=0` where signless.
  const auto effect_sign = [](int s) -> std::string {
    if (s < 0) return "-";
    if (s > 0) return "+";
    return "sign=0";
  };
  // A vec effect (append/drain/clear) with a null value_table references a
  // vec by ROLE, not a per-table vec — the join-pivots vec on a join-terminal
  // seed. Resolve its index via the op's terminal join (grammar §2.5.b / the
  // join-pivots append carries no table); render `$role.idx`. `pivot_vec`
  // is the enclosing op's resolved join-pivots vec index (or ~0u if none).
  const auto emit_effect = [&](const DREffect &e, unsigned pivot_vec) {
    os << EffKindName(e.kind);
    switch (e.kind) {
      case EffKind::kVecAppend:
      case EffKind::kVecDrain:
      case EffKind::kVecClear:
        if (!e.value_table) {
          os << "($" << VecRoleSigil(e.vec_role);
          if (pivot_vec != ~0u) os << "." << pivot_vec;
          os << ")";
        } else {
          os << "(" << tid(e.value_table) << ", " << VecRoleIdent(e.vec_role)
             << ")";
        }
        break;
      case EffKind::kCounter:
        os << "(" << tid(e.counter_table) << ", " << effect_sign(e.sign) << ", "
           << DerivClassName(e.klass) << ")";
        break;
      case EffKind::kFlagRead:
        os << "(" << tid(e.read_table) << ", " << PredName(e.pred) << ")";
        break;
      case EffKind::kFlagWrite:
        os << "(" << tid(e.write_table);
        if (e.sign) os << ", " << effect_sign(e.sign);
        os << ")";
        break;
      case EffKind::kInIReadFrozen:
        os << "(" << tid(e.read_table) << ", " << PredName(e.pred) << ", "
           << CtxName(e.ctx) << ")";
        break;
      case EffKind::kStateFold:
        os << "(" << tid(e.value_table) << ", " << effect_sign(e.sign) << ")";
        break;
      case EffKind::kStateEmit:
      case EffKind::kStateOld:
        os << "(" << tid(e.read_table) << ")";
        break;
      case EffKind::kInstanceRebuild:  // WRITE current, signed
        os << "(" << tid(e.value_table) << ", " << effect_sign(e.sign) << ")";
        break;
      case EffKind::kInstanceDemand:  // frozen demand-key read
        os << "(" << tid(e.read_table) << ")";
        break;
    }
  };

  // Render one arm's PlanNode spine chain (grammar §2.5.c). Empty / null body
  // renders `—` (em-dash, UTF-8 0xE2 0x80 0x94).
  const auto emit_spine = [&](const DROp &op) {
    const PlanNode *node = nullptr;
    if (!op.arms.empty()) {
      node = op.arms.front().body.get();
    }
    os << "    spine: ";
    if (!node) {
      os << "\xE2\x80\x94\n";  // `—`
      return;
    }
    bool first = true;
    for (const PlanNode *n = node; n; n = n->child.get()) {
      if (!first) os << " -> ";
      first = false;
      os << PlanKindName(n->kind);
      switch (n->kind) {
        case PlanKind::kAccess:
        case PlanKind::kGate:
          os << "(";
          if (n->table) os << tid(n->table) << ", ";
          os << LoweringName(n->lowering) << ")";
          break;
        case PlanKind::kFold:
          os << "(" << tid(n->fold_table) << ", " << effect_sign(n->fold_sign)
             << ", " << DerivClassName(n->fold_class) << ")";
          break;
      }
    }
    os << "\n";
  };

  // Resolve an op's join-pivots vec index via its terminal join (used to
  // render `$join-pivots.N` on join-terminal seed effects/args). ~0u if none.
  const auto op_pivot_vec = [&](const DROp &op) -> unsigned {
    if (op.kind == DROpKind::kSeedFold && op.join_pivot &&
        op.seed_branch < flow.branches.size()) {
      const int ji = join_index_of(flow.branches[op.seed_branch]);
      if (ji >= 0) return flow.joins[ji].pivot_vec;
    }
    // kPivotAssemble stores its vec directly (DeltaRel.cpp:1726) — without
    // this arm a multi-join SCC dump rendered an index-less `($join-pivots)`,
    // ambiguous across the program's pivot vecs (the T2b review's catch).
    if (op.kind == DROpKind::kPivotAssemble) {
      return op.pivot_vec_index;
    }
    return ~0u;
  };

  // A GROUP_UPDATE's summarized input table = its first drained vec's table
  // (band (a) drains the input net frontiers). Shared by the header `input=`
  // and the `args:` line (the T2b review's duplication catch).
  const auto gu_input_table = [](const DROp &op) -> TABLE * {
    for (const DREffect &e : op.effects) {
      if (e.kind == EffKind::kVecDrain) {
        return e.value_table;
      }
    }
    return nullptr;
  };

  const auto emit_effects = [&](const DROp &op) {
    os << "    effects: {";
    const unsigned pv = op_pivot_vec(op);
    bool first = true;
    for (const DREffect &e : op.effects) {
      if (e.kind == EffKind::kFlagRead) {
        continue;  // rendered on reads: (grammar §2.5.a)
      }
      if (!first) os << ", ";
      first = false;
      emit_effect(e, pv);
    }
    os << "}\n";
  };

  // reads: = the op's kFlagRead effects rendered as Pred(%table:N). Omitted if
  // none (grammar §2.5.a).
  const auto emit_reads = [&](const DROp &op) {
    bool any = false;
    for (const DREffect &e : op.effects) {
      if (e.kind == EffKind::kFlagRead) {
        if (!any) {
          os << "    reads: ";
          any = true;
        } else {
          os << ", ";
        }
        os << PredName(e.pred) << "(" << tid(e.read_table) << ")";
      }
    }
    if (any) os << "\n";
  };

  // The agg= name (grammar GU-1); see adjudication note for the KV case.
  const auto agg_name = [&](const DROp &op) -> std::string {
    if (op.provenance == AggProvenance::kOver && op.agg_view.has_value()) {
      return std::string(QueryAggregate::From(*op.agg_view).Functor()
                             .NameAsString());
    }
    if (op.statecell_id < flow.statecells.size()) {
      const DRStateCell &cell = flow.statecells[op.statecell_id];
      if (cell.algebra_functor) {
        return std::string(cell.algebra_functor->NameAsString());
      }
    }
    fprintf(stderr,
            "DELTAREL-DUMP: kGroupUpdate agg functor unresolvable\n");
    abort();
  };

  // Render a published-position column NAME via the DRInstance's pub_view
  // (crit-grammar-1: positions decide ik-vs-row, but a NAME needs the view).
  // fprintf+abort on an out-of-range position (crit-pins-3 / t2-dump-spec p12).
  const auto pub_col_name = [&](const DRInstance &in, unsigned pos) -> std::string {
    if (pos >= in.pub_view.Columns().size()) {
      fprintf(stderr, "DELTAREL-DUMP: instance pub-row position out of range\n");
      abort();
    }
    return col.Name(in.pub_view.Columns()[pos]);
  };

  // ---- instances (D1.b; after joins:, before ops:; p11 empty-section law —
  // render NOTHING when there are no instances, exactly like vecs/branches/
  // joins). ALWAYS empty at D1.b (the mint is gated off) ⇒ zero bytes. ----
  if (!flow.instances.empty()) {
    os << "\n";
    os << "instances:\n";
    for (unsigned i = 0u; i < flow.instances.size(); ++i) {
      const DRInstance &in = flow.instances[i];
      os << "  DRInstance i#" << i << " forcing=" << in.forcing_name
         << " key=" << tid(in.demand_table) << " pub=" << tid(in.pub_table)
         << " input=" << tid(in.input_table) << " store=I#" << i
         << " key_cols=[";
      for (unsigned k = 0u; k < in.key_cols.size(); ++k) {
        if (k) os << ",";
        os << pub_col_name(in, in.key_cols[k]);
      }
      os << "] row_cols=[";
      for (unsigned r = 0u; r < in.row_cols.size(); ++r) {
        if (r) os << ",";
        os << pub_col_name(in, in.row_cols[r]);
      }
      os << "]\n";
    }
  }

  // Render an instance op's `pub_row=[ik:.. | row:..]` partition + `nested=<>`
  // (HP-6 contract). Sourced from the op's DRInstance (via instance_store_id) —
  // NOT from context_col_sources (D2.b). Dead at D1.b (no instance op minted).
  const auto emit_pub_row = [&](const DROp &op) {
    if (op.instance_store_id >= flow.instances.size()) {
      fprintf(stderr, "DELTAREL-DUMP: instance op store id out of range\n");
      abort();
    }
    const DRInstance &in = flow.instances[op.instance_store_id];
    std::unordered_set<unsigned> keyset(in.key_cols.begin(), in.key_cols.end());
    const unsigned npub =
        static_cast<unsigned>(in.pub_view.Columns().size());
    os << " pub_row=[";
    for (unsigned p = 0u; p < npub; ++p) {
      if (p) os << ",";
      os << (keyset.count(p) ? "ik:" : "row:") << pub_col_name(in, p);
    }
    os << "] nested=<";
    for (unsigned r = 0u; r < in.row_cols.size(); ++r) {
      if (r) os << ",";
      os << pub_col_name(in, in.row_cols[r]);
    }
    os << ">";
  };

  // ---- ops (pinned_order; p11 empty-section guard as above) ----
  if (!flow.pinned_order.empty()) os << "\n";
  for (unsigned pi = 0u; pi < flow.pinned_order.size(); ++pi) {
    const unsigned oi = flow.pinned_order[pi];
    const DROp &op = flow.ops[oi];
    os << "op." << oi << " " << DROpKindName(op.kind);

    switch (op.kind) {
      case DROpKind::kGroupUpdate: {
        os << " sign=" << SignGlyph(0) << " ctx=" << CtxName(op.ctx)
           << " stratum=" << DROpStratum(flow, op) << " sc#" << op.statecell_id
           << "\n";
        os << "    agg=" << agg_name(op)
           << " prov=" << AggProvenanceName(op.provenance)
           << " algebra=" << AlgebraName(op.algebra)
           << " agg_table=" << tid(op.agg_table) << "\n";
        os << "    group@{";
        for (unsigned i = 0u; i < op.group_cols.size(); ++i) {
          if (i) os << ",";
          os << col.Typed(op.group_cols[i]);
        }
        os << "} summary@{";
        for (unsigned i = 0u; i < op.summary_cols.size(); ++i) {
          if (i) os << ",";
          os << col.Typed(op.summary_cols[i]);
        }
        os << "} config=" << op.num_config_cols;
        // input table id: the GROUP_UPDATE's band-(a) drain references the
        // input table via its net-* vec effects; render the effect's table
        // (the walked-to input table BuildGroupUpdateOps set on the drain).
        {
          TABLE *input_table = gu_input_table(op);
          if (input_table) os << " input=" << tid(input_table);
        }
        os << "\n";
        emit_effects(op);
        emit_spine(op);
        os << "    args: agg_table=" << tid(op.agg_table);
        {
          TABLE *input_table = gu_input_table(op);
          if (input_table) os << " input=" << tid(input_table);
        }
        os << " statecell=sc#" << op.statecell_id << "\n";
        break;
      }

      case DROpKind::kStateSeal: {
        os << " sign=" << SignGlyph(0) << " ctx=" << CtxName(op.ctx)
           << " band=10 sc#" << op.statecell_id << "\n";
        emit_effects(op);
        os << "    args: agg_table=" << tid(op.agg_table)
           << " statecell=sc#" << op.statecell_id << "\n";
        break;
      }

      case DROpKind::kIngestFold: {
        os << " sign=" << SignGlyph(op.ingest_sign) << " ctx=" << CtxName(op.ctx)
           << " stratum=" << DROpStratum(flow, op) << "\n";
        emit_reads(op);
        emit_effects(op);
        emit_spine(op);
        os << "    args: table=" << tid(op.ingest_table);
        if (op.ingest_message.has_value()) {
          os << " message=" << std::string(op.ingest_message->NameAsString())
             << "/" << op.ingest_message->Arity();
        }
        os << "\n";
        break;
      }

      case DROpKind::kSeedFold: {
        os << " sign=" << SignGlyph(op.seed_sign) << " ctx=" << CtxName(op.ctx)
           << " stratum=" << DROpStratum(flow, op) << " src=" << tid(op.seed_source);
        if (op.join_pivot) {
          os << " join_pivot";
        } else {
          os << " tgt=" << tid(op.seed_target)
             << " class=" << DerivClassName(op.seed_class);
        }
        os << "\n";
        emit_reads(op);
        emit_effects(op);
        emit_spine(op);
        os << "    args: src=" << tid(op.seed_source);
        if (op.join_pivot) {
          // pivots vec resolved via the terminal join (grammar AR-2).
          int ji = -1;
          if (op.seed_branch < flow.branches.size()) {
            ji = join_index_of(flow.branches[op.seed_branch]);
          }
          if (ji >= 0) {
            unsigned pv = flow.joins[ji].pivot_vec;
            os << " pivots=$" << VecRoleSigil(flow.vecs[pv].role) << "." << pv;
          }
        } else {
          os << " tgt=" << tid(op.seed_target);
        }
        os << "\n";
        break;
      }

      case DROpKind::kClaimDrain: {
        os << " sign=" << SignGlyph(op.table_op_sign) << " ctx=" << CtxName(op.ctx)
           << " stratum=" << DROpStratum(flow, op)
           << " form=" << ClaimFormName(op.claim_form)
           << " gate=" << ClaimGateName(op.claim_gate) << "\n";
        emit_reads(op);
        emit_effects(op);
        os << "    args: table=" << tid(op.table_op_table) << "\n";
        break;
      }

      case DROpKind::kFrontierFilter: {
        os << " sign=" << SignGlyph(op.table_op_sign) << " ctx=" << CtxName(op.ctx)
           << " stratum=" << DROpStratum(flow, op)
           << " deferral=" << DeferralName(op.deferral) << "\n";
        emit_reads(op);
        emit_effects(op);
        os << "    args: table=" << tid(op.table_op_table) << "\n";
        break;
      }

      case DROpKind::kCommitSweep: {
        os << " sign=" << SignGlyph(0) << " ctx=" << CtxName(op.ctx)
           << " band=9 flavor=" << SweepFlavorName(op.sweep_flavor)
           << " publish_target=" << (op.publish_target ? "true" : "false")
           << "\n";
        emit_reads(op);
        emit_effects(op);
        os << "    args: table=" << tid(op.table_op_table) << "\n";
        break;
      }

      // Keyed-instance op p-rules (D1.b). NEVER reached at D1.b (no instance op
      // in pinned_order); compile-covered + `-Wswitch`-total. [ADJ:crit-grammar-2]
      // header renders `i#` only; full `store=I#` on args.
      case DROpKind::kSubgraphInstantiate: {
        os << " sign=" << SignGlyph(op.table_op_sign) << " ctx=" << CtxName(op.ctx)
           << " stratum=" << DROpStratum(flow, op) << " i#"
           << op.instance_store_id << "\n";
        os << "    demand=" << tid(op.demand_table)
           << " pub=" << tid(op.table_op_table)
           << " input=" << tid(op.input_table);
        emit_pub_row(op);  // pub_row=[ik:..,row:..] nested=<...> (HP-6)
        os << "\n";
        emit_reads(op);
        emit_effects(op);
        emit_spine(op);
        os << "    args: demand=" << tid(op.demand_table)
           << " pub=" << tid(op.table_op_table)
           << " input=" << tid(op.input_table) << " store=I#"
           << op.instance_store_id << "\n";
        break;
      }

      case DROpKind::kInstanceDeath: {  // R-DIFF only; never rendered at D1.b/D2.b
        os << " sign=" << SignGlyph(op.table_op_sign) << " ctx=" << CtxName(op.ctx)
           << " stratum=" << DROpStratum(flow, op) << " i#"
           << op.instance_store_id << "\n";
        emit_reads(op);
        emit_effects(op);
        os << "    args: demand=" << tid(op.demand_table)
           << " pub=" << tid(op.table_op_table) << " store=I#"
           << op.instance_store_id << "\n";
        break;
      }

      case DROpKind::kInstanceSeal: {
        os << " sign=" << SignGlyph(0) << " ctx=" << CtxName(op.ctx)
           << " band=11 i#" << op.instance_store_id << "\n";
        emit_effects(op);
        os << "    args: pub=" << tid(op.table_op_table) << " store=I#"
           << op.instance_store_id << "\n";
        break;
      }

      // R1 (design §C.2, ADJ-S1/S3 applied): the two eager-web marker ops.
      // DEDICATED cases (NOT the :823 default — that would silently render
      // empty `effects:`/`spine:` sublines and NO `args:` line, DS-ADJ-5).
      // sign=· (table_op_sign 0); ctx=eager; stratum=0 (DROpStratum default
      // arm). NO reads/effects/spine sublines (effect-free markers, §A.2). The
      // `table=` token is OMITTED when null (ADJ-S3 — the `input=` OMIT-when-
      // null precedent; tid() has no null guard, so the `if` is load-bearing).
      case DROpKind::kEagerForward: {
        os << " sign=" << SignGlyph(op.table_op_sign)
           << " ctx=" << CtxName(op.ctx)
           << " stratum=" << DROpStratum(flow, op) << "\n";
        os << "    args:";
        if (op.table_op_table) os << " table=" << tid(op.table_op_table);
        os << "\n";
        break;
      }

      case DROpKind::kEagerInsert: {
        os << " sign=" << SignGlyph(op.table_op_sign)
           << " ctx=" << CtxName(op.ctx)
           << " stratum=" << DROpStratum(flow, op)
           << " sink=" << EagerSinkName(op.eager_sink) << "\n";
        os << "    args:";
        if (op.table_op_table) os << " table=" << tid(op.table_op_table);
        if (op.eager_message.has_value()) {
          os << " message=" << std::string(op.eager_message->NameAsString())
             << "/" << op.eager_message->Arity();
        }
        os << "\n";
        break;
      }

      // R2 (r2-design §B5, E-71-adjudicated productions): the CMP-filter and
      // MAP-call marker ops. Same effect-free shape as the R1 pair — NO
      // reads/effects/spine sublines, `table=` omitted when null. `cmp=` is a
      // HEADER token (the `sink=` precedent); `functor=` an ARGS token (the
      // `message=` precedent). Both re-derive from the STORED eager_view (a
      // pure function of a stored field — the agg_name precedent), never a
      // context re-computation.
      case DROpKind::kEagerCompare: {
        const QueryCompare cmp = QueryCompare::From(*op.eager_view);
        os << " sign=" << SignGlyph(op.table_op_sign)
           << " ctx=" << CtxName(op.ctx)
           << " stratum=" << DROpStratum(flow, op)
           << " cmp=" << ComparisonOperatorName(cmp.Operator()) << "\n";
        os << "    args:";
        if (op.table_op_table) os << " table=" << tid(op.table_op_table);
        os << "\n";
        break;
      }

      case DROpKind::kEagerGenerate: {
        const ParsedFunctor functor =
            QueryMap::From(*op.eager_view).Functor();
        os << " sign=" << SignGlyph(op.table_op_sign)
           << " ctx=" << CtxName(op.ctx)
           << " stratum=" << DROpStratum(flow, op) << "\n";
        os << "    args:";
        if (op.table_op_table) os << " table=" << tid(op.table_op_table);
        os << " functor=" << std::string(functor.NameAsString()) << "/"
           << functor.Arity();
        os << "\n";
        break;
      }

      // R3 (r3-design §C.2, E-71-adjudicated productions): the MERGE-union
      // and SELECT-rebind marker ops. Byte-identical to the kEagerForward
      // shape — NO extra token (a union carries no operator/functor; the
      // select's unit-condition-ness stays un-rendered, owner-declined §F.4).
      // DEDICATED cases per M7 — the generic default would silently render
      // empty effects:/spine: sublines and no args: line (§20(K)).
      case DROpKind::kEagerUnion: {
        os << " sign=" << SignGlyph(op.table_op_sign)
           << " ctx=" << CtxName(op.ctx)
           << " stratum=" << DROpStratum(flow, op) << "\n";
        os << "    args:";
        if (op.table_op_table) os << " table=" << tid(op.table_op_table);
        os << "\n";
        break;
      }

      case DROpKind::kEagerSelect: {
        os << " sign=" << SignGlyph(op.table_op_sign)
           << " ctx=" << CtxName(op.ctx)
           << " stratum=" << DROpStratum(flow, op) << "\n";
        os << "    args:";
        if (op.table_op_table) os << " table=" << tid(op.table_op_table);
        os << "\n";
        break;
      }

      default: {
        // Generic fallback (crossover, product-arm, fixpoint-fire, chain-fold,
        // retire, rederive, negate-gate, pivot-assemble). Renders the common
        // header + reads/effects/spine; args are kind-specific and omitted for
        // kinds this witness does not exercise (report if reached).
        os << " sign=" << SignGlyph(0) << " ctx=" << CtxName(op.ctx)
           << " stratum=" << DROpStratum(flow, op) << "\n";
        emit_reads(op);
        emit_effects(op);
        emit_spine(op);
        break;
      }
    }
  }

  // ---- rounds ----
  os << "\n";
  os << "rounds:\n";
  for (const DRRound &r : flow.rounds) {
    os << "  scc=" << r.scc_group << " phase="
       << (r.phase == RoundPhase::kOverdelete ? "overdelete" : "insert")
       << " body_ops=[";
    for (unsigned i = 0u; i < r.body_ops.size(); ++i) {
      if (i) os << ",";
      os << "op." << r.body_ops[i];
    }
    os << "] output_ops=[";
    for (unsigned i = 0u; i < r.output_ops.size(); ++i) {
      if (i) os << ",";
      os << "op." << r.output_ops[i];
    }
    os << "]\n";
  }

  // ---- deps: canonically sorted AND deduped on the FULL rendered tuple
  // (from, to, kind, scope, loop_carried) — pin p14. (Grammar §4.a's
  // three-field phrasing understates the key; ties on (from,to,kind) that
  // differ in scope/carried order on the remaining fields.) ----
  os << "\n";
  os << "deps:\n";
  std::vector<DRDep> deps = flow.dep_edges;
  const auto dep_tuple = [](const DRDep &d) {
    return std::make_tuple(d.from, d.to, static_cast<uint8_t>(d.kind),
                           static_cast<uint8_t>(d.scope), d.loop_carried);
  };
  std::sort(deps.begin(), deps.end(), [&](const DRDep &a, const DRDep &b) {
    return dep_tuple(a) < dep_tuple(b);
  });
  // Dedup exact-duplicate rows (pin p14): the flag-enrollment walk mints the
  // same (from, to, kind) edge once per access pair; the deps section renders
  // the dependence RELATION. Deduping identical rows elides no edge.
  deps.erase(std::unique(deps.begin(), deps.end(),
                         [&](const DRDep &a, const DRDep &b) {
                           return dep_tuple(a) == dep_tuple(b);
                         }),
             deps.end());
  for (const DRDep &d : deps) {
    os << "  op." << d.from << " -> op." << d.to << " " << DepKindName(d.kind)
       << " " << DepScopeName(d.scope);
    if (d.loop_carried) os << " loop-carried";
    os << "\n";
  }

  // ---- census (24 DROpKind counts, enum order, one line; grammar R-10) ----
  os << "\n";
  const auto count_kind = [&](DROpKind k) -> unsigned {
    unsigned n = 0u;
    for (const DROp &op : flow.ops) {
      if (op.kind == k) ++n;
    }
    return n;
  };
  static const DROpKind kAllKinds[] = {
      DROpKind::kCrossover,   DROpKind::kProductArm,
      DROpKind::kSeedFold,    DROpKind::kFixpointFire,
      DROpKind::kChainFold,   DROpKind::kClaimDrain,
      DROpKind::kRetire,      DROpKind::kRederive,
      DROpKind::kFrontierFilter, DROpKind::kCommitSweep,
      DROpKind::kNegateGate,  DROpKind::kPivotAssemble,
      DROpKind::kIngestFold,  DROpKind::kGroupUpdate,
      DROpKind::kStateSeal,
      DROpKind::kSubgraphInstantiate, DROpKind::kInstanceDeath,
      DROpKind::kInstanceSeal,
      DROpKind::kEagerForward, DROpKind::kEagerInsert,
      DROpKind::kEagerCompare, DROpKind::kEagerGenerate,
      DROpKind::kEagerUnion,   DROpKind::kEagerSelect};
  os << "census:";
  unsigned census_total = 0u;
  for (DROpKind k : kAllKinds) {
    const unsigned n = count_kind(k);
    census_total += n;
    os << " " << DROpKindName(k) << "=" << n;
  }
  os << "\n";
  if (census_total != flow.ops.size()) {  // a 25th DROpKind not in kAllKinds
    fprintf(stderr,
            "DELTAREL-DUMP: census covers %u of %zu ops (kAllKinds is "
            "missing a DROpKind)\n",
            census_total, flow.ops.size());
    abort();
  }
}

// The lib/DeltaRel-owned sink (spec §2.1 (i)).
static OutputStream *gDeltaRelDumpStream = nullptr;

}  // namespace

void SetDeltaRelDumpStream(OutputStream *stream) {
  gDeltaRelDumpStream = stream;
}

void DumpDeltaRelIfEnabled(const DRFlowGraph &flow) {
  if (gDeltaRelDumpStream) {  // PRE-guard: format only when a stream is set.
    EmitDRFlow(*gDeltaRelDumpStream, flow);
    gDeltaRelDumpStream->Flush();
  }
}

}  // namespace hyde
