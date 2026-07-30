# D3.a.3 (multi-adornment) — STAGE (a) SUBSTRATE (COMMITTED; ledger §20(AQ);
# ORCHESTRATOR-RE-VERIFIED — the ten §9 anchors re-read at code + the g6/misnomer
# behavioral adjudications RE-RUN as scratch compiles by the orchestrator
# pre-commit; §6 is the BINDING per-pub RULE-AT-CODE ruling, the analogue of the
# d3a2 §7 R-A2-TRIGGER ruling)

Tip 1f74b5c9 (docs-only atop the D3.a.2 landing bfc068d1; **code bytes == the
landing**). Repo READ-ONLY. Every anchor below is a CURRENT line number re-read
at code this session; the consolidator personally re-verified the load-bearing
ones (§9) and personally ran the four scratch compiles the adjudications rest on
(§2, §3.3, §4.4). This is the adornment-axis analogue of `d3a2-substrate.md`,
written in its §8 idiom: **the whole-program state as it stands → the path
forward as diffs → the ritual-head questions surfaced for stage (b)**.

ORCHESTRATOR PRE-COMMIT VERIFICATION (2026-07-30, E-77): the g6 adjudication
(§2.2/§2.3) and the §4.4 misnomer were RE-RUN as scratch compiles — the `fb`
half of `demand_multi_adorn_1` hits the demand-propagation reject (Demand.cpp
:717-722) and STAYS diagnostic; the §2.3 success seed compiles BOTH adornments
in isolation and hits :457 together; a one-`bf`-adornment swap recursion fires
the body-walk "Multi-adornment" message (the misnomer). The §6 O1 anchors
(Rel.cpp:1111 `table_op_table=pub_table`, :4406/:4454-4458 the abort, :992 pub
by `Id()`), the g1 kBody-only input derivation (:976-982), the linearizer
+1/+1 commute (:5299-5305), the per-store five-way coupling (Database.cpp
:2343-2390, namespaced by `region.StoreId()`), and the band-(b) never-scans-pub
reference-counted publish (:2736/:2801) were all re-read verbatim. VERDICT:
substrate SOUND; the per-pub RULE-AT-CODE (O1) stands.

The ruling context is OD-15 (`d3a-ruling-brief.md`, RATIFIED, not re-litigated
here): **N DISJOINT STORES, one per (query, BindingPattern) forcing; the demand
pass loops STEP 1b→10 per adornment; every registry/lookup keys (query,
BindingPattern).** Recursive demand stays DEFERRED all-epoch (FENCE (i)). The
ritual-head precedent (d2 / R-A2-TRIGGER): if NO admissible design option
changes observable behavior (flat-oracle answers + published deltas identical),
the orchestrator rules AT CODE with no owner brief; only a genuine observable
divergence or an OD-15 conflict escalates.

===============================================================================
## §0 CONSOLIDATOR HEADLINE

1. **THE CENTRAL RITUAL-HEAD RULING (g5/ADV-1): RULE AT CODE — adopt O1.** Relax
   `V-INST-SOLE`'s `inst_per_pub` (Rel.cpp:4406) to key on
   `(pub_table, forcing_index)`, abort unchanged (`!=1u`). Byte-neutral for the
   whole single-adornment corpus (a validator emits nothing; `1==1` under both
   keyings), OD-15-aligned (keeps N disjoint stores), protection-preserving. No
   admissible option moves flat-oracle answers or published deltas — the pub is
   the reference-counted union of the N forcings' demanded rows in every option.
   **O2 (merged publish) REJECTED** — answer-neutral but conflicts with the
   ratified OD-15 N-disjoint-stores mandate and diverges the census (N→1). **No
   owner brief.** (§6.)

2. **g1 is DEFENSIVE, not load-bearing-today, and orchestrator-rules-at-code.**
   The both-set guard-annotation fold arm (View.cpp:651-679) is **DORMANT even
   under multi-adornment** for every shape reachable in the slice (proof §3.2).
   The one required change is the SURVIVOR-RECORD POLICY (force the `kBody` stamp
   to survive), a pure internal-correctness install. The `instance_key` predicate
   keeps its exact behavior (no false-abort is reachable). Both changes MUST
   co-land with the g3 fence-lift (the only removal that could make
   `instance_key` diverge within a forcing). (§3.)

3. **ADJUDICATED CONTRADICTION — g6 disposition. Lane B is CORRECT; Lane A and
   Lane C are WRONG.** `demand_multi_adorn_1` does **NOT** flip diagnostic→golden.
   I compiled its `fb` half in isolation: it hits the LEFT-LINEAR reject
   (Demand.cpp:717-722), not a multi-adornment reject, because bound `To` over a
   right-linear TC never traces off a read of `path`. Post-lift the case STAYS
   diagnostic (its reject MOVES from :457 to the per-adornment :717-722). **g6
   needs a NEW From-preserving two-adornment success witness — I constructed a
   concrete seed** (a non-recursive symmetric relation; both `bf` and `fb`
   compile in isolation, together they hit :457). (§2, §7-g6.)

4. **The message text "Multi-adornment demand is not yet supported" is a
   MISNOMER at the body-walk (Demand.cpp:668-672 / :723-726).** I compiled a swap
   recursion `p(X,Y):p(Y,X),other_1(X)` with ONE declared `bf` adornment: it
   fires that message. The fence is over a SIP-DERIVED SIDEWAYS adornment
   (recursion shape), not a second DECLARED binding pattern. g3 RE-MESSAGES these
   two (condition unchanged, they STAY as fences); the text becomes false the
   moment multi-DECLARED-adornment lands. (§4.)

5. **g4 is a VERIFICATION sweep, not a rewrite. ADV-7 holds in full, item by
   item at code.** 34 keying sites enumerated; all N-safe with NO re-key except
   the three fixes owned by g1/g2/g3 and the one obstruction ruled in §6. (§5.)

===============================================================================
## §1 THE ADORNMENT AXIS AS IT STANDS

### 1.1 The single-adornment pass + lowering (whole-program state)

`QueryImpl::ApplyDemandTransform` (Demand.cpp:385-1167, 1169 lines) is written
end-to-end for **exactly ONE (query, adornment)**. It runs only under
`demand_mode` (:393-395), at most once per module (G2 re-entry reject :399-405,
keyed `module.DemandMessagesFabricated()`). The body:

- **Step 1** collects bound `#query` RELs (:417-429), rejects `>1 bound QUERY
  name` (:435-439), picks `q_rel = bound_queries[0]` / `q_decl` (:441-442), and
  **rejects `>1 binding pattern per name` (:444-462)** — the fence g3 lifts.
- **Steps 2-3** trace the query projection to the read of `p` and locate a SIP
  guard site per rule body (:471-759).
- **Step 4** stray-consumer accounting (:761-791).
- **Step 5** fabricates a demand `#message` + `#local` under the reserved
  `demand__<name>_<adorn>` prefix (:793-849).
- **Step 6** mints the demand seed (IO + receive SELECT + relation + MERGE +
  reader) and **snapshots `forcing_index`/`first_annotation` (:983-985)**.
- **Step 7/8** mint the body guard JOINs (`kBody`) and the query-projection
  guard (`kQueryProjection`), stamping a `GuardAnnotation` carrying
  `forcing_index` per guard (:987-1066).
- **Step 8b/10** register one `RecognizedSubgraph` (:1068-1083) and one
  `QueryDemandForcing` (:1123-1130) per forcing.
- **Step 11** the annotation census (:1132-1163) + `MarkDemandFabricated()`
  (:1165), once per module.

Lowering: `ResolveLiveRecognition` (Rel.cpp:933-1022) re-derives the
(demand, input, pub) triple **per `forcing_index`** from the LIVE (CSE-migrated)
guard JOINs; `BuildSubgraphInstanceOps` (Rel.cpp:1035-1197) mints one
`kSubgraphInstantiate` (+ optional `kInstanceDeath` + `kInstanceSeal`) per live
`RecognizedSubgraph`, allocating `sid = flow.instances.size()` (:1077) → one
`SUBGRAPHINSTANCE` region per store (Procedure.cpp:279ff). Entry points already
fan out per `UniqueRedeclarations()` (Build.cpp:676-689); the forcer/retract
registry matches are already **(query, BindingPattern)**-keyed (Build.cpp:469-471
/ :571-574).

### 1.2 The multi-adornment reject (the single lift point)

Observed at code (scratch, both flags):
`demand_multi_adorn_1.dr` (`reachable_from` at `bf` + `fb`) rejects under BOTH
`-demand` and `-demand -demand-instance` with the **identical** diagnostic at
`patterns.size() != 1u` (**Demand.cpp:457-461**). `-demand-instance` implies
`-demand`; the DataFlow pass rejects first, so the nested pre-pass
(Build.cpp:1504-1552) is never reached. This is the SINGLE lift point.

### 1.3 The N-safe-already inventory (ADV-7 — CONFIRMED item-by-item)

Confirmed at code, NO re-key needed:

| machinery | anchor | why N-safe |
|---|---|---|
| fabricated decl naming/uniquing | Demand.cpp:807-810 | `_<adorn>` suffix ⇒ distinct `demand__q_bf` vs `demand__q_fb`; fresh `DeclarationContext` per fabricate (Parse/Demand.cpp:187-192) ⇒ not merged |
| `decl_to_input`/`decl_to_relation` | Demand.cpp:859/880 | decl-keyed, distinct suffix ⇒ distinct slots; `assert(!io_slot)`/`assert(!rel_slot)` guard double-insert |
| `messsage_handler` registration | Procedure.cpp:706 | ParsedMessage-keyed; N messages → N entries |
| ABI suppression | Database.cpp:1511/3742 | per-message `IsDemandMessage` scan; each suppressed independently |
| nested pre-pass fence | Build.cpp:1513 | already bucketed by `forcing_index` |
| entry-point fan-out | Build.cpp:676-689 | per `UniqueRedeclarations()`, dedup by BindingPattern string |
| forcer / retract match | Build.cpp:469-471 / :571-574 | already (query, BindingPattern[, differential])-keyed |
| Id()-keyed empty-fallback presence set | Build.cpp:1642 | presence-only; two specs share Id() → one bit → both correctly skip `BuildEmptyQueryEntryPoint` |
| mint loop / store-id alloc | Rel.cpp:1051/1077 | per-RecognizedSubgraph; `sid=flow.instances.size()` ⇒ distinct ids |
| all per-store validators | Rel.cpp:4404/4460-4483/4969-4977 | keyed `instance_store_id` |
| census recount | Rel.cpp:3998-4020 | count expectations, scale to N |
| linearizer WAW on shared pub | Rel.cpp:5299-5305 | two +1 folds tie on all but ctor, COMMUTE |

**REFUTED-as-hazard (nothing):** no ADV-7 item was found to secretly need a
re-key. The keying sweep (g4) is a probe/verification pass — §5.

**The exceptions** (three fixes owned by other g-lanes + one obstruction ruled
in §6): g3 snapshot-in-loop (Demand.cpp:983) + stray-union (:770) + once-per-
module census (:1147); g2 the `[F]` always-on handler guard (Build.cpp:393 vs
508); g1 the kBody-survivor policy; and **V-INST-SOLE per-pub** (Rel.cpp:4406) —
the g5/ADV-1 obstruction.

===============================================================================
## §2 THE FIRST TWO-ADORNMENT PROGRAM (the g6 witness seed)

### 2.1 The committed reject shape (verbatim, `demand_multi_adorn_1.dr`)

```
#message edge_2(u64 From, u64 To).
#local path(u64 From, u64 To).
path(F, T) : edge_2(F, T).
path(F, T) : path(F, M), edge_2(M, T).
#query reachable_from(bound u64 From, free u64 To).
#query reachable_from(free u64 From, bound u64 To).
reachable_from(From, To) : path(From, To).
```

ONE query name `reachable_from`, arity 2, TWO binding patterns (`bf` + `fb`)
sharing ONE `DeclarationContext`, over the right-linear TC body. `.drflags` is a
bare `-demand`. Today-diagnostic (observed at code): `Multi-adornment demand is
not yet supported (a demanded query name with more than one binding pattern)
under -demand`, at **Demand.cpp:457-461**, identical under `-demand-instance`.

### 2.2 ADJUDICATION — this case is NOT a two-adornment success case

**Contradiction: Lane A + Lane C claim `demand_multi_adorn_1` flips
diagnostic→golden and becomes the eqgate witness. Lane B claims it STAYS
diagnostic. I resolved this at code (never split the difference):**

Compiling the `fb` half **in isolation** (`reachable_from(free From, bound To)`
over the right-linear TC):
```
error: The demanded relation's bound columns do not trace to a recursive read
or a source atom (unsupported demand propagation shape) under -demand
```
— the **LEFT-LINEAR reject at Demand.cpp:717-722**, NOT a multi-adornment
reject. Bound `To` flows off `edge_2`, never off a read of `path`; right-linear
recursion cannot propagate `To`-demand. The `bf` half in isolation COMPILES
(exit 0 — it is the `demand_tc_witness` shape).

**RULING: Lane B is correct.** Post-lift, `demand_multi_adorn_1`'s reject MOVES
from the per-name :457 to the per-adornment :717-722 (its `fb` half); it STAYS
diagnostic. Lane A/C reasoned about the reject-lift abstractly without checking
whether both adornments compile post-lift; Lane B ran the compile. The eqgate
g6 witness must be a NEW program where BOTH declared adornments are
From-preserving.

### 2.3 THE CONSTRUCTED g6 SUCCESS SEED (new — no lane produced this)

I compiled a non-recursive symmetric relation, both adornments in isolation:

```
#message edge_2(u64 A, u64 B).
#local rel(u64 A, u64 B).
rel(A, B) : edge_2(A, B).
#query q(bound u64 A, free u64 B).   ; (or: free u64 A, bound u64 B)
q(A, B) : rel(A, B).
```

- `q(bound A, free B)` in isolation under `-demand`: **exit 0** (compiles).
- `q(free A, bound B)` in isolation under `-demand`: **exit 0** (compiles).
- BOTH declared together: **hits Demand.cpp:457** (`Multi-adornment demand …`) —
  the exact reject g3 lifts.

Both bound columns trace to the base atom `edge_2`'s OWN position (the direct
copy `rel(A,B):edge_2(A,B)` is From-preserving for BOTH positions), so neither
trips the body-walk position fences (:668/:723). This is the **intended
post-lift behavior**: after g3 lifts :457 and loops per adornment, this program
compiles under `-demand` (flat) and `-demand -demand-instance` (nested) to N=2
disjoint stores; the eqgate asserts flat==nested==golden with sorted
published-delta identity.

**Stage-(b) note:** this exact seed still needs a DRIVER + probes proving the
`bf` probe == neighborhood-by-A and the `fb` probe == neighborhood-by-B, plus a
decision on whether the g6 flagship is this minimal non-recursive shape or a
richer recursion that preserves BOTH positions (e.g. a symmetric-closure body).
The minimal shape is the honest floor; it exercises N=2 stores, the shared-pub
fold, and the per-pub validator relaxation (§6) end-to-end.

===============================================================================
## §3 THE FOLD-ARM SUBSTRATE (g1)

### 3.1 The two directions (View.cpp:571-583 LABELED RESIDUAL, verbatim intent)

The residual comment binds g1 to re-derive `GuardAnnotationsCompatible`
(View.cpp:584-588) in BOTH directions before multi-guard folds first go live:
**(a) survivorship** — the surviving record's `role` is load-bearing downstream;
**(b) invariance** — the `instance_key`-invariance argument holds only while
annotations sit on guard JOINs, and the propagate arm migrates them onto proxy
TUPLEs whose `Equals` ignores pivot vectors.

### 3.2 THE CENTRAL FACT — the arm is DORMANT even under multi-adornment

CSE folds two guard JOINs only if `QueryJoinImpl::Equals` holds (Join.cpp:473-
520): same `num_pivots`, `out_to_in`, and **`joined_views[i]->Equals` pairwise**.
Within one forcing, the body guard reads `.in1 = site.read` (the RAW body/edge)
while the query-projection guard reads `.in1 = q_read` = the body-guard's OUTPUT
— a structurally DIFFERENT view. So `joined_views[1]->Equals` fails ⇒ the two
guards of one forcing are NEVER Equals ⇒ never fold. Verified in real df dumps
(recursive `demand_tc_witness`: 3 distinct guard JOINs, `folded=0`; non-recursive
`demand_neighborhood_mono_witness`: `raw_seed` folds into `d_reader` but the two
GUARDS stay distinct, `folded=0`). Cross-adornment folds are impossible too:
distinct fabricated demand relations ⇒ distinct `joined_views[0]` ⇒ not Equals.

**CONCLUSION: the both-set fold arm stays dormant under multi-adornment for
every shape reachable in the slice.** The "D3.a.0 debug-assert evidence expires"
premise is CONSERVATIVE; g1's work is DEFENSIVE. (Lane A could not construct a
firing fold; the consolidator concurs from the Equals structure.)

### 3.3 Direction (a) — survivorship: a REAL latent cliff, DEFENSIVE fix

The fold arm keeps `that`'s record verbatim (View.cpp:675-681): survivor =
`that` (chosen by CSE depth/det_seq at Optimize.cpp:365-378, NOT by `role`);
the loser clears its index (:680-681). Downstream, `ResolveLiveRecognition`
derives `input_table`/`input_key_cols` **ONLY from a `role==kBody` stamp**
(Rel.cpp:976-982; kQueryProjection explicitly excluded). A both-set fold whose
survivor carries `kQueryProjection` → `ri.input_table` null → `ri.ok` false →
the instance is SILENTLY SKIPPED at mint (Rel.cpp:1053-1055) → missing answer
rows (OBSERVABLE, if reachable). Nothing in the fold arm or the census PREVENTS
it; the census still balances after a role-losing fold.

**Directed witness (a):** a RelValidators death TEST (carried by g2 design-1)
that hand-mutates a flow to force a `kBody→kQueryProjection` survivor fold and
asserts the policy REPAIRS it (survivor ends `kBody`, `ri.ok` holds). Observable
if it ever went live: instance materializes; probe == full neighborhood.

**THE POLICY (pseudocode), landed in the both-set arm after the compat gate,
before the loser's index clears (View.cpp:678→680):**
```
GuardAnnotation &loser = query->guard_annotations[guard_annotation_index];
GuardAnnotation &surv  = query->guard_annotations[that->guard_annotation_index];
// SURVIVOR-RECORD POLICY (g1-a): the surviving RECORD must carry kBody if
// EITHER side is kBody — ResolveLiveRecognition derives input_table ONLY from a
// kBody stamp. The compat gate already proved forcing_index + instance_key
// match, so promoting to kBody (and adopting the kBody side's demand_side/kind/
// guarded_read) is answer-neutral when both are kBody and RECOVERS the input
// when CSE made the kQueryProjection guard the survivor.
if (loser.role == kBody && surv.role == kQueryProjection) {
  surv.role         = kBody;
  surv.demand_side  = loser.demand_side;    // kDReader
  surv.kind         = loser.kind;
  surv.guarded_read = loser.guarded_read;   // the real body read
  // instance_key already equal by the compat gate; demanded_view opaque.
}
// surv==kBody/loser==kQueryProjection: no-op (kBody already survives).
// kBody/kBody keeps kBody; qp/qp keeps qp (no body input to lose).
```
**Belt (g8/g2):** after the census, assert "every live forcing with any
surviving guard has ≥1 kBody survivor" — a policy regression aborts loudly
instead of silently skipping.

### 3.4 Direction (b) — proxy-TUPLE: NOT a reachable false-abort

`CopyDifferentialAndGroupIdsTo` is called from canonicalization proxy sites
(Join.cpp:279, Merge.cpp:647/865/888/924, Link.cpp:98/163/219, Connect.cpp:38/
143) that migrate an annotation onto a proxy TUPLE. `TUPLE::Equals`
(Tuple.cpp:270-290) has NO pivot concept — two proxy TUPLEs can be Equals with
different migrated `instance_key`. For the predicate to FALSE-ABORT, two proxy
TUPLEs must (i) be Equals, (ii) share `forcing_index`, (iii) differ in
`instance_key`. Even under g3's fence-lift (which allows a forcing's body pivot ≠
query-proj pivot, making (iii) possible in principle), (i) still cannot hold: the
two same-forcing guards forward DIFFERENT incoming views ⇒ `ColumnsEq(input_
columns)` fails (Tuple.cpp:284) ⇒ never Equals. A cross-FORCING proxy fold is a
genuine mis-key and the abort is CORRECT.

**DISPOSITION: KEEP the predicate unchanged** (`forcing_index == && instance_key
==`, View.cpp:586-587). Re-derive only its JUSTIFICATION: `forcing_index` is the
load-bearing identity; `instance_key` is a defensive belt — fold-invariant on
JOIN carriers, stale-but-harmless on TUPLE carriers (it can only ever mismatch
across forcings, which `forcing_index` already rejects). Replace the single-
adornment "Equals JOINs share instance_key" comment with "fold-eligible
annotated views of one forcing forward the same incoming view ⇒ identical
instance_key."

**Directed witness (b):** a RelValidators unit builds two same-forcing/different-
key records and asserts `GuardAnnotationsCompatible` returns FALSE (belt intact),
plus the live distinct-bound-column two-adornment shape (bf binds col 0, fb binds
col 1) asserts census `folded == 0`. Observable: bf-probe == neighborhood-by-A,
fb-probe == neighborhood-by-B; no abort.

### 3.5 THE OBSERVABLE CLASSIFICATION (g1 ruling axis)

Both g1 changes are **PURE INTERNAL CORRECTNESS while the arm is dormant** — no
admitted program's flat-oracle answers or published deltas move. → **orchestrator
rules at code** (d2 / R-A2-TRIGGER precedent, no owner brief). The ONLY path to
an owner brief: a later lane DEMONSTRATES a live role-divergent fold — none
found. **Binding co-land constraint:** the survivor policy MUST land in the SAME
slice that lifts the From-preservation fences (Demand.cpp:668/:723) — those are
the only removals that could, in principle, make `instance_key` diverge within a
forcing (§3.4). g1 is the FIRST/BINDING g-lane per the charter: it must land
before g3 puts two forcings' guards on shared readers.

===============================================================================
## §4 THE PASS LOOP (g3)

### 4.1 The STEP map (M / A / S / below)

**(M) once-per-module, ABOVE the loop:** mode gate (:393-395), G2 reject
(:399-405), `reject` lambda (:407-411), bound-query collection (:417-429), empty
check (:431-433), **`>1 bound QUERY` reject (:435-439) STAYS** (different axis —
multiple query NAMES; two adornments of one name = ONE REL), `q_rel/q_decl`
(:441-442), the single-materialization check + `q_insert` (:477-482). **DELETE
the :444-462 per-name multi-adornment belt** — the loop enumerator replaces it.

**(A) per-(query, adornment), the LOOP BODY** iterating
`q_decl.UniqueRedeclarations()`: Step 1's `bound_indices` (:464-469, derived from
the per-iteration `redecl`, NOT `q_decl`), Step 2 trace (:484-577), Step 3 SIP
(:584-759), Step 5 fabricate (:797-849, `redecl`-derived names/types), Step 6
mint (:858-985 INCLUDING the :983-985 snapshots), Step 7/8 guards (:987-1066),
Step 8b subgraph register (:1068-1083), Step 9 tripwire (:1085-1121), Step 10
forcing register (:1123-1130, `ParsedQuery::From(redecl)` + per-adornment
`bound_indices`).

**(S) shared module-global vectors** the loop appends to: `guard_annotations`
(:1001/:1059), `recognized_subgraphs` (:1080), `demand_forcings` (:1130),
`guard_annotation_folded_count` (written only by CSE at View.cpp:678). The maps
`decl_to_input`/`decl_to_relation` are decl-keyed, distinct per adornment.

**(below) once-per-module, AFTER the last adornment:** Step 11 census (:1132-
1163, the two module-global count equalities) + `MarkDemandFabricated()` (:1165).

### 4.2 ADV-2 (DISCHARGED) — snapshots stay INSIDE the loop

`forcing_index = demand_forcings.size()` (:983) and `first_annotation =
guard_annotations.size()` (:984-985) capture the append offsets BEFORE this
adornment stamps. `forcing_index` is the SOLE bucket key at **Rel.cpp:953**
(`guards[annots[ai].forcing_index]`) and **Build.cpp:1513**
(`fguards[annots[ai].forcing_index]`). A naive hoist above the loop → every
adornment reads size 0 → stamps `forcing_index=0` (the append at :1130 is Step
10, AFTER the stamps) → both stores' guards bucket under forcing 0 → the second
store's input/pub resolve off the first's guards → silent mis-key. **They are
loop-body locals, re-read at the top of each iteration.**

### 4.3 ADV-3 (DISCHARGED) — the stray-consumer UNION, two-phase

Step 4 (:761-791) builds `known_consumers` from ONE adornment's `q_consumer` +
site consumers, then walks `CollectColUsers(p_merge)`. Both adornments demand the
SAME `p`, so they SHARE `p_merge` and its readers. If adornment A completes its
Step 7 (minting A's guard JOINs, which become NEW consumers of the shared reader
`t` via `MintGuardJoin({d_reader, t})`) before B's Step 4 runs, B's Step 4 sees
A's guards as untraced → false reject at :784-788. **Fix — restructure into two
phases so Step 4 runs ONCE over a UNION on the PRE-MINT graph:**
```
// Phase 1 (per adornment): TRACE + SIP LOCATE only (Steps 2,3). NO minting.
known_consumers = {}            // pass-level union
plan = []
for redecl in q_decl.UniqueRedeclarations():
  … Step 2 trace (per-adornment locals) …    // rejects :491,498,507-512,514,
                                              //   524,533-537,545-549,559-560
  … Step 3 SIP locate → sites, pushdown_reads …  // STAY rejects §4.4;
                                                  //   :668-672/:723-726 RE-MESSAGED
  known_consumers.insert(q_consumer)
  for s in sites: known_consumers.insert(s.consumer)
  assert(plan.empty() || p_merge == plan[0].p_merge)   // one relation p
  plan.push_back({redecl, bound_indices, p_bound, q_read, q_consumer,
                  p_merge, sites, pushdown_reads})

// Step 4 ONCE over the union, on the pre-mint graph (all p_merge identical).
for user in CollectColUsers(this, plan[0].p_merge):
  t = user.AsTuple(); if !t || !IsFullWidthReaderOf(t, p_merge): reject
  for ruser in CollectColUsers(this, t):
    if ruser not in known_consumers: reject          // union, not per-adornment

// Phase 2 (per adornment): FABRICATE + MINT + GUARD + REGISTER (Steps 5-10),
//   each snapshotting forcing_index/first_annotation at its Step 6 (ADV-2).
for a in plan: … Steps 5-10 …

// below: Step 11 census + MarkDemandFabricated, ONCE.
```
Deferring the check to AFTER a single Steps-2-10 loop is WRONG (minted guards
become untraced consumers). For `|plan|==1` the two-phase split is **[BYTE]-
neutral**: emission order 2,3→4→5..10→11,mark is identical to today, at the same
graph states (Step 4 still pre-mint). The `p_merge == plan[0].p_merge` assert
encodes the one-relation-`p` invariant the loop relies on.

### 4.4 The narrowed rejects — RE-MESSAGE, do NOT narrow the condition

**Observed at code (grounds the re-message):** a swap recursion
`p(X,Y):p(Y,X),other_1(X)` with ONE declared `bf` adornment fires `Multi-
adornment demand is not yet supported under -demand` — the body-walk :723 reject.
The bound value comes off the recursive read at position 1 ≠ adornment position
0. So :668-672 / :723-726 fence a **SIP-DERIVED SIDEWAYS adornment (recursion
shape)**, not a second DECLARED binding pattern. The MESSAGE is a MISNOMER.

No body-walk reject fires spuriously on a legit SECOND DECLARED adornment (each
declared adornment is an independent From-preserving SIP with its own `p_bound`;
none trips another's position check). Only change: **RE-MESSAGE :668-672 /
:723-726** to "sideways/non-From-preserving demand propagation is not yet
supported under -demand" (mirroring the :717-722 left-linear message). All other
body-walk rejects STAY unconditionally: NEGATE/AGG on demand path (:625-628),
self-join (:633-637), left-linear (:717-722), and the malformed-shape belts
(:590,:622,:630,:647,:657,:692,:705,:733-734,:745-748).

### 4.5 The single-adornment [BYTE]-neutrality obligation

For the existing single-adornment corpus, the whole g3 restructure MUST be
byte-identical (|plan|==1):
- Two-phase split → identical emission order/graph states.
- `redecl`-derived `bound_indices`/`adorn`/`bound_types` == `q_decl`-derived (the
  sole `UniqueRedeclarations()` element IS the stored declaration).
- `ParsedQuery::From(redecl)` at Step 10 == `From(q_decl)` for one adornment.
- Stray-consumer union == today's single set.
- The two RE-MESSAGED rejects are text-only; no single-adornment corpus program
  reaches them (they are exit paths, and the corpus compiles).

**Genuinely NEW (no existing corpus program hits it):** the :444-462 reject is
GONE — `demand_multi_adorn_1`'s golden CHANGES (its `bf` half now compiles, its
`fb` half fires the per-adornment :717-722 reject; expected diagnostic-shape
change, bless-reviewed, STAYS diagnostic per §2.2). N>1 forcings arm the g5/
ADV-1 per-pub validator and (in principle) the g1 fold arm — g1 lands FIRST.

===============================================================================
## §5 THE KEYING SWEEP + N-STORE (g4/g5)

### 5.1 The cross-wire table (g4 — 34 sites, verdict abridged)

| # | site | key | verdict |
|---|------|-----|---------|
| 2 | multi-adorn reject Demand.cpp:457-461 | per-NAME | **LIFT (g3)** |
| 3 | `>1 bound query` :435-439 | per-(name,arity) REL | **STAYS, N-safe** |
| 4 | `forcing_index`/`first_annotation` :983-985 | positional | **N-safe IFF snapshotted inside loop (g3/ADV-2)** |
| 5 | `known_consumers` / stray reject :770-790 | per-adornment set | **HAZARD → g3 UNION (ADV-3)** |
| 6 | `decl_to_input`/`decl_to_relation` :859/880 | fabricated decl | N-safe (distinct suffix + assert) |
| 7-9 | `guard_annotations`/`recognized_subgraphs`/`demand_forcings` appends | positional, carry `forcing_index` | N-safe if #4 holds |
| 10 | Step-11 census + `MarkDemandFabricated` :1147/1165 | module-global counts | **once-per-module (g3/ADV-6)** |
| 11 | `IsDemandMessage` :310-317 | ParsedMessage | N-safe (per-message) |
| 12 | body-walk :668/:723 | positional | **g3 RE-MESSAGE (not narrow)** |
| 14/15 | forcer/retract match Build.cpp:469-471/:571-574 | (query, BindingPattern[, differential]) | **N-safe (VERIFIED)** |
| 16 | nested pre-pass fence Build.cpp:1513 | forcing_index | N-safe |
| 17/18 | entry-point fan-out :676-689 / query spec :617 | BindingPattern | N-safe (N entry points; shared pub table :601) |
| 19 | empty-fallback presence set :1642 | Id() | N-safe (presence-only) |
| 20 | `[F]` handler-map guard :393-394 vs :508-512 | ParsedMessage | **HAZARD → g2 always-on fence (ADV-8)** |
| 21 | `messsage_handler.emplace` Procedure.cpp:706 | ParsedMessage | N-safe |
| 23 | ABI suppression Database.cpp:1511/3742 | ParsedMessage | N-safe |
| 24/25 | per-store types/ops Database.cpp:903ff/2341ff | store id | N-safe |
| 26/28 | recognition buckets / input resolution Rel.cpp:953/976 | forcing_index + kBody | N-safe (g1 owns kBody-survivor) |
| 27 | pub resolution Rel.cpp:992 | Id()=(name,arity) | **SHARED-BY-DESIGN** — root of #29 (§6) |
| 29 | **V-INST-SOLE per-pub Rel.cpp:4406/:4454-4458** | pub table pointer | **THE OBSTRUCTION (§6)** |
| 30-34 | census recount / per-store validators / mint sids / linearizer WAW / V-INST-DRAIN | count / store id / table | N-safe (VERIFIED) |

**Verdict: N-safe except the three fixes owned by g1/g2/g3 (#4/#5/#10/#20/#28)
and the one obstruction (#29). ADV-7 holds in full.** No name-only keying
survives once g3 stamps `forcing_index` correctly.

### 5.2 g5 mint readiness

The mint (`BuildSubgraphInstanceOps`, Rel.cpp:1035-1197) is ALREADY N-forcing:
per-`RecognizedSubgraph` enumeration (:1051), fully-dead skip (:1053-1055),
`sid=flow.instances.size()` (:1077), per-store `kSubgraphInstantiate`/optional
`kInstanceDeath`/`kInstanceSeal`, `instance_stratum[sid]` (:1194). N live forcings
→ N stores → N SUBGRAPHINSTANCE regions. **READY except the one per-pub
validator** (§6).

### 5.3 Census arithmetic (two mono-demand adornments over mono input)

For the g6 seed (`bf` + `fb` over a monotone relation, bare `-demand`), N=2:

| kind | value | derivation |
|---|---|---|
| `kSubgraphInstantiate` | **2** (=N) | one mint per live forcing (Rel.cpp:1051/1109); recount `expect(…,2)` :4018 |
| `kInstanceSeal` | **2** (=N) | 1:1 with instantiate (:1176) |
| `kInstanceDeath` | **0** | monotone demand → `TableIsDifferential(demand)` false → gate :1162 mints none |
| `kIngestFold` | **3** | 1 shared input receive + 2 fabricated demand receives (`MakeMonotoneIngestFold`) |
| `kSeedFold` | `shared_seeds + 2·per_adorn_demand_seeds` | demand-side web duplicates per adornment; input/pub seeds count once (exact integer needs a build the g3 fence currently blocks) |

The instance census is **per-pub-option-INDEPENDENT under O1** — V-INST-SOLE's
keying decides only whether the validator aborts, not how many ops are minted.
**O2 alone would perturb kSubgraphInstantiate/kInstanceSeal N→1** — a census
divergence and an OD-15 conflict (§6).

===============================================================================
## §6 THE PER-PUB RITUAL-HEAD QUESTION (g5/ADV-1) — THE HEADLINE

### 6.1 The obstruction, precisely

`V-INST-SOLE` tallies, per pub table, how many `kSubgraphInstantiate` ops derive
it (Rel.cpp:4404-4407):
```
++inst_per_store[op.instance_store_id];
if (op.table_op_table) {
  ++inst_per_pub[reinterpret_cast<uintptr_t>(op.table_op_table)];
}
```
and aborts if any count `!= 1u` (Rel.cpp:4454-4458). Every instantiate sets
`table_op_table = pub_table` (:1111); `pub_table` resolves by
`ins.Declaration().Id() != q_decl.Id()` (Rel.cpp:992) — name+arity. N adornments
of one query share `Id()` → the SAME model table → `inst_per_pub[pub] == N` →
**LOUD abort.** This is a genuine INCOMPLETENESS: "one instantiate deriver per
pub table" is a TAUTOLOGY of the single-adornment slice, never a correctness
requirement. Under N adornments the LEGAL deriver count IS N — `p` is one table
holding the reference-counted union of what any adornment demands.

### 6.2 Why the shared pub is CORRECT (verified at band-(b), Database.cpp:2711-2833)

Band-(b) publish scans each store's OWN `Touched()` keys (:2736) and folds its
delta into pub via `AddDerivation`/`SubDerivation` (:2801/:2763) — it NEVER scans
pub. A row demanded by BOTH adornments gets `C_nr=2`; losing one adornment's
demand leaves `C_nr=1` — the row STAYS present, `was!=now` false → NO published
delta — identical to flat `-demand`. Monotone pub uses idempotent `TryAdd`
(:2785) → set-union. The two +1 instantiate folds are a forward WAW the
linearizer serializes by ctor (Rel.cpp:5299-5305); they COMMUTE. The shared input
frontier is READ non-destructively (range-for :2560). **No shared-resource hazard
beyond the validator.** (Consolidator adopts Lane C's band-(b) trace; the anchors
:2711-2833/:2560 are Lane C's — the orchestrator should spot-check them, they lie
outside the §9 core set this consolidator re-verified personally.)

### 6.3 The enumerated options + observable-behavior analysis

**(O1) Relax `inst_per_pub` to key on `(pub_table, forcing_index)`** (change the
map key at Rel.cpp:4406 to the pair; keep `!=1u`). Each live forcing mints
exactly one instantiate (per-store `inst_per_store[sid]==1` already), so this
stays a real bug-catcher (two instantiates for ONE forcing still aborts) while
admitting N forcings per pub.
- Emitted code / published deltas: UNCHANGED (a validator emits nothing).
- BYTE-neutral for single-adornment corpus: YES — each pub has one forcing →
  `inst_per_pub[(pub,0)] == 1`, exactly as before; no golden moves.
- Protection lost: NONE — the residual bug (a spurious extra instantiate for one
  forcing) is subsumed by the census recount `expect(kSubgraphInstantiate,…)`
  (:4018) + per-store `inst_per_store==1` (:4467); O1 keeps per-(pub,forcing)
  sanity on top.

**(O2) Merge the N publishes into one deriver** (one instantiate reading N stores).
- Observable: IDENTICAL pub content.
- **CONFLICTS with OD-15** (RATIFIED "one SUBGRAPHINSTANCE region per adornment
  forcing") — a merged deriver is ONE region, not N.
- Census divergence: kSubgraphInstantiate/kInstanceSeal N→1.
- Higher diff cost (new multi-store fold shape, new accounting; abandons the
  per-store factoring the whole D3.a substrate rests on).

**(O3) Drop the per-pub check entirely.** Answer/byte-neutral like O1 but strictly
weaker — discards the per-(pub,forcing) sanity O1 keeps for free. Dominated.

### 6.4 THE RULING

Does any admissible option change OBSERVABLE behavior (flat-oracle answers +
published deltas)? **NO** — O1/O2/O3 all leave the materialized pub (and its
deltas) identical; the pub is the reference-counted union of the N forcings'
demanded rows in every option (§6.2 is the proof). Per the d2 / R-A2-TRIGGER
precedent:

> **RULE AT CODE — adopt O1.** Relax V-INST-SOLE's `inst_per_pub` to key on
> `(pub_table, forcing_index)` (Rel.cpp:4406), abort unchanged. Grounds:
> (a) diff-minimal (one map-key change, no emission touched); (b) byte-neutral
> for the whole single-adornment corpus; (c) OD-15-aligned (keeps N disjoint
> stores / N instantiate ops / N seals); (d) protection-preserving. **O2 REJECTED**
> (conflicts with the ratified OD-15 N-disjoint-stores mandate + census divergence
> + higher cost). **O3 dominated by O1.**

**ESCALATION: NONE.** The observable test is met and O1 satisfies OD-15 as
written; O2's OD-15 conflict is resolved BY choosing O1, not by amending the
ruling. This is the central D3.a.3 design question and it is DISCHARGED at code.

===============================================================================
## §7 THE GAP LEDGER (g1-g8 → stage-(b) sub-lane)

- **g1 (fold predicate, BINDING/FIRST) — SUBSTANTIALLY CLOSED at design.** The
  arm is dormant (§3.2); direction (a) needs the SURVIVOR-RECORD POLICY (§3.3
  pseudocode) + the "≥1 kBody survivor" belt; direction (b) keeps the predicate,
  re-derives the justification (§3.4). Rule-at-code. **OPEN for stage (b):** the
  two directed RelValidators witnesses (carried by g2 design-1); the binding
  co-land with the :668/:723 fence-lift.
- **g2 (builder dedup + [F] guard) — OPEN (design sub-lane).** ONE parameterized
  forcer/retract builder + one dispatcher; harden Build.cpp:393-394 to an
  always-on fence (mirror :508-512). CARRIES the g1 RelValidators death TEST +
  the E2c a2/a2' gate-clone dedup. Anchors: twins Build.cpp:385/:494.
- **g3 (pass loop) — CLOSED at design (§4).** Two-phase locate/check/mint loop;
  lift :444-462; RE-MESSAGE :668-672/:723-726; snapshots-in-loop; stray-union;
  once-per-module census. **OPEN:** the [BYTE] proof harness for |plan|==1.
- **g4 (keying sweep) — CLOSED as a verification (§5.1).** ADV-7 holds. **OPEN:**
  the directed two-adornment probe per site (a stage-(b) test-authoring task).
- **g5 (N disjoint stores) — CLOSED once §6 O1 lands.** Mint is READY. **OPEN:**
  the O1 one-line change + its co-land; ADV-10 per-store five-way-coupling
  confirmation (Database.cpp:2343-2390 written for ONE region — verify TouchedFlag
  / V-INST-FRESH occupancy / demand-liveness gate are per-store, no cross-store
  aliasing).
- **g6 (witness family) — REDISPOSED (§2).** `demand_multi_adorn_1` STAYS
  diagnostic (its reject moves :457→:717-722). **OPEN:** author the NEW
  From-preserving two-adornment SUCCESS witness (the §2.3 seed) + driver + probes
  + `.eqgate` sidecar (flat==nested==golden, sorted-delta identity), lifting the
  eqgate family to five.
- **g7 (surviving fences) — CONFIRMED OPEN as fences.** Recursive demand FENCE
  (i) all-epoch (NeedsInductionCycleVector); the R-5 OB8(i) widening obligation
  (ADV-9) — any body-walk/recognition WIDENING must re-derive the derived-input
  branch with a directed witness FIRST (Rel.cpp:4903-4905 + the induction-owned
  belt :4397-4402). Not touched by the adornment axis, but load-bearing text.
- **g8 (liveness) — OPEN (ritual).** L-rows both fold directions, keying-sweep
  probes, the dedup builder fence, the L15 support-1 red-team lesson (a masked
  negative that red-teams NOTHING is a lie — every belt AND the eqgate must have
  teeth).

===============================================================================
## §8 THE RITUAL-HEAD QUESTIONS FOR STAGE (b) (most-important-first)

1. **[RULE-AT-CODE — DISCHARGED §6] V-INST-SOLE per-pub.** Adopt O1: key
   `inst_per_pub` on `(pub_table, forcing_index)` (Rel.cpp:4406). The central
   question; no owner brief. Stage (b) confirms the one-line change is byte-
   neutral against the pinned .irgold + nested census goldens.
2. **[RULE-AT-CODE — §3] The g1 survivor-record policy + predicate justification.**
   Pure internal correctness while dormant. Land the kBody-promotion policy;
   keep the `instance_key` predicate. Co-land with the :668/:723 fence-lift.
3. **[RULE-AT-CODE — §4] g3 pass-loop restructure.** Two-phase locate/check/mint;
   RE-MESSAGE (not narrow) :668/:723; snapshots-in-loop; stray-union; once-per-
   module census. [BYTE]-neutral for the single-adornment corpus — a mechanical
   obligation, not a judgment call.
4. **[RULE-AT-CODE — §2] g6 witness disposition.** `demand_multi_adorn_1` STAYS
   diagnostic (adjudicated at code). Author the NEW From-preserving success
   witness from the §2.3 seed. Whether the flagship is minimal-non-recursive or a
   both-positions-preserving recursion is a stage-(b) design choice (default: the
   minimal seed as the honest floor).
5. **[RULE-AT-CODE — g2] The [F] handler-map guard dedup.** One always-on fence
   (mirror :508-512 at :393-394) + one parameterized builder. Mechanical.
6. **[OWNER-BRIEF TRIGGER, CONTINGENT] A live role-divergent both-set fold.** If
   stage (b) ever DEMONSTRATES a reachable both-set fold with role divergence
   (making §3.3 observable), escalate — the survivor policy stops being purely
   internal. Lane A + the consolidator found NONE; flagged so a future widening
   re-checks.
7. **[OWNER-BRIEF TRIGGER, CONTINGENT] Any body-walk/recognition WIDENING
   (ADV-9 / R-5).** If D3.a.3 widens admitted body shapes beyond the current
   From-preserving SIP, the OB8(i) derived-input branch must be re-derived with a
   directed witness FIRST — potentially an owner brief. The multi-adornment loop
   as scoped does NOT widen shapes (each declared adornment is an independent
   From-preserving SIP), so this stays a dormant trigger.

**No stage-(b) question currently requires an owner brief.** The central one
(per-pub) and all mechanical ones rule at code; the two owner-brief triggers are
contingent on findings neither this stage nor any lane produced.

===============================================================================
## §9 THE TEN MOST LOAD-BEARING ANCHORS (orchestrator: re-verify personally)

All re-read at code this session by the consolidator; quotes verified verbatim.

1. **Demand.cpp:457-461** — the multi-adornment reject g3 lifts (`patterns.size()
   != 1u`). Observed to fire under both flags for `demand_multi_adorn_1` and the
   §2.3 seed.
2. **Demand.cpp:983-985** — `forcing_index`/`first_annotation` snapshots (ADV-2);
   MUST stay inside the g3 loop.
3. **Demand.cpp:668-672 / :723-726** — the two body-walk position fences;
   RE-MESSAGE (condition stays). Observed to fire (misnamed) on a ONE-adornment
   swap recursion.
4. **Demand.cpp:717-722** — the left-linear reject; where `demand_multi_adorn_1`'s
   `fb` half lands post-lift (observed at code — the g6 adjudication).
5. **Rel.cpp:4404-4407 / :4454-4458** — V-INST-SOLE `inst_per_pub` key + abort;
   the §6 O1 relaxation target.
6. **Rel.cpp:990-997** — pub resolution by `q_decl.Id()` (name+arity); the root of
   the shared-pub / per-pub obstruction.
7. **Rel.cpp:976-982** — input resolution ONLY from a `role==kBody` stamp; the
   downstream that makes the g1 survivor policy load-bearing.
8. **Rel.cpp:1051-1055 / :1077** — the mint loop per-`RecognizedSubgraph` + per-
   store `sid`; the N-store readiness core.
9. **View.cpp:584-588 (predicate) + :675-682 (fold-arm survivor)** — g1's two
   sites; the predicate is KEPT, the fold arm gains the survivor policy.
10. **Build.cpp:469-471 (forcer match) / :393-394 vs :508-512 ([F] guard
    asymmetry) / :1513 (forcing bucket)** — the (query, BindingPattern) keying
    that is already N-safe + the one release-SIGSEGV asymmetry g2 fixes + the
    per-forcing bucket.

**Anchors adopted from lanes but OUTSIDE this consolidator's personal re-verify
(orchestrator should spot-check):** the band-(b) publish trace
Database.cpp:2711-2833 / :2560 (Lane C, §6.2); the five-way coupling
Database.cpp:2343-2390 (ADV-10, §7-g5); the linearizer WAW Rel.cpp:5299-5305
(Lane C, §6.2).
