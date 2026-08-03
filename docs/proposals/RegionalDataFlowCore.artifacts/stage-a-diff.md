# Stage A — make identity explicit: the hunk-by-hunk diff

Authored 2026-08-02 against tip f0c913e0 (branch keyed-instances). This
document expands `regional-arch-pseudocode.md` §6 "Stage A" (a four-line
sketch) into an implementer-grade diff. Every hunk names the pseudocode
section it modifies, the real file/function it lands in, and its exit
gate. Anchors verified against code this pass; the SINGLE-PASS RULE
applies — the next fleet re-verifies line numbers before building on this.

Stage A charge (proposal §13 Stage A, §4): make member identity, field
identity, and aggregate input identity *typed and provable* over the
existing `QueryView` graph, with **no pipeline change** (no new pass slot
reorders, no Rel/ControlFlow edits, no runtime edits). It is a pure
front-end identity refactor. Everything Stage A introduces is either (i) a
new strong type, (ii) a build-time immutable discriminant folded into node
structural identity, or (iii) a recomputable analysis materialized once in
the `Query::Build` tail. Nothing Stage A introduces is a satellite
annotation that optimization must be *taught to preserve* — that is the F1
lesson (§design-question-c), and it is enforced structurally, not by
review.

---

## 0. Diff map (what each hunk touches)

| Hunk | Pseudocode anchor | Real site | Kind |
| --- | --- | --- | --- |
| H-A1 | §5 (identity story) | new `include/drlojekyll/DataFlow/Identity.h` | + types |
| H-A2 | §5 / §4.3 | `lib/DataFlow/Query.h` `QueryTupleImpl`, `Tuple.cpp` `Hash/Equals` | + discriminant into node identity |
| H-A3 | §5 → §4.4 | new `lib/DataFlow/RowContract.{h,cpp}` | + recomputable analysis |
| H-A4 | §1 `Query::Build` tail | `lib/DataFlow/Build.cpp:2631-2637` | + one call, post-Stratify |
| H-A5 | §5 (aggregate input) / §4.4 | `Build.cpp` clause build + `RowContract.cpp` | + explicit input member key |
| H-A6 | §5 (`- LintAggregateProjection`) | `lib/Parse/Aggregate.cpp:86-144,500` | − advisory lint, + contract validation |
| H-A7 | §6 Stage A ("new validator") + proposal §11 | `RowContract.cpp` | + V-* validators |
| H-A8 | §1 dump-timing / §4b none | new `-contract-out` sink | + dump surface (opt-in; .df untouched) |
| H-A9 | §6 Stage A exit gate | `tests/OptDiff/cases/` | witness re-pin + one new negative |

No hunk edits `lib/Rel/*`, `lib/ControlFlow/*`, `lib/CodeGen/*`, or
`include/drlojekyll/Runtime/*`. Stage A is entirely upstream of Rel.

---

## H-A1 — the named identities (diff on §5)

New public header `include/drlojekyll/DataFlow/Identity.h`, included by the
private `lib/DataFlow/Query.h` and by `RowContract.h`. Strong types, not
aliases — each is a single-field struct with an explicit domain, no
cross-domain `operator+`/`operator<`, and only intra-domain equality +
(where an order is genuinely needed) an explicit `key()` accessor.

```diff
+ // Identity.h — Stage A subset of proposal §4.1. Each type below reserves a
+ // DOMAIN; no two domains share an operator. Dense runtime indices are NOT
+ // here (ControlFlow lowering mints those — proposal §4.1 last sentence).
+
+ // ---- arrive NOW (Stage A produces AND consumes them) ----
+ struct FieldId          { uint32_t v; };   // semantic field, view-relative,
+                                             //   stable under column renumber
+ struct FieldExpression  { ... };            // a field or a proven-equal class
+                                             //   of fields (candidate-key atoms;
+                                             //   §4.4 join minimization needs it)
+ struct SemanticMemberKey{ SmallVec<FieldExpression> };  // the RowContract key
+ enum class DeltaSign : uint8_t { kAdd, kRemove };       // typed sign for the
+                                             //   support-transfer rules (§4b Step 8)
+
+ // ---- arrive NOW as a TYPE, but Rel is the OWNER (§1.3) ----
+ struct DerivationSupportCount { int64_t nr; int64_t r; };  // packed C_nr/C_r
+                                             //   domain; Stage A only NAMES it in
+                                             //   RowContract.derivation_support so a
+                                             //   count can never be silently used as
+                                             //   demand ownership. Rel keeps producing
+                                             //   the real counters; this reserves the
+                                             //   domain so §5's "count stands in for an
+                                             //   owner set" confusion is a type error.
+
+ // ---- declared NOW, INERT until later (reserve the domain early) ----
+ struct DemandSupportCount { uint32_t v; };  // proposal: "derived from exact
+                                             //   edges" — no producer until Stage C
+                                             //   (RequestEdgeRelation). Declared now
+                                             //   ONLY to forbid arithmetic against
+                                             //   DerivationSupportCount pre-emptively.
```

> **AMENDED 2026-08-02 (D3.4 candidate 1 / A-nec-1, D3.4 candidate 3, X3).**
> The block above is superseded by this authoritative note.
>
> - **candidate 1 / A-nec-1 (support types stay, populated value goes).** KEEP
>   the four domain types `DeltaSign`, `DerivationSupportCount`,
>   `DemandSupportCount`, `SupportAlgebra` exactly as declared — but Stage A
>   produces NO support VALUE. The populated `RowContract.derivation_support`
>   field, the merge-arm `DeltaSign` add/remove fold, and the join-arm support
>   product are ALL struck (they land struck in H-A3). The types earn their
>   place SOLELY via the H-A1 `static_assert` cross-domain-disjointness battery
>   (the F4 deliverable): they exist so a count can never be silently used as
>   demand ownership. The `DerivationSupportCount` comment above ("Stage A only
>   NAMES it in `RowContract.derivation_support`") is now FALSE — Stage A names
>   it only in the battery. The real support algebra reappears at Stage C, where
>   `RequestEdgeRelation` is its first genuine consumer.
> - **X3 reconciliation.** That battery is a `static_assert` unit
>   (`tests/DataFlow/IdentityTypes`), NOT a `V-*` validator. Stage C H-F's cite
>   of `V-DEMAND-SUPPORT-DERIVED` as a Stage-A *named* validator is a miscite;
>   it reconciles to this static_assert battery (Errata-5 disposition).
> - **candidate 3 (flat-key defers FieldExpression).** `FieldExpression` — and
>   the antichain-as-identity + `Minimize` machinery it feeds — DEFER to Stage B.
>   In Stage A, `SemanticMemberKey` is a flat `SmallVec<FieldId>` (a vector of
>   field ids, no proven-equal classes). Treat the `FieldExpression` line above
>   as moved into the "declared later" bucket. This is the type-level half of
>   the H-A3 flat-key RowContract; H-A3 carries the struct.

Types **deliberately NOT in Stage A** (named so the reader knows the
omission is intentional, not forgotten): `LogicalNodeId` (see OWNER-GATE
O-A2 — recommend-now, defer-allowed), `RegionId`/`LocalNodeId`/`EdgeId`/
`PortId`/`CallSiteId` (Stage B), all `*InstanceId`/`InstanceKey`/
`InstancePath`/`RootLeaseId`/`PermanentRootId` (Stage C/D),
`RegionalRequesterId`/`RequestOwnerId`/`RequestEdgeId` (Stage C),
`PortFieldId`/`PhysicalColumnId` (ControlFlow lowering), `EpochId` (Stage C
epoch semantics).

Exit gate: header compiles; a `static_assert` battery in a new
`tests/DataFlow/IdentityTypes` unit proves no cross-domain operator exists
(e.g. `!requires { DemandSupportCount{} + DerivationSupportCount{}; }`).
Zero corpus impact — no golden touches.

---

## H-A2 — MemberProjection vs DistinctProjection as a build-stamped, identity-folded discriminant (diff on §5, realizing §4.3)

**This is the load-bearing hunk and the F1-avoidance hunk.** See
OWNER-GATE O-A1 for the new-subclass alternative. The recommended Stage A
realization is a normalization-time immutable discriminant on
`QueryTupleImpl` (the node that today realizes *all* projection), folded
into the node's structural identity so CSE/canonicalization **cannot**
merge or flip it without extra migration code.

```diff
  class QueryTupleImpl final : public QueryViewImpl {         // Query.h:703
   public:
+   // Set ONCE by BuildClause / normalization (before Optimize). NEVER
+   // mutated. Part of structural identity (see Hash/Equals below), so no
+   // pass can migrate, drop, or convert it — there is nothing to preserve.
+   enum class ProjectionRole : uint8_t {
+     kMember,    // payload-hiding: preserves the input SemanticMemberKey;
+                 //   dropped columns MUST be proven functionally determined
+                 //   by the retained key (V-NO-COLLAPSE, H-A7).
+     kDistinct   // set boundary: visible output tuple BECOMES the member key;
+                 //   collapse of equal projected values is INTENDED.
+   } projection_role;
```

Normalization rule (`lib/DataFlow/Build.cpp` clause construction +
`Tuple.cpp` constructors — the sites that mint TUPLEs):

```diff
  # Every projection TUPLE is stamped at creation:
+ #   - a TUPLE realizing a CLAUSE HEAD (source-level projection into a
+ #     relation; the output relation is a Datalog SET) -> kDistinct.
+ #     This includes the aggregate over(){} synthetic body-clause head
+ #     (H-A5): its member key IS the over() column list.
+ #   - a TUPLE minted INTERNALLY by the optimizer to hide attached columns
+ #     behind a stable facade (GuardWithTuple / GuardWithOptimizedTuple,
+ #     Query.h:303/310) -> kMember. Those TUPLEs preserve the member by
+ #     construction (keep-last-edge rule) and never intend collapse.
```

> **AMENDED 2026-08-02 (D2.9 / O-A1, BROKEN-3).**
>
> - **D2.9 / O-A1 RATIFIED.** The realization is Variant 2 — the enum folded
>   into Hash/Equals structural identity (below), plus the §11 no-convert
>   validator (`V-PROJ-ROLE-STABLE`, H-A7). Node-minting Variant 1 (new
>   `QueryViewImpl` subclasses) is RECORDED REJECTED (OWNER-GATED O-A1). This is
>   no longer an open owner gate.
> - **BROKEN-3 — the COMPLETE stamping story.** The two-bullet rule above
>   accounts for only 2 of ~48 `tuples.Create()` mint sites across `lib/DataFlow`
>   (fleet audit this pass). The full story is a role default plus an enumerable
>   kDistinct exception set:
>
>   | Mint family (file, count) | Role | Why |
>   | --- | --- | --- |
>   | source-level clause-head projection (Build.cpp, the head mints among its 9) | **kDistinct** | the output relation is a Datalog SET boundary |
>   | over(){} synthetic body-clause head (H-A5) | **kDistinct** | its member key IS the over() list |
>   | ProxySelects facades (Connect.cpp ×3) | kMember | key-preserving relation facade |
>   | merge-arm facades (Merge.cpp ×10) | kMember | preserve each arm's key |
>   | guard/root/member mints (Demand.cpp ×7) | kMember | demand facades hide, never collapse |
>   | ProxyMergedViews etc. (Link.cpp ×5) | kMember | key-preserving |
>   | CMP canon facades (Compare.cpp ×3) | kMember | key-preserving |
>   | JOIN canon facades (Join.cpp ×3) | kMember | key-preserving |
>   | GuardWithTuple / GuardWithOptimizedTuple (View.cpp ×3) | kMember | the keep-last-edge facade |
>   | dead-flow folds incl. CollectDeadCycles NEGATE-fold (DeadFlowElimination.cpp ×2) | kMember | key-preserving |
>   | IdentityJoin.cpp ×1, KVIndex.cpp ×1, Negate.cpp ×1, non-head Build.cpp mints | kMember | key-preserving |
>
>   **DEFAULT for any unlisted / future passthrough facade = kMember.** It is
>   applied automatically: the `ProjectionRole` constructor argument DEFAULTS to
>   `kMember` at the one TUPLE construction helper, and ONLY the two enumerated
>   kDistinct sites (source head, over-body head) pass `kDistinct` explicitly —
>   so a new facade site inherits kMember without touching this table.
>   Justification: an optimizer facade NEVER introduces a set boundary (only
>   source heads and INSERT dedup do); it hides/reorders columns behind a stable
>   facade preserving the input member key by the keep-last-edge rule. kMember is
>   therefore CORRECT, not merely safe — and it is ALSO the safe default because
>   kMember forbids unproven column drops (V-NO-COLLAPSE), so a mis-stamp
>   hard-FAILS the compile rather than silently collapsing a set (the F2 shape).
>   > **AMENDED 2026-08-03 (panel close-out: determinism-2 / necessity-3).** The
>   > table row "demand facades hide, never collapse" and the
>   > "preserving the input member key by the keep-last-edge rule" justification
>   > are TRUE only for ACYCLIC demand facades (H-A3 Phase 2). A demand facade ON
>   > a recursive SCC MAY drop a column: in `demand_tc_witness`, `tuple.5` maps
>   > `merge.17 (F,T)` → `(From)`, dropping `T` (a `Demand.cpp` propagation
>   > projection, `:987`, off the recursive read). This does NOT collapse a set
>   > and does NOT violate V-NO-COLLAPSE — a cyclic view takes
>   > `key = AllFields(columns)` DIRECTLY from H-A3 Phase 1, and the
>   > dropped-column `DeterminedBy` check runs ONLY in Phase-2 (acyclic)
>   > transfer, never on a cyclic view. Correct scoping: demand facades are
>   > key-preserving where ACYCLIC, and cycle-covered (AllFields) where
>   > recursive; neither path collapses a set. Per-block audit +
>   > restamp (tuple.5 is `kMember`, not `kDistinct`) is in
>   > df-stage-a-desired-states.md §4.2.
> - **§11 no-convert soundness now matches coverage.** `V-PROJ-ROLE-STABLE`'s
>   structural classifier is TOTAL: it partitions every mint into
>   {source-head, over-body-head} → kDistinct vs {everything else} → kMember;
>   the audit table above IS the totality proof. The no-convert claim is sound
>   because (a) the classifier is total and deterministic, and (b) role is in
>   structural identity (Hash/Equals below), so no canonicalization path can
>   assign or flip it.

The F1-proof ownership rule (design-question-c), realized in `Tuple.cpp`:

```diff
  uint64_t QueryTupleImpl::Hash() {
-   hash = combine(kind, columns...);
+   hash = combine(kind, projection_role, columns...);   // role in identity
  }
  bool QueryTupleImpl::Equals(EqualitySet &eq, QueryViewImpl *that) {
-   ... structural compare ...
+   if (this->projection_role != that->AsTuple()->projection_role) return false;
+   ... structural compare ...
  }
```

Consequence: CSE literally cannot fold a `kMember` TUPLE into a `kDistinct`
one (they are unequal by identity), and canonicalization rewrites columns
*within* a role but has no code path that assigns `projection_role` — so it
cannot flip it. **There is no annotation to migrate through CSE.** This is
the structural inverse of `guard_annotation_index` (Query.h:479), the F1
carrier that View.cpp:684-724 *must* teach CSE to migrate. Stage A adds
zero such migration code.

Refinement-safety note (feeds the exit gate): folding role into `Equals`
can only ever *refuse* a merge that today succeeds. It cannot create a new
merge. So the only possible golden effect is a CSE merge that used to fire
no longer firing → a `.df`/`.rel` dump line that used to be shared becoming
two lines. **If any corpus `.df`/`.rel` golden changes from this hunk, that
merge was folding a member-preserving projection into a set-collapsing one
— i.e. it was silently doing exactly the F2 bug this hunk exists to
outlaw.** That divergence is CORRECT-BY-DESIGN and blessed with review +
I0 adjudication (see Escalation E-A1). The empirical claim to verify: on
the current 180-case corpus this refinement changes **zero** dumps
(expected, because internal facade TUPLEs and source-head TUPLEs are not
structurally identical over identical i/o today).

Deletion inside this hunk: none. (No forward-dangling deletion.)

---

## H-A3 — InferConservativeRowContracts over the QueryView graph (diff on §5, realizing §4.2/§4.4)

New `lib/DataFlow/RowContract.{h,cpp}`. A **pure, recomputable function of
the final graph**, materialized once (H-A4) into a `QueryImpl`-owned
side-table. Never present during Optimize → CSE/canonicalization have
nothing to preserve (design-question-c, second half). Keyed by view
(pointer today; by `LogicalNodeId` if O-A2 is taken).

> **AMENDED 2026-08-02 (T-conf-1 / T-conf-2 / D1.2(ii), A-corr-3 / BROKEN-1,
> BROKEN-2, D3.4 candidate 3).** The `RowContract` struct, the
> `InferConservativeRowContracts` driver, and the per-operator transfer arms
> below are SUPERSEDED by this block (authoritative). The original `struct`,
> `InferConservativeRowContracts`, and `TransferContract` listings are retained
> only as the pre-amendment record; where they read `impl->equivalence_sets`,
> that field DOES NOT EXIST (BROKEN-1) and the read is void.
>
> **Flat-key RowContract (D3.4 candidate 3).**
>
> ```
> struct RowContract {                    // proposal §4.2, Stage-A subset
>   SmallVec<FieldId> visible_fields;
>   SemanticMemberKey member_key;         // SmallVec<FieldId>; the RECORDED key
> };                                      // NO candidate_member_keys (antichain
>                                         //   defers to Stage B); NO
>                                         //   derivation_support (candidate 1
>                                         //   / A-nec-1 strip).
> using RowContractMap = flat_map<QueryViewImpl *, RowContract>;   // O-A2 key
> ```
>
> **Inference is a two-phase PURE GRAPH FUNCTION, not a fixpoint (T-conf-1
> rule (ii), owner D1.2).** The comment "Depth order guarantees inputs precede
> users" is FALSE — `Depth()` is a cycle-CUT, and because
> `ConnectInsertsToSelects` / `ProxySelects` replace every internal-relation
> SELECT with a kMember proxy TUPLE, the contract-dependency graph of any
> recursive program is cyclic with NO contract-leaf inside the cycle. The
> conservative cycle rule DISSOLVES the fixpoint:
>
> ```
> RowContractMap InferConservativeRowContracts(QueryImpl *impl) {
>   RowContractMap out;
>
>   // PHASE 1 — cycle rule (D1.2(ii)): every view ON A CYCLE gets the
>   // conservative key AllFields(columns), a pure function of SCC structure
>   // (NOT visitation order). "On a cycle" is realized from the STRATIFY SCC
>   // condensation (BROKEN-2 resolution): view->stratum is populated at the
>   // H-A4 slot (post impl->Stratify(log), Build.cpp:2632); Stratify assigns
>   // equal stratum ids IFF same SCC (Query.h:573; Stratify.cpp:239). A view is
>   // "on a cycle" iff its stratum has >1 member view. A size-1 SCC is never
>   // cyclic — the core invariant forbids a view being its own direct user
>   // (RelabelGroupIDs), so there is no self-loop.
>   auto stratum_size = CountViewsPerStratum(impl);   // id-ordered, deterministic
>   for (QueryViewImpl *v : impl->ViewsInDepthOrder())
>     if (stratum_size[*v->stratum] > 1)
>       out[v] = { AllFields(v->columns), AllFields(v->columns) };
>
>   // PHASE 2 — acyclic transfer: remaining views are single-view strata.
>   // Cross-SCC data edges go low->high stratum, so depth order over the
>   // acyclic condensation IS topological here: an acyclic view's inputs are
>   // either acyclic-and-earlier (already in `out`) or cyclic (Phase 1 set
>   // AllFields — a real value, NEVER empty). One pass, no fixpoint, bounded by
>   // |columns|.
>   for (QueryViewImpl *v : impl->ViewsInDepthOrder())
>     if (stratum_size[*v->stratum] == 1)
>       out[v] = TransferContract(v, out, impl);      // NO eqsets arg (BROKEN-1)
>   return out;
> }
> ```
>
> **DeterminedBy / the JOIN key are RE-SOURCED (A-corr-3 / BROKEN-1).** There is
> no `impl->equivalence_sets`. The only equivalence datum is per-view
> `view->equivalence_set` (Query.h:579) — the table-SHARING union-find, which
> carries NO column-value equality and MUST NOT be read here. Column equality is
> sourced from: (1) **column-id identity** — shared ids assigned at
> `FinalizeColumnIDs` (:2624); `all_cols_match` already compares `.Id()`; (2)
> **persisted JOIN structure** — `QueryJoinImpl::out_to_in` / `num_pivots` /
> `joined_views`; (3) **persisted CMP operand structure** —
> `QueryCompareImpl` lhs/rhs; (4) **const facts** — `const_after_init` (:2629).
>
>     DeterminedBy(col, key, impl) :=  col is const
>                                  OR  col shares a column id with a key column
>                                  OR  col is a JOIN pivot output whose pivot
>                                        partner is in the key
>                                  OR  col is a CMP operand equated to a key col
>                                  OR  col is a functor output whose bound inputs
>                                        are all in the key
>
> It reads column-id identity + persisted JOIN/CMP nodes + consts — NEVER the
> union-find. **`Minimize` DEFERS to Stage B (D3.4 candidate 3):** the Stage-A
> JOIN arm takes the conservative candidate directly (no minimization); a cyclic
> JOIN is already AllFields from Phase 1.
>
> **Per-operator transfer (acyclic views only; flat-key, no-support forms):**
>
> ```
> SELECT   : key = DeclaredKeyOf(relation) or AllFields(columns); leaf.
> TUPLE    : kMember   -> key = mapped input key; every dropped col REQUIRE
>                        DeterminedBy(col, key, impl) else V-NO-COLLAPSE.
>            kDistinct -> key = AllFields(columns).
> CMP      : key = mapped input key (compared cols are requirements, not key).
> MAP      : injective -> key = mapped input key;
>            one-to-many -> mapped input key UNION free-output fields.
> MERGE    : key = AllFields(columns).                 (no DeltaSign fold — struck)
> JOIN     : key = UNION over pivots of mapped contributor keys.   (no Minimize)
> NEGATE   : key = mapped positive-input key.
> AGGREGATE: output key = group_by UNION config; input member key read from the
>            kDistinct over-body head (H-A5); REQUIRE it realized else
>            V-AGG-INPUT-KEY.                           (no support term)
> KVINDEX  : key = explicit key columns.
> INSERT   : key = mapped input key.
> ```
>
> Escalation E-A4 (below) carries the cyclic-graph soundness gate.

```diff
+ struct RowContract {                                  // proposal §4.2
+   SmallVec<FieldId>           visible_fields;
+   SemanticMemberKey           member_key;             // the PROVEN key
+   Antichain<SemanticMemberKey> candidate_member_keys; // pre-minimization
+   SupportAlgebra              derivation_support;      // typed DerivationSupportCount
+ };
+
+ // Side-table, QueryImpl-owned, rebuilt wholesale by the call in H-A4.
+ using RowContractMap = flat_map<QueryViewImpl *, RowContract>;
+
+ RowContractMap InferConservativeRowContracts(QueryImpl *impl) {
+   RowContractMap out;
+   // Depth order guarantees inputs precede users (FinalizeDepths ran in the
+   // tail already, §1). Const facts (TrackConstAfterInit) and proven
+   // equalities (BuildEquivalenceSets) are read-only inputs — H-A4 ordering.
+   for (QueryViewImpl *v : impl->ViewsInDepthOrder())
+     out[v] = TransferContract(v, out, impl->equivalence_sets,
+                               impl->const_after_init);
+   return out;
+ }
```

### Per-operator transfer rules (proposal §4.4, spelled against the real node classes)

Each arm is written against the actual class in `lib/DataFlow/Query.h` and
its actual `UseList` members. `key_in(u)` = `out[u].member_key`; column
mapping uses the view's `input_columns`/`columns` (the in→out pairing the
canonicalizer already maintains).

```text
TransferContract(v, out, eqsets, consts):

  QuerySelectImpl (:668)            # declared relation / stream read
      # §4.4 "Declared relation": declared key else ALL declared fields
      key = DeclaredKeyOf(v.relation)  or  AllFields(v.columns)
      support = kBaseInput            # leaf; no arm collisions

  QueryTupleImpl (:703)             # THE projection node (H-A2)
      in = sole input view;  in_key = key_in(in) mapped in->out
      if v.projection_role == kMember:                     # §4.4 "Member projection"
          # preserve hidden input key; dropped cols must be determined
          key = in_key                                     # unchanged member
          for col dropped by this TUPLE:
              REQUIRE col in DeterminedBy(in_key, eqsets, consts)  # else V-NO-COLLAPSE
      else: # kDistinct                                    # §4.4 "Distinct projection"
          key = AllFields(v.columns)                       # visible tuple IS the key
      support = passthrough(in)

  QueryCompareImpl (:908)           # §4.4 "Filter/compare"
      key = key_in(input) mapped in->out          # preserve input key
      # the compared columns (lhs/rhs) become value+presence UseRequirements,
      # NOT key members
      support = passthrough(input)

  QueryMapImpl (:796)               # §4.4 pure functor, one-to-one / one-to-many
      in_key = key_in(bound-columns' source view)
      if functor is injective on its free outputs (declared @range/1-to-1):
          key = in_key  OR  equivalent output key if injectivity proves one
      else:                                         # one-to-many
          key = in_key  UNION  emitted_output_member_key(v.free_columns)
      # NOTE: impure functors are already a Program::Build feature-gap reject;
      #       Stage A does not touch that.
      support = passthrough(in)

  QueryMergeImpl (:862)             # §4.4 "Merge/union"
      key = AllFields(v.columns)                    # output member key
      # arm collisions (two arms producing the same visible tuple) CONTRIBUTE
      # derivation support -- exactly the phantom-pair source in §4b Step 8:
      support = fold over arms with DeltaSign-typed add/remove contribution
                (the NetAdded/NetDeleted asymmetric guard, Table.h:433-525,
                 lifted from comment to typed rule -- proposal §5 RUNTIME ECHO)

  QueryJoinImpl (:755)              # §4.4 "Join"
      candidate = UNION over pivots of key_in(each contributor)
      key = Minimize(candidate, eqsets)             # proven equalities collapse
            # pivot-equal columns fold to one FieldExpression class; a column
            # functionally determined by the pivot drops from the key
      support = product of contributor supports

  QueryNegateImpl (:950)            # §4.4 "Negation"
      key = key_in(positive input) mapped in->out   # positive member key
      # negated columns become a PresenceRequirement, never key members
      support = passthrough(positive input)

  QueryAggregateImpl (:826)         # §4.4 "Aggregate"  (see also H-A5)
      group = FieldsOf(v.group_by_columns) UNION FieldsOf(v.config_columns)
      key   = group                                 # OUTPUT member key = group key
      # MEMBERSHIP (the fold domain) = the INPUT view's member key, which for
      # the over(){} DistinctProjection is exactly the over() column list.
      input_member_key = key_in(v summarized-input view)   # H-A5 wiring
      REQUIRE input_member_key realized               # else V-AGG-INPUT-KEY
      support = derivation over distinct input_member_key tuples

  QueryKVIndexImpl (:731)           # §4.4 "Key/value state"
      key = FieldsOf(explicit key columns)
      # value/member identity + update algebra come from the @-algebra pragma
      # (already validated in Functor.cpp); Stage A only records the key.
      support = kv update algebra

  QueryInsertImpl (:974)            # the set boundary into a relation
      key = key_in(input) mapped in->out            # passthrough; INSERT dedups
      support = passthrough(input)
```

`DeterminedBy(key, eqsets, consts)` is the one proof primitive: a column is
determined by the key if it is a constant (`const_after_init`), or
proven-equal (`eqsets`) to a key column, or a functor output whose bound
inputs are all in the key. This is deliberately CONSERVATIVE — an
unprovable drop under `kMember` is a hard failure (V-NO-COLLAPSE), never a
silent pass. Under `kDistinct` no proof is needed (collapse is the point).

Exit gate: `TransferContract` has a unit test per arm
(`tests/DataFlow/RowContractTransfer`) with hand-built micro-graphs
asserting the produced `member_key` — this is proposal §12.1 bullet 1
("Row-contract and requirement transfer for every logical operator").

---

## H-A4 — where contracts are computed in the Query::Build tail (diff on §1, resolving design-question-d)

Contracts are computed AFTER the graph is final and AFTER the two fact
sources they read (`TrackConstAfterInit`, `BuildEquivalenceSets`) and after
`Stratify` (so the slot matches the Stage-B constraint that a demanded
subgraph's stratum is settled before anything downstream freezes). Exact
insertion, on the fleet-verified tail (`Build.cpp:2623-2637`):

> **AMENDED 2026-08-02 (BROKEN-2 slot pin).** The slot MUST be AFTER
> `impl->Stratify(log)` (Build.cpp:2632) and its error check, before the
> `return Query` at :2637 — because H-A3 Phase 1's cycle rule READS
> `view->stratum`, which is unpopulated until Stratify runs. The diff and the
> ordering paragraph below are re-anchored to the real tail and re-source the
> equivalence read (no `equivalence_sets` field exists — A-corr-3 / BROKEN-1).

```diff
   impl->FinalizeDepths();                              // :2623
   impl->FinalizeColumnIDs();                           // :2624  final column ids
   impl->TrackDifferentialUpdates(log, /*force=*/true); // :2625  SECOND call
   if (num_errors != log.Size()) return std::nullopt;   // :2626-2628
   impl->TrackConstAfterInit();                         // :2629  const facts
   BuildEquivalenceSets(impl.get());                    // :2631  table-share union-find
   impl->Stratify(log);                                 // :2632  populates view->stratum
   if (num_errors != log.Size()) return std::nullopt;   // :2633-2635
+  // Stage A: identity is now provable over the FINAL graph. view->stratum is
+  // set (BROKEN-2: the cycle rule reads it), column ids are final, const facts
+  // ready. Pure function; NOT stored during Optimize.
+  impl->row_contracts = InferConservativeRowContracts(impl.get());
+  if (!ValidateRowContracts(impl.get(), log)) {         // H-A7 validators
+    return std::nullopt;                                //   (clean diagnostics)
+  }
   return Query(std::move(impl));                        // :2637
```

Ordering dependencies made explicit (all satisfied by the slot):
`InferConservativeRowContracts` reads `view->stratum` (needs Stratify, :2632 —
BROKEN-2), column ids (needs :2624), and `const_after_init` (needs :2629). It
does NOT read the table-share union-find `view->equivalence_set` (BROKEN-1 /
A-corr-3): column equality comes from column-id identity + persisted JOIN/CMP
structure (H-A3). `projection_role` (H-A2) was stamped at build and survived
Optimize by identity. The validator call reuses the existing `num_errors !=
log.Size()` clean-diagnostic idiom — a contract failure returns
`std::nullopt` exactly like Stratify's reject, so the compile fails
cleanly, never aborts.

Because the slot is post-Stratify, the H-A8 `.contract` dump reflects
POST-Stratify state — which AGREES with H-A8's framing (contracts are a
post-Optimize graph property, pinned opt-mode only). No conflict.

Storage: `RowContractMap row_contracts;` added to `QueryImpl` (Query.h:994
class body) and exposed read-only via `Query::RowContracts()` (public
accessor, mirrors the `DemandForcings()`/`GuardAnnotations()` read-seam
pattern of §2). Rel and codegen do NOT read it in Stage A; it exists for
validators and the H-A8 dump today, and becomes Stage B's input.

Exit gate: all stdout goldens byte-identical across all 4 modes (corpus is
190 cases per `runall.sh`, not 180 — AMENDED 2026-08-03, panel close-out:
testability-oracle-5) (the call is post-everything, adds no graph node,
changes no emission).
Referee: byte-compare via `runall.sh`.

---

## H-A5 — aggregates consume an explicit input member key (diff on §5, realizing §4.4 aggregate row)

Today the aggregate reads `aggregated_columns`/`group_by_columns`/
`config_columns` (Query.h:850-859) as raw column UseLists; its multiplicity
("distinct over() projection tuples") is an EMPIRICAL property documented
in CLAUDE.md, not a typed one. Stage A makes it typed by routing the
aggregate's summarized input through the already-existing over(){} synthetic
body-clause head TUPLE, stamped `kDistinct` by H-A2, whose member key IS the
over() column list.

```diff
  # In clause construction for `f(...) over (params) { body }`:
  #   the parser already synthesizes an unnamed local whose last clause is the
  #   over-body (Aggregate.cpp:100-111). Its head projection TUPLE:
+ #   - is stamped projection_role = kDistinct (H-A2);
+ #   - its RowContract.member_key = FieldsOf(over() params)  (H-A3 kDistinct arm).
  #   The QueryAggregateImpl's fold domain is DEFINED as that member key:
+ #     aggregate multiplicity == count of distinct input-member-key tuples
+ #   which EQUALS today's "distinct over() projection tuples" -- zero behavior
+ #   change, now provable. NO new stored field on QueryAggregateImpl (the key
+ #   is read from row_contracts, keeping it recomputable / F1-safe).
```

The wiring is a read, not a store: `TransferContract`'s aggregate arm
(H-A3) already fetches `key_in(summarized-input view)`. H-A5 is the
normalization guarantee that the summarized-input view is a `kDistinct`
projection with a realized key, plus V-AGG-INPUT-KEY (H-A7) asserting it.

Exit gate: `aggregate_1`, `average_weight`, `pairwise_average_weight`,
`config_agg_1/2`, `agg_distinct_1` stdout + `.oracle`/`.monotone`/`.batches`
goldens byte-identical (multiplicity semantics unchanged). Referee:
byte-compare + the derivation-counter oracle. This is the strong evidence
that "explicit input member key" is a re-description, not a re-computation.

---

## H-A6 — replace the advisory lint with precise contract validation (diff on §5: `- LintAggregateProjection`)

DELETION. Replacement lands **in this same hunk** (H-A6/H-A7) — no
forward-dangling deletion.

```diff
- // lib/Parse/Aggregate.cpp:86-144
- static void LintAggregateProjection(ParsedAggregateImpl *agg,
-                                     const ErrorLog &log) { ... 5-warning
-   AppendWarning shapes over single-use over-body vars + wildcard args ... }
- // and its call site:
- // Aggregate.cpp:500   LintAggregateProjection(agg, context->error_log);
```

What replaces it, and the disposition of the 5 warnings (design-question-b):

The lint fired because the *intent* of a dropped column was ambiguous at
the parse layer. Under H-A2 the intent is TYPED: the over(){} body head is a
`kDistinct` projection, so dropping a non-over() column is **definitionally
the intended set collapse**. Therefore:

- **All 5 agg_distinct_1 warnings become VALIDATED-SAFE, zero become hard
  errors on this witness.** `cnt_inv`/`cnt_rec` (`_B` dropped), `cnt_ux`
  (two `_` wildcards), `cnt_rows` (`_` dropped) are each a `kDistinct`
  projection whose member key = the declared over() list; the dropped
  columns are outside the member key *by construction*. The H-A3 `kDistinct`
  arm requires no proof → no diagnostic. stdout byte-identical; **stderr
  loses its 5 advisory lines.**

- The **hard-error path** ("compile error on unproven collapse", §5 diff)
  is V-NO-COLLAPSE (H-A7): a `kMember` projection dropping a column that
  `DeterminedBy` cannot prove redundant. This shape **cannot arise from any
  existing corpus program** (source heads are `kDistinct`; internal facade
  TUPLEs preserve keys by the keep-last-edge rule), so it fires on ZERO of
  the 180. It is exercised only by the new negative witness (H-A9).

- V-AGG-INPUT-KEY (H-A7) is the aggregate-specific hard error: an aggregate
  whose summarized input has NO realizable member key. Also unreachable on
  the current corpus (every aggregate input is a realizable `kDistinct`
  projection).

Because the lint was ADVISORY-only and stderr is NOT golden-compared
(CLAUDE.md: agg_distinct_1 "only stdout is golden-compared"), removing it
touches no golden. The behavior CHANGE is: a class of programs that today
compile-with-warning now compile-clean (the ambiguity is typed away), and a
NEW class (unprovable `kMember` collapse) now hard-rejects.

> **AMENDED 2026-08-02 (T-oracle-4).** "stderr is not golden-compared" is NOT
> coverage. Pin an EXPLICIT expected-diagnostics assertion that `agg_distinct_1`
> emits **ZERO** warnings after the lint→contract swap (authored as part of the
> H-A9 witness re-pin — an expected-diagnostic count of 0, not merely "stdout
> unchanged"). Under the typed model there is no ambiguous-collapse class left
> to diagnose (kDistinct collapse = intended; kMember unprovable collapse = hard
> error), so the original "genuinely-ambiguous collapse still produces a
> diagnostic" half is DROPPED. The remaining risk is a lost courtesy warning
> (UX), not a semantic regression — the multiplicity trap stays refereed by
> `bin/Oracle`.

---

## H-A7 — validators (diff on §6 Stage A "new validator"; proposal §11 rows)

All in `RowContract.cpp`, called by `ValidateRowContracts` from H-A4. Two
severity classes, matching §2's two-failure-class discipline: user-facing
clean diagnostics (`log.Append`, return false, no abort) vs internal-
invariant belts (fprintf+abort, survive NDEBUG). Named V-* per house style.

```diff
+ V-MEMBERKEY-REALIZED   (clean diag)  proposal §11 "unrealized semantic member
+     identity": every live view's RowContract.member_key is non-empty and every
+     FieldExpression in it resolves to a live column.
+
+ V-NO-COLLAPSE          (clean diag)  proposal §11 "accidental collapse outside
+     DistinctProjection": a kMember TUPLE dropping a column not DeterminedBy its
+     retained key. THE replacement for the deleted lint's hard half.
+
+ V-AGG-INPUT-KEY        (clean diag)  proposal §4.4 aggregate / §13 A.4: every
+     QueryAggregateImpl's summarized-input contract has a realized member_key.
+
+ V-PROJ-ROLE-STABLE     (internal belt, fprintf+abort)  design-question-c backstop:
+     re-derive each TUPLE's role from its structural position (source-head vs
+     internal-facade) and assert == the build-stamped projection_role. This is a
+     BELT, not the mechanism -- the mechanism is role-in-identity (H-A2). It fires
+     only if some future pass invents a code path that assigns projection_role.
+
+ V-CONTRACT-CENSUS      (internal belt, fprintf+abort)  parallels the .df
+     bijection witness (Format.cpp:770): |row_contracts| == |live views|, exactly
+     one contract per live view, no dead view carrying one.
```

Exit gate: the three clean diagnostics each get a directed reject witness
(V-NO-COLLAPSE and V-AGG-INPUT-KEY share the H-A9 negative witness or get
their own; V-MEMBERKEY-REALIZED is belt-and-suspenders and may be
belt-only if no surface program can violate it — see Escalation E-A2). The
two internal belts are unit-tested by fault injection in
`tests/DataFlow/RowContractValidators` (flip a role, drop a contract,
assert abort under a death-test harness).

---

## H-A8 — the contract dump surface (diff on §1 dump-timing; resolving design-question-a)

**Does Stage A change the `.df` dump surface? NO — by design.** The default
`-df-out` dump stays byte-identical so all 5 `.df.opt.golden` sidecars
(`aggregate_1`, `barrier_neck_1`, `demand_tc_witness`, `negate_1`,
`symrec_tie_1`), all 15 `.rel*` goldens, AND the 4 codegen goldens (2
`.h.opt` + 2 `.ir.opt`) are untouched — 24 pinned dump/codegen goldens in all
(AMENDED 2026-08-03, panel close-out: testability-oracle-5; the prior "5 + 15"
omitted the codegen goldens — see the exit-gate recount note). `projection_role`
and `RowContract` are NOT rendered on the existing `.df` ATTRIBUTES lines.

> **AMENDED 2026-08-02 (D2.3 RATIFIED).** The separate `.contract` sink is now
> owner-RATIFIED (no longer a recommendation): `.df` stays byte-untouched;
> contracts live in their own `*.contract.opt.golden`, keyed by the SAME view
> ids as the `.df` dump for side-by-side review. This dump reflects POST-Stratify
> state (H-A4 slot), consistent with the opt-mode-only pinning below.

Instead, a NEW opt-in sink:

```diff
+ // bin/drlojekyll/Main.cpp: -contract-out <file>  (mirrors -df-out plumbing,
+ //   drained AFTER Program::Build in the -ir-out/-df-out AFTER shape -- §1
+ //   dump-timing split; needs finalized column ids, same as -df-out).
+ // lib/DataFlow/Format.cpp: OutputStream& operator<<(os, QueryContracts{query})
+ //   deterministic, det_seq-ordered like QueryDF: one block per live view --
+ //     <kind> ^<id> member_key=(fields...) role=member|distinct
+ //                  support=<algebra> candidate_keys={...}
+ //   with the SAME always-on det_seq bijection tripwire as QueryDF (Format.cpp
+ //   :770) so the contract traversal is provably the live-view set.
```

Re-bless policy: no existing golden is re-blessed (default `.df` unchanged).
New `.contract.opt.golden` sidecars are blessed FRESH for the witness set
(agg_distinct_1 + the projection witnesses + the new negative witness),
via the normal `runall.sh --bless` after review. Referee: byte-compare
(contracts are a deterministic pure function — no published-delta
permutation, so permcheck.py does NOT apply here).

Runall/diffrun wiring: add a `.contract` sidecar recognizer that, when
present for a case, emits `-contract-out` and byte-compares against
`goldens/<name>.contract.opt.golden` (opt mode only — contracts are a
post-Optimize graph property; the 4 optimization modes would produce
DIFFERENT contracts for a case where Optimize changes the graph, so pin
opt-mode only, exactly as the `.df.opt.golden`/`.irgold` sidecars already
do).

Escalation-adjacent risk: if H-A2's role-in-identity refinement DOES change
a corpus `.df`/`.rel` dump (it should not — see H-A2 refinement-safety),
that IS a re-bless of an existing golden and requires I0 adjudication
(Escalation E-A1).

---

## H-A9 — witness changes (diff on §6 Stage A exit gate)

### Re-pin: `agg_distinct_1`

The witness's PURPOSE migrates from "carries the projected-column lint
shape" to "carries the contract-validated distinct-projection shape". No
`.dr`, no `.main.cpp`, no `.stdout` change (byte-identical). Additions:

```diff
+ tests/OptDiff/goldens/agg_distinct_1.contract.opt.golden   (NEW sidecar)
+   asserts: each over(){} head is role=distinct with member_key = the over()
+   column list; each aggregate's input member_key == that list; no kMember
+   collapse anywhere; zero contract diagnostics.
- CLAUDE.md note "DELIBERATELY carries the projected-column lint shape"
+ CLAUDE.md note "witnesses the typed distinct-projection contract: the 5
+   former advisory warnings are now VALIDATED-SAFE (kDistinct collapse is
+   definitional); stdout unchanged, stderr clean."
```

(The CLAUDE.md edit is a doc note only — flagged here for the Stage-A
lander; this design session writes no code and edits no existing file.)

### New negative witness: `member_collapse_1` (name owner-adjustable)

The V-NO-COLLAPSE / V-AGG-INPUT-KEY carrier — an all-4-modes-diagnostic
case (encoded in `runall.sh` alongside the other expected-diagnostic cases)
whose program forces a `kMember` projection to drop a column that
`DeterminedBy` cannot prove redundant, OR an aggregate over an input with
no realizable member key. **Construction is an open item** — see Escalation
E-A2: it is not yet proven that surface Datalog can express a `kMember`
unprovable-collapse (internal facade TUPLEs are always key-preserving). If
no surface program can, V-NO-COLLAPSE degrades to a belt-only internal
invariant and the negative witness targets V-AGG-INPUT-KEY only (an
aggregate whose input is a non-key-realizable shape — likelier
constructible, e.g. an aggregate directly over a raw multi-set-valued
functor output).

> **AMENDED 2026-08-02 (T-oracle-1 / T-oracle-3 / T-oracle-4, D2.10).**
>
> - **`member_collapse_1` is MODE-SPLIT, not all-4-modes-diagnostic
>   (T-oracle-3 / D2.10).** kMember facade TUPLEs are minted only inside
>   `::Canonicalize`, which is SKIPPED under `-disable-dataflow-opt`, so
>   V-NO-COLLAPSE has nothing to fire on in `nodf`/`none`: the case REJECTS
>   under `opt`/`nocf` and COMPILES under `nodf`/`none` — exactly the
>   `kvindex_1` shape. Encode it in `runall.sh` as MODE-SPLIT (alongside
>   `kvindex_1`), NOT in the all-4-modes-diagnostic list.
>   > **AMENDED 2026-08-03 (panel close-out: testability-oracle-2).** The
>   > mode-split rationale in the bullet above rests on a FALSE BLANKET premise:
>   > "kMember facade TUPLEs are minted only inside `::Canonicalize`" is NOT
>   > true. Verified against the tree this pass:
>   > - `ConnectInsertsToSelects` (`Build.cpp`, the
>   >   `if (!impl->ConnectInsertsToSelects(log))` call — UNCONDITIONAL, runs in
>   >   ALL 4 modes) mints kMember `ProxySelects` TUPLEs via `tuples.Create()`
>   >   (`Connect.cpp:137`, called at `Connect.cpp:219` and `:273`).
>   > - `ApplyDemandTransform` (`Demand.cpp`) mints kMember demand facades
>   >   (`tuples.Create()` at `Demand.cpp:214/951/961/987/996/1033/1093`), also
>   >   un-gated w.r.t. the 4 optimization modes (the pass gates only on
>   >   `-demand`, and runs BEFORE `Optimize`).
>   > - ONLY `GuardWithTuple`/`GuardWithOptimizedTuple` (`View.cpp`) are
>   >   opt-gated: their sole callers are per-node `::Canonicalize` methods
>   >   (`Aggregate.cpp:219`, `Compare.cpp:239`, `KVIndex.cpp:271`,
>   >   `Map.cpp:219`, `Negate.cpp:191`), which run only inside
>   >   `QueryImpl::Optimize` (skipped under `-disable-dataflow-opt`).
>   >
>   > So `member_collapse_1`'s mode behavior is a FUNCTION OF THE MINT SITE its
>   > (not-yet-authored) shape routes through — NOT unconditionally MODE-SPLIT:
>   > - **Branch A — the kMember drop routes through `GuardWithTuple` /
>   >   `GuardWithOptimizedTuple` (a canon facade):** MODE-SPLIT exactly as the
>   >   bullet claims (rejects `opt`/`nocf`, compiles `nodf`/`none`, like
>   >   `kvindex_1`), because the offending facade only exists post-Canonicalize.
>   > - **Branch B — the kMember drop routes through a `ProxySelects` (Connect)
>   >   or a `Demand.cpp` mint:** ALL-4-MODES-diagnostic, because those facades
>   >   are minted before/outside `Optimize`, so V-NO-COLLAPSE has a target in
>   >   every mode. Encode in `runall.sh` as all-4-modes, NOT MODE-SPLIT.
>   >
>   > PICKING THE MINT SITE IS PART OF THE WITNESS-AUTHORING STEP (H-A9 / E-A2):
>   > the author fixes the shape, reads its actual mint family off the compiled
>   > graph, and encodes the matching branch's `runall.sh` classification. The
>   > bullet above pre-committed to Branch A on a premise that holds ONLY for the
>   > canon-facade shape. (The exit-gate `member_collapse_1` row is amended to
>   > match.)
> - **V-AGG-INPUT-KEY 4-mode-reject authoring attempt (D2.10 / T-oracle-3).**
>   During Stage-A authoring, ATTEMPT exactly one surface-constructible 4-mode
>   reject for V-AGG-INPUT-KEY — a build-time, non-canon aggregate-input shape
>   (an aggregate directly over a raw multiset-valued functor output with no
>   realizable member key) that would fire in ALL four modes because it is set
>   at mint, not at Canonicalize. **If no such shape is surface-constructible,**
>   state the belt-only softening plainly: the deleted lint's user-facing half
>   is replaced only at belt level (V-NO-COLLAPSE / V-AGG-INPUT-KEY as internal
>   invariants), and reconcile that with §13.5 (proposal §5's "compile error on
>   unproven collapse" becomes "typed away + belt", the E-A2 outcome).
> - **NEW positive witness — the role-in-`Equals` refusal branch (T-oracle-1).**
>   The load-bearing "CSE cannot fold kMember into kDistinct" property is
>   asserted-unreachable by all 180 cases (the H-A2 zero-dump gate is
>   observationally identical whether the branch refuses a real fold or is a
>   no-op). Add BOTH:
>   1. a directed positive golden — a hand-built micro-graph (or `.dr`) whose
>      `df.opt` dump would SHARE one TUPLE line pre-refinement and PROVABLY
>      splits into two lines post-refinement, pinned as a `.df.opt.golden`;
>   2. a unit test (`tests/DataFlow/RowContractValidators` or `IdentityTypes`)
>      asserting `Hash()` DIFFERS and `Equals()` returns FALSE for a TUPLE pair
>      identical except `projection_role`.
>   Without this, the refusal branch is untested, not merely unfired.
> - **`agg_distinct_1` zero-warnings assertion (T-oracle-4).** The re-pin below
>   ADDS an expected-diagnostics assertion that `agg_distinct_1` emits ZERO
>   warnings post-swap (see H-A6 amendment).

---

## Exit gate (consolidated, per surface + referee)

| Surface | Stays byte-identical? | Referee | Notes |
| --- | --- | --- | --- |
| all `.stdout` (×4 modes; 169 files, corpus 190 cases — see recount note) | YES, all | `runall.sh` byte-compare | Stage A adds no node, no emission |
| `.oracle`/`.monotone`/`.batches` | YES | `bin/Oracle` + monotone proj | multiplicity unchanged (H-A5) |
| 5 `.df.opt.golden` | YES | byte-compare | role/contract NOT rendered in `.df` (H-A8) |
| 15 `.rel*.golden` (12 opt + 3 nocf/nodf/none) | YES | byte-compare | Stage A is upstream of Rel |
| 4 codegen goldens (2 `.h.opt` + 2 `.ir.opt`) | YES | byte-compare | Stage A upstream of codegen (recount note) |
| stderr (agg_distinct_1) | NO (loses 5 warns) | not suite-compared; witness note | typed-away ambiguity (H-A6) |
| NEW `.contract.opt.golden` | n/a (fresh) | byte-compare + review bless | deterministic; permcheck N/A |
| NEW `member_collapse_1` | n/a (fresh diagnostic) | `runall.sh` expected-diag | Mode behavior is MINT-SITE-DEPENDENT (AMENDED 2026-08-03, panel close-out: testability-oracle-2): MODE-SPLIT (like `kvindex_1`) only if the kMember drop routes through `GuardWithTuple`/`GuardWithOptimizedTuple` (opt-gated canon facade); ALL-4-MODES if it routes through a `ProxySelects` (Connect, unconditional) or `Demand.cpp` mint. Pick the mint site during witness authoring, then encode the matching branch (T-oracle-3 / testability-oracle-2, H-A9) |
| NEW role-in-`Equals` positive witness | n/a (fresh) | byte-compare `.df.opt.golden` + `ctest` unit | shared TUPLE line splits in two post-refinement; Hash≠/Equals=false unit (T-oracle-1, H-A9) |
| NEW V-AGG-INPUT-KEY reject (if constructible) | n/a (fresh diagnostic) | `runall.sh` expected-diag | authoring ATTEMPT for one 4-mode surface reject; else belt-only softening (T-oracle-3, H-A9) |
| unit: IdentityTypes / TransferTransfer / Validators | n/a (fresh) | `ctest` | §12.1 bullets 1-2 |

Hard requirement: **zero stdout goldens and zero of the 24 pinned dump/codegen
goldens change** (see the AMENDED recount below). If H-A2's identity refinement
forces a dump change, STOP and route through E-A1 (I0 adjudication) — do not
bless a red-to-green or an unexplained dump delta on vibes.

> **AMENDED 2026-08-03 (panel close-out: testability-oracle-5).** The golden
> inventory in the exit-gate table above (and in the structured summary and
> H-A8) was a stale hand-count. RECOUNTED from the tree this pass:
> - **Case count = 190**, not 180 (`ls tests/OptDiff/cases/*.dr | wc -l` → 190),
>   the figure `runall.sh` reports as `SUITE: PASS (190 cases)` after the D3.3
>   landing — runall is the case-count AUTHORITY. Every "180-case" / "180
>   stdout" figure in this doc is pre-D3.3 and reads as 190 now.
> - **Pinned dump/codegen goldens** are NOT a fixed "5 `.df` + 15 `.rel` = 20".
>   The correct, drift-proof formulation is "every `.irgold`-pinned surface ×
>   mode, as compared by `runall.sh` `run_irgold`" — a tree-tracked set. Actual
>   current inventory (so a reader can sanity-check drift): **5 `.df.opt.golden`
>   + 12 `.rel.opt.golden` + 3 `.rel.{nocf,nodf,none}.golden` + 2 `.h.opt.golden`
>   + 2 `.ir.opt.golden` = 24 goldens across 15 `.irgold`-pinned cases**. The
>   old "20" UNDERCOUNTED: it dropped the 2 `.h.opt` + 2 `.ir.opt` CODEGEN
>   goldens entirely and flattened the 15 `.rel*` as one mode (they are 12 opt +
>   3 cross-mode). Stage A's byte-identity gate applies to all 24 — Stage A is
>   upstream of codegen too, so the `.h`/`.ir` goldens must stay byte-identical
>   as well. Execution goldens (a SEPARATE family, not "dump" goldens): 169
>   `.stdout` + 63 `.oracle.stdout` + 63 `.monotone.stdout`.

The single missing referee for Stage A's one risky sub-hunk (H-A2 dump
divergence) is I0, which the review ranks AFTER Stage A. Resolution
(E-A1): Stage A's SAFE completion condition is the EMPIRICAL check "role-in-
identity changes zero corpus dumps," which is mechanically verifiable
without I0. Stage A proceeds before I0 iff that check passes; only a
surprise divergence blocks on I0.

---

## OWNER-GATED decisions

> **AMENDED 2026-08-02 — ALL THREE RATIFIED (owner-adjudication-record D2.9).**
> This section is no longer open. **O-A1 = Variant 2** (enum folded into
> Hash/Equals identity + the §11 no-convert validator); node-minting Variant 1
> is RECORDED REJECTED. **O-A2 = contract map keyed by `QueryViewImpl *`**
> (`LogicalNodeId` deferred to Stage B). **O-A3 = declare `DemandSupportCount`
> now, inert** — folded into D3.4 candidate 1 (the domain type is KEPT for the
> H-A1 static_assert battery; no producer until Stage C). The recommendations
> below stand as the ratified picks.

### O-A1 — realization of the Member/Distinct distinction (§4.3)

The proposal says "two different logical operators." Two realizations:

- **Variant 1 — new `QueryViewImpl` subclasses** (`QueryMemberProjectionImpl`
  / `QueryDistinctProjectionImpl` + DefLists + `.df` tags). Most faithful to
  §4.3. Cost: every kind-switch forks (Canonicalize, CSE Hash/Equals,
  `BuildDRInventory`, the 10-kind `.df` enum at Format.cpp:743-763, Rel
  lowering); `.df` gains two tags → mass re-bless of all 5 `.df` goldens +
  ripple risk into `.rel`. Contradicts Stage A "no pipeline change." Belongs
  in Stage B, where the regional representation re-classes nodes anyway.
- **Variant 2 — build-stamped immutable `ProjectionRole` on `QueryTupleImpl`,
  folded into Hash/Equals (H-A2) + recomputable RowContract (H-A3)**
  [RECOMMENDED]. "Distinct nodes" in the TYPED sense (a domain-typed role
  that is part of node identity), zero new subclasses, `.df` untouched, F1
  handled structurally. Lower blast radius; matches "no pipeline change."

Recommendation: Variant 2 for Stage A; revisit Variant 1 at Stage B's node
re-classification. **Owner picks.**

### O-A2 — contract-map key: `QueryViewImpl *` vs `LogicalNodeId`

`RowContractMap` is keyed by raw view pointer in H-A3. Introducing
`LogicalNodeId` now (the §1.3 "Logical node identity" type) would key
contracts by stable identity instead of pointer, pre-paying the Stage-B
`(RegionId, LocalNodeId)` migration and directly addressing the §1.3
finding "equality, ordering, and origin are not one contract." Cost: a
`LogicalNodeId` stamp pass + threading. Recommend introducing it NOW as the
map key (cheap, and the contract map is its natural first consumer); defer
ALLOWED to Stage B. **Owner picks now-vs-B.**

### O-A3 — `DemandSupportCount` now-vs-Stage-C

Declared inert in H-A1 (reserve the domain, forbid cross-domain arithmetic
early) vs not declared until Stage C (no producer/consumer until
RequestEdgeRelation exists). Recommend declare-now-inert (zero cost, buys
the type-safety property early). **Owner picks.**

---

## Escalations / missing oracles

- **E-A1 (dump-divergence oracle):** H-A2's role-in-identity refinement
  *could* change a corpus `.df`/`.rel` dump if a merge that folds a member-
  preserving into a set-collapsing projection exists today. Such a change
  is CORRECT-BY-DESIGN (it is the F2 bug being outlawed) but has NO referee
  until I0 exists, and the review ranks A before I0. Mitigation: the
  mechanical "zero corpus dump change" check gates Stage A; a surprise
  divergence blocks on I0. Recorded, not resolved on vibes.

- **E-A2 (negative-witness constructibility):** it is NOT yet proven that
  surface Datalog can express a `kMember` unprovable-collapse (internal
  facade TUPLEs are key-preserving by the keep-last-edge rule; source heads
  are `kDistinct`). If no surface program can violate V-NO-COLLAPSE, that
  validator is belt-only (internal invariant) rather than a user-facing
  reject, and the H-A9 negative witness must target V-AGG-INPUT-KEY instead
  (an aggregate over a non-key-realizable input). Owner-directed: confirm
  the intended user-facing reject surface for "unproven collapse," or accept
  that Stage A's user-facing replacement for the deleted lint is its
  REMOVAL (the ambiguity is typed away) plus internal belts — a defensible
  outcome the proposal's §5 diff ("compile error on unproven collapse")
  should be reconciled with.

- **E-A3 (SupportAlgebra shape):** `RowContract.derivation_support` typed as
  `DerivationSupportCount` (Rel-owned per §1.3) is populated by Stage A only
  as a transfer-rule PLACEHOLDER (merge arm collisions, join products). The
  real support algebra is Rel's. Escalate whether Stage A should populate it
  at all or leave it a declared-but-unfilled field until Stage B/C wire Rel
  through the frozen contract. Recommend: transfer the *shape* (which arms
  contribute) but not the counts; mark the field "shape-only, Stage A."
  **AMENDED 2026-08-02 (D3.4 candidate 1 / A-nec-1):** SUPERSEDED — Stage A
  transfers NEITHER shape NOR counts. `RowContract.derivation_support`, the
  merge-arm fold, and the join-arm product are STRUCK (H-A1/H-A3 amendments);
  only the domain TYPES survive, via the H-A1 static_assert battery. This
  escalation is closed by that strip.

- **E-A4 (cyclic-graph contract soundness) — ADDED 2026-08-02 (T-conf-2 /
  D1.2).** The conservative cycle rule (D1.2(ii): every view in a multi-view
  stratum → `member_key = AllFields(columns)`, H-A3 Phase 1) makes the contract
  a PURE graph function and DISSOLVES the T-conf-1 fixpoint — a back-edge input's
  contract is never read empty, because cyclic views are assigned directly from
  SCC structure, not by transfer. **LIVE-VALIDATOR PRECONDITION:** before ANY
  `.contract` golden is blessed, `V-MEMBERKEY-REALIZED` and `V-NO-COLLAPSE` must
  run LIVE (compiled-in, not asserted on paper) over the recursive subset —
  `demand_tc_witness`, `d5_recursive_negate`, `fixpoint_stress_1`,
  `reconverge_1` — and each must compile CLEAN (zero contract diagnostics).
  Until that live run passes, the "fires on ZERO of the 180" claim is unproven
  on cyclic input. Because the cycle rule makes the contract order-independent,
  the H-A8 `.contract.opt.golden` byte-compare no longer pins a run-order
  artifact for recursive-view contracts (the pre-rule hazard T-conf-2 flagged);
  the live run is the evidence.

---

## Mechanisms carried forward or introduced (necessity-audit input)

Introduced by Stage A (one line each):

- `FieldId` / `FieldExpression` / `SemanticMemberKey` — typed field + member identity (H-A1).
- `DeltaSign` — typed add/remove for support-transfer rules (H-A1).
- `DerivationSupportCount` — typed derivation-support domain, Rel-owned, named-in-contract only (H-A1).
- `DemandSupportCount` — declared-inert reserved domain, no producer until Stage C (H-A1, O-A3).
- `QueryTupleImpl::ProjectionRole {kMember,kDistinct}` — build-stamped immutable projection discriminant (H-A2).
- role-in-Hash/Equals — the F1-proof structural ownership rule; no CSE migration code (H-A2).
- `RowContract` + `RowContractMap` — recomputable member-identity side-table, `QueryImpl`-owned (H-A3).
- `InferConservativeRowContracts` / `TransferContract` — the per-operator transfer analysis (H-A3).
- `DeterminedBy` — the conservative functional-determination proof primitive (H-A3).
- `Query::RowContracts()` — read-only accessor seam (H-A4), Stage B's input.
- explicit aggregate input member key — over(){} head as typed `kDistinct` source (H-A5).
- `ValidateRowContracts` + V-MEMBERKEY-REALIZED / V-NO-COLLAPSE / V-AGG-INPUT-KEY (clean diags) (H-A7).
- V-PROJ-ROLE-STABLE / V-CONTRACT-CENSUS (internal fprintf+abort belts) (H-A7).
- `-contract-out` sink + `QueryContracts` dump + `.contract.opt.golden` sidecar family (H-A8).
- `member_collapse_1` negative witness (or its V-AGG-INPUT-KEY variant) (H-A9).

> **AMENDED 2026-08-02.** Three bullets above are re-scoped by the amendments:
> `FieldExpression` DEFERS to Stage B (D3.4 candidate 3) — Stage-A
> `SemanticMemberKey` is a flat `SmallVec<FieldId>`; `DerivationSupportCount` is
> "named in the H-A1 static_assert battery ONLY", NOT in `RowContract` (the
> populated field is struck, D3.4 candidate 1 / A-nec-1) — likewise the
> `DeltaSign` "support-transfer rules" reduce to the static_assert battery (no
> merge/join fold). ADDED: the role-in-`Equals` positive witness + its
> Hash≠/Equals=false unit (T-oracle-1, H-A9). `member_collapse_1` is MODE-SPLIT
> (T-oracle-3).

Deleted by Stage A (replacement lands same stage — no forward dangle):

- `LintAggregateProjection` + its call site (Aggregate.cpp:86-144, :500) → V-NO-COLLAPSE + typed `kDistinct` (H-A6/H-A7).

Carried UNCHANGED across Stage A (explicitly not touched):

- the whole demand layer (`ApplyDemandTransform`, GuardAnnotation,
  RecognizedSubgraph, guard_annotation_index carrier, DemandForcings) — Stage C deletes it.
- all of Rel (BuildDRInventory, DRInstance, the 29-kind census, the V-* Rel
  validators, `.rel` dump) — survives as the local-graph lowering.
- all of ControlFlow / codegen / runtime (InstanceStore, StateCellStore,
  cursor contract) — untouched upstream.
- `Query::Build` pass sequence and the shared `gPassPolicy` bisect counter — one call appended, no reorder.

---

## Structured summary

- **Stage:** A — make identity explicit (proposal §13 Stage A; pseudocode §6 Stage A).
- **Artifact file:** `/Users/pag/Code/DrLojekyll/docs/proposals/RegionalDataFlowCore.artifacts/stage-a-diff.md`
- **Exit gate:** all `.stdout` goldens (×4 modes; corpus is 190 cases per
`runall.sh` `SUITE: PASS (190 cases)`, NOT 180 — AMENDED 2026-08-03, panel
close-out: testability-oracle-5), all 24 `.irgold`-pinned dump/codegen goldens
(5 `.df.opt` + 12 `.rel.opt` + 3 `.rel.{nocf,nodf,none}` + 2 `.h.opt` + 2
`.ir.opt` — the prior "5 `.df` + 15 `.rel*` = 20" omitted the 4 codegen
goldens), and all `.oracle`/`.monotone`/`.batches` goldens BYTE-IDENTICAL
(referee: `runall.sh` byte-compare + `bin/Oracle`); the ONLY visible corpus
change is agg_distinct_1's stderr losing 5 advisory warnings (not
suite-compared) and its purpose re-pin. New surfaces adjudicated by byte-compare + review bless: `.contract.opt.golden` sidecars (deterministic, permcheck N/A) and the `member_collapse_1` diagnostic (mode behavior MINT-SITE-DEPENDENT — see the H-A9 / exit-gate amendment). Unit tests (`ctest`) cover identity-type disjointness, per-operator transfer, and validator fault-injection. Hard requirement: zero of the 24 pinned dump/codegen goldens change (AMENDED 2026-08-03, panel close-out: testability-oracle-5 — was "20"); a surprise `.df`/`.rel` divergence from role-in-identity (H-A2) blocks on I0 (E-A1) rather than being blessed on vibes.
- **Mechanisms (full list):** see the "Mechanisms carried forward or introduced" section above — 15 introduced (5 typed ids, 1 identity-folded discriminant, RowContract + inference + proof primitive + accessor, explicit aggregate key, 3 clean-diagnostic + 2 internal-belt validators, the contract dump family, 1 negative witness), 1 deleted with same-stage replacement (LintAggregateProjection → V-NO-COLLAPSE + typed kDistinct), and the entire demand/Rel/ControlFlow/runtime stack carried UNCHANGED.
- **Escalations:** E-A1 (H-A2 dump-divergence has no referee until I0; gate on the mechanical zero-dump-change check, block on I0 only on surprise); E-A2 (whether surface Datalog can express a `kMember` unprovable-collapse — if not, V-NO-COLLAPSE is belt-only and the deleted lint's user-facing replacement is its removal, to be reconciled with proposal §5's "compile error on unproven collapse"); E-A3 (SupportAlgebra — CLOSED 2026-08-02 by the D3.4 candidate-1 strip: Stage A populates neither shape nor counts); **E-A4 (ADDED 2026-08-02, T-conf-2 / D1.2): cyclic-graph contract soundness — the conservative cycle rule (H-A3 Phase 1) dissolves the T-conf-1 fixpoint; LIVE-VALIDATOR PRECONDITION: V-MEMBERKEY-REALIZED + V-NO-COLLAPSE must run live over the recursive subset (`demand_tc_witness`, `d5_recursive_negate`, `fixpoint_stress_1`, `reconverge_1`) and compile clean before any `.contract` golden is blessed.**
- **Owner-gated (ALL RATIFIED 2026-08-02, D2.9):** O-A1 = Variant 2 (identity-folded `ProjectionRole` + §11 no-convert validator; Variant 1 rejected); O-A2 = contract-map key `QueryViewImpl*` (`LogicalNodeId` deferred to Stage B); O-A3 = declare `DemandSupportCount` now, inert (folded into D3.4 candidate 1). No longer open.
- **AMENDED 2026-08-02 corrections to this summary:** the exit-gate bullet's "`member_collapse_1` all-4-modes-diagnostic" is WRONG — it is MODE-SPLIT (T-oracle-3); the ONLY visible corpus change is agg_distinct_1's stderr losing 5 warnings, now pinned as an explicit ZERO-warnings expected-diagnostic assertion (T-oracle-4). The mechanisms count adds the role-in-`Equals` positive witness + unit (T-oracle-1); the RowContract is flat-key `{visible_fields, member_key}` (D3.4 candidate 3), `FieldExpression`/antichain/`Minimize` deferred to Stage B, `derivation_support` struck (candidate 1). DeterminedBy/JOIN key are re-sourced onto column-id identity + persisted JOIN/CMP structure, never `impl->equivalence_sets` (which does not exist — A-corr-3 / BROKEN-1). The inference slot is pinned AFTER `impl->Stratify(log)` so the cycle rule can read `view->stratum` (BROKEN-2).
- **AMENDED 2026-08-03 corrections to this summary (panel close-out).**
  (testability-oracle-2) `member_collapse_1` is not flatly MODE-SPLIT: its mode
  behavior is MINT-SITE-DEPENDENT — MODE-SPLIT only if the kMember drop routes
  through the opt-gated `GuardWithTuple`/`GuardWithOptimizedTuple` canon facade,
  ALL-4-MODES if through a `ProxySelects` (Connect, unconditional) or
  `Demand.cpp` mint (the "minted only inside `::Canonicalize`" premise is false).
  (testability-oracle-5) The pinned dump-golden count is 24, not 20 (5 `.df.opt`
  + 12 `.rel.opt` + 3 `.rel.{nocf,nodf,none}` + 2 `.h.opt` + 2 `.ir.opt`; the
  prior count omitted the 2 `.h.opt` + 2 `.ir.opt` codegen goldens), and the
  corpus is 190 cases (`runall.sh` `SUITE: PASS (190 cases)`), not 180.
