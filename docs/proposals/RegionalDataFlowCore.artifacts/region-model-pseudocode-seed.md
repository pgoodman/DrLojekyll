# Region-model pseudocode SEED — current architecture + path forward as diffs

Purpose: ground the 2026-08-03 region-model design thread
(`running-example-disassembler.md`) against the ACTUAL code as pseudocode,
with the path forward expressed as DIFFS on that pseudocode, so the direction
sits in a whole-program view rather than prose. This is a SEED (author:
current session, first pass) — the next session is charged with the rigorous
build-out (fleet-verified anchors, critique panels, desired IR states); see
`next-session-prompt.md`. Anchors are line-approximate at HEAD 6eb39b05 +
the uncommitted Stage-A tree; a fleet MUST re-verify before building.

Related authorities: `regional-arch-pseudocode.md` (the whole-program
pseudocode of the current demand/keyed-instance layer), `RegionalDataFlowCore.md`
(the normative target), `owner-adjudication-record.md` (ratified decisions +
the 2026-08-03 design addenda).

---

## Part 1 — CURRENT ARCHITECTURE as pseudocode (the layer the region model replaces)
##
## SUPERSEDED 2026-08-03 (session 3): Part 1 below is SUPERSEDED by
## `regional-arch-pseudocode.md` **Part R** (2026-08-03) — the fleet-verified
## current-architecture pseudocode re-anchored against the branch tip, with
## every drifted/broken seed claim corrected in place (see Part R's DRIFT
## LEDGER; notable fixes: the kInstanceDeath gate tests the DEMAND table, not
## the input; InstanceStore is a two-BUFFER not two-word store;
## view_to_swap_vec belongs to the eager-descent induction, not LowerDRRounds).
## Read Part R for the authoritative anchors. Part 1 is kept below for history
## only; Part 2 (the path-forward diffs) remains LIVE until the formalization
## lands.

### 1.1 The demand transform (flat lowering) — `lib/DataFlow/Demand.cpp`

`QueryImpl::ApplyDemandTransform` (:385), run at the post-`ConnectInsertsToSelects`
slot in `Build.cpp`. MODE-GATED: returns a no-op when `demand_mode==false`.

```
ApplyDemandTransform(module, log, demand_mode, demand_retract):
  if not demand_mode: return true                         # orthogonal to 4 golden modes
  bound_queries := [ rel for rel in relations
                     if rel.IsQuery() and rel.arity>0 and rel.hasBoundParam ]
  if bound_queries empty: return true
  if |bound_queries| > 1: reject("multiple bound queries")   # C14 fence (LIFT candidate)
  q_rel := bound_queries[0]

  # D3.a.3 TWO-PHASE per-adornment loop over q_decl.UniqueRedeclarations():
  # Phase 1 (Loop 1): per adornment, Steps 1b+2+3 — LOCATE + CHECK, NO mint
  for redecl in q_decl.UniqueRedeclarations() (dedup seen_variants):
    if redecl has BOUND and an all-free sibling: reject("all-free sibling")  # fence
    locate q_consumer (the view whose read of p is guarded)
    SIP walk from bound cols backward over p's MERGE members (rule bodies):
      per member, locate the GUARD SITE (recipe N3):
        - interior read of p (recursive body): guard JOINs at the push-down site
        - base rule: guard JOINs at the raw-seed site
      reject on: NEGATE/AGG sink, left-linear >1 adornment, stray consumer, ...
  Step 4 (ONCE, between loops): stray-consumer union check
  # Phase 2 (Loop 2): per adornment, Steps 5-10 — MINT
  for adornment:
    Fabricate demand message/local (FabricateDemandMessage/Local, reserved demand__ prefix)
    Mint guard JOIN  demand_side ⋈ read  pivoting read     # d_p ⋈ p, distinctness kept
    Mint restoring TUPLE (re-establish guarded read's original schema)
    (R-DUP) multi-guard group → MERGE UNION of guards' restored outputs
  Inject the demand SEED via a forcer proc from the QueryDemandForcing registry
```

Key property: the demand relation is JUST ANOTHER RELATION; the region call is
an IMPLICIT guard-JOIN; the key stays a COLUMN. No explicit call/region node.

### 1.2 The keyed-instance (nested) lowering — `lib/Rel/Rel.cpp`

`-demand-instance` (implies `-demand`; a lowering selector, not a pass). Gated
on `context.demand_instance_enabled`. Flag-off: zero instance ops minted.

```
BuildStratumPhases(... ):
  ... build the DR flow ...
  if demand_instance_enabled:
    BuildSubgraphInstanceOps(flow, impl, context, query, scc_map)   # :2065

BuildSubgraphInstanceOps(...):                                       # :1036
  lr := ResolveLiveRecognition(impl, query)   # ABA-safe re-resolve; handles dangle
  for rs in query.RecognizedSubgraphs():       # each recognized demanded subgraph
    check rs.input not induction-owned         # feature-gap fences
    mint kSubgraphInstantiate:                 # birth/rebuild + band-(b) publish
      band-(a1): birth   — full input-frontier rescan, keyed on the demand key
      band-(a2): edge-add rebuild (EDGE-AFTER-DEMAND, D2.b R-a2)
      band-(a2'): edge-remove drain (D3.a.2 diff-input, -demand-retract)
      each Present-filtered (skip physically-present-but-dead input rows)
      band-(b): emit_touched — ONE-NET-PAIR guard into the instance's counters/queues
    if input differential and demand_retract:
      mint kInstanceDeath (R-DIFF, D3.a.1)     # demand-liveness gate
```

Runtime state: `InstanceStore<Key,RowT>` (`include/drlojekyll/Runtime/InstanceStore.h`):

```
InstanceStore:
  FindInstance(key) -> iid | kNoInstance      # the memo lookup
  FindOrAddInstance(key) -> iid               # activate (memoize) a key
  Touch(iid); TouchedFlag(iid)                # per-round touched set
  Working/Sealed cells; Seal()                # two-word sealed/working per instance
  RecycleCurrent(iid)                         # death
  # dense per-instance id space; occupancy bits; rehash
```

So the InstanceStore ALREADY IS the memo table of activations (rows = keys);
the recognition is subgraph-shaped (SIP-reachable), not SCC-shaped.

### 1.3 The recursion / fixpoint lowering — `lib/ControlFlow/Build/` + `Stratum.cpp`

```
Stratify (DataFlow): Tarjan SCC condensation; view->stratum ids
DeriveDRStrata (Rel): monotone integer lift of DR ops
LowerDRRounds (Stratum.cpp): per-SCC×phase fixpoint ROUND SHELLS
  body is ACYCLIC; the inductive MERGE is the loop-header join point;
  view_to_swap_vec = the loop-carried variables (semi-naive frontier)
  differential: per-stratum OVERDELETE -> REDERIVE -> INSERT, split counters
LowerCommitSweeps: commit + Seal (epoch boundary = the only publish point)
```

### 1.4 The Stage-A identity/contract layer (LANDED this session)

```
Query::Build tail (after impl->Stratify(log)):
  impl->row_contracts := InferConservativeRowContracts(impl)   # RowContract.cpp
    Phase 0: stratum histogram (multi-view stratum = a cycle)
    Phase 1: cyclic rule — every view on a multi-view stratum gets
             member_key = AllFields(columns)                    # conservative
    Phase 2: acyclic per-operator transfer, flat-key {visible_fields, member_key}
  ValidateRowContracts(impl, log)   # V-CONTRACT-CENSUS / V-MEMBERKEY-REALIZED /
                                    # V-AGG-INPUT-KEY (always-on); V-NO-COLLAPSE (belt)
ProjectionRole (enum) folded into QueryTupleImpl::Equals ONLY (Tuple.cpp)
-contract-out dump (Format.cpp): flat-key contracts, id-ordered
```

---

## Part 2 — THE PATH FORWARD as DIFFS on the pseudocode

Each diff carries: goal, the pseudocode delta, the code sites it touches, the
soundness obligation, and the owner decision it depends on. These are SEEDS —
the next session formalizes, critiques, and pins desired IR states.

### DIFF-R1 — the explicit REQUEST-EDGE / region-call node (the epoch's core)

Goal: make the region call a FIRST-CLASS node instead of an implicit
guard-JOIN (1.1) / instance lowering (1.2). Notation `rel[Bound...](Free...)`.

```
  # DataFlow IR (lib/DataFlow/Query.h): NEW node kind
+ REQUEST(rel, key_cols=[Bound...], answer_cols=(Free...), edge_kind)
+   edge_kind ∈ { LAZY, FORCE_COMPLETE }        # see DIFF-R4 (monotonicity)
  # ApplyDemandTransform's guard-JOIN mint (1.1) is REPLACED by:
- mint guard JOIN demand_side ⋈ read pivoting read
+ mint REQUEST(rel, key=bound_cols, answer=free_cols, edge_kind=inferred)
  # the request edge's runtime realization IS the InstanceStore lookup (1.2):
+   REQUEST(rel,key,...) lowers to InstanceStore.FindOrAddInstance(key)
+   + the activation's fixpoint (LowerDRRounds, per instance)
```
Touches: Query.h (node kind), Demand.cpp (mint site), Rel.cpp
(BuildSubgraphInstanceOps becomes the request-edge lowering), the `.rel`/
`-region-out` dumps, the DOT twin. Obligation: request edge must satisfy the
Stage-C validators (V-EDGE-BALANCE etc.). Depends on: D2.6 (edge row schema),
the §6-vs-§11 routing rule.

### DIFF-R2 — the pivot split as SYNTACTIC self-vs-call (down/across)

Goal: read self-recursion vs cross-region call off bracket-var identity, no SIP
inference. In `tc[F](T) : tc[F](X), tc[X](T)`, body `tc[F]` = feedback,
`tc[X]` = REQUEST.

```
  # recursive-body lowering: per recursive body atom rel[K](...):
+ if K == head.key_var:   lower as FEEDBACK (same-activation, view_to_swap_vec)
+ else:                   lower as REQUEST(rel, key=K, ...)   # cross-region call
  # activation-graph stratification (NEW, mirrors Stratify at the KEY level):
+ build activation-call graph (which key requests which); Tarjan SCC:
+   DOWN (cross activation-SCC): well-founded descent, no iteration
+   ACROSS (intra activation-SCC): coupled fixpoint (V-CW widened)
```
Touches: the recursive-body lowering (Stratum.cpp / Build.cpp), a NEW
activation-SCC pass. Obligation: termination — the ACROSS case needs the
finite-activation argument (demand_cyclic_1 fence today). Depends on: D2.12
(V-CW), the coupled-fixpoint restatement of the disjoint-union lemma.

### DIFF-R3 — user-DECLARED regions (surface: brackets on internal relations)

Goal: extend mode-declaration inward; region INFERENCE -> CHECKING. Optional,
hint-not-mandate.

```
  # Parser (lib/Parse): accept rel[Bound...](Free...) on internal relations
+ ParsedRelation gains an optional bracket key spec (ordered col list)
  # ApplyDemandTransform:
+ if rel has a DECLARED bracket: SKIP SIP inference; CHECK the declared key
+   (contract validates it is a real key; DIFF-R5 logical/physical split)
+ else: infer as today (SIP walk)                    # complement, not replace
```
Touches: parser, Demand.cpp (inference vs checking fork), the contract checker.
Obligation: exhaustiveness/soundness of the declared factoring; answer-identity
with the flat+demand path (I0 referee). Depends on: the declared-regions
owner-call (hint-vs-mandate, sequence slot).

### DIFF-R4 — lazy vs force-complete request edges (the negation/aggregate barrier)

Goal: type request-edge laziness by consumer monotonicity (the CALM boundary);
LIFT the negation/aggregate-in-demanded-body fences soundly.

```
  # request-edge lowering (DIFF-R1):
+ if consumer is MONOTONE (join/union/proj):  edge_kind = LAZY
+     (partial answer sound by irrevocability; flows as derived)
+ if consumer is NON-MONOTONE (negation/aggregate):  edge_kind = FORCE_COMPLETE
+     (drive target region to QUIESCENCE for the requested key domain,
+      THEN read the gate)                            # the stratum barrier, per key
  # today: ApplyDemandTransform rejects NEGATE/AGG in a demanded body — that
  # reject IS this barrier drawn conservatively; DIFF-R4 replaces reject with
  # the FORCE_COMPLETE edge kind (needs per-key quiescence detection).
```
Touches: Demand.cpp (the NEGATE/AGG fences), the request-edge lowering, a NEW
quiescence-detection mechanism. Obligation: the force-complete edge must prove
target completeness before the gate (soundness of absence/summary). Depends on:
Variant B (recursive stays a reject), the CALM/free-termination framing.

### DIFF-R5 — non-prefix ordered bracket keys; logical/physical key split

Goal: brackets bind arbitrary ordered composite keys (join pivots, nested keys)
with no flat-schema name; separate the semantic key from the arrangement order.

```
  # region key:
+ LOGICAL key = the SET of bracket cols (the region partition; semantic;
+   rel[A,C] == rel[C,A]) — this is the Stage-A contract member_key
+ PHYSICAL order = the sort order WITHIN the bracket (the arrangement;
+   optimization; share a trie iff prefix-compatible) — D5/planning tier
  # canonicalize logical key for equivalence/CSE; choose physical order for
  # arrangement-sharing (the McSherry cost decision) SEPARATELY
```
Touches: the contract (logical key, LANDED), the InstanceStore key
(= bracket tuple), the D5/planning arrangement-order choice. Obligation: the
key must be a real functional key (contract check). Depends on: D5 (arrangement
sharing), the declared-regions surface (DIFF-R3).

### DIFF-R6 — the matrix / semiring frame (D5 storage + cost)

Goal: keyed relation = sparse semiring matrix; instances = rows; non-linear
recursion = matrix product; TC = Kleene star. Informs storage + cost, not a
near-term lowering change.

```
  # InstanceStore = row-wise sparse matrix storage = an arrangement = a trie by key
  # non-linear rule rel(F,T):rel(F,X),rel(X,T) = M ∨ M·M (region calls = inner sum)
  # demand-effectiveness: endpoint-keyed query over source-invariant recursion
  #   PRUNES; over composition (pivot-generating) recursion largely does NOT
  #   (demanded key set == answer set). Cost-model discriminator.
  # InstanceStore-as-memo keeps non-linear demand QUADRATIC not exponential.
```
Touches: D5 storage direction, the cost model (`docs/proposals/CostModel.md`),
the demand-pays analysis. Depends on: D5, cost-model integration.

---

## Part 3 — SEQUENCING (candidate; owner ranks)

The existing plan is A → I0 → B → C → D (A landed). The region-model direction
inserts/reframes:
- DIFF-R1 (request-edge node) IS the Stage-C cutover — already the plan; the
  design thread ENRICHES it (edge_kind, the notation, the routing rule).
- DIFF-R2 (activation-SCC / pivot split) sharpens Stage D (V-CW coupled).
- DIFF-R3 (declared regions) is the NEW candidate: slot after I0 + Stage B
  (gives C/D user-written test input independent of the demand transform).
- DIFF-R4 (lazy/force barrier) is a Stage-C+ capability lift (the fence removal).
- DIFF-R5/R6 are D5/planning-tier (storage, cost, arrangement sharing).

The immediate buildable slice remains I0 (the referee that makes every
region-model equivalence claim — flat == declared == nested — checkable).
