// Copyright 2020, Trail of Bits. All rights reserved.

#include <iostream>

#include "Program.h"

namespace hyde {
namespace {

// Optimizes a PARALLEL region, whose child regions have no mutual ordering
// constraints and may execute concurrently. The pass first canonicalizes
// the region: a single child is elevated to replace the PARALLEL, a
// PARALLEL nested inside another PARALLEL donates its children to the
// parent, and no-op children are deleted. It then performs common
// subexpression elimination across the remaining children: children are
// bucketed by structural hash, children that are structurally identical at
// unlimited depth are deduplicated outright, and children whose root
// operations are identical (Equals at depth 0, e.g. two UPDATECOUNTs on
// the same table over the same tuple) but whose bodies differ are merged
// via MergeEqual, so a single copy of the root operation dispatches into
// the distinct bodies through a new nested PARALLEL. Because the children
// are unordered and the deduplicated work is identical, this preserves
// behavior while shrinking the program and executing shared work (state
// transitions, comparisons, calls) once instead of once per child. No
// child of a PARALLEL ends with a RETURN (asserted).
//
//    if exactly one child:  replace PARALLEL with that child
//    if parent is PARALLEL: move all children into the parent
//    remove no-op children; if any removed, re-run on this node
//    bucket children by Hash(0)
//    within each bucket:
//      drop children Equals() at unlimited depth      // exact CSE
//      MergeEqual() children Equals() at depth 0      // factor roots
//    if anything was dropped or merged, re-run on this node
//
// Flattening a nested PARALLEL:
//
//    PARALLEL              PARALLEL
//    ├─ A                  ├─ A
//    └─ PARALLEL    ==>    ├─ B
//       ├─ B               └─ C
//       └─ C
//
// Deduplicating an identical child, then merging a same-rooted one:
//
//    PARALLEL                             PARALLEL
//    ├─ UPDATECOUNT T (x)                 └─ UPDATECOUNT T (x)
//    │  └─ bodyA                             └─ PARALLEL
//    ├─ UPDATECOUNT T (x)          ==>          ├─ bodyA
//    │  └─ bodyA          (duplicate)           └─ bodyB
//    └─ UPDATECOUNT T (x)
//       └─ bodyB          (same root)
//
// TODO(pag): Find all ending returns in the children of the par, and if there
//            are any, check that they all match, and if so, create a sequence
//            that moves the `return <X>` to after the parallel, and also
//            assert(false).
static bool OptimizeImpl(ProgramImpl *prog, PARALLEL *par) {
  if (!par->IsUsed() || !par->parent) {
    return false;

  // This is a parallel region with only one child, so we can elevate the
  // child to replace the parent.
  } else if (par->regions.Size() == 1u) {
    const auto only_region = par->regions[0u];
    assert(only_region->parent == par);
    par->regions.Clear();
    par->ReplaceAllUsesWith(only_region);
    return true;

  // This parallel node's parent is also a parallel node.
  } else if (auto parent_par = par->parent->AsParallel();
             parent_par && !par->regions.Empty()) {

    for (auto child_region : par->regions) {
      assert(child_region->parent == par);
      child_region->parent = parent_par;
      parent_par->AddRegion(child_region);
    }

    par->regions.Clear();
    return true;
  }

  // Erase any empty or no-op child regions.
  auto changed = false;
  auto has_ends_with_return = false;
  par->regions.RemoveIf(
      [&changed, &has_ends_with_return](REGION *child_region) {
        if (child_region->EndsWithReturn()) {
          has_ends_with_return = true;
        }

        if (child_region->IsNoOp()) {
          child_region->parent = nullptr;
          changed = true;
          return true;
        } else {
          return false;
        }
      });

  if (changed) {
    OptimizeImpl(prog, par);
    return true;
  }

  assert(!has_ends_with_return);

  //  // One or more of the children of the parallel regions ends with a return.
  //  // That's a bit problematic.
  //  if (has_ends_with_return) {
  //    auto seq = prog->series_regions.Create(par->parent);
  //    par->ReplaceAllUsesWith(seq);
  //    par->parent = seq;
  //    seq->AddRegion(par);
  //  }

  // The PARALLEL node is "canonical" as far as we can tell, so check to see
  // if any of its child regions might be mergeable.

  // Group together all children of this parallel region; we'll use this
  // grouping to identify merge candidates, as well as strip-mining candidates.
  std::unordered_map<uint64_t, std::vector<REGION *>> grouped_regions;
  for (auto region : par->regions) {
    assert(region->parent == par);
    grouped_regions[region->Hash(0)].push_back(region);
  }

  // Go remove duplicate child regions. Note: we remove the regions from the
  // `par` node below by looking for regions whose parents are `nullptr`.
  //
  // NOTE(pag): This is is basically an application of common subexpression
  //            elimination.
  EqualitySet eq;
  for (const auto &hash_to_regions : grouped_regions) {
    const auto &similar_regions = hash_to_regions.second;
    const auto num_similar_regions = similar_regions.size();
    for (auto i = 1u; i < num_similar_regions; ++i) {
      REGION *region1 = similar_regions[i - 1u];
      if (!region1->parent) {
        continue;  // Already removed;
      }

      for (auto j = i; j < num_similar_regions; ++j) {
        REGION *region2 = similar_regions[j];
        if (!region2->parent) {
          continue;  // Already removed.
        }

        eq.Clear();
        if (region1->Equals(eq, region2, UINT32_MAX)) {
          assert(region1 != region2);
          region2->parent = nullptr;
          changed = true;
        }
      }
    }
  }

  // Go try to merge similar child regions. Note: we remove the regions from the
  // `par` node below by looking for regions whose parents are `nullptr`.
  //
  // This looks for child regions that are superficially the same, so that the
  // grand-children of two similar child regions can be merged under a single
  // child region.
  std::vector<REGION *> merge_candidates;
  for (const auto &hash_to_regions : grouped_regions) {
    const auto &similar_regions = hash_to_regions.second;
    const auto num_similar_regions = similar_regions.size();
    for (auto i = 1u; i < num_similar_regions; ++i) {
      REGION *region1 = similar_regions[i - 1u];
      if (!region1->parent) {
        continue;  // Already removed;
      }

      merge_candidates.clear();

      for (auto j = i; j < num_similar_regions; ++j) {
        REGION *region2 = similar_regions[j];
        if (!region2->parent) {
          continue;  // Already removed.
        }

        eq.Clear();
        if (region1->Equals(eq, region2, 0)) {
          assert(region1 != region2);
          merge_candidates.push_back(region2);
        }
      }

      // NOTE(pag): This clear the `parent` pointer of each region in
      //            `merge_candidates`.
      if (!merge_candidates.empty() &&
          region1->MergeEqual(prog, merge_candidates)) {
        changed = true;
      }
    }
  }

  if (changed) {

    // Remove any redundant or strip-minded regions in bulk.
    const auto old_num_children = par->regions.Size();
    par->regions.RemoveIf([=](REGION *r) { return !r->parent; });
    assert(old_num_children > par->regions.Size());
    OptimizeImpl(prog, par);
    (void) old_num_children;
    return true;
  }

  return false;
}

// Optimize an INDUCTION region. An INDUCTION is the fixpoint loop that
// implements an inductive UNION(MERGE) in the dataflow: its `init_region`
// fills the induction vectors with the initial tuples, its `cyclic_region`
// iterates until no new tuples appear, and its `output_region` processes the
// accumulated results. This pass does two things. First, it deletes no-op
// init and output regions so the induction carries only meaningful work.
// Second, when the induction's parent is a SERIES, it hoists the init region
// out of the induction and splices it into that parent SERIES immediately
// before the induction itself, flattening the init region's children into
// the parent if the init region is itself a SERIES. Hoisting is sound
// because the init region executes exactly once, before the first iteration
// of the fixpoint loop -- precisely the behavior of a preceding sibling in
// the parent SERIES. It is beneficial because the hoisted regions become
// visible to SERIES-level optimizations (no-op removal, flattening, merging
// with neighbors), and the induction itself is reduced to a pure fixpoint
// loop plus output.
//
//    if induction is unused or unlinked:      return false
//    if init_region is a no-op:               remove it
//    if output_region is a no-op:             remove it
//    if parent is a SERIES and init_region exists:
//      splice init_region's contents (flattened when it is a SERIES)
//      into the parent SERIES immediately before the induction
//    return whether anything changed
//
// Before:                             After:
//
//   SERIES                             SERIES
//   +-- A                              +-- A
//   +-- INDUCTION                      +-- I1
//   |     init:   SERIES [I1, I2]      +-- I2
//   |     cyclic: PARALLEL [loop...]   +-- INDUCTION
//   |     output: PARALLEL [no-op]     |     init:   <none>
//   +-- B                              |     cyclic: PARALLEL [loop...]
//                                      |     output: <none>
//                                      +-- B
//
// TODO(pag): Check if the fixpoint loop region ends in a return. If so, bail
//            out.
static bool OptimizeImpl(ProgramImpl *prog, INDUCTION *induction) {
  if (!induction->IsUsed() || !induction->parent) {
    return false;
  }

  auto changed = false;

  // Clear out empty init regions of inductions.
  if (induction->init_region && induction->init_region->IsNoOp()) {
    induction->init_region->parent = nullptr;
    induction->init_region.Clear();
    changed = true;
  }

  // Clear out empty output regions of inductions.
  if (induction->output_region && induction->output_region->IsNoOp()) {
    induction->output_region->parent = nullptr;
    induction->output_region.Clear();
    changed = true;
  }

  // If the parent of this induction is a sequence, then move all of the init
  // code in this induction up into that sequence.
  if (SERIES *parent_seq = induction->parent->AsSeries()) {
    if (REGION *init_region = induction->init_region.get()) {
      assert(init_region->parent == induction);

      UseList<REGION> new_parent_regions(parent_seq);

      for (REGION *region : parent_seq->regions) {
        if (region == induction) {
          if (SERIES *init_seq = init_region->AsSeries()) {
            for (REGION *sub_region : init_seq->regions) {
              assert(sub_region->parent == init_seq);
              sub_region->parent = parent_seq;
              new_parent_regions.AddUse(sub_region);
            }
            init_seq->regions.Clear();
            init_seq->parent = nullptr;
          } else {
            init_region->parent = parent_seq;
            new_parent_regions.AddUse(init_region);
          }

          induction->init_region.Clear();
        }

        assert(region->parent == parent_seq);
        new_parent_regions.AddUse(region);
      }

      parent_seq->regions.Swap(new_parent_regions);
      changed = true;
    }
  }

  return changed;

  //
  //  auto parent_region = induction->parent;
  //  if (!parent_region) {
  //    return changed;
  //  }
  //
  //  auto parent_induction = parent_region->AsInduction();
  //  if (!parent_induction) {
  //    return changed;
  //  }
  //
  //  // Optimize nested inductions.
  //
  //  // Form like
  //  // induction
  //  //   init
  //  //    induction
  //  if (induction == parent_induction->init_region.get()) {
  //
  //    // Fixup vectors
  //    for (auto def : induction->vectors) {
  //      changed = true;
  //      parent_induction->vectors.AddUse(def);
  //    }
  //    induction->vectors.Clear();
  //
  //    // Fixup output region
  //    if (auto output_region = induction->output_region.get(); output_region) {
  //      assert(!output_region->IsNoOp());  // Handled above.
  //      induction->output_region.Clear();
  //      output_region->parent = parent_induction;
  //      if (auto parent_output_region = parent_induction->output_region.get();
  //          parent_output_region) {
  //        output_region->ExecuteBefore(prog, parent_output_region);
  //      } else {
  //        parent_induction->output_region.Emplace(parent_induction,
  //                                                output_region);
  //      }
  //    }
  //
  //    // Fixup init region
  //    auto init_region = induction->init_region.get();
  //    induction->init_region.Clear();
  //    init_region->parent = parent_induction;
  //    parent_induction->init_region.Emplace(parent_induction, init_region);
  //
  //    // Fixup cyclic region
  //    auto cyclic_region = induction->cyclic_region.get();
  //    induction->cyclic_region.Clear();
  //    cyclic_region->parent = parent_induction;
  //    if (auto parent_cyclic_region = parent_induction->cyclic_region.get();
  //        parent_cyclic_region) {
  //      cyclic_region->ExecuteBefore(prog, parent_cyclic_region);
  //    } else {
  //      parent_induction->cyclic_region.Emplace(parent_induction, cyclic_region);
  //    }
  //
  //    induction->parent = nullptr;
  //
  //    changed = true;
  //
  //  // Form like
  //  // induction:
  //  // init:
  //  //   init-code-0
  //  // fixpoint-loop:
  //  //   induction:
  //  //     init:
  //  //       init-code-1
  //  //     fixpoint-loop:
  //  //       code-2
  //  //   code-3
  //  } else if (induction == parent_induction->cyclic_region.get()) {
  //
  //    // TODO(ekilmer)
  //  }
  //
  //  return changed;
}

// Optimizes a SERIES region, whose child regions execute strictly in
// order. The pass canonicalizes the region and performs local dead-code
// elimination: a single child is elevated to replace the SERIES; a SERIES
// nested inside another SERIES is spliced inline into its parent's child
// list at its own position (order preserved); no-op children are deleted;
// and any children following a child through which every path returns —
// either a direct RETURN operation or a region whose every path ends in a
// RETURN — are unreachable and are truncated. Flattening produces one
// canonical sequence per straight-line stretch of code, which makes
// sibling regions hash/compare consistently for the PARALLEL merging pass,
// and truncation removes code that can never execute.
//
//    if exactly one child:  replace SERIES with that child
//    if parent is SERIES:   splice children into parent at this position
//    else:
//      scan children for a no-op, or for any child positioned after a
//      child that ends with a return
//      if found: rebuild the child list, dropping no-ops and stopping
//      after the first child that ends with a return
//
// Splicing a nested SERIES:            Dead-code elimination:
//
//    SERIES                              SERIES
//    ├─ A                                ├─ LET []        (no-op: dropped)
//    ├─ SERIES        ==>  SERIES        ├─ UPDATECOUNT ...
//    │  ├─ B               ├─ A          ├─ RETURN-true
//    │  └─ C               ├─ B          └─ CALL ...      (unreachable)
//    └─ D                  ├─ C                    ==>
//                          └─ D          SERIES
//                                        ├─ UPDATECOUNT ...
//                                        └─ RETURN-true
static bool OptimizeImpl(SERIES *series) {
  if (!series->IsUsed() || !series->parent) {
    return false;

  // This is a series region with only one child, so we can elevate the
  // child to replace the parent.
  } else if (series->regions.Size() == 1u) {
    const auto only_region = series->regions[0u];
    assert(only_region->parent == series);
    series->regions.Clear();
    series->ReplaceAllUsesWith(only_region);
    return true;

  // This series node's parent is also a series node.
  } else if (auto parent_series = series->parent->AsSeries();
             parent_series && !series->regions.Empty()) {
    UseList<REGION> new_siblings(parent_series);
    auto found = false;

    for (auto sibling_region : parent_series->regions) {
      assert(sibling_region->parent == parent_series);
      if (sibling_region == series) {
        for (auto child_region : series->regions) {
          assert(child_region->parent == series);
          new_siblings.AddUse(child_region);
          child_region->parent = parent_series;
          found = true;
        }
      } else {
        new_siblings.AddUse(sibling_region);
      }
    }

    assert(found);
    (void) found;

    series->regions.Clear();
    series->parent = nullptr;
    parent_series->regions.Swap(new_siblings);
    return true;

  // Erase any empty child regions.
  } else {

    auto has_unneeded = false;
    auto seen_return = false;
    REGION *seen_indirect_return = nullptr;

    for (auto region : series->regions) {
      assert(region->parent == series);

      // There's a region following a `RETURN` in a series. This is unreachable.
      if (seen_return) {
        assert(region->AsOperation() && region->AsOperation()->AsReturn() &&
               "Unreachable code in SERIES region");
        has_unneeded = true;
        break;

      } else if (auto op = region->AsOperation(); op && op->AsReturn()) {
        assert(!seen_return);
        if (seen_indirect_return) {
          has_unneeded = true;
        }
        seen_return = true;

      } else if (seen_indirect_return) {
        assert(false);
        seen_indirect_return->comment += "????? INDIRECT RETURN HERE?????";
        has_unneeded = true;
        break;

      // This region is a No-op, it's not needed.
      } else if (region->IsNoOp()) {
        has_unneeded = true;
        break;

      } else if (region->EndsWithReturn()) {
        seen_indirect_return = region;
        if (auto sub_op = region->AsOperation(); sub_op && sub_op->AsReturn()) {
          seen_return = true;
        }
      }
    }

    // Remove no-op regions, and unreachable regions.
    if (!has_unneeded) {
      return false;
    }

    UseList<REGION> new_regions(series);
    for (auto region : series->regions) {
      assert(region->parent == series);
      if (region->IsNoOp()) {
        region->parent = nullptr;

      } else if (region->EndsWithReturn()) {
        new_regions.AddUse(region);
        break;

      } else {
        new_regions.AddUse(region);
      }
    }

    series->regions.Swap(new_regions);
    return true;
  }
}

// Down-propagates all bindings of a LET region, then dissolves the region.
// A LET is a pure renaming: it pairwise binds each variable in
// `defined_vars` to the variable in `used_vars` that it aliases, and
// executes its body under those names. Because each defined variable is
// exactly an alias of its paired used variable, rewriting every use of the
// defined variable to refer directly to the used variable (copy
// propagation) preserves semantics. After propagation the pass clears both
// variable lists and replaces the LET with its body when one exists; a
// bindingless, bodiless LET reports no change and is deleted by dead-
// region removal. Eliminating LETs removes a layer of nesting, and the
// resulting uniform variable naming lets structurally identical sibling
// regions hash and compare as equal in the PARALLEL merging pass.
//
//    for (def, use) in zip(defined_vars, used_vars):
//      replace all uses of def with use
//    clear defined_vars and used_vars
//    if body: replace LET with body
//
//    SERIES                              SERIES
//    ├─ LET [x := y]                     ├─ TUPLECMP (y == z)
//    │  └─ TUPLECMP (x == z)      ==>    │  └─ UPDATECOUNT T (y, z)
//    │     └─ UPDATECOUNT T (x, z)       └─ ...
//    └─ ...
static bool OptimizeImpl(LET *let) {
  if (!let->IsUsed() || !let->parent) {
    return false;
  }

  bool changed = false;

  for (auto i = 0u, max_i = let->defined_vars.Size(); i < max_i; ++i) {
    changed = true;
    const auto var_def = let->defined_vars[i];
    const auto var_use = let->used_vars[i];
    var_def->ReplaceAllUsesWith(var_use);
  }

  let->defined_vars.Clear();
  let->used_vars.Clear();

  if (const auto body = let->body.get(); body) {
    changed = true;
    let->body.Clear();
    let->ReplaceAllUsesWith(body);
  }

  return changed;
}

// Simplify a TUPLECMP region, which pairwise compares the variables of
// `lhs_vars` against those of `rhs_vars` using `cmp_op`, executing `body`
// when the comparison holds and `false_body` when it does not. This pass
// prunes no-op bodies, statically decides variable pairs whose outcome is
// knowable at compile time (both sides are the same variable, or both sides
// are constants with visible values), and canonicalizes the operand order of
// single-pair equality comparisons. Statically deciding pairs shrinks or
// entirely eliminates the comparison, deleting whole unreachable subtrees;
// canonicalization makes structurally equivalent TUPLECMPs hash and compare
// alike, so the PARALLEL pass can deduplicate and merge them.
//
//    if body / false_body is a no-op, unlink and clear it
//    if there are no variable pairs:
//      kEqual: vacuously true  -> replace TUPLECMP with body
//      other:  vacuously false -> replace TUPLECMP with false_body
//    for each pair (l, r):
//      l is r (same variable)                     -> trivially equal
//      l, r both constant (not in a tuple finder):
//        same tag value / same literal /
//        same foreign constant                    -> trivially equal
//        distinct tags, or distinct foreign
//        constants where one is @unique           -> never equal
//    if some pair can never be equal and cmp_op is kEqual:
//      drop `body`; replace TUPLECMP with false_body
//    else drop trivially equal pairs; if none remain, recurse
//    if exactly one pair remains and cmp_op is kEqual:
//      put the higher-id variable, then any constant, on the LHS
//
// Statically false equality (C1, C2 known-distinct constants):
//
//    TUPLECMP{=, [(C1,C2)]}
//     +- body          (deleted)    ======>    false_body
//     +- false_body
//
// Trivially equal pair removal plus canonicalization (C0 a constant):
//
//    TUPLECMP{=, [(A,A),(B,C0)]}    ======>    TUPLECMP{=, [(C0,B)]}
//     +- body                                   +- body
static bool OptimizeImpl(TUPLECMP *cmp) {
  if (!cmp->IsUsed() || !cmp->parent) {
    return false;
  }

  assert(cmp->cmp_op != ComparisonOperator::kNotEqual);

  bool changed = false;

  if (auto true_body = cmp->body.get()) {
    assert(true_body->parent == cmp);
    if (true_body->IsNoOp()) {
      cmp->body->parent = nullptr;
      cmp->body.Clear();
      changed = true;
    }
  }

  if (auto false_body = cmp->false_body.get()) {
    assert(false_body->parent == cmp);
    if (false_body->IsNoOp()) {
      cmp->false_body->parent = nullptr;
      cmp->false_body.Clear();
      changed = true;
    }
  }

  auto max_i = cmp->lhs_vars.Size();

  //  if (auto parent_op = cmp->parent->AsOperation();
  //      max_i && parent_op &&
  //      cmp->cmp_op == ComparisonOperator::kEqual &&
  //      !cmp->false_body) {
  //
  //    if (auto parent_cmp = parent_op->AsTupleCompare();
  //        parent_cmp && parent_cmp->cmp_op == ComparisonOperator::kEqual &&
  //        !parent_cmp->false_body) {
  //
  //      for (auto i = 0u; i < max_i; ++i) {
  //        parent_cmp->lhs_vars.AddUse(cmp->lhs_vars[i]);
  //        parent_cmp->rhs_vars.AddUse(cmp->rhs_vars[i]);
  //        changed = true;
  //      }
  //
  //      cmp->lhs_vars.Clear();
  //      cmp->rhs_vars.Clear();
  //      max_i = 0u;
  //    }
  //  }

  // This compare has no variables being compared, so replace it with its
  // body.
  if (!max_i) {
    if (cmp->cmp_op == ComparisonOperator::kEqual) {

      if (const auto false_body = cmp->false_body.get(); false_body) {
        false_body->parent = nullptr;
        cmp->false_body.Clear();
        changed = true;
      }

      if (const auto body = cmp->body.get(); body) {
        cmp->body.Clear();
        cmp->ReplaceAllUsesWith(body);
        changed = true;
      }

    } else {

      // A strict comparison with no column pairs left (every pair was
      // trivially equal, e.g. `X < X`) compares empty tuples, which is
      // definitively false: drop the true body and promote the false body.
      if (const auto body = cmp->body.get(); body) {
        body->parent = nullptr;
        cmp->body.Clear();
        changed = true;
      }

      if (const auto false_body = cmp->false_body.get(); false_body) {
        cmp->false_body.Clear();
        cmp->ReplaceAllUsesWith(false_body);
        changed = true;
      }
    }

    return changed;
  }

  auto has_equal = false;
  auto has_unequal = false;

  std::vector<bool> equal(max_i);

  for (auto i = 0u; i < max_i; ++i) {

    VAR *const lhs = cmp->lhs_vars[i];
    VAR *const rhs = cmp->rhs_vars[i];

    if (lhs == rhs) {
      equal[i] = true;
      has_equal = true;
      continue;
    }

    DataVariable lhs_var(lhs);
    DataVariable rhs_var(rhs);
    if (!lhs_var.IsConstant() || !rhs_var.IsConstant()) {
      continue;
    }

    const auto lhs_const = lhs_var.Value();
    const auto rhs_const = rhs_var.Value();
    if (!lhs_const || !rhs_const) {
      continue;
    }

    auto lhs_is_tag = lhs_const->IsTag();
    auto rhs_is_tag = rhs_const->IsTag();
    if (lhs_is_tag && rhs_is_tag) {
      if (QueryTag::From(*lhs_const).Value() ==
          QueryTag::From(*rhs_const).Value()) {
        equal[i] = true;
        has_equal = true;
      } else {
        has_unequal = true;
      }

    } else if (!lhs_is_tag && !rhs_is_tag) {
      auto lhs_lit = lhs_const->Literal();
      auto rhs_lit = rhs_const->Literal();
      if (!lhs_lit || !rhs_lit) {
        continue;
      }

      if (lhs_lit->Literal() == rhs_lit->Literal()) {
        equal[i] = true;
        has_equal = true;

      } else if (lhs_lit->IsConstant() && rhs_lit->IsConstant()) {
        auto foreign_lhs = ParsedForeignConstant::From(*lhs_lit);
        auto foreign_rhs = ParsedForeignConstant::From(*rhs_lit);

        if (foreign_lhs == foreign_rhs) {
          equal[i] = true;
          has_equal = true;

        } else if (foreign_lhs.IsUnique() || foreign_rhs.IsUnique()) {
          has_unequal = true;
        }
      }
    }
  }

  if (has_unequal && cmp->cmp_op == ComparisonOperator::kEqual) {
    cmp->lhs_vars.Clear();
    cmp->rhs_vars.Clear();

    if (const auto body = cmp->body.get(); body) {
      body->parent = nullptr;
      cmp->body.Clear();
    }

    if (auto false_body = cmp->false_body.get(); false_body) {
      cmp->false_body.Clear();
      cmp->ReplaceAllUsesWith(false_body);
    }

    return true;
  }

  if (has_equal) {
    changed = true;

    // This comparison had some redundant comparisons, swap in the less
    // redundant ones.
    UseList<VAR> new_lhs_vars(cmp);
    UseList<VAR> new_rhs_vars(cmp);
    for (auto i = 0u; i < max_i; ++i) {
      if (!equal[i]) {
        new_lhs_vars.AddUse(cmp->lhs_vars[i]);
        new_rhs_vars.AddUse(cmp->rhs_vars[i]);
      }
    }

    max_i = new_lhs_vars.Size();
    cmp->lhs_vars.Swap(new_lhs_vars);
    cmp->rhs_vars.Swap(new_rhs_vars);

    // This comparison is trivially true or false, replace it with one of its
    // bodies.
    if (!max_i) {
      OptimizeImpl(cmp);
      return true;
    }
  }

  // Arrange things to have constant comparisons on the left-hand side.

  if (max_i == 1u && cmp->cmp_op == ComparisonOperator::kEqual) {
    assert(cmp->lhs_vars.Size() == 1u);

    VAR *lhs = cmp->lhs_vars[0];
    VAR *rhs = cmp->rhs_vars[0];

    auto orig_lhs = lhs;

    if (lhs->id < rhs->id) {
      cmp->lhs_vars.Clear();
      cmp->rhs_vars.Clear();
      cmp->lhs_vars.AddUse(rhs);
      cmp->rhs_vars.AddUse(lhs);
      std::swap(lhs, rhs);
    }

    if (!DataVariable(lhs).IsConstant() && DataVariable(rhs).IsConstant()) {
      cmp->lhs_vars.Clear();
      cmp->rhs_vars.Clear();
      cmp->lhs_vars.AddUse(rhs);
      cmp->rhs_vars.AddUse(lhs);
      std::swap(lhs, rhs);
    }

    if (orig_lhs != lhs) {
      changed = true;
    }
  }

  return changed;
}

// Prune the conditional branches of a CALL region. A CALL invokes another
// IR procedure and conditionally executes `body` if the callee returns
// `true`, or `false_body` if it returns `false`. When either branch region
// is a no-op, this pass unlinks and clears it, so the call's return value no
// longer guards anything on that path. The CALL
// itself is always kept (calls are never considered no-ops, since the
// callee may have side effects); with its branches gone, the enclosing
// SERIES/PARALLEL structure simplifies, and procedure-level deduplication
// can merge structurally identical callers and callees.
//
//    if body is a no-op:       unlink it; clear body
//    if false_body is a no-op: unlink it; clear false_body
//
//    CALL $proc(args)                          CALL $proc(args)
//     +- body:       LET []        ======>      (no conditional
//     +- false_body: PARALLEL []                 branches remain)
static bool OptimizeImpl(ProgramImpl *impl, CALL *call) {
  auto changed = false;

  if (auto true_body = call->body.get()) {
    assert(true_body->parent == call);

    if (true_body->IsNoOp()) {
      true_body->parent = nullptr;
      call->body.Clear();
      changed = true;
    }
  }

  if (auto false_body = call->false_body.get()) {
    assert(false_body->parent == call);

    if (false_body->IsNoOp()) {
      false_body->parent = nullptr;
      call->false_body.Clear();
      changed = true;
    }
  }

  if (call->body && call->false_body) {
    assert(call->body.get() != call->false_body.get());
  }

  return changed;
}

// Prune the conditional branches of a GENERATOR region. A GENERATOR applies
// a functor to its `used_vars`; `body` executes once per generated result
// (binding `defined_vars`), and `empty_body` executes when the functor
// produces no results. When either branch is a no-op, this pass unlinks and
// clears it. Once both branches are gone, a pure functor's application has
// no observable effect, so the region's IsNoOp() reports it as dead and the
// enclosing SERIES/PARALLEL passes delete it entirely; an impure functor is
// retained for its side effects.
//
//    if body is a no-op:       unlink it; clear body
//    if empty_body is a no-op: unlink it; clear empty_body
//
//    GENERATOR sum(A,B; S)                     GENERATOR sum(A,B; S)
//     +- body:       LET []        ======>     (branchless; removed
//     +- empty_body: SERIES []                  later if `sum` is pure)
static bool OptimizeImpl(ProgramImpl *impl, GENERATOR *gen) {

  auto changed = false;

  if (auto some_body = gen->body.get()) {
    assert(some_body->parent == gen);

    if (some_body->IsNoOp()) {
      some_body->parent = nullptr;
      gen->body.Clear();
      changed = true;
    }
  }
  if (auto empty_body = gen->empty_body.get()) {
    assert(empty_body->parent == gen);

    if (empty_body->IsNoOp()) {
      empty_body->parent = nullptr;
      gen->empty_body.Clear();
      changed = true;
    }
  }

  if (gen->body && gen->empty_body) {
    assert(gen->body.get() != gen->empty_body.get());
  }

  return changed;
}

// Try to eliminate an unnecessary child of an UPDATECOUNT (derivation
// counter fold) region. An UPDATECOUNT applies one signed +1/-1 to the
// tuple's derivation counter in `table` and executes `body` only on a zero
// crossing. A body that does nothing observable (`IsNoOp`) is detached.
// This is sound because a no-op child has no effect on any table, vector,
// variable, or control flow that outlives it, and it is beneficial because
// enclosing SERIES/PARALLEL passes can then erase the empty scaffolding and
// the dead-region sweep can reclaim the detached subtree. The UPDATECOUNT
// itself is never removed: the fold mutates the table's counters, so the
// region is a side effect even when the crossed body is gone.
//
//    changed = false
//    if body and body.IsNoOp():                // crossing continuation
//      detach body; changed = true
//    return changed
//
//    Before:                               After:
//      UPDATECOUNT(T, +nonrecursive)         UPDATECOUNT(T, +nonrecursive)
//      +-crossed: LET {}       (no-op)         (no child; the counter fold
//                                               still executes)
static bool OptimizeImpl(ProgramImpl *impl, UPDATECOUNT *fold) {
  auto changed = false;
  (void) impl;

  if (auto crossed_body = fold->body.get()) {
    assert(crossed_body->parent == fold);

    if (crossed_body->IsNoOp()) {
      crossed_body->parent = nullptr;
      fold->body.Clear();
      changed = true;
    }
  }

  return changed;
}

// Try to eliminate an unnecessary child of a CHANGERECORD (record-flavored
// counter fold) region. A CHANGERECORD, like an UPDATECOUNT, applies one
// signed fold to the tuple's derivation counter in `table`, but it also
// defines fresh `record_vars` bound to the table-resident record for that
// tuple, so that its child operates on the canonical stored values rather
// than the input tuple values. A `body` that does nothing observable
// (`IsNoOp`) is detached, dropping that child's uses of the record
// variables and letting enclosing SERIES/PARALLEL passes and the
// dead-region sweep clean up around it. The CHANGERECORD itself is never
// removed: the fold mutates the table's counters, so the region is a side
// effect even when the child (and therefore all uses of `record_vars`) is
// gone.
//
//    changed = false
//    if body and body.IsNoOp():                // crossing continuation
//      detach body; changed = true
//    return changed
static bool OptimizeImpl(ProgramImpl *impl, CHANGERECORD *fold) {
  auto changed = false;
  (void) impl;

  if (auto crossed_body = fold->body.get()) {
    assert(crossed_body->parent == fold);

    if (crossed_body->IsNoOp()) {
      crossed_body->parent = nullptr;
      fold->body.Clear();
      changed = true;
    }
  }

  return changed;
}

// Try to eliminate an unnecessary child of a CLAIM (row claim) region. A
// CLAIM claims the tuple's row in `table` into the overdeletion or
// addition set and the current frontier round, and executes `body` only
// when the claim succeeds (the row's first claim this batch). A body that
// does nothing observable (`IsNoOp`) is detached, letting enclosing
// SERIES/PARALLEL passes erase the empty scaffolding and the dead-region
// sweep reclaim the detached subtree. The CLAIM itself is never removed:
// the claim mutates the row's batch-scratch flags, so the region is a side
// effect even when the first-claim body is gone.
//
//    changed = false
//    if body and body.IsNoOp():                // first-claim continuation
//      detach body; changed = true
//    return changed
//
//    Before:                               After:
//      CLAIM(T, del)                         CLAIM(T, del)
//      +-claimed: LET {}       (no-op)         (no child; the claim
//                                               still executes)
static bool OptimizeImpl(ProgramImpl *impl, CLAIM *claim) {
  auto changed = false;
  (void) impl;

  if (auto claimed_body = claim->body.get()) {
    assert(claimed_body->parent == claim);

    if (claimed_body->IsNoOp()) {
      claimed_body->parent = nullptr;
      claim->body.Clear();
      changed = true;
    }
  }

  return changed;
}

// Try to eliminate unnecessary children of a CHECKMEMBER (membership gate)
// region. A CHECKMEMBER reads one named membership predicate of the tuple
// `col_values` in `table` and dispatches to at most one of two children:
// `body` if the predicate holds and `absent_body` if it does not. Each
// child that does nothing observable (`IsNoOp`) is detached. Because the
// gate itself is a pure read of table state, once both children are gone
// the whole region is dead code: it is replaced with an empty LET (a no-op
// placeholder that the surrounding LET/SERIES/PARALLEL passes then erase)
// and orphaned so the dead-region sweep reclaims it.
//
//    changed = false
//    for child in {body (member), absent_body}:
//      if child and child.IsNoOp():
//        detach child; changed = true
//    if not (body or absent_body):
//      replace check with empty LET; orphan check; changed = true
//    return changed
//
//    Before:                             After:
//      CHECKMEMBER(in-new, T, [v0, v1])    CHECKMEMBER(in-new, T, [v0, v1])
//      +-member:  SERIES [...]             +-member: SERIES [...]
//      +-absent:  LET {}       (no-op)
//
//    When every child is pruned, the pure read drives nothing:
//
//      CHECKMEMBER(in-new, T, [v0, v1]) =>  LET {}  (no-op; later erased)
//      (no children)
static bool OptimizeImpl(ProgramImpl *impl, CHECKMEMBER *check) {
  auto changed = false;

  if (auto present_body = check->body.get()) {
    assert(present_body->parent == check);

    if (present_body->IsNoOp()) {
      present_body->parent = nullptr;
      check->body.Clear();
      changed = true;
    }
  }

  if (auto absent_body = check->absent_body.get()) {
    assert(absent_body->parent == check);

    if (absent_body->IsNoOp()) {
      absent_body->parent = nullptr;
      check->absent_body.Clear();
      changed = true;
    }
  }

  // Dead code eliminate the membership gate.
  if (!check->body && !check->absent_body) {
    auto let = impl->operation_regions.CreateDerived<LET>(check->parent);
    check->ReplaceAllUsesWith(let);
    check->parent = nullptr;
    changed = true;
  }

  return changed;
}

// Try to eliminate unnecessary children of a CHECKRECORD (record-flavored
// membership gate / get record) region. A CHECKRECORD, like a CHECKMEMBER,
// reads one named membership predicate of the tuple `col_values` in
// `table` and dispatches to at most one of two children (`body` if the
// predicate holds, `absent_body` if not), but it also defines fresh
// `record_vars` bound to the table-resident record so its children operate
// on the canonical stored values. Each child that does nothing observable
// (`IsNoOp`) is detached, dropping that child's uses of the record
// variables. Because the gate is a pure read of table state and the record
// variables have no uses outside its children, once both children are gone
// the whole region is dead code: it is replaced with an empty LET (a no-op
// placeholder that the surrounding LET/SERIES/PARALLEL passes then erase)
// and orphaned so the dead-region sweep reclaims it.
//
//    changed = false
//    for child in {body (member), absent_body}:
//      if child and child.IsNoOp():
//        detach child; changed = true
//    if not (body or absent_body):
//      replace check with empty LET; orphan check; changed = true
//    return changed
static bool OptimizeImpl(ProgramImpl *impl, CHECKRECORD *check) {
  auto changed = false;

  if (auto present_body = check->body.get()) {
    assert(present_body->parent == check);

    if (present_body->IsNoOp()) {
      present_body->parent = nullptr;
      check->body.Clear();
      changed = true;
    }
  }

  if (auto absent_body = check->absent_body.get()) {
    assert(absent_body->parent == check);

    if (absent_body->IsNoOp()) {
      absent_body->parent = nullptr;
      check->absent_body.Clear();
      changed = true;
    }
  }

  // Dead code eliminate the membership gate.
  if (!check->body && !check->absent_body) {
    auto let = impl->operation_regions.CreateDerived<LET>(check->parent);
    check->ReplaceAllUsesWith(let);
    check->parent = nullptr;
    changed = true;
  }

  return changed;
}

// Per-procedure rewrite hook of the optimization driver, invoked once for
// every PROC on each fixpoint iteration. It checks structural invariants of
// the procedure -- a PROC is its own containing procedure, so its `parent`
// must point at itself, and its body (when present) must name the PROC as
// its parent -- and applies no transformation, always reporting no change.
// Dead argument elimination is intended to live here.
//
//    assert proc.parent == proc.containing_procedure  (self-parented)
//    if proc has a body: assert body.parent == proc
//    return false
//
// Before:                         After (identical):
//
//   PROC(vars..., vecs...)          PROC(vars..., vecs...)
//   +-- body                        +-- body
static bool OptimizeImpl(PROC *proc) {
  assert(proc->parent == proc->containing_procedure);
  if (auto body = proc->body.get()) {
    assert(body->parent == proc);
    (void) body;
  }
  return false;
}

static void CheckProcedures(ProgramImpl *impl) {
#ifndef NDEBUG
  for (PROC *proc : impl->procedure_regions) {
    assert(proc->parent == proc->containing_procedure);
  }
#endif
  (void) impl;
}

}  // namespace

// Optimize the whole control-flow IR to a fixpoint, then deduplicate
// structurally identical procedures. The main loop repeatedly sweeps every
// region kind with its region-local rewrite. Each region list is sorted
// deepest-first (by cached lexical depth) before its sweep because most
// rewrites either call `IsNoOp()`, which inspects children, or hoist
// children into parents; starting at the leaves lets removals "bubble up":
// deleting an innermost no-op body can turn its parent into a no-op that the
// same sweep then deletes. A sweep visits PARALLEL regions (flattening,
// duplicate-child elimination, and merging of equal children -- merging can
// append new PARALLELs, so that loop iterates by index), then INDUCTION
// regions (no-op init/output removal, init hoisting), then SERIES regions
// (flattening, no-op and unreachable-code removal), then all OPERATION
// regions dispatched by kind (LET binding down-propagation, TUPLECMP
// simplification, and no-op body pruning for CALL, GENERATOR, CHECKMEMBER,
// CHECKRECORD, UPDATECOUNT, CLAIM, CHANGERECORD, and generically every
// other operation), and finally each PROC. Regions already detached by an
// earlier rewrite in the same sweep -- unused, or whose `Ancestor()` is no
// longer linked under a procedure -- are skipped. After every sweep,
// `remove_unused` erases unreferenced regions and procedures, always keeping
// initializers, entry/primary dataflow functions, and message handlers.
// Once no sweep changes anything, procedures are deduplicated: every used
// (or raw-used) procedure other than the entry-point kinds and query message
// injectors is bucketed by `Hash`, buckets are compared pairwise with
// `Equals`, and each duplicate is replaced by one representative via
// `ReplaceAllUsesWith`. Raw (non-use-list) references held by `ProgramQuery`
// forcing functions are rewritten to the representative, which inherits
// `has_raw_use`. A final `remove_unused` deletes the dead
// duplicates.
//
//    repeat until a full pass changes nothing:
//      for KIND in [PARALLEL, INDUCTION, SERIES, OPERATION, PROC]:
//        sort regions of KIND deepest-first
//        for region in that list:
//          if region is used and still linked into a procedure:
//            changed |= OptimizeImpl(region)
//      remove unused regions and procedures
//    bucket eligible PROCs by Hash()
//    for each pair in a bucket where Equals():
//      duplicate.ReplaceAllUsesWith(representative)
//      rewrite raw uses in query forcing_function
//    remove unused regions and procedures
//
// No-op bubbling within one pass (deepest regions rewritten first):
//
//   PROC                    PROC                    PROC
//   +-- SERIES              +-- SERIES              +-- <no body>
//       +-- LET        =>       +-- <removed>  =>
//           +-- TUPLECMP
//               (no body)
//
// Procedure deduplication:
//
//   PROC $p1 { X }   CALL $p1        PROC $p1 { X }   CALL $p1
//   PROC $p2 { X }   CALL $p2   =>   (removed)        CALL $p1
void ProgramImpl::Optimize(void) {

  // A bunch of the optimizations check `region->IsNoOp()`, which looks down
  // to their children, or move children nodes into parent nodes. Thus, we want
  // to start deep and "bubble up" removal of no-ops and other things.
  auto depth_cmp =
      +[](REGION *a, REGION *b) { return a->CachedDepth() > b->CachedDepth(); };

  // Iteratively remove unused regions.
  auto remove_unused = [this](void) {
    for (size_t changed = 1; changed;) {
      CheckProcedures(this);
      changed = 0;
      changed |= parallel_regions.RemoveUnused();
      changed |= series_regions.RemoveUnused();
      changed |= operation_regions.RemoveUnused();
      changed |= join_regions.RemoveUnused();
      changed |= procedure_regions.RemoveIf([](PROC *proc) {
        switch (proc->kind) {
          case ProcedureKind::kInitializer:
          case ProcedureKind::kEntryDataFlowFunc:
          case ProcedureKind::kPrimaryDataFlowFunc:
          case ProcedureKind::kMessageHandler: return false;
          default: return !proc->has_raw_use && !proc->IsUsed();
        }
      });
      CheckProcedures(this);
    }
  };

  for (auto changed = true; changed;) {
    changed = false;

    CheckProcedures(this);

    parallel_regions.Sort(depth_cmp);

    // NOTE(pag): Optimizing parallel regions may create more parallel regions
    //            via `MergeEquals`.
    for (auto i = 0ull; i < parallel_regions.Size(); ++i) {
      const auto par = parallel_regions[i];
      if (!par->IsUsed() || !par->Ancestor()->parent) {
        continue;
      }
      changed = OptimizeImpl(this, par) | changed;
    }

    CheckProcedures(this);

    induction_regions.Sort(depth_cmp);
    for (auto induction : induction_regions) {
      if (!induction->IsUsed() || !induction->Ancestor()->parent) {
        continue;
      }
      changed = OptimizeImpl(this, induction) | changed;
    }

    CheckProcedures(this);

    series_regions.Sort(depth_cmp);
    for (auto series : series_regions) {
      if (!series->IsUsed() || !series->Ancestor()->parent) {
        continue;
      }
      changed = OptimizeImpl(series) | changed;
    }

    CheckProcedures(this);

    operation_regions.Sort(depth_cmp);
    for (auto i = 0u; i < operation_regions.Size(); ++i) {
      const auto op = operation_regions[i];
      if (!op->IsUsed() || !op->Ancestor()->parent) {
        continue;
      }

      // We try to aggressively eliminate LET bindings by down-propagating
      // variable assignments.
      if (auto let = op->AsLetBinding(); let) {
        changed = OptimizeImpl(let) | changed;
        CheckProcedures(this);

      } else if (auto tuple_cmp = op->AsTupleCompare(); tuple_cmp) {
        changed = OptimizeImpl(tuple_cmp) | changed;
        CheckProcedures(this);

      } else if (auto call = op->AsCall(); call) {
        changed = OptimizeImpl(this, call) | changed;
        CheckProcedures(this);

      } else if (auto gen = op->AsGenerate(); gen) {
        changed = OptimizeImpl(this, gen) | changed;
        CheckProcedures(this);

      } else if (auto check = op->AsCheckMember(); check) {
        changed = OptimizeImpl(this, check) | changed;
        CheckProcedures(this);

      } else if (auto get = op->AsCheckRecord(); get) {
        changed = OptimizeImpl(this, get) | changed;
        CheckProcedures(this);

      } else if (auto fold = op->AsUpdateCount(); fold) {
        changed = OptimizeImpl(this, fold) | changed;
        CheckProcedures(this);

      } else if (auto claim = op->AsClaim(); claim) {
        changed = OptimizeImpl(this, claim) | changed;
        CheckProcedures(this);

      } else if (auto emplace = op->AsChangeRecord(); emplace) {
        changed = OptimizeImpl(this, emplace) | changed;
        CheckProcedures(this);

      // All other operations check to see if they are no-ops and if so
      // remove the bodies.
      } else if (op->body) {
        assert(op->body->parent == op);
        if (op->body->IsNoOp()) {

          //          op->body->comment = "NOP? " + op->body->comment;

          op->body->parent = nullptr;
          op->body.Clear();
          changed = true;
        }
      }
    }

    for (auto proc : procedure_regions) {
      changed = OptimizeImpl(proc) | changed;
    }

    remove_unused();
  }

  // Go find possibly similar procedures.
  std::unordered_map<uint64_t, std::vector<PROC *>> similar_procs;
  for (auto proc : procedure_regions) {
    switch (proc->kind) {
      case ProcedureKind::kInitializer:
      case ProcedureKind::kEntryDataFlowFunc:
      case ProcedureKind::kPrimaryDataFlowFunc:
      case ProcedureKind::kMessageHandler:
      case ProcedureKind::kQueryMessageInjector:
        continue;
      default:
        if (proc->IsUsed() || proc->has_raw_use) {
          const auto hash = proc->Hash(UINT32_MAX);
          similar_procs[hash].emplace_back(proc);
        }
        break;
    }
  }

  std::unordered_map<PROC *, PROC *> raw_use_change;

  // Go through an compare procedures for equality and replace any unused ones.
  EqualitySet eq;
  for (auto &hash_to_procs : similar_procs) {
    auto &procs = hash_to_procs.second;

    std::vector<bool> dead(procs.size());
    for (size_t i = 0u, max_i = procs.size(); i < max_i; ++i) {
      if (dead[i]) {
        continue;
      }

      PROC *&i_proc = procs[i];

      eq.Clear();
      raw_use_change.clear();
      for (auto j = i + 1u; j < max_i; ++j) {
        if (dead[j]) {
          continue;
        }

        PROC *&j_proc = procs[j];

        if (i_proc->Equals(eq, j_proc, UINT32_MAX)) {

          if (j_proc->has_raw_use) {
            raw_use_change.emplace(j_proc, i_proc);
            i_proc->has_raw_use = true;
            j_proc->has_raw_use = false;
          }

          j_proc->ReplaceAllUsesWith(i_proc);
          dead[j] = true;

        } else {
          eq.Clear();
        }
      }

      if (raw_use_change.empty()) {
        continue;
      }

      // Rewrite the raw uses, which should be isolated to being forcing
      // functions in queries.
      for (auto &query : queries) {
        if (query.forcing_function) {
          auto &ref_to_proc = query.forcing_function->impl;
          if (auto new_proc_it = raw_use_change.find(ref_to_proc);
              new_proc_it != raw_use_change.end()) {
            ref_to_proc = new_proc_it->second;
          }
        }
      }
    }
  }

  remove_unused();
}

}  // namespace hyde
