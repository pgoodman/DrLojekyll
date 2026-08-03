# The `.rel` dump AFTER Stage C — desired states as diffs on the collected dumps

Surface: the delta-relational IR (`.rel`, `-rel-out`) dump, AFTER the Stage C
cutover (RegionalDataFlowCore.md §13 Stage C; hunks in `stage-c-diff.md`
H-A..H-J). This artifact expresses, per witness, the DESIRED `.rel` dump as a
DIFF from the CURRENT collected dump in
`.../scratchpad/phase4/*.rel` (verified 2026-08-02 at tip f0c913e0), with a
prose contract per changed construct, a determinism contract, and the
adjudication inputs the census-line change forces.

Grounding note: current lines quoted below are copied verbatim from the
collected dumps. This is DESIGN — no file other than this one is modified.
Line-exact claims are marked **[pinned]**; shape-exact claims (ids/counts drift
in a greenfield cutover — there is no pre-existing golden to preserve for the
lifecycle body) use metavariables (`<id>`, `e#<n>`, `R#<n>`) and are marked
**[shape]**.

Witnesses (charge):
- `join_1` — no-demand control; must isolate the census-line field-set change
  from all body change.
- `demand_neighborhood_witness` (**-nested arm** = current state) — the
  differential single-adornment instance carrier whose
  `kSubgraphInstantiate`/`kInstanceDeath`/`kInstanceSeal` ops the desired state
  retires.
- `demand_multi_adorn_witness` (**-nested arm**) — the two-stores-one-pub shape
  becoming two `ChildInstanceId` spaces over one shared pub.
- `demand_tc_witness` (flat `-demand`) — recursive demanded body; the
  variant-dependent case (H-J).

---

## 0. Surface conventions PRESERVED (this is an extension of a pinned surface)

Every one of these survives byte-for-byte; Stage C touches none of them:

- **`rel` header token** (line 1) + trailing blank line. The OD-14/E-71
  in-dump token stays `rel`. **[pinned]**
- **Section order and their blank-line separators:** `vec` block → `branch.*`
  → `join.*` → *(instance/region section)* → op listing → `rounds:` →
  `deps:` → `census:`. Stage C RENAMES the one instance section (see §5) and
  leaves every other section header token unchanged. **[pinned]**
- **`vec $<name>.<idx> <ids %table:N> uniq=… def=[…] use=[…]`** line grammar,
  and the six-vec-per-differential-table family
  (`delete-queue`/`add-queue`/`overdelete-set`/`addition-set`/
  `net-removal`/`net-addition`). **[pinned]** — the request-edge relation
  REUSES exactly this family (§4).
- **`op.<n> kKind sign=… ctx=… stratum=…`** op-line grammar, the `effects:
  {…}`, `spine:`, `args:` continuation lines, and the eager-marker render
  shapes (`table=`, `form=`, `order=`, `seq=`, `cmp=`, `functor=`,
  `message=`). The entire eager-web op family
  (`kEagerForward`/`kEagerInsert`/`kEagerCompare`/`kEagerGenerate`/
  `kEagerUnion`/`kEagerSelect`/`kEagerJoin`/`kEagerProduct`/`kIngestLoop`/
  `kJoinEmit`/`kProductEmit`) is UNCHANGED — it lowers the region-body local
  graph, which survives greenfield-at-the-demand-layer-only (stage-c-diff.md
  MECH "carried forward"). **[pinned]**
- **`census:` LINE PREFIX + `k…=<count>` token grammar.** The token SET
  changes (§1); the grammar does not. **[pinned]**
- **Dump order remains a pure function of the graph** (R-FINAL invariant): the
  op listing is the Kahn linearization under the band-key tie-break; the
  census is deterministic counts; `deps:` is derived. Stage C preserves this
  (§6).

---

## 1. THE census-line transformation (the one pinned surface Stage C perturbs for EVERY program)

Current census token order (29 kinds, uniform across all four witnesses;
verified — the three instance fields sit contiguously between `kStateSeal` and
`kEagerForward`):

```
… kGroupUpdate=N kStateSeal=N kSubgraphInstantiate=N kInstanceDeath=N kInstanceSeal=N kEagerForward=N …
```

Desired: DELETE the three instance fields (grep gate EG.6 forbids the literals
`kSubgraphInstantiate`/`kInstanceDeath`/`kInstanceSeal`), INSERT the ten
lifecycle kinds (stage-c-diff.md H-G.1, proposal §9 `RelRegionLifecycle`) IN
THE SAME SLOT, preserving the eager-marker block at the tail:

```diff
-… kGroupUpdate=N kStateSeal=N kSubgraphInstantiate=N kInstanceDeath=N kInstanceSeal=N kEagerForward=N …
+… kGroupUpdate=N kStateSeal=N kRequestEdgeAdd=N kRequestEdgeRemove=N kInputDelta=N kLocalFixpoint=N kChildResultAdd=N kChildResultRemove=N kRoutedResultAdd=N kRoutedResultRemove=N kRetireInactive=N kSealEpoch=N kEagerForward=N …
```

Census kind count: **29 → 36** (`29 − 3 + 10`). Field ORDER is a pure design
choice fixed here: the ten lifecycle kinds occupy the retired slot in the exact
`RelRegionLifecycle` field order of proposal §9
(`request_edge_add, request_edge_remove, input_delta, local_fixpoint,
child_result_add, child_result_remove, routed_result_add, routed_result_remove,
retire_inactive, seal_epoch`). Placing them in the vacated slot keeps the
eager-marker block's byte layout identical, which minimizes the diff on
no-demand programs to exactly this token-set edit. **[pinned — this artifact
pins the new field order]**

### 1.1 ADJUDICATION INPUT A — the census line grows fields for ALL 180 cases

The census line is a HARD byte-compare surface (permcheck.py only relaxes
published-delta token order, never the census). Because three zero-fields are
removed and ten zero-fields added, **no program's `.rel` census line is
byte-identical to its current golden** — including every no-demand program
(`join_1`, `booleans`, `merge_2`, `map_3`, `elim-cond-cycle-simple`, the
join/product carriers). This forces a re-bless of ALL eleven `.rel` pins
(and their `.irgold` opt-mode sidecars), not only the demand-touched ones.

Owner options (this artifact recommends the first):
1. **Accept a whole-corpus `.rel` census re-bless** under
   `runall.sh --bless` after review. The op BODIES of no-demand pins stay
   byte-identical (only the census line changes), so the review is mechanical
   — a permcheck-style referee could even auto-verify "body byte-identical,
   census differs only by the fixed 3-out/10-in token edit."
2. Emit only NONZERO census fields (variable-length census). REJECTED: it
   breaks the current fixed-width census contract, changes far more goldens,
   and defeats "cross-mode agreement is a byte-compare of the same census."
3. Keep the retired-kind fields as permanent `=0` for golden compat.
   REJECTED by the deletion manifest (D7) and grep gate EG.6.

Recommendation 1 is the delta-relational-IR golden policy applied as-is:
goldens change only via explicit bless after review; the census growth is the
reviewed change.

---

## 2. `join_1` — no-demand control: census-line-ONLY delta

`join_1` has no bound `#query` (both `q(free B)` and `never(free B)` are
fully-free observations → no demand → ProgramRoot materialization, zero
regions, zero request edges). Therefore EVERY lifecycle census field is `0`
and NO lifecycle op appears. The entire op body, `vec` block, `branch`/`join`
sections, `rounds:`, and `deps:` are **byte-identical**.

Current census (line 52) **[pinned]**:
```
census: kCrossover=0 kProductArm=0 kSeedFold=0 kFixpointFire=0 kChainFold=0 kClaimDrain=0 kRetire=0 kRederive=0 kFrontierFilter=0 kCommitSweep=0 kNegateGate=0 kPivotAssemble=0 kIngestFold=0 kGroupUpdate=0 kStateSeal=0 kSubgraphInstantiate=0 kInstanceDeath=0 kInstanceSeal=0 kEagerForward=6 kEagerInsert=2 kEagerCompare=6 kEagerGenerate=0 kEagerUnion=0 kEagerSelect=0 kEagerJoin=4 kEagerProduct=0 kIngestLoop=2 kJoinEmit=2 kProductEmit=0
```

Desired census **[pinned]**:
```
census: kCrossover=0 kProductArm=0 kSeedFold=0 kFixpointFire=0 kChainFold=0 kClaimDrain=0 kRetire=0 kRederive=0 kFrontierFilter=0 kCommitSweep=0 kNegateGate=0 kPivotAssemble=0 kIngestFold=0 kGroupUpdate=0 kStateSeal=0 kRequestEdgeAdd=0 kRequestEdgeRemove=0 kInputDelta=0 kLocalFixpoint=0 kChildResultAdd=0 kChildResultRemove=0 kRoutedResultAdd=0 kRoutedResultRemove=0 kRetireInactive=0 kSealEpoch=0 kEagerForward=6 kEagerInsert=2 kEagerCompare=6 kEagerGenerate=0 kEagerUnion=0 kEagerSelect=0 kEagerJoin=4 kEagerProduct=0 kIngestLoop=2 kJoinEmit=2 kProductEmit=0
```

Contract: **`join_1.rel` is byte-identical EXCEPT the census line**, whose sole
change is the §1 token-set edit (three `=0` fields out, ten `=0` fields in).
This is the concrete answer to the charge's "join_1 must be byte-identical
modulo census-line zero-count fields — or state why not": it is byte-identical
in body, and the census line is NOT byte-identical because the census FIELD SET
grows (Adjudication Input A). No `regions:`/`request-edges:` section is
emitted for a program with zero regions and zero edges — the section is present
only when non-empty (mirroring today's `instances:` section, which is absent
from the flat dumps).

Determinism: unchanged — the whole dump is the same pure graph function; only
the census token vocabulary is wider.

---

## 3. `demand_neighborhood_witness` — the differential single-adornment carrier (instance ops → lifecycle ops)

Current = the **-nested arm** (`-demand -demand-instance -demand-retract`),
239 lines. This is the full transform: the demand table (`%table:4`), the
input edge table (`%table:7`), the recursive body (`%table:11`), the pub
(`%table:15`), one `DRInstance`, and the `kSubgraphInstantiate`/
`kInstanceDeath`/`kInstanceSeal` triple.

### 3.1 `instances:` section → `regions:` + `request-edges:` + `child-results:`

Current **[pinned]** (lines 32-33):
```
instances:
  DRInstance i#0 forcing=neighborhood key=%table:4 pub=%table:15 input=%table:7 store=I#0 key_cols=[Start] row_cols=[Node]
```

Desired **[shape]** — the `DRInstance` record (which conflated allocation
identity, lexical scope, and liveness — proposal §1.3) splits into three
authoritative records; the runtime `store=I#0` (the `InstanceStore` handle, D8)
disappears entirely because results are stored ONCE in the ordinary pub:
```
regions:
  region R#0 owner=ProgramRoot origin=neighborhood
    request_port  = <key=[Start]>
    input_ports   = [%table:7]
    result_port   = %table:15  key=[Start] row=[Node]
request-edges:
  edge-rel %table:4  owner=RootLease  call_site=cs#0  child=<R#0 key=[Start]>  differential=true
child-results:
  child-result %table:15  child=<R#0>  member=[Start,Node]
routed-results:
  routed rr#0  edges=%table:4  child_result=%table:15  join_on=child(Start)
```

Prose contract per record:
- **`region R#0`** replaces the `forcing=neighborhood` + `key/pub/input`
  columns of `DRInstance`. `owner=ProgramRoot` (one-level extraction; the
  observation root owns it). `origin=neighborhood` is the `LogicalNodeId`
  origin (Stage A), NOT a fabricated `demand__` name — the `demand__…` message
  identity is GONE (grep gate: no `demand__` token in the dump). The
  `result_port … key=[Start] row=[Node]` carries exactly the old
  `key_cols=[Start] row_cols=[Node]` schema (which the current
  `kSubgraphInstantiate` line renders as `pub_row=[ik:Start,row:Node]`).
- **`edge-rel %table:4`** is the RequestEdgeRelation (proposal §5.2). It REUSES
  the current demand table's physical table id `%table:4` and its full
  differential vec family (§4). `owner=RootLease` + `call_site=cs#0` are the
  new exact-ownership columns (F4 fix) that the demand-row-presence model
  lacked. `differential=true` because the witness runs `-demand-retract` (the
  edge is retractable — a `kRequestEdgeRemove` exists).
- **`child-result %table:15`** names the ordinary pub DiffTable as the
  ChildResultRelation — results stored ONCE, the shared pub (H-G.2). No
  per-instance store.
- **`routed rr#0`** is the RoutedResultRelation = edges ⋈ child-result ON child
  (proposal §5.3), rendered as a derived join, NEVER materialized. It rides the
  pub-table queues (§4.3).

### 3.2 The instance ops retire; lifecycle ops appear

Current instance ops **[pinned quotes]** (the three to retire):
```
op.1 kInstanceDeath sign=- ctx=seed stratum=1 i#0
    effects: {kVecDrain(%table:4, kNetRemoval), kInstanceDemand(%table:4), kStateOld(%table:15), kInstanceRebuild(%table:15, -)}
    args: demand=%table:4 pub=%table:15 store=I#0
op.0 kSubgraphInstantiate sign=+ ctx=seed stratum=1 i#0
    demand=%table:4 pub=%table:15 input=%table:7 pub_row=[ik:Start,row:Node] nested=<Node>
    reads: Present(%table:7)
    effects: {kVecDrain(%table:4, kNetAddition), kVecDrain(%table:7, kNetAddition), kInstanceDemand(%table:4), kInstanceRebuild(%table:15, +), kStateEmit(%table:15), kStateOld(%table:15), kCounter(%table:15, +, NonRecursive), kInIReadFrozen(%table:15, InI, seed), kVecAppend(%table:15, kAddQueue), kCounter(%table:15, -, NonRecursive), kInIReadFrozen(%table:15, InI, seed), kVecAppend(%table:15, kDeleteQueue)}
    spine: kAccess(%table:7, section-walk) -> kFold(%table:15, +, NonRecursive)
    args: demand=%table:4 pub=%table:15 input=%table:7 store=I#0
…
op.2 kInstanceSeal sign=· ctx=seed band=11 i#0
    effects: {kStateFold(%table:15, sign=0)}
    args: pub=%table:15 store=I#0
```

Desired lifecycle ops **[shape]** — the band→op mapping of H-G.2 (the current
op's a0/a1/a2/a2'/b bands unbundle into the ten-op vocabulary). The
`kInstanceDemand`/`kInstanceRebuild`/`kInstanceKeySlot` eff/binding kinds are
GONE with the op kinds that carried them (D7):
```
op.<a> kRequestEdgeAdd    sign=+ ctx=seed stratum=1  edge=%table:4
    reads: —                                  # net-additions of the edge relation
    effects: {kVecDrain(%table:4, kNetAddition)}
    args: edge=%table:4 owner=RootLease
op.<b> kRequestEdgeRemove sign=- ctx=seed stratum=1  edge=%table:4
    effects: {kVecDrain(%table:4, kNetRemoval)}   # was the a0 death drain
    args: edge=%table:4 owner=RootLease
op.<c> kInputDelta        sign=· ctx=seed stratum=<s>  input=%table:7
    reads: Present(%table:7)
    effects: {kVecDrain(%table:7, kNetAddition)}  # the a2/a2' split COLLAPSES to one signed op
    args: input=%table:7 region=R#0
op.<d> kLocalFixpoint     sign=· ctx=seed stratum=<s>  region=R#0
    # the §7.1 Rel differential fixpoint over the region body — the SURVIVING
    # rel-arch §7(B) machinery. Unstratified at the epoch tail in Stage C
    # (same asymmetry as SUBGRAPHINSTANCE today); Stage D qualifies it by
    # (RegionId, InstanceId).
    args: region=R#0 body=[%table:11]
op.<e> kChildResultAdd    sign=+ ctx=seed stratum=<s>  child_result=%table:15
    effects: {kCounter(%table:15, +, NonRecursive), kInIReadFrozen(%table:15, InI, seed), kVecAppend(%table:15, kAddQueue)}
    spine: kAccess(%table:7, section-walk) -> kFold(%table:15, +, NonRecursive)
    args: child_result=%table:15 region=R#0
op.<f> kChildResultRemove sign=- ctx=seed stratum=<s>  child_result=%table:15
    effects: {kCounter(%table:15, -, NonRecursive), kInIReadFrozen(%table:15, InI, seed), kVecAppend(%table:15, kDeleteQueue)}
    args: child_result=%table:15 region=R#0
op.<g> kRoutedResultAdd    sign=+ ctx=seed stratum=<s>  routed=rr#0
    reads: Present(%table:4)                  # a result publishes to an owner iff a live edge routes it
    effects: {kVecAppend(%table:15, kAddQueue)}
    args: routed=rr#0 edges=%table:4 child_result=%table:15
op.<h> kRoutedResultRemove sign=- ctx=seed stratum=<s>  routed=rr#0
    effects: {kVecAppend(%table:15, kDeleteQueue)}
    args: routed=rr#0 edges=%table:4 child_result=%table:15
op.<i> kRetireInactive    sign=- ctx=seed stratum=<s>  region=R#0
    # replaces the a0 death→InstanceRebuild(-) content teardown; fires only
    # after the child's last edge dies AND its routed removals are visible
    args: region=R#0 child_result=%table:15
op.<j> kSealEpoch         sign=· ctx=seed band=11 region=R#0
    effects: {kStateFold(%table:15, sign=0)}   # was kInstanceSeal
    args: region=R#0 result=%table:15
```

Key contract points (H-G.2/H-G.3 made concrete against the quoted ops):
- **The E8d `si->demand_table` Present-probe is REPLACED by
  `RoutedResultRelation` membership.** Where `op.0` gated publication on the
  demand-row's presence, `kRoutedResultAdd/Remove` gate on a live
  `RequestEdgeId` routing the result (`reads: Present(%table:4)` on the edge
  relation). This is the F4 fix rendered in the dump: per-owner routing is an
  edge-join, not a shared count.
- **`kInstanceRebuild`/`kStateEmit`/`kInstanceDemand` DISAPPEAR.** The
  full-rescan `spine:` `kAccess(%table:7, section-walk) -> kFold(%table:15, …)`
  MOVES onto `kChildResultAdd` (the region body is maintained by
  `kLocalFixpoint`, not a rescan-on-touch — D8); the frozen-read
  `kInIReadFrozen(%table:15, InI, seed)` folds survive on the child-result ops
  because the pub is still an ordinary DiffTable.
- **The R-MONO/R-DIFF split collapses (H-G.3).** Because demand is uniformly
  retractable (a `kRequestEdgeRemove` op always exists), the "soundness by
  irrevocability" MONO special case is gone; the differential arm is the ONLY
  arm. See Adjudication Input C.

### 3.3 New DRVecs for the edge relation (frontier placement)

The current demand-table vec family survives verbatim in SHAPE, only its
`use=` edges re-point from instance ops to lifecycle ops. Current **[pinned]**:
```
vec $delete-queue.0 <ids %table:4> uniq=sort-unique-at-drain def=[op.28] use=[op.10]
vec $add-queue.1 <ids %table:4> uniq=sort-unique-at-drain def=[op.27] use=[op.11]
vec $overdelete-set.2 <ids %table:4> uniq=multiset def=[] use=[op.12]
vec $addition-set.3 <ids %table:4> uniq=multiset def=[] use=[op.13]
vec $net-removal.4 <ids %table:4> uniq=multiset def=[] use=[op.1,op.3,op.5]
vec $net-addition.5 <ids %table:4> uniq=multiset def=[] use=[op.0,op.4,op.6]
```

Desired **[shape]** — same six-vec family for the edge relation `%table:4`, but
DEF'd by the lease source instead of a fabricated-message ingest fold, and the
net-frontier USES re-point to the request-edge ops:
```
vec $delete-queue.0 <ids %table:4> uniq=sort-unique-at-drain def=[op.<lease-rel>] use=[op.<claimdel>]
vec $add-queue.1 <ids %table:4> uniq=sort-unique-at-drain def=[op.<lease-acq>] use=[op.<claimadd>]
vec $overdelete-set.2 <ids %table:4> uniq=multiset def=[] use=[op.<ff->]
vec $addition-set.3 <ids %table:4> uniq=multiset def=[] use=[op.<ff+>]
vec $net-removal.4 <ids %table:4> uniq=multiset def=[] use=[op.<b:kRequestEdgeRemove>,op.<seedfolds…>]
vec $net-addition.5 <ids %table:4> uniq=multiset def=[] use=[op.<a:kRequestEdgeAdd>,op.<seedfolds…>]
```

Contract: the edge relation is a small differential table. Its
`$net-addition`/`$net-removal` frontiers ARE the epoch's edge births/deaths —
frontier placement is UNCHANGED from today's demand-table frontiers (the
claim-drain + frontier-filter machinery, `op.10`-`op.13` today, survives to net
edge deltas per epoch). What retires is the SOURCE: the two
`kIngestFold sign=±` ops for `demand__neighborhood_bf/1` (current `op.27`/
`op.28`) disappear (no fabricated demand message exists — H-C); the edge rows
enter from the RootLease acquire/release (H-H). **[shape]** — whether the lease
source appears as a distinct op or as a `def=[]` external seed (like today's
`net-removal.4 def=[]`) is a lowering choice; either keeps the frontier grammar
byte-stable.

The pub table `%table:15` vec family (current `delete-queue.12`..
`net-addition.17`) is UNCHANGED — it is the ChildResultRelation store, still an
ordinary DiffTable riding its own queues + commit sweep. RoutedResult adds NO
new six-vec family (it is a derived join over the pub's existing queues).

### 3.4 Desired census

Current (line 239) **[pinned]**:
```
census: … kGroupUpdate=0 kStateSeal=0 kSubgraphInstantiate=1 kInstanceDeath=1 kInstanceSeal=1 kEagerForward=1 …
```
Desired **[shape — counts are the plausible 1:1 band-to-op mapping; the
greenfield lowering fixes the exact counts at bless time]**:
```
census: … kGroupUpdate=0 kStateSeal=0 kRequestEdgeAdd=1 kRequestEdgeRemove=1 kInputDelta=1 kLocalFixpoint=1 kChildResultAdd=1 kChildResultRemove=1 kRoutedResultAdd=1 kRoutedResultRemove=1 kRetireInactive=1 kSealEpoch=1 kEagerForward=1 …
```
The eager-marker tail (`kEagerForward=1 … kJoinEmit=2 kProductEmit=0`) and the
whole eager op body (`op.29 kEagerForward`, `op.30`/`op.31 kJoinEmit
form=delta`, the `kSeedFold`/`kClaimDrain`/`kFrontierFilter`/`kCommitSweep`
families) are UNCHANGED — the region body's local-graph lowering survives.

---

## 4. `demand_multi_adorn_witness` — two `ChildInstanceId` spaces over one shared pub

Current = the **-nested arm** (`-demand -demand-instance`), 76 lines — the
flagship two-DRInstance shape. It is the cleanest witness for the shared-pub
generalization: TWO instance stores (`I#0` keyed `[A]`, `I#1` keyed `[B]`) over
ONE pub `%table:4` and ONE input `%table:11`.

### 4.1 `instances:` (two DRInstances, shared pub) → two regions / two edge-rels / ONE child-result

Current **[pinned]** (lines 3-5):
```
instances:
  DRInstance i#0 forcing=q key=%table:8 pub=%table:4 input=%table:11 store=I#0 key_cols=[A] row_cols=[B]
  DRInstance i#1 forcing=q key=%table:19 pub=%table:4 input=%table:11 store=I#1 key_cols=[B] row_cols=[A]
```

Desired **[shape]**:
```
regions:
  region R#0 owner=ProgramRoot origin=q  request_port=<key=[A]>  input_ports=[%table:11]  result_port=%table:4 key=[A] row=[B]
  region R#1 owner=ProgramRoot origin=q  request_port=<key=[B]>  input_ports=[%table:11]  result_port=%table:4 key=[B] row=[A]
request-edges:
  edge-rel %table:8   owner=RootLease call_site=cs#0 child=<R#0 key=[A]>  differential=false
  edge-rel %table:19  owner=RootLease call_site=cs#1 child=<R#1 key=[B]>  differential=false
child-results:
  child-result %table:4  child=<R#0>  member=[A,B]
  child-result %table:4  child=<R#1>  member=[A,B]      # SAME pub table, two ChildInstanceId spaces
routed-results:
  routed rr#0  edges=%table:8   child_result=%table:4  join_on=child(A)
  routed rr#1  edges=%table:19  child_result=%table:4  join_on=child(B)
```

Contract — this is the charge's "two stores → two `ChildInstanceId` spaces over
one shared pub":
- The two `store=I#0`/`store=I#1` disjoint `InstanceStore`s (D8) are GONE. Their
  disjointness is now expressed at the SEMANTIC layer: two `ChildInstanceId`
  spaces (`<R#0 key=[A]>` vs `<R#1 key=[B]>`) with DISTINCT key schemas, both
  routing into the SAME `child-result %table:4`. Results are stored ONCE
  (proposal §5.3); the two adornments are two request/route families over one
  ChildResultRelation.
- This makes the D3.a.3 `V-INST-SOLE` `(pub_table, forcing_index)` re-key (which
  the current dump's shared `pub=%table:4` witnesses) OBSOLETE (H-F): the shared
  pub is no longer a re-keyed sole-pub invariant but the NATURAL consequence of
  two edge relations joining one ChildResultRelation. The R-DUP
  refcounted-union-pub (the flat arm's MERGE-of-guards) is likewise obsolete —
  routing IS the union, derived per-owner.
- `differential=false` on both edge-rels because this witness is bare `-demand`
  (mono; no `-demand-retract`). Under Stage C every edge is STILL retractable
  (the lease destructor), so `kRequestEdgeRemove` ops DO appear even here — the
  MONO `kInstanceDeath=0` special case is gone (H-G.3). See Adjudication
  Input C: this witness's current `kInstanceDeath=0` beside `kInstanceSeal=2`
  is exactly the R-MONO shape that collapses.

### 4.2 Ops + census

Current instance ops (op.0/op.2 `kSubgraphInstantiate`, op.1/op.3
`kInstanceSeal`) → two lifecycle-op groups (one per region), each with its own
`kRequestEdgeAdd`/`kRequestEdgeRemove`/`kInputDelta`/`kLocalFixpoint`/
`kChildResultAdd`/`kRoutedResultAdd`/`kSealEpoch`. The `kInputDelta`s from BOTH
regions drain the SAME `%table:11` net-addition frontier (current op.0 and op.2
both carry `kVecDrain(%table:11, kNetAddition)` — the shared input is preserved
as two reads of one frontier vec).

Current census (line 76) **[pinned]**:
```
census: … kCommitSweep=3 … kStateSeal=0 kSubgraphInstantiate=2 kInstanceDeath=0 kInstanceSeal=2 kEagerForward=3 …
```
Desired **[shape]**:
```
census: … kCommitSweep=3 … kStateSeal=0 kRequestEdgeAdd=2 kRequestEdgeRemove=2 kInputDelta=2 kLocalFixpoint=2 kChildResultAdd=2 kChildResultRemove=2 kRoutedResultAdd=2 kRoutedResultRemove=2 kRetireInactive=2 kSealEpoch=2 kEagerForward=3 …
```
Note `kRequestEdgeRemove=2`/`kRetireInactive=2` are NONZERO despite the current
`kInstanceDeath=0` — the collapse of the MONO arm (H-G.3) MEANS the mono witness
now carries removal/retire ops. This is the most behavior-bearing shape change
in the two-store witness and the direct visual of Adjudication Input C. The
`kEagerForward=3` tail and the three `kCommitSweep` ops (one per ingest table
8/11/19) are unchanged.

---

## 5. `demand_tc_witness` — recursive demanded body (VARIANT-DEPENDENT, H-J)

Current = the flat `-demand` arm, 70 lines. Its census ALREADY reads
`kSubgraphInstantiate=0 kInstanceDeath=0 kInstanceSeal=0` (no instance store —
flat guard-web). Its demand is realized as the fabricated
`demand__reachable_from_bf/1` message (current `op.1 kIngestFold … message=
demand__reachable_from_bf/1`) pushed into the recursive `reachable_from` join
via guard joins (`kEagerJoin=8`, `kJoinEmit=5`).

This case is the clean witness that **the census field-set changes even when
NO instance op existed to retire** (its three retiring fields are already `0`).
But its OP BODY is variant-dependent, because `reachable_from` is a
transitive-closure — a **recursive relation inside the demanded body**, which
Stage C's one-level extraction cannot lower as a keyed recursive child (that is
Stage D's "induction PER INSTANCE" — the direct inverse of today's
`demand_cyclic_1` fence). So under Stage C the recursive demand is INADMISSIBLE
for extraction and the queried relation materializes in the observation root.

### 5.1 Desired body (Variant A / non-recursive-B — full materialization)

Under §8.1's inadmissible-extraction rule, the recursive `reachable_from`
materializes fully in ProgramRoot and the bound query becomes a lease-scoped
filtering cursor. Consequences on the `.rel` body **[shape]**:
- The fabricated demand ingest fold DISAPPEARS. Current **[pinned]**:
  ```
  op.1 kIngestFold sign=+ ctx=eager stratum=0
      effects: {kCounter(%table:23, +, NonRecursive)}
      spine: —
      args: table=%table:23 message=demand__reachable_from_bf/1
  ```
  retires (no `demand__` message exists — H-C; grep gate). `%table:23` (the
  demand relation) and the guard-join web it fed retire with it →
  `kEagerJoin` and `kJoinEmit` DROP to the values of the plain recursive TC (a
  strict decrease from `kEagerJoin=8`/`kJoinEmit=5`).
- The bound query `reachable_from(bound A, free)` acquires a ROOT request edge
  (a RootLease over the observation root): `kRequestEdgeAdd=1`,
  `kRequestEdgeRemove=1`. But NO child region exists (the TC stays in the root),
  so `kLocalFixpoint`, `kChildResult*`, `kRoutedResult*`, `kRetireInactive`,
  `kSealEpoch` are all `0` — the recursive induction stays ordinary Rel rounds
  in the root (`rounds:`/`kFixpointFire`/`kRederive` machinery, not shown in
  this flat dump because it lowered acyclically today, would now carry the full
  recursive TC).

Desired census **[shape]**:
```
census: … kStateSeal=0 kRequestEdgeAdd=1 kRequestEdgeRemove=1 kInputDelta=0 kLocalFixpoint=0 kChildResultAdd=0 kChildResultRemove=0 kRoutedResultAdd=0 kRoutedResultRemove=0 kRetireInactive=0 kSealEpoch=0 kEagerForward=<n'> …
```
(`kIngestFold` drops 2→1 — only the real `edge_2/2` fold survives; the eager
counts drop to the plain-TC values.)

### 5.2 ADJUDICATION INPUT B — tc is a demand PERF regression under Stage C one-level

Full materialization of the TC is answer-correct (the cursor filters by the
bound column) but defeats demand's pruning — exactly the demand-cost-model
concern. Whether tc REMAINS a witness, is rewritten against a Stage-D keyed
recursive child, or is retired from the corpus is an **owner decision** tied to
H-J's variant choice AND the Stage-C-vs-Stage-D recursion boundary. Under H-J:
- **Variant A** — tc compiles as full materialization (above); its `.stdout`
  stays byte-identical (answer-neutral), only shape/perf changes.
- **Variant B** — if the admissibility gate keeps a reject for recursive
  demanded content (mapping tc onto the `demand_recursive_content_1`/
  recursive-region-body exclusion, §2.2), tc FLIPS to expected-diagnostic and
  has no `.rel` at all. This artifact CANNOT pin tc's `.rel` until the owner
  chooses; §5.1 is the Variant-A desired state, presented as the concrete
  option, not a pin.

---

## 6. Determinism contract (per section)

- **`rel` header, section order, blank-line separators:** fixed constants —
  pure function of "a dump exists." **[pinned]**
- **`vec` block order + `def=[…]`/`use=[…]` edges:** derived from the DR flow
  graph exactly as today (id-ordered vec declarations; def/use are graph
  edges). The edge-relation vecs (§3.3) inherit the demand-table's existing
  deterministic placement. **[pinned grammar; shape ids]**
- **`regions:` / `request-edges:` / `child-results:` / `routed-results:`
  order:** MUST be a pure function of the graph, derived from Stage A
  `LogicalNodeId` order (HP-9 — never pointer-derived, never vector-append
  order). For multi_adorn the two regions render in adornment-declaration order
  (`R#0`=bf before `R#1`=fb) because that is the `UniqueRedeclarations()` walk
  order Stage A preserves as `LogicalNodeId` order — the SAME determinism source
  that today orders `i#0` before `i#1`. **[shape; determinism pinned]**
- **op listing order:** the Kahn linearization under the band-key tie-break
  (`LinearizeAndValidateDRFlow`), UNCHANGED. The lifecycle ops slot into the
  seed/band lattice where the instance ops sat (`ctx=seed`, the request-edge
  ops at the demand-frontier stratum, `kSealEpoch` at `band=11` where
  `kInstanceSeal` sat). **[pinned mechanism]**
- **`deps:`:** derived edges — a pure graph function. **[pinned]**
- **`census:`:** deterministic counts; the field ORDER is the fixed 36-token
  vocabulary of §1. This is a byte-compare surface. **[pinned]**
- **permcheck.py referee:** unchanged charter — published-delta tokens compare
  order-free per epoch, all other lines byte-identical. Under the shared-pub
  RoutedResult realization the published deltas are the pub-table
  `kAddQueue`/`kDeleteQueue` drains (same vectors as today), so the
  permutation arm is UNCHANGED. The census line is NOT in permcheck's order-free
  set — it must byte-match post-bless.
- **Cross-mode agreement:** the four optimization modes remain byte-identical to
  each other per case (every mode's census carries the same 36 tokens); the
  `.irgold` opt-mode sidecars re-bless in lockstep.

---

## 7. V-* validator hooks the `.rel` dump surfaces

The dump is the witness surface for these Stage-C validators (proposal §11;
stage-c-diff.md H-I). Each is checkable FROM the dump:

- **V-LIFECYCLE-CENSUS** — the `census:` line's lifecycle counts must equal the
  recount over `regions:`/`request-edges:`/`child-results:` (successor to the
  retired keyed-instance census recount at `ValidateDROps` Rel.cpp:3999-4021).
  Surfaced directly: `kSealEpoch` == |regions with a result port|;
  `kRequestEdgeAdd` == |edge-rels|; etc.
- **V-EDGE-BALANCE** — every `edge-rel` has BOTH a `kRequestEdgeAdd` and a
  `kRequestEdgeRemove` op (balanced add/remove path). Surfaced: an edge-rel
  line with no matching remove op is a violation (replaces the Demand.cpp
  Step-9 tripwire + OWN-3 census aborts).
- **V-ROUTED-FANOUT** — every `child-result` participates in a `routed-result`
  fanout (a `routed rr#N` references it). Surfaced by the `routed-results:`
  section covering every `child-results:` entry.
- **V-ROUTE-EXACT** — every `kRoutedResultRemove` has an exact routed-result
  identity (`routed rr#N` back-reference), never a bare pub delete.
- **V-INACTIVE-AFTER** — `kRetireInactive` for a region orders AFTER its
  `kRoutedResultRemove`s in the linearization (`deps:` edges witness it); the
  typed form of the F3 "key existence ≠ liveness" hazard.
- **V-OWNER-EXACT** — no ambiguous `owner=`/`call_site=` on an edge-rel (every
  edge has a single `RequestOwnerId`).
- **V-REGION-ACYCLIC** — `regions:` owner edges form no cycle and every child
  call targets a direct child (in Stage C's one level: `owner=ProgramRoot`
  only; no region owns another).
- **V-PORT-CLOSED** — asserts here that no open planning port reached the
  frozen program (a Stage-B invariant, re-checked at Rel).
- **SURVIVING (unchanged) validators** the dump still surfaces: the four B-3
  intrinsics (V-XOVER-ONE/V-PROD-MONO/V-PROD-CLASS/V-JOIN-ONE), V-PRED-XCHECK
  and the Site-5 eager-web multisets, V-JOIN-EMIT-XCHECK — they validate the
  region-body local-graph lowering, which is unchanged.

Retired (no longer surfaced): V-INST-EFFECT/V-INST-SOLE/V-INST-PAIR/
V-INST-DIFF-COHERENCE/V-INST-INPUT-COHERENCE/V-INST-DEATH-COHERENCE/V-ALPHA/
V-INST-ORDER and the keyed-instance census recount (D5/D7).

---

## 8. Adjudication inputs (summary for the owner)

- **A — census line grows for ALL 180 cases (§1.1).** The 3-out/10-in token
  edit means NO `.rel` census line is byte-identical to its current golden,
  including no-demand programs (`join_1` body is byte-identical, census is not).
  Forces a whole-corpus `.rel` re-bless (recommended: accept it; the no-demand
  bodies stay byte-identical, so review is mechanical). This is the answer to
  the charge's "join_1 byte-identical modulo census-line zero-count fields — or
  state why not": **why not = the census FIELD SET grows, not just counts.**
- **B — `demand_tc_witness` is a recursive demanded body (§5.2).** Stage C's
  one-level extraction cannot lower it as a keyed recursive child (Stage D
  territory), so under §8.1 it either full-materializes (Variant A;
  answer-neutral, perf regression, defeats demand's pruning) or reject-flips
  (Variant B). Its `.rel` cannot be pinned until the owner resolves H-J AND the
  Stage-C/Stage-D recursion boundary. §5.1 gives the Variant-A desired state as
  the concrete option.
- **C — the R-MONO arm collapse makes mono witnesses gain removal/retire ops
  (§4.2, §3.2).** `demand_multi_adorn_witness` (bare `-demand`,
  `kInstanceDeath=0` today) gains `kRequestEdgeRemove`/`kRetireInactive` ops
  because every lease is retractable under Stage C (H-G.3). The pre-cutover
  tagged binary has NO differential-published-answer demand-instance witness
  to adjudicate the now-sole differential arm (`DRInstance::differential==false`
  program-wide today) — this is stage-c-diff.md ESCALATION E1: I0 must carry
  that case or one must be authored, or the mono witnesses' new removal-arm
  goldens have no oracle.
- **D — the `regions:`/`request-edges:`/`child-results:`/`routed-results:`
  section vocabulary is NEW dump surface (§3.1, §4.1).** This artifact proposes
  the token grammar (`region R#N`, `edge-rel %table:N owner=… call_site=… child=
  <R#N key=[…]>`, `child-result`, `routed rr#N`); the implementing fleet may
  refine token spelling, but the DETERMINISM source (LogicalNodeId order) and
  the one-shared-pub / two-ChildInstanceId realization are load-bearing and
  fixed here.
