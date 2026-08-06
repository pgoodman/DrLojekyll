# S5-PRIME authoritative mint-tag table — DataFlow slice 1 (253 sites)

Consolidated from the 253 fleet proposals. Convention: `const char*` string
literal, lowercase-kebab, exactly two levels `<pass>/<what>`, per-SITE constant,
survivor keeps its own tag across CSE/RAUR, never moved by field-copy helpers.
Duplicate tags are legal only for genuinely same-concept mints.

## Normalization rules applied

- **R1 (columns inherit).** A column mint carries its parent node's construction
  tag verbatim. The `.df` emitter renders the tag only on the per-VIEW ATTRIBUTES
  line (`lib/DataFlow/Format.cpp`, the `attrs_line` path — matching the diff §2b
  choice; columns get no tagged row of their own), so all the
  fleet's `-col`/`-pivot-col`/`-passthrough-col`/`-key-column`/`-value-column`
  variants are dropped — a node and its own columns are one construction concept
  (A3 same-concept sharing). Column-only canonicalization rebuilds (receiver
  `new_columns`/`new_output_columns`, where no node is minted in the same pass)
  take the pass's `canon` what-term.
- **R2 (pass, not file).** `<pass>` names the minting pass/phase, never the file.
  `constant` = the ConvertConstantInputsToTuples pass (Build.h helper called from
  Constant.cpp), not "build.h". Merge/Compare's union/comparison *sinking* is a
  distinct phase from their *canonicalization*, so the sinking sites take
  `merge-sink`/`cmp-sink`, the canon sites `merge-canon`/`cmp-canon`. The fleet's
  strategy-named prefixes (`sink-tuple`, `sink-map`, `sink-negate`, `sink-join`,
  `merge-tag`, `cmp-sink-merge`, `cmp-sink-negate`) collapse into these two, with
  the sink strategy carried in `<what>`.
- **R3 (synonyms unified).** `proxy` = a stand-in that EITHER preserves the
  interface of a real node persisting beneath it, OR assumes an eliminated node's
  interface where that node has no users to forward (the sanctioned example being
  `connect/insert-proxy`, the per-clause INSERT → TUPLE — the term is entrenched:
  `CreateProxyOfInserts`, the K5 "insert-proxy mint", so tag-vs-code vocabulary
  alignment is kept). `forward` = pass-through TUPLE that
  REPLACES an eliminated node, forwarding its users to a survivor. "stand-in" and
  "shim" are dropped in favor of `proxy`; "forwarding-tuple" → `forward`. A CMP is
  never a "proxy" (`view/comparison-proxy` → `view/comparison`).
- **R4 (no `demand/demand-`).** The pass is already `demand`; the fleet's
  `demand-relation`/`demand-merge`/`demand-reader` lose the stutter →
  `relation`/`relation-union`/`relation-reader`.
- **R5 (drop redundant node-kind suffix).** `-tuple`/`-join`/`-merge` suffixes on
  `<what>` are dropped where the qualifier alone identifies the node
  (`guard-tuple`→`guard`, `forwarding-tuple`→`forward`), and KEPT only where they
  disambiguate two same-qualifier concepts (`product-join` vs `pivot-join`;
  `negate-lifted-tuple` vs `negate-lifted`).

**Dead code note:** `merge-sink/*` (Merge.cpp 572–1676) and `cmp-sink/*`
(Compare.cpp 479–869) are the union/comparison-sinking optimizations, currently
UNREACHABLE (`do_sink` commented out — CLAUDE.md). Their tags never render in any
golden today; they are named for correctness and future re-enablement.

---

## VOCABULARY

### `<pass>` prefixes

| prefix | pass/phase |
|---|---|
| `build` | clause → data-flow builder (Build.cpp) |
| `constant` | ConvertConstantInputsToTuples (Build.h helper / Constant.cpp) |
| `merge-canon` | QueryMergeImpl canonicalization — union simplify/narrow |
| `merge-sink` | union-sinking optimization (Merge.cpp; **dead**) |
| `cmp-canon` | QueryCompareImpl canonicalization |
| `cmp-sink` | comparison-sinking optimization (Compare.cpp; **dead**) |
| `demand` | magic-sets demand transform (Demand.cpp) |
| `connect` | ConnectInsertsToSelects (Connect.cpp) |
| `link` | proxy-insertion/linking pass (Link.cpp) |
| `view` | QueryViewImpl guard/comparison construction (View.cpp) |
| `join` | QueryJoinImpl canonicalization/simplification (Join.cpp) |
| `negate` | QueryNegateImpl canonicalization (Negate.cpp) |
| `kvindex` | QueryKVIndexImpl canonicalization (KVIndex.cpp) |
| `dfe` | dead-flow elimination / dead-cycle collection (DeadFlowElimination.cpp) |
| `map` | QueryMapImpl canonicalization (Map.cpp) |
| `induction` | IdentifyInductions (Induction.cpp) |
| `identity-join` | EliminateIdentityJoins (IdentityJoin.cpp) |
| `aggregate` | QueryAggregateImpl canonicalization (Aggregate.cpp) |
| `tuple` | QueryTupleImpl canonicalization (Tuple.cpp) |

### Cross-cutting `<what>` terms (used across passes)

| term | meaning |
|---|---|
| `guard` | filtering/gating node (CMP, or column-dropping TUPLE) |
| `unused-col-guard` | proxy TUPLE dropping a view's columns unused downstream |
| `proxy` | interface-preserving stand-in interposed above the real node |
| `forward` | pass-through TUPLE replacing an eliminated node, users → survivor |
| `canon` | node's own columns rebuilt in place during its Canonicalize (the single repo-wide term for in-place output-column rebuild) |
| `union` | a MERGE node (the sole term for the MERGE node kind in `<what>` position) |
| `select` | an initial read view |
| `restore` | TUPLE re-establishing an earlier column order/shape post-transform |
| `member` | MERGE-member proxy TUPLE (Connect two-tuple clause-head shape) |
| `seed`/`prop` | demand seed / propagation machinery |
| `tag` | 16-bit disambiguating tag identity + its producing SELECT (merge-sink) |

### Per-pass `<what>` terms

- **build**: `unique-col-guard` (dup-var equality CMP), `message-io` (message
  stream), `predicate-select` (initial predicate read), `relation` (decl table),
  `inequality-guard` (non-eq body CMP), `constant-assign-guard` (var=const CMP),
  `all-constants` (all-const body TUPLE), `clause-head` (kDistinct head
  projection), `product-join` (@product cross-JOIN), `functor-map` (functor MAP),
  `functor-union` (functor-redecl equivalence UNION), `negation-subset` (named-arg
  projection of a negated select), `negate-matched` (matched cols feeding a
  NEGATION), `negate-predicate` (the `!foo(..)` NEGATION), `aggregate` (over(){}
  AGG), `pivot-join` (SIP pivot JOIN), `true-constant` (shared true token CONST +
  its SELECT), `unit-relation` (zero-arity condition relation), `condition-extend`
  (view + true-token TUPLE), `condition-select` (unit-relation read),
  `condition-test` (positive zero-arity test JOIN), `condition-restore` (post-test
  shape restore), `condition-negate-matched` (matched side of a negative test),
  `condition-negate` (negative zero-arity test NEGATION), `literal-constant`
  (assigned literal CONST + its SELECT), `message-insert` (head → message),
  `relation-insert` (head → relation), `condition-witness` (token+witness TUPLE for
  a zero-arity export head), `condition-insert` (token → unit relation).
- **constant**: `input-tuple` (all-constant inputs materialized into a TUPLE).
- **merge-canon**: `forward` (union reduced to one operand), `unused-col-guard`
  (per-operand column-drop guard + the union's own narrowed output columns).
- **merge-sink** (dead): `tuple-union`/`tuple-narrow` (sink-through-tuple sunk
  union + narrowing TUPLE), `map-input-proxy`/`map-union`/`shared-map`
  (sink-through-map), `negate-union`/`negate-input-proxy` (non-tag negate sink),
  `negate-tag-proxy`/`negate-tagged-pred`/`negate-tagged-view-union`/
  `negate-tagged-pred-union`/`negate-tag-aware`/`negate-tag-strip` (tag-mode negate
  sink), `negate-shared` (non-tag shared NEGATE), `tag` (CreateTag identity + tag
  SELECT), `join-lifted`/`join-union`/`join-tagged-view`/`join-tag-strip`
  (lift-join-through-merge).
- **cmp-canon**: `trivial-eq` (A=A forward), `trivial-ne` (distinct-const forward),
  `canon` (CMP's own rebuilt columns).
- **cmp-sink** (dead): `merge-lifted`/`merge-replica` (sink through predecessor
  MERGE), `negate-lifted-tuple`/`negate-lifted`/`negate-lowered` (sink through
  NEGATE).
- **demand**: `guard-join` (d_p ⋈ p), `guard-restore` (post-join shape restore),
  `seed-io`/`seed-receive`/`seed-head`/`seed-member`/`raw-seed` (fabricated demand
  seed), `relation`/`relation-union`/`relation-reader` (the d_p #local relation, its
  MERGE, its shared reader), `prop-projection`/`prop-member` (per-subgoal demand
  propagation), `guard-union` (R-DUP multi-guard MERGE).
- **connect**: `insert-proxy` (per-clause INSERT → TUPLE), `insert-union` (the
  always-present per-relation UNION), `kv-index` (KVINDEX + key/value columns),
  `kv-reorder` (split-column reassembly TUPLE), `select-proxy` (per-clause SELECT →
  TUPLE), `transmit-insert` (unified message-transmit INSERT), `receive-select`
  (unified multi-receive SELECT), `query-insert` (#query relation's re-materialized
  INSERT).
- **link**: `insert-proxy`, `negated-view-proxy`, `negate-source-proxy`,
  `join-input-proxy`, `merge-input-proxy` (each a TUPLE proxying the named
  operand/source so the invariant holds).
- **view**: `guard` (pass-through interface guard), `opt-guard` (best-source guard),
  `comparison` (explicit lhs-op-rhs CMP), `comparison-restore` (post-CMP shape
  restore).
- **join**: `trivial` (single-view JOIN → TUPLE), `unused-col-guard`, `dedup-restore`
  (pre-dedup shape restore), `canon` (JOIN's own rebuilt columns — narrow & dedup).
- **negate**: `vacuous-true` (unsatisfiable NEGATE → TUPLE), `canon`.
- **kvindex**: `unused-values` (all-value-unused KVINDEX → TUPLE), `canon`.
- **dfe**: `negate-passthrough` (empty/dead negated view folded to a TUPLE).
- **map** / **aggregate** / **tuple**: `canon`.
- **induction**: `leave-union` (induction-leave exit UNION).
- **identity-join**: `forward` (identity JOIN eliminated → survivor TUPLE).

---

## TABLE

### lib/DataFlow/Build.cpp (76)

| line | receiver | kind | tag |
|---|---|---|---|
| 197 | compares | CMP | `build/unique-col-guard` |
| 201 | cmp->columns | COL | `build/unique-col-guard` |
| 208 | cmp->columns | COL | `build/unique-col-guard` |
| 230 | ios | IO | `build/message-io` |
| 233 | selects | SELECT | `build/predicate-select` |
| 244 | relations | RELATION | `build/relation` |
| 247 | selects | SELECT | `build/predicate-select` |
| 265 | view->columns | COL | `build/predicate-select` |
| 348 | compares | CMP | `build/inequality-guard` |
| 355 | filter->columns | COL | `build/inequality-guard` |
| 356 | filter->columns | COL | `build/inequality-guard` |
| 361 | filter->columns | COL | `build/inequality-guard` |
| 463 | compares | CMP | `build/constant-assign-guard` |
| 470 | cmp->columns | COL | `build/constant-assign-guard` |
| 476 | cmp->columns | COL | `build/constant-assign-guard` |
| 498 | tuples | TUPLE | `build/all-constants` |
| 503 | tuple->columns | COL | `build/all-constants` |
| 540 | tuples | TUPLE | `build/clause-head` |
| 555 | tuple->columns | COL | `build/clause-head` |
| 638 | joins | JOIN | `build/product-join` |
| 652 | join->columns | COL | `build/product-join` |
| 709 | maps | MAP | `build/functor-map` |
| 727 | map->columns | COL | `build/functor-map` |
| 731 | map->columns | COL | `build/functor-map` |
| 749 | map->columns | COL | `build/functor-map` |
| 763 | map->columns | COL | `build/functor-map` |
| 797 | merges | MERGE | `build/functor-union` |
| 803 | merge->columns | COL | `build/functor-union` |
| 870 | tuples | TUPLE | `build/negation-subset` |
| 883 | tuple->columns | COL | `build/negation-subset` |
| 892 | tuples | TUPLE | `build/negate-matched` |
| 899 | negated_view->columns | COL | `build/negate-matched` |
| 904 | negations | NEGATION | `build/negate-predicate` |
| 914 | negate->columns | COL | `build/negate-predicate` |
| 929 | negate->columns | COL | `build/negate-predicate` |
| 1054 | aggregates | AGG | `build/aggregate` |
| 1068 | view->columns | COL | `build/aggregate` |
| 1095 | view->columns | COL | `build/aggregate` |
| 1133 | view->columns | COL | `build/aggregate` |
| 1247 | joins | JOIN | `build/pivot-join` |
| 1270 | join->columns | COL | `build/pivot-join` |
| 1289 | join->columns | COL | `build/pivot-join` |
| 1397 | constants | CONST | `build/true-constant` |
| 1399 | selects | SELECT | `build/true-constant` |
| 1400 | select->columns | COL | `build/true-constant` |
| 1412 | relations | RELATION | `build/unit-relation` |
| 1424 | tuples | TUPLE | `build/condition-extend` |
| 1434 | ext->columns | COL | `build/condition-extend` |
| 1440 | ext->columns | COL | `build/condition-extend` |
| 1467 | selects | SELECT | `build/condition-select` |
| 1470 | sel->columns | COL | `build/condition-select` |
| 1477 | joins | JOIN | `build/condition-test` |
| 1490 | join->columns | COL | `build/condition-test` |
| 1501 | join->columns | COL | `build/condition-test` |
| 1510 | tuples | TUPLE | `build/condition-restore` |
| 1515 | proj->columns | COL | `build/condition-restore` |
| 1540 | selects | SELECT | `build/condition-select` |
| 1543 | sel->columns | COL | `build/condition-select` |
| 1546 | tuples | TUPLE | `build/condition-negate-matched` |
| 1550 | negated_view->columns | COL | `build/condition-negate-matched` |
| 1557 | negations | NEGATION | `build/condition-negate` |
| 1565 | negate->columns | COL | `build/condition-negate` |
| 1571 | negate->columns | COL | `build/condition-negate` |
| 1576 | tuples | TUPLE | `build/condition-restore` |
| 1581 | proj->columns | COL | `build/condition-restore` |
| 1754 | constants | CONST | `build/literal-constant` |
| 1755 | selects | SELECT | `build/literal-constant` |
| 1757 | select->columns | COL | `build/literal-constant` |
| 2232 | ios | IO | `build/message-io` |
| 2234 | inserts | INSERT | `build/message-insert` |
| 2241 | relations | RELATION | `build/relation` |
| 2243 | inserts | INSERT | `build/relation-insert` |
| 2263 | tuples | TUPLE | `build/condition-witness` |
| 2273 | witness->columns | COL | `build/condition-witness` |
| 2279 | witness->columns | COL | `build/condition-witness` |
| 2282 | inserts | INSERT | `build/condition-insert` |

### lib/DataFlow/Merge.cpp (54)

| line | receiver | kind | tag |
|---|---|---|---|
| 260 | query->tuples | TUPLE | `merge-canon/forward` |
| 265 | tuple->columns | COL | `merge-canon/forward` |
| 302 | query->tuples | TUPLE | `merge-canon/unused-col-guard` |
| 313 | guarded_view->columns | COL | `merge-canon/unused-col-guard` |
| 355 | new_columns | COL | `merge-canon/unused-col-guard` |
| 572 | impl->merges | MERGE | `merge-sink/tuple-union` |
| 579 | sunk_merge->columns | COL | `merge-sink/tuple-union` |
| 586 | impl->tuples | TUPLE | `merge-sink/tuple-narrow` |
| 591 | merged_tuple->columns | COL | `merge-sink/tuple-narrow` |
| 646 | impl->tuples | TUPLE | `merge-sink/map-input-proxy` |
| 652 | tuple->columns | COL | `merge-sink/map-input-proxy` |
| 658 | tuple->columns | COL | `merge-sink/map-input-proxy` |
| 777 | impl->merges | MERGE | `merge-sink/map-union` |
| 784 | sunk_merge->columns | COL | `merge-sink/map-union` |
| 790 | impl->maps | MAP | `merge-sink/shared-map` |
| 795 | merged_map->columns | COL | `merge-sink/shared-map` |
| 850 | impl->merges | MERGE | `merge-sink/negate-union` |
| 856 | merge->columns | COL | `merge-sink/negate-union` |
| 864 | impl->tuples | TUPLE | `merge-sink/negate-input-proxy` |
| 868 | tuple->columns | COL | `merge-sink/negate-input-proxy` |
| 896 | impl->tuples | TUPLE | `merge-sink/negate-tag-proxy` |
| 897 | tuple->columns | COL | `merge-sink/negate-tag-proxy` |
| 905 | tuple->columns | COL | `merge-sink/negate-tag-proxy` |
| 917 | impl->tuples | TUPLE | `merge-sink/negate-tagged-pred` |
| 919 | tuple->columns | COL | `merge-sink/negate-tagged-pred` |
| 927 | tuple->columns | COL | `merge-sink/negate-tagged-pred` |
| 937 | tuple->columns | COL | `merge-sink/negate-tagged-pred` |
| 965 | impl->tags | TAG | `merge-sink/tag` |
| 967 | impl->selects | SELECT | `merge-sink/tag` |
| 968 | select->columns | COL | `merge-sink/tag` |
| 1119 | impl->merges | MERGE | `merge-sink/negate-tagged-view-union` |
| 1124 | negated_view_merge->columns | COL | `merge-sink/negate-tagged-view-union` |
| 1129 | impl->merges | MERGE | `merge-sink/negate-tagged-pred-union` |
| 1131 | input_merge->columns | COL | `merge-sink/negate-tagged-pred-union` |
| 1135 | input_merge->columns | COL | `merge-sink/negate-tagged-pred-union` |
| 1145 | impl->negations | NEGATION | `merge-sink/negate-tag-aware` |
| 1156 | merged_negation->columns | COL | `merge-sink/negate-tag-aware` |
| 1165 | merged_negation->columns | COL | `merge-sink/negate-tag-aware` |
| 1174 | merged_negation->columns | COL | `merge-sink/negate-tag-aware` |
| 1180 | impl->tuples | TUPLE | `merge-sink/negate-tag-strip` |
| 1183 | output->columns | COL | `merge-sink/negate-tag-strip` |
| 1223 | impl->negations | NEGATION | `merge-sink/negate-shared` |
| 1235 | merged_negation->columns | COL | `merge-sink/negate-shared` |
| 1546 | impl->joins | JOIN | `merge-sink/join-lifted` |
| 1559 | impl->merges | MERGE | `merge-sink/join-union` |
| 1575 | impl->tuples | TUPLE | `merge-sink/join-tagged-view` |
| 1584 | tagged_ith->columns | COL | `merge-sink/join-tagged-view` |
| 1600 | tagged_ith->columns | COL | `merge-sink/join-tagged-view` |
| 1615 | tagged_ith->columns | COL | `merge-sink/join-tagged-view` |
| 1631 | sunk_merge->columns | COL | `merge-sink/join-union` |
| 1644 | lifted_join->columns | COL | `merge-sink/join-lifted` |
| 1658 | lifted_join->columns | COL | `merge-sink/join-lifted` |
| 1673 | impl->tuples | TUPLE | `merge-sink/join-tag-strip` |
| 1676 | join_without_tag->columns | COL | `merge-sink/join-tag-strip` |

### lib/DataFlow/Compare.cpp (28)

| line | receiver | kind | tag |
|---|---|---|---|
| 157 | query->tuples | TUPLE | `cmp-canon/trivial-eq` |
| 162 | tuple->columns | COL | `cmp-canon/trivial-eq` |
| 166 | tuple->columns | COL | `cmp-canon/trivial-eq` |
| 199 | query->tuples | TUPLE | `cmp-canon/trivial-ne` |
| 204 | tuple->columns | COL | `cmp-canon/trivial-ne` |
| 206 | tuple->columns | COL | `cmp-canon/trivial-ne` |
| 211 | tuple->columns | COL | `cmp-canon/trivial-ne` |
| 326 | new_columns | COL | `cmp-canon/canon` |
| 332 | new_columns | COL | `cmp-canon/canon` |
| 334 | new_columns | COL | `cmp-canon/canon` |
| 353 | new_columns | COL | `cmp-canon/canon` |
| 479 | query->merges | MERGE | `cmp-sink/merge-lifted` |
| 488 | lifted_merge->columns | COL | `cmp-sink/merge-lifted` |
| 499 | query->compares | CMP | `cmp-sink/merge-replica` |
| 523 | sunk_cmp->columns | COL | `cmp-sink/merge-replica` |
| 531 | sunk_cmp->columns | COL | `cmp-sink/merge-replica` |
| 533 | sunk_cmp->columns | COL | `cmp-sink/merge-replica` |
| 543 | sunk_cmp->columns | COL | `cmp-sink/merge-replica` |
| 549 | sunk_cmp->columns | COL | `cmp-sink/merge-replica` |
| 603 | query->tuples | TUPLE | `cmp-sink/negate-lifted-tuple` |
| 604 | query->negations | NEGATION | `cmp-sink/negate-lifted` |
| 605 | query->compares | CMP | `cmp-sink/negate-lowered` |
| 691 | lifted_tuple->columns | COL | `cmp-sink/negate-lifted-tuple` |
| 703 | lifted_negate->columns | COL | `cmp-sink/negate-lifted` |
| 716 | lowered_cmp->columns | COL | `cmp-sink/negate-lowered` |
| 727 | lowered_cmp->columns | COL | `cmp-sink/negate-lowered` |
| 801 | lifted_negate->columns | COL | `cmp-sink/negate-lifted` |
| 869 | lowered_cmp->columns | COL | `cmp-sink/negate-lowered` |

### lib/DataFlow/Demand.cpp (25)

| line | receiver | kind | tag |
|---|---|---|---|
| 168 | query->joins | JOIN | `demand/guard-join` |
| 188 | join->columns | COL | `demand/guard-join` |
| 205 | join->columns | COL | `demand/guard-join` |
| 217 | query->tuples | TUPLE | `demand/guard-restore` |
| 226 | proj->columns | COL | `demand/guard-restore` |
| 1097 | ios | IO | `demand/seed-io` |
| 1100 | selects | SELECT | `demand/seed-receive` |
| 1107 | recv->columns | COL | `demand/seed-receive` |
| 1118 | relations | RELATION | `demand/relation` |
| 1122 | tuples | TUPLE | `demand/seed-head` |
| 1129 | root_head->columns | COL | `demand/seed-head` |
| 1132 | tuples | TUPLE | `demand/seed-member` |
| 1139 | root_member->columns | COL | `demand/seed-member` |
| 1158 | tuples | TUPLE | `demand/prop-projection` |
| 1165 | proj->columns | COL | `demand/prop-projection` |
| 1167 | tuples | TUPLE | `demand/prop-member` |
| 1174 | prop_member->columns | COL | `demand/prop-member` |
| 1188 | merges | MERGE | `demand/relation-union` |
| 1194 | d_merge->columns | COL | `demand/relation-union` |
| 1203 | tuples | TUPLE | `demand/relation-reader` |
| 1210 | d_reader->columns | COL | `demand/relation-reader` |
| 1263 | tuples | TUPLE | `demand/raw-seed` |
| 1270 | raw_seed->columns | COL | `demand/raw-seed` |
| 1417 | merges | MERGE | `demand/guard-union` |
| 1424 | um->columns | COL | `demand/guard-union` |

### lib/DataFlow/Connect.cpp (16)

| line | receiver | kind | tag |
|---|---|---|---|
| 38 | impl->tuples | TUPLE | `connect/insert-proxy` |
| 48 | proxy->columns | COL | `connect/insert-proxy` |
| 57 | impl->merges | MERGE | `connect/insert-union` |
| 63 | merge->columns | COL | `connect/insert-union` |
| 86 | impl->kv_indices | KVINDEX | `connect/kv-index` |
| 95 | index->columns | COL | `connect/kv-index` |
| 108 | index->columns | COL | `connect/kv-index` |
| 118 | impl->tuples | TUPLE | `connect/kv-reorder` |
| 121 | proxy->columns | COL | `connect/kv-reorder` |
| 139 | impl->tuples | TUPLE | `connect/select-proxy` |
| 150 | proxy->columns | COL | `connect/select-proxy` |
| 195 | inserts | INSERT | `connect/transmit-insert` |
| 212 | selects | SELECT | `connect/receive-select` |
| 214 | selects | SELECT | `connect/receive-select` |
| 219 | select->columns | COL | `connect/receive-select` |
| 304 | inserts | INSERT | `connect/query-insert` |

### lib/DataFlow/Link.cpp (12)

| line | receiver | kind | tag |
|---|---|---|---|
| 13 | impl->tuples | TUPLE | `link/insert-proxy` |
| 26 | proxy->columns | COL | `link/insert-proxy` |
| 36 | proxy->columns | COL | `link/insert-proxy` |
| 62 | impl->tuples | TUPLE | `link/negated-view-proxy` |
| 76 | tuple->columns | COL | `link/negated-view-proxy` |
| 91 | impl->tuples | TUPLE | `link/negate-source-proxy` |
| 105 | proxy->columns | COL | `link/negate-source-proxy` |
| 111 | proxy->columns | COL | `link/negate-source-proxy` |
| 155 | impl->tuples | TUPLE | `link/join-input-proxy` |
| 172 | proxy->columns | COL | `link/join-input-proxy` |
| 211 | impl->tuples | TUPLE | `link/merge-input-proxy` |
| 247 | proxy->columns | COL | `link/merge-input-proxy` |

### lib/DataFlow/View.cpp (10)

| line | receiver | kind | tag |
|---|---|---|---|
| 855 | query->tuples | TUPLE | `view/guard` |
| 862 | tuple->columns | COL | `view/guard` |
| 942 | query->tuples | TUPLE | `view/opt-guard` |
| 959 | tuple->columns | COL | `view/opt-guard` |
| 1062 | query->compares | CMP | `view/comparison` |
| 1066 | cmp->columns | COL | `view/comparison` |
| 1078 | cmp->columns | COL | `view/comparison` |
| 1091 | cmp->columns | COL | `view/comparison` |
| 1098 | query->tuples | TUPLE | `view/comparison-restore` |
| 1104 | tuple->columns | COL | `view/comparison-restore` |

### lib/DataFlow/Join.cpp (9)

| line | receiver | kind | tag |
|---|---|---|---|
| 132 | impl->tuples | TUPLE | `join/trivial` |
| 137 | tuple->columns | COL | `join/trivial` |
| 266 | impl->tuples | TUPLE | `join/unused-col-guard` |
| 271 | tuple->columns | COL | `join/unused-col-guard` |
| 311 | new_columns | COL | `join/canon` |
| 317 | new_columns | COL | `join/canon` |
| 434 | query->tuples | TUPLE | `join/dedup-restore` |
| 436 | tuple->columns | COL | `join/dedup-restore` |
| 458 | new_columns | COL | `join/canon` |

### lib/DataFlow/Negate.cpp (4)

| line | receiver | kind | tag |
|---|---|---|---|
| 148 | query->tuples | TUPLE | `negate/vacuous-true` |
| 151 | tuple->columns | COL | `negate/vacuous-true` |
| 278 | new_columns | COL | `negate/canon` |
| 288 | new_columns | COL | `negate/canon` |

### lib/DataFlow/KVIndex.cpp (4)

| line | receiver | kind | tag |
|---|---|---|---|
| 220 | query->tuples | TUPLE | `kvindex/unused-values` |
| 227 | tuple->columns | COL | `kvindex/unused-values` |
| 283 | new_output_columns | COL | `kvindex/canon` |
| 303 | new_output_columns | COL | `kvindex/canon` |

### lib/DataFlow/DeadFlowElimination.cpp (4)

| line | receiver | kind | tag |
|---|---|---|---|
| 266 | this->tuples | TUPLE | `dfe/negate-passthrough` |
| 269 | tuple->columns | COL | `dfe/negate-passthrough` |
| 500 | this->tuples | TUPLE | `dfe/negate-passthrough` |
| 503 | tuple->columns | COL | `dfe/negate-passthrough` |

### lib/DataFlow/Map.cpp (2)

| line | receiver | kind | tag |
|---|---|---|---|
| 307 | new_columns | COL | `map/canon` |
| 323 | new_columns | COL | `map/canon` |

### lib/DataFlow/Induction.cpp (2)

| line | receiver | kind | tag |
|---|---|---|---|
| 429 | merges | MERGE | `induction/leave-union` |
| 436 | new_union->columns | COL | `induction/leave-union` |

### lib/DataFlow/IdentityJoin.cpp (2)

| line | receiver | kind | tag |
|---|---|---|---|
| 165 | query->tuples | TUPLE | `identity-join/forward` |
| 184 | tuple->columns | COL | `identity-join/forward` |

### lib/DataFlow/Build.h (2)

| line | receiver | kind | tag |
|---|---|---|---|
| 14 | impl->tuples | TUPLE | `constant/input-tuple` |
| 24 | tuple->columns | COL | `constant/input-tuple` |

### lib/DataFlow/Aggregate.cpp (2)

| line | receiver | kind | tag |
|---|---|---|---|
| 252 | new_columns | COL | `aggregate/canon` |
| 263 | new_columns | COL | `aggregate/canon` |

### lib/DataFlow/Tuple.cpp (1)

| line | receiver | kind | tag |
|---|---|---|---|
| 249 | new_columns | COL | `tuple/canon` |

---

## Counts

- Sites: 253 (Build 76, Merge 54, Compare 28, Demand 25, Connect 16, Link 12,
  View 10, Join 9, Negate 4, KVIndex 4, DFE 4, Map 2, Induction 2, IdentityJoin 2,
  Build.h 2, Aggregate 2, Tuple 1).
- Distinct tags: 103.
- Fleet proposals changed: 187 (66 unchanged).
- No true input collisions (the fleet's prefixes kept concepts distinct);
  4 near-collisions were prevented under the `merge-sink` prefix unification
  (`tuple-union`/`map-union`/`join-union` disambiguated from a shared `sunk-union`
  what-term; tag-mode `negate-tagged-view-union` vs `negate-tagged-pred-union`
  kept distinct).
