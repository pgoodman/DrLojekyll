<!-- Copyright 2026, Peter Goodman. All rights reserved. -->
# Session-33 seed — InstanceFlow: whole-program view at tip + the path forward as diffs

> START-HERE for the next session. Written at the close of session 32 (branch
> `keyed-instances`, tip `1e734d33`). InstanceFlow Phase A (the flat empty-context
> grove + `-instanceflow-out` dump + the always-on `V-IF-*` validators) is LANDED
> and byte-identical (an OBSERVER at the Query→Rel seam — codegen untouched). This
> doc re-expresses the InstanceFlow-relevant pipeline as current-tip-anchored
> pseudocode and states the path forward (CP2 → Phase B/C/D) as DIFFS on it, so
> the next session is grounded in a whole-program view before touching code.
>
> The next session's charter (`session-33-prompt.md`) is to GO DEEPER on this:
> build out the pseudocode via workflows, formulate + critique the design diffs,
> do the same for the desired IR output-states, then EXECUTE a REAL next slice.

## §0 Where we are (session-32 decisions — do NOT re-litigate)

- **Owner fork (s32):** the branch pursues **InstanceFlow** — the NEW context-family
  IR between the optimized Query and Rel (`docs/proposals/InstanceFlow.md`, the
  2198-line vision) — NOT the pragmatic keyed-`InstanceStore` restoration. The
  InstanceStore grounding is PRESERVED as the proven back-end for a LATER phase
  (`docs/proposals/RegionalDataFlowCore.artifacts/session-32-keyed-monotone-grounding.md`
  + empirical pre-cut dumps under its `.artifacts/`).
- **Owner slice (s32):** the first slice is **Phase A + the flat grove + dump**
  (InstanceFlow.md §20 Phase A + §7.3 FlatFamily). It is a FOUNDATIONAL OBSERVER —
  codegen-byte-unchanged BY DESIGN. Codegen-MOVING specialization (join-pivot
  K-context families) is Phase D, gated behind "flat-IF is Rel's sole input"
  (§24 go/no-go ruling #1). This resolves the "do something REAL vs shadow"
  tension: Phase A is real, landable, correctness-bearing infrastructure that the
  whole pipeline needs; it is honestly a shadow to codegen until Phase D.
- **LANDED (s32):** CP0 (`2a041ee3`) the typed-id catalogs + SCC-condensation
  families + the dump skeleton; CP1 (`1e734d33`) the OriginUse walk + coverage
  bijection + `covers` lines. Both gates GREEN: **OptDiff SUITE PASS (227)
  byte-identical** (observer proof; all 5 validators quiescent on every corpus
  shape) + **ctest 5/5**. Grounding: `session-32-phaseA-grounding.md` (4 forced
  re-scopes B1–B4, adopted).
- **Grounding re-scopes carried (B1–B4, all landed):** B1 total-order the use
  catalog (determinism); B2 DECOUPLE coverage from emission (V-IF-COVERAGE per
  consumer-obligation, V-IF-EMISSION only on terminal INSERTs); B3 fold into
  `lib/DataFlow` (the RowContract precedent — a separate lib caused a link cycle
  AND turned the "always-on" validators off in `bin/Oracle`); B4 the grove is an
  SCC-partitioned SET of empty-context families (§6 typed `scc_ownership`).

## §1 The whole-program pipeline as pseudocode (current tip)

```
CompileModule(module):                                    # bin/drlojekyll/Main.cpp
  query   = Query::Build(module, log, policy, demand_mode=gDemand)   # lib/DataFlow
  frozen  = FrozenRegionalProgram::Build(query, log)                 # lib/Regional
  program = Program::Build(frozen, log, first_id, policy)            # lib/ControlFlow (+ lib/Rel)
  emit C++ (program)                                                 # lib/CodeGen/CPlusPlus
  if gInstanceFlowStream: os << QueryInstanceFlow{query}             # <-- NEW s32 dump drain

Query::Build(module, log, policy, demand_mode, ...):      # lib/DataFlow/Build.cpp:2530
  impl = build initial dataflow graph from clauses
  impl.ConnectInsertsToSelects(...)
  impl.ApplyDemandTransform(...)             # demand OFF by default (flag-gated)
  if !policy.skip: impl.Optimize()           # CSE + Canonicalize fixpoint + dead-flow
  impl.ConvertConstantInputsToTuples(); impl.LinkViews(); impl.IdentifyInductions()
  impl.FinalizeDepths(); impl.FinalizeColumnIDs()
  BuildEquivalenceSets(impl); impl.Stratify()             # multi-view stratum = an SCC
  impl.row_contracts = InferConservativeRowContracts(impl)           # Stage-A member keys
  # ---- s32 InstanceFlow tail (Build.cpp:2664-2666), post row-contracts ----
  impl.instance_flow = BuildFlatInstanceFlow(Query(impl))            # <-- NEW, see §2
  ValidateInstanceFlow(Query(impl), impl.instance_flow, log)         # <-- NEW, see §3
  return Query(impl)
```

Key architectural facts (verified at tip):
- InstanceFlow lives INSIDE `lib/DataFlow` (B3). `InstanceFlowProgram` is a
  by-value member of `QueryImpl` beside `row_contracts` — the RowContract
  precedent. It is a PURE, RECOMPUTABLE function of the FINAL (post-Optimize,
  post-Stratify) graph, never present during Optimize (the F1 lesson).
- It is an OBSERVER: nothing downstream (Regional/Rel/ControlFlow/codegen) reads
  `instance_flow`. The only reader is the opt-in `-instanceflow-out` dump. This
  is WHY codegen is byte-identical, and it is the invariant CP2 must preserve.
- Determinism: every id derives from `QueryView::DeterministicOrder()` (`det_seq`)
  and canonical catalog order — NEVER `UniqueId()` (pointer-derived) or container
  iteration order (HP-9 / §16.1).

## §2 The flat grove builder — pseudocode (matches lib/DataFlow/InstanceFlow.cpp:75)

```
BuildFlatInstanceFlow(query) -> InstanceFlowProgram flow:
  # 1. origins (det_seq catalog)  [InstanceFlow.cpp:78]
  by_det = {}                                  # det_seq -> (view, kind, tag)
  ForEachViewKindTagged(query, (v,kind,tag) => by_det[v.det_seq] = (v,kind,tag))
  flow.origins = [ QueryOriginId{d} for d in 0..N )     # N = live view count

  # 2. collections + sites (INSERT-target only, S1)  [:94]
  for iv in query.Inserts() where !dead:
    cid = intern(iv.Declaration().Id())        # LogicalCollectionId, first-seen order
    site = DerivationSite{ new id, writer=iv.det_seq, cid }   # one per live INSERT
    site_of_origin[iv.det_seq] = site.id

  # 3. SCC-condensation families + nodes (B4)  [:122]
  stratum_size = histogram(v.Stratum() for live v)         # multi-view stratum = SCC
  for d in 0..N in det_seq order:
    recursive = v.Stratum() has_value AND stratum_size[stratum] > 1
    fam = recursive ? scc_family(stratum, ownership=kWholeQueryScc, QuerySccId dense)
                    : the ONE acyclic_family(ownership=kAcyclic)
    node = FamilyNode{ (fam,local), origin=d, kind, tag,
                       role=kInterior,                      # <-- CP2: real root selection
                       residual = full visible schema,      # empty-context: residual == schema
                       output_collection = cid if INSERT }
    node_of_origin[d] = node.id ;  fam.nodes.push(node)

  # 4. emission authorities (one per site, domain=All)  [:186]
  for site in flow.sites:
    flow.authorities.push( EmissionAuthority{ new id, site.id, node_of_origin[site.writer] } )

  # 5. uses + coverage (B1 total order, B2 decoupled)  [:193]
  raw = []
  for d in 0..N:
    v.ForEachUse( (in, role, out) =>            # in = producer col, out = consumer col (nullopt=terminal)
      producer = in.IsConstant ? kConstProducer(sentinel, col=*)   # S4
                               : (Containing(in).det_seq, in.Index)
      cls = (kind==INSERT && !out) ? kTerminalInsert(site_of_origin[d]) : kInterior
      raw.push( OriginUse{ consumer=d, producer, producer_col, consumer_out_col, role, cls } ) )
  sort raw by (consumer, producer, producer_col, role, consumer_out_col)   # B1 strict total order
  flow.uses = raw with OriginUseId = index
  for u in flow.uses:                            # B2: coverage is a per-consumer bijection
    occ = node_of_origin[u.consumer]             # terminal-insert: consumer IS the writer node
    flow.coverage.push( {u.id, occ} ) ;  families[occ.family].covers.push(u.id)

  # 6. seeds — DEFERRED (S5): candidate JoinPivot/AggregateGroup/BoundaryBinding, NOT goldened

  return flow
```

## §3 The validators — pseudocode (lib/DataFlow/InstanceFlow.cpp:266)

All always-on `fprintf+abort` (survive NDEBUG); abort-only (a fire is a compiler
bug). They re-derive the source (`by_det`, `stratum_size`) — never trust the
built grove blindly.

```
ValidateInstanceFlow(query, flow):
  V-IF-ORIGIN   : the union of families' nodes is a det_seq bijection onto live views;
                  each node.residual width == its origin's visible-column count.  [:277]
  V-IF-CONTEXT  : (vacuous this slice) every node's context/transfers empty; residual == full schema.
  V-IF-SCC      : every kWholeQueryScc family's nodes share ONE multi-view stratum;
                  the kAcyclic family holds only single-member/no-stratum views (no half-SCC). [:317]
  V-IF-EMISSION : authorities <-> sites is a bijection; every writer is an INSERT node.  [:361]
  V-IF-COVERAGE : coverage <-> uses is a bijection; every use covered exactly once;
                  every occurrence resolves to a live node.  [:381]
```

## §4 The dump grammar (`-instanceflow-out`, Format.cpp:1862) — current + open

```
instanceflow  origins=N uses=M collections=C sites=D families=F
collections
  lc#k decl=<name>/<arity>
sites
  ds#k writer=q#<det_seq> -> lc#k
family if#f <acyclic | whole-query-scc scc#s>
  node if#f.l origin=q#<det_seq> <kind> residual=(<names>) [-> lc#k]
  covers u#k <col=<n|*> role=<name> src=<q#p|const> | insert -> lc#k ea#a> occ=if#f.l
authorities
  ea#k site=ds#k domain=all writer=if#f.l
```

Deterministic, human-legible, goldenable. OPEN (CP2, see §5): the per-node
`role=`/`root=` tokens are NOT yet emitted (root selection deferred), and
bound-`#query`-read covers are absent. The §16-vs-B2 reconciliation (authority
renders INLINE only on a terminal-insert cover) is landed.

## §5 The path forward as DIFFS

### CP2 — finalize the grammar + bless witness goldens (the immediate next slice)

Still an OBSERVER (codegen byte-identical). Makes the dump COMPLETE + regression-pinned.

```diff
 BuildFlatInstanceFlow(query):
   ... families + nodes ...
+  # 6a. root selection (RR1 — pin OccurrenceRole/root_use against §6/§16 verbatim FIRST):
+  for fam in flow.families:
+    fam.root_use = min OriginUseId among fam's boundary/terminal uses
+                   (pure-interior SCC family: min OriginUseId consuming a member from outside)
+    mark that use's occurrence node role=kRoot; others kInterior
+  # 6b. bound-#query-read uses (§7.2 item 5):
+  for each bound #query q (decl.IsQuery() && >=1 bound param, cf Demand.cpp read enumeration):
+    flow.uses += OriginUse{ cls=kBoundQueryRead, read_collection=<the read LC>, /* no authority */ }
+    cover it by the collection's model/writer node   # occ semantics: CP2-verify (RR5)
```
```diff
 operator<<(QueryInstanceFlow):
   node ... <kind>
+       role=<root|interior>          # emit once root selection lands
   covers ...
+  covers u#k query <name> bound=(...) -> reads lc#k occ=if#f.l    # bound-query-read cover
```
- Add V-IF-COVERAGE arm for `kBoundQueryRead`; keep V-IF-EMISSION unchanged
  (reads carry no authority — B2).
- **Goldens:** add `.irgold` `instanceflow opt` steps (peer of `region`/`contract`)
  for `join_1` (single acyclic family, `col=*` constant edges), `merge_2`
  (maximal-sharing, disjoint authorities), `transitive_closure` (two families,
  SCC split, decoupled authority, bound reads), and `barrier_neck_1`
  (condition-relation LC path). Opt-mode only (post-Optimize property).
- **Gate:** OptDiff SUITE PASS byte-identical (observer proof) + the 4 new
  `.instanceflow` goldens + ctest 5/5.

### Phase B — MaterializationPlan + abstract resources (InstanceFlow.md §10, §20-B)

Still no codegen move; the first step toward Rel consuming IDs not `TABLE*`.

```diff
+ MaterializationPlan = PlanMaterialization(query, instance_flow, cost)
+   resources   = one authority per stateful LogicalCollection
+   arrangements = use-specific physical forms (initially = current runtime requirements)
+   bindings/aliases/replicas ; ValidateMaterialization (V-MAT-*)
+ # replaces BuildEquivalenceSets' storage-sharing role (migration evidence, not authority)
```

### Phase C — the Rel authority cutover (InstanceFlow.md §11, §20-C)

```diff
- BuildDRInventory(query, context)            # rediscovers branches/joins/keyed subgraphs from Query
+ BuildRel(query, instance_flow, materialization, sccs, cost)   # consumes IDs + occurrence topology
+ # delete ControlFlow's Query-shape dispatch + Rel's demand-subgraph recognition
```

### Phase D — first CODEGEN-MOVING slice: join-pivot K-context families (§20-D)

```diff
  build_grove:
-   every family binds empty context
+   seed JoinPivot contexts for monotone acyclic equijoins; bind K across left/right;
+   VisibilityFedGroupedColumns (sorted key directory + residual columns);
+   Rel SortedSectionProbe / SortedBatchMerge
  # THE first codegen-moving win; gated behind "flat-IF is Rel's sole input" (§24 #1)
```

## §6 Anchors (re-verify at next tip — the pipeline drifts)

| Fact | Source (tip 1e734d33) |
|---|---|
| Typed IDs + object model + decls | `lib/DataFlow/InstanceFlow.h` |
| Grove builder | `lib/DataFlow/InstanceFlow.cpp:75` (`BuildFlatInstanceFlow`) |
| Shared live-view walk | `lib/DataFlow/InstanceFlow.cpp:26` (`ForEachViewKindTagged`) |
| Validators | `lib/DataFlow/InstanceFlow.cpp:266` (`ValidateInstanceFlow`) |
| Query::Build tail wiring | `lib/DataFlow/Build.cpp:2664-2666` |
| Storage member | `lib/DataFlow/Query.h` (`QueryImpl::instance_flow`, beside `row_contracts`) |
| Dump tag + friend | `include/drlojekyll/DataFlow/Format.h` (`QueryInstanceFlow`); `Query.h` friend |
| Dump emitter | `lib/DataFlow/Format.cpp:1862` |
| CLI wiring | `bin/drlojekyll/Main.cpp` (`gInstanceFlowStream`, `-instanceflow-out`) |
| The vision | `docs/proposals/InstanceFlow.md` (§4 types, §6 model, §7 build, §8 coverage, §16 dump, §17 validators, §20 phases) |
| The grounding | `docs/proposals/InstanceFlow.artifacts/session-32-phaseA-grounding.md` |
| Identity idiom | `lib/DataFlow/Identity.h` (`FieldId` etc. + IdentityTypes ctest) |
| RowContract precedent | `lib/DataFlow/RowContract.{h,cpp}` |

## §7 Open questions / residual risks carried forward

- **RR1 (gates CP2 goldens):** `OccurrenceRole` values + `root_use` selection are
  NOT yet pinned against InstanceFlow.md §6/§16 verbatim. READ §6/§16 before
  blessing any golden. The §16 example shows `role=root`/`role=input` and a
  per-node occurrence token (join/left/right) — reconcile with the landed
  `{kRoot,kInterior}`.
- **RR5:** the `occ` (occurrence node) of a bound-`#query`-read use — §7.2 item 5's
  exact reader-node semantics are unconfirmed. Assign + CP2-verify.
- **Bound-query reads in the non-demand graph:** these are boundary obligations
  with no ForEachUse edge; enumerate them like the demand pass does
  (`decl.IsQuery()` + bound params). Deferred at CP1; the coverage bijection
  currently covers interior + terminal-insert uses only (documented scope).
- **S5 seed vocabulary:** candidate JoinPivot seeds need the §5.1 literal-operand
  vocabulary before they can be goldened. Compute for review, keep un-goldened.
- **The observer invariant:** every slice through Phase C must keep codegen
  byte-identical (nothing consumes the grove). The FIRST codegen move is Phase D
  by design — do not let an earlier slice silently move a golden.
```
