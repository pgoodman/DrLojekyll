# D1.a DESIGN — annotation + recognition registry (DataFlow-side, INERT)

Repo: /Users/pag/Code/DrLojekyll  Branch: keyed-instances  Tip: **0475306d**
(docs-only above 99f211f5; all code line numbers below re-verified at this tip).
Absolute paths throughout. Precedence stack: d1-pinned.md (WINS) >
d1-design-consolidated.md §A.1/§B > d1-desired-states.md (background). Ledger:
KeyedInstances.md §19(C) D1.a block, §19(I) rulings, §19(J) E-81/E-82,
§1 E-46/E-52.

D1.a = the annotation FIELD + recognition REGISTRY on the DataFlow side, stamped
by the (already-landed) demand pass and migrated through the existing metadata
choke point. It is **INERT**: zero DR-IR/emission change, zero golden churn,
PassPolicy untouched, no Runtime edit (d1-pinned §3 "D1.a — unchanged, PLUS
HP-9"; consolidated §B D1.a). The three DROpKinds, validators, and dump grammar
are D1.b — NOT here.

> **VERIFICATION NOTE.** Every load-bearing shape below was checked against the
> real code, and the two witnesses were **compiled at tip** with `-df-out` (the
> DataFlow BB-with-args dump, Main.cpp:207) to confirm the guard-JOIN structure
> empirically. Dumps in this scratchpad: `tc.df`, `nbhd.df`. Where I correct a
> pinned/consolidated claim I flag it LOUDLY in §5 and the return payload.

======================================================================
## §1. CURRENT-STATE PSEUDOCODE (the touched surfaces, from real code)
======================================================================

### §1.1 `QueryImpl::ApplyDemandTransform` — step sequence + the stamp sites

Entry `lib/DataFlow/Demand.cpp:363` (called from `Query::Build` at
`lib/DataFlow/Build.cpp:2587`, AFTER `ConnectInsertsToSelects` :2570, BEFORE
`impl->Optimize` :2599). Abbreviated to the D1.a-relevant spine:

```
ApplyDemandTransform(module, log, demand_mode):
  if !demand_mode: return true                       # :370  total no-op, nothing minted
  if module.DemandMessagesFabricated(): reject       # :376  single-shot re-entry
  ... STEP 1 collect the single bound query  (:394-446)
  ... STEP 2 trace query projection chain -> q_read (full-width reader of p's
             MERGE), q_consumer, p_merge, p_bound        (:454-554)
  ... STEP 3 SIP walk: build `sites` (a std::vector<GuardSite>) by iterating
             `for member_v : p_merge->merged_views { ... sites.push_back(site) }`
                                                          (:564-736)   <-- ORDER SOURCE
  ... STEP 4 stray-consumer accounting                    (:746-768)
  ... STEP 5 fabricate demand message + #local            (:770-825)
  ... STEP 6 mint the demand-seed graph: recv, d_merge, and
             d_reader = TUPLE over d_merge                (:827-952)
  # ---- STEP 7: guard each rule body at its site ----     (:954-981)
  for (const GuardSite &site : sites) {                    # :957  <-- STAMP LOOP
    JOIN *guard = MintGuardJoin(this, site.read, d_reader, site.pivot_pos, ...) # :959  STAMP SITE A
    if site.kind == kReadAtTuple:  RewireConsumer(site.consumer, site.read, out, guard)   # :967
    else /*kPushDown|kBaseAtom*/:  restore = MintRestoringTuple(...);           # :974
                                   RewireConsumer(site.consumer, site.read, restore, restore)  # :979
  }
  # ---- STEP 8: query-projection guard on the RAW seed ----(:983-1004)
  { raw_seed = tuples.Create(); ... project recv->columns  # :990-998
    JOIN *guard = MintGuardJoin(this, q_read, raw_seed, p_bound, ...)  # :1001  STAMP SITE B
    RewireConsumer(q_consumer, q_read, out, guard); }       # :1003
  ... STEP 9 TRIPWIRE (fprintf+abort if no seed)            (:1006-1042)
  ... STEP 10 demand_forcings.emplace_back(forcing); MarkDemandFabricated()  (:1051/:1053)
  return true
```

**The two stamp-candidate sites are the two `MintGuardJoin` calls** (definition
`Demand.cpp:150-196`): `:959` (step 7, body, demand side = `d_reader`) and
`:1001` (step 8, query projection, demand side = `raw_seed`).

### §1.2 The THREE GuardSite kinds — where each is classified (E-46)

`struct GuardSite { enum Kind { kReadAtTuple, kPushDown, kBaseAtom } kind{kReadAtTuple}; ... }`
at `Demand.cpp:127-140`. The kind is assigned ONLY inside STEP 3's per-body
site-location trace (`Demand.cpp:632-708`), never at STEP 7/8:

| kind          | assigned at | condition (verified) |
|---------------|-------------|----------------------|
| `kReadAtTuple`| `Demand.cpp:650` | bound value comes straight off a full-width read of p via a **TUPLE** (`IsFullWidthReaderOf(pt,p_merge)` :639) at α's own position (`in_col->Index()==pos` :645) — a DIRECT recursive read; consumer = the TUPLE/INSERT |
| `kBaseAtom`   | `Demand.cpp:659` | traced TUPLE is a **message-receive atom** read (`AtomReceiveReadOf(pt)` :656) — a base rule; consumer = the TUPLE/INSERT |
| `kPushDown`   | `Demand.cpp:704` | bound value emerges from the body's **JOIN** tree (`pv->AsJoin()` :675) with a full-width p read at α's position among its inputs (`found_col->Index()==pos` :700) — a recursive subgoal; consumer = the JOIN |

Per-body cross-column consistency: after the first bound column fixes `site`,
every later bound column must yield the SAME kind+consumer+read (else reject,
`:718-727`).

**Load-bearing for the stamp (E-46):** STEP 7 branches on `site.kind` — only
`kReadAtTuple` skips the restoring TUPLE (`:962` no restore; else
`MintRestoringTuple` `:974`). So the kind is real, distinguishable structure
that the D1.a annotation must record per site.

**STEP 8 has NO GuardSite.** The query-projection guard (`:1001`) is minted
outside the `sites` machinery: `q_read` is by construction the full-width reader
of p's MERGE and `q_consumer` reads it directly (the kReadAtTuple SHAPE, but not
classified through the enum). D1.a must therefore ASSIGN this stamp a kind — see
§2.4; the value that matches its shape (and makes P-D1a.2 hold) is `kReadAtTuple`,
distinguished from body sites by `role=kQueryProjection` + `demand_side=kRawSeed`.

### §1.3 DemandForcings registration (the recognition-unit source, X-9)

`struct QueryDemandForcing { ParsedQuery query; ParsedMessage message;
std::vector<unsigned> bound_params; }` — public,
`include/drlojekyll/DataFlow/Query.h:957`. Storage
`std::vector<QueryDemandForcing> demand_forcings;` on QueryImpl at
`lib/DataFlow/Query.h:1133` (internal header — E-82: NOT the public :1133).
Populated at exactly one site, `Demand.cpp:1051` (STEP 10). Accessor
`Query::DemandForcings()` declared `include/drlojekyll/DataFlow/Query.h:995`,
defined `Demand.cpp:283`. This is the accessor shape the new accessors mirror.

### §1.4 `CopyDifferentialAndGroupIdsTo` — the metadata choke point + funnel

Definition `lib/DataFlow/View.cpp:557-571` (decl `lib/DataFlow/Query.h:282`).
It is a `QueryViewImpl` method taking ONLY `that`; it touches **only view
members** — no QueryImpl in scope:

```
void QueryViewImpl::CopyDifferentialAndGroupIdsTo(QueryViewImpl *that):
  that->group_ids += this->group_ids ; std::sort(that->group_ids)   # :560-562
  if this->can_receive_deletions:  that->can_receive_deletions = true   # :564-566
  if this->can_produce_deletions:  that->can_produce_deletions = true   # :568-570
```

`group_ids`/`can_receive_deletions`/`can_produce_deletions` are per-view fields
at `Query.h:458/505/506`. Purely additive/monotone.

**The RAUW funnel** (all routes pass through the ONE :588 call):
- `QueryViewImpl::SubstituteAllUsesWith(that)` (View.cpp:575-598): moves
  is_used_by_negation, RAUWs each column + the view def, then
  `CopyDifferentialAndGroupIdsTo(that)` at **View.cpp:588** (unconditional),
  then merges color.
- `QueryViewImpl::ReplaceAllUsesWith(that)` (View.cpp:602-605):
  `SubstituteAllUsesWith(that); PrepareToDelete();`.

**Every EXPLICIT hand call site** (E-81: TWELVE, the 12th at Merge.cpp:924):
Connect.cpp:38/143 · Merge.cpp:647/865/888/**924** · Link.cpp:70/98/163/219 ·
Join.cpp:279 · Induction.cpp:444 — plus the View.cpp:588 funnel. The Join
self-canon RAUWs (Join.cpp:158/424) also route through the funnel. So a hook
inside CopyDifferentialAndGroupIdsTo covers **CSE, both Join self-canon sites,
every RAUW route, and all 12 hand sites** — exactly group_ids coverage.

**The demand pass NEVER calls this funnel.** `ApplyDemandTransform` rewires via
the targeted `RewireConsumer` (Demand.cpp:220-279), so its minted guard JOINs /
d_reader / raw_seed start with EMPTY group_ids and both deletion flags false;
any (A)-metadata they gain arrives later, from the post-demand Optimize/CSE
funnel. The D1.a stamp is therefore the FIRST metadata the demand pass authors.

======================================================================
## §2. THE DIFF (exact insertion-point pseudocode)
======================================================================

### §2.1 `GuardAnnotation` — new struct (VERBATIM d1-design-consolidated §A.1.1)

Home: `include/drlojekyll/DataFlow/Query.h`, beside `QueryDemandForcing` (:957).

```cpp
struct GuardAnnotation {
  enum Kind : uint8_t { kReadAtTuple, kPushDown, kBaseAtom };
  enum DemandSide : uint8_t { kDReader, kRawSeed };
  enum Role : uint8_t { kBody, kQueryProjection };
  Kind kind; DemandSide demand_side; Role role;
  bool is_instance_key;                // marks RECURSIVE-subgoal sites (a D3
                                       //   boundary), NOT the top instance [X-9]
  std::vector<unsigned> instance_key;  // pivot positions within the guarded read
  QueryView guarded_read;              // OPAQUE handle; equality/lookup ONLY,
  QueryView demanded_view;             //   NEVER enters any order [F1-annot]
  unsigned forcing_index;              // index into query.DemandForcings()
};
```

> **[ADJ:crit-correctness-3 / crit-pins-4] REQUIRED.** `GuardAnnotation::Kind`
> and `GuardSite::Kind` (`Demand.cpp:127-132`) are two independently-declared
> enums in different TUs; §2.4 bridges them by `static_cast`. That cast is
> value-correct ONLY while both share `{kReadAtTuple,kPushDown,kBaseAtom}=0,1,2`.
> A reorder/insert on EITHER silently mis-kinds every body stamp with no
> diagnostic. Couple them at the cast site with a compile-time assert (place
> beside `GuardAnnotation` or beside the STEP-7 stamp; the cast site must see
> `GuardSite::Kind`, so home it in Demand.cpp):
> ```cpp
> static_assert((int)GuardAnnotation::kReadAtTuple == (int)GuardSite::kReadAtTuple &&
>               (int)GuardAnnotation::kPushDown     == (int)GuardSite::kPushDown     &&
>               (int)GuardAnnotation::kBaseAtom     == (int)GuardSite::kBaseAtom,
>               "GuardAnnotation::Kind must stay value-aligned with GuardSite::Kind");
> ```

The `Kind`/`DemandSide`/`Role` fields mirror the code's three real axes: the
GuardSite kind (§1.2), which MintGuardJoin demand side was used (§1.1 A vs B),
and body-vs-query-projection. `is_instance_key` stays `false` for the whole D1.a
slice (recursive-subgoal marking is a D3 concern — X-9). `guarded_read` /
`demanded_view` are `QueryView` opaque handles (equality/lookup only — F1 pin;
`UniqueId()` is a pointer, Node.h:31-33, so they must NEVER be sorted/iterated).

### §2.2 `RecognizedSubgraph` — new struct (VERBATIM §A.1.5)

```cpp
struct RecognizedSubgraph {           // ONE PER DemandForcing entry (X-9)
  unsigned forcing_index;             // -> query.DemandForcings()[i]
  QueryView demanded_view;            // p_merge (opaque handle)
  std::vector<unsigned> key_cols;     // the forcing's bound α positions
  QueryView pub_view;                 // the answer view (insert target)
  std::vector<unsigned> guard_annotation_indices;  // its guards
};
```

### §2.3 Storage on QueryImpl + the per-view association field

`lib/DataFlow/Query.h`, beside `demand_forcings` (:1133):

```cpp
// Append order = the deterministic stamp-loop order of ApplyDemandTransform
// (STEP-3 merged_views walk, then the single query-projection). NEVER re-sorted.
std::vector<GuardAnnotation> guard_annotations;

// One per DemandForcing; append order = forcing order.
std::vector<RecognizedSubgraph> recognized_subgraphs;

// [ADJ:crit-pins-5] Count of both-annotated guard folds (survivor's entry kept,
// loser's index folded away). Consumed by the §2.6 census equation; incremented
// only in the both-annotated transfer branch (§2.5). Dormant in the D1/D2 slice.
unsigned guard_annotation_folded_count{0u};
```

> **[ADJ:crit-pins-1] OWNER-RATIFIABLE MECHANISM SWAP (OD-2 shape).** The per-view
> `guard_annotation_index` field below REPLACES the pinned/consolidated §A.1.2
> `std::unordered_map<QueryViewImpl*,unsigned> guard_annotation_of` + §A.1.3
> `impl->TransferGuardAnnotation(this,that)`, which d1-pinned §3 folds under "D1.a
> — unchanged." The swap is FORCED and CORRECT (FLAG-1, verified: View.cpp:557/575/602
> take only `that`; QueryViewImpl at Query.h:262 has no QueryImpl member, so `impl->`
> is out of scope at the choke point). But it is a first-class mechanism substitution
> against a top-of-stack "unchanged" baseline, NOT a no-op refinement — the OWNER MUST
> RATIFY it (same posture as OD-2's §18(B)(4) substitution), it may not ship under
> "unchanged." The architecture (choke-point ride, group_ids coverage, three-case
> transfer, census) is preserved intact; only the association carrier changes.

**Association field on `QueryViewImpl`** (`lib/DataFlow/Query.h`, beside
group_ids :458):

```cpp
// Index into QueryImpl::guard_annotations, or ~0u for "not a guard view".
// Migrates through CopyDifferentialAndGroupIdsTo EXACTLY like group_ids do.
unsigned guard_annotation_index{~0u};
```

> **DESIGN CORRECTION (flagged in §5, FLAG-1).** d1-design-consolidated §A.1.2
> puts the association in `std::unordered_map<QueryViewImpl*, unsigned>
> guard_annotation_of` on QueryImpl and §A.1.3 writes
> `impl->TransferGuardAnnotation(this, that)` INSIDE CopyDifferentialAndGroupIdsTo.
> **That is not implementable at the choke point as written:** `QueryViewImpl`
> has NO QueryImpl back-pointer and `CopyDifferentialAndGroupIdsTo` /
> `SubstituteAllUsesWith` / `ReplaceAllUsesWith` take no QueryImpl argument
> (verified: View.cpp:557/575/602 signatures; no `query` member on QueryViewImpl).
> To reach an `impl`-side map from the choke point you would have to thread
> QueryImpl* through all three signatures + all 12 hand sites + both RAUW
> spellings — invasive and needless. **Resolution:** carry the association as a
> per-view field (`guard_annotation_index`), exactly the group_ids precedent
> (group_ids is a view member migrated by the same function with no QueryImpl).
> The transfer is then view-local (§2.5). A `guard_annotation_of` lookup map, if
> still wanted, is DERIVED from the fields by a DefList walk at the end of the
> pass and is lookup-only — but it is redundant (the per-view field already gives
> O(1) view→annotation) and I recommend NOT materializing it, which also removes
> HP-9's only "map iteration" hazard by construction.

### §2.4 The stamp insertions (Demand.cpp STEP 7 :959 / STEP 8 :1001)

> **[ADJ:crit-correctness-4] REQUIRED PRECONDITION.** Both stamps and the
> registry push read a `forcing_index` local, but the real pass does not emplace
> the DemandForcing until STEP 10 (`Demand.cpp:1051`, `demand_forcings.emplace_back(
> std::move(forcing))`), AFTER the STEP-7 loop and STEP-8. Establish the symbol and
> pin the "exactly one forcing" precondition explicitly: immediately before STEP 7
> introduce `const unsigned forcing_index = demand_forcings.size();` (== 0 in this
> single-adornment slice — the pass mints exactly one forcing; enforced by the
> `bound_queries.size()>1` reject at `Demand.cpp:412-416` and the single-shot
> re-entry guard at `:376`). This grounds the census identity
> `recognized_subgraphs.size() == DemandForcings().size()`.

> **[ADJ:crit-pins-3] OWNER-RATIFIABLE KIND CHOICE.** STEP 8 has NO GuardSite
> (§1.2), so no classifier assigns it a Kind; the `kReadAtTuple` below is a DESIGN
> INVENTION, and pinned prediction P-D1a.2's "incl. kReadAtTuple sites" holds ONLY
> because of it (on demand_tc_witness the sole kReadAtTuple stamp is this
> query-projection guard; the two real body sites are kBaseAtom + kPushDown — §5).
> The choice is shape-correct (q_consumer reads q_read directly — the direct-read
> shape MintGuardJoin's kReadAtTuple names) and is the recommended value, but it is
> the OWNER'S to ratify, not a pinned fact. (This is distinct from FLAG-2, the
> separate consolidated §0.1 erratum → ledger.)

STEP 7 (inside the `for (const GuardSite &site : sites)` loop, after the JOIN is
minted at :959, before/after the RewireConsumer):

```
guard->guard_annotation_index = guard_annotations.size();
guard_annotations.push_back(GuardAnnotation{
    .kind          = static_cast<GuardAnnotation::Kind>(site.kind),  // §1.2 kind
    .demand_side   = kDReader,                                       // STEP-7 side
    .role          = kBody,
    .is_instance_key = false,                                        // D3 marker only
    .instance_key  = site.pivot_pos,                                 // pivots in read
    .guarded_read  = QueryView(site.read),
    .demanded_view = QueryView(d_reader),
    .forcing_index = forcing_index });        // this pass mints exactly one forcing
```

STEP 8 (inside the query-projection block, after the JOIN at :1001):

```
guard->guard_annotation_index = guard_annotations.size();
guard_annotations.push_back(GuardAnnotation{
    .kind          = GuardAnnotation::kReadAtTuple,   // q_consumer reads q_read
                                                      //   directly (the direct-read
                                                      //   shape); no GuardSite exists
    .demand_side   = kRawSeed,                        // STEP-8 side (pre-CSE!)
    .role          = kQueryProjection,
    .is_instance_key = false,
    .instance_key  = p_bound,
    .guarded_read  = QueryView(q_read),
    .demanded_view = QueryView(raw_seed),
    .forcing_index = forcing_index });
```

Populate the registry once, after STEP 8, before STEP 10's return (recognition
unit = the forcing, X-9):

```
recognized_subgraphs.push_back(RecognizedSubgraph{
    .forcing_index = forcing_index,
    .demanded_view = QueryView(p_merge),
    .key_cols      = p_bound,
    .pub_view      = QueryView(q_insert /*the answer INSERT target*/),
    .guard_annotation_indices = <the indices just appended for this forcing> });
```

The stamp is keyed on the **guard JOIN** (`guard->guard_annotation_index`), NOT
on the demand-side child. Rationale: the guarded read is SHARED (in tc, the path
reader feeds both the recursive-body guard and the query-projection guard — see
nbhd.df/tc.df) so it cannot be the key; the demand-side child folds under CSE
(§4) so it cannot be the key; the guard JOIN is unique and CSE-stable.

### §2.5 `TransferGuardAnnotation` — view-local, inside CopyDifferentialAndGroupIdsTo

Insert at the tail of `CopyDifferentialAndGroupIdsTo` (View.cpp, after :570).
`this` = loser (being replaced), `that` = survivor — same polarity as group_ids:

**SHIPPABLE D1.a FORM (field-only, QueryImpl-free — this is the one an implementer
copies):**

```cpp
// --- guard-annotation transfer (rides the group_ids choke point) ---
// this = loser (being replaced), that = survivor.  [ADJ:crit-pins-2]
if (this->guard_annotation_index != ~0u) {
  if (that->guard_annotation_index == ~0u) {
    that->guard_annotation_index = this->guard_annotation_index;   // MIGRATE
    this->guard_annotation_index = ~0u;   // [ADJ:crit-correctness-1] CLEAR-ON-MOVE:
        // unlike group_ids (a monotone set-union, harmlessly duplicated,
        // View.cpp:560-562), the index is a UNIQUE scalar the census counts once.
        // A surviving-`this` funnel (SubstituteAllUsesWith keeps `this` valid —
        // View.cpp:573-574,588; reachable via Join.cpp:424 dup-output self-canon,
        // and via every ...->ReplaceAllUsesWith where PrepareToDelete leaves the
        // dead loser in the DefList, View.cpp:457-464) would otherwise leave ONE
        // index live on TWO views → census double-count / mis-attributed downstream
        // read. Clearing the loser restores the unique view→annotation invariant.
  } else {
    // BOTH annotated: two guards folded into one survivor. DORMANT in the D1/D2
    // slice (guard JOINs are structurally distinct + CSE-stable, §4). Ship the
    // field-only compatibility check — NO QueryImpl deref (none is in scope here,
    // FLAG-1). A genuine two-distinct-guard fold trips it LOUDLY today; the rich
    // record-comparing diagnostic is a D3 deliverable (see below).
    assert(this->guard_annotation_index == that->guard_annotation_index);
    ++query_folded_count_ref;   // see note — the ONLY QueryImpl-side write; wire it
                                // as described below, or defer the counter to D3.
    this->guard_annotation_index = ~0u;   // clear-on-move here too
  }
}
```

Three-case semantics (§A.1.3): loser unannotated → no-op (the field guard
`!= ~0u` handles it); survivor unannotated → migrate + clear loser; both annotated
→ field-equality assert + fold-count + clear loser.

> **[ADJ:crit-pins-2] The primary block above is the COMPILABLE D1.a form.** The
> earlier draft's both-annotated branch dereferenced
> `query->guard_annotations[...]` and `++query->guard_annotation_folded_count`
> INSIDE `CopyDifferentialAndGroupIdsTo`, which FLAG-1 proves has no QueryImpl in
> scope — that block does not compile at the choke point and must not be the copied
> sample. Two ways to satisfy the fold-count / rich-diagnostic need:
> (1) **Defer to D3 (recommended for D1.a):** the branch is provably dormant now,
> so ship the field-only `assert` with NO record deref and NO counter at all,
> and land `guard_annotation_folded_count` + the record-comparing incompatible-fold
> abort at D3 when the branch first goes live (multi-guard folds). Under this
> option the census equation (§2.6) reads `folded_count == 0` as a literal, or
> drops the term entirely for D1.a.
> (2) **Thread QueryImpl minimally:** give `CopyDifferentialAndGroupIdsTo` (and its
> two callers) a `QueryImpl *query` parameter, OR set a private `QueryImpl *query`
> back-pointer on QueryViewImpl once at graph build. Then the counter write and the
> rich diagnostic are available. This is the invasive route FLAG-1 argues against;
> take it only if the owner wants the fold-count live at D1.a.
> Either way D1.a stays inert. `query_folded_count_ref` in the sample marks the sole
> QueryImpl-touching line — delete it (option 1) or bind it (option 2).

### §2.6 Census (§A.1.4) — order-free counts; the severity live NOW

Run once at the end of ApplyDemandTransform — **NOT as a post-Optimize validator**
([ADJ:crit-correctness-2], see banner) — driven by a **DefList walk** (never the
map), filtering dead views:

```
n_stamped = 0
for view in <DefList / det_seq deterministic view walk>:
  if view.is_dead: continue                              # [ADJ:crit-correctness-2]
  if view.guard_annotation_index != ~0u: ++n_stamped     # lookup on the walked view
assert  n_stamped + guard_annotation_folded_count == guard_annotations.size()   # no orphans
assert  recognized_subgraphs.size() == DemandForcings().size()
```

> **[ADJ:crit-correctness-2] CENSUS IS PRE-OPTIMIZE-ONLY AT D1.a.** The equation is
> sound ONLY on the freshly-stamped graph, at the end of ApplyDemandTransform
> (`Build.cpp:2587`), STRICTLY BEFORE `impl->Optimize` (`Build.cpp:2599`). Between
> the stamps and this census NO funnel call and NO `PrepareToDelete` runs (the pass
> rewires via targeted `RewireConsumer` only, Demand.cpp:220-279), so `n_stamped=3,
> folded=0, size=3`. The doc's earlier "(or a debug validator)" post-Optimize
> variant is WITHDRAWN for D1.a: `PrepareToDelete` sets `is_dead` but leaves both the
> field set and the view in the DefList (View.cpp:457-464), and dead-flow elimination
> deletes annotated views OUTRIGHT with no transfer and no orphan bucket
> (DeadFlowElimination.cpp:273/276, Join.cpp:371, Select.cpp:196) — so a post-Optimize
> walk would either double-count (unfiltered) or strand (filtered) an annotation and
> spuriously abort/silently lose on a CORRECT graph. The `is_dead` filter above +
> clear-on-move (§2.5) make the census robust to a migrate fold, but the OUTRIGHT-
> deletion orphan term has no home until a dead-view sweep clears the field; that is
> a D1.b/D2 concern (the field is not yet consumed post-Optimize at D1.a). Pin: the
> census runs exactly once, pre-Optimize; no debug-validator re-run post-Optimize
> until the field is made dead-view-safe.

**SEVERITY SPLIT — what is live at D1.a.** §A.1.4 splits severity on whether a
reader consumes the annotation: `fprintf+abort` under `-demand-instance` (a lost
annotation = a silently-skipped instance), `debug-only assert` under flat
`-demand`. **At D1.a the `-demand-instance` knob DOES NOT EXIST** (it lands at
D2.b, consolidated §B). So the ONLY live severity NOW is the **flat `-demand`
debug-only assert** (`assert(...)`, NDEBUG-compiled-out); there is no abort arm
yet. The `-demand-instance` fprintf+abort arm is authored at D2.b when the knob
and the consuming reader arrive. D1.a ships the debug assert + the counting
scaffold so the numbers are checkable on `demand_tc_witness` today (P-D1a.2).

### §2.7 Public accessors (mirror `DemandForcings()`, Query.h:995)

Declared in `include/drlojekyll/DataFlow/Query.h` beside :995, defined in
`lib/DataFlow/Query.cpp` (or Demand.cpp beside :283), same const-ref noexcept
shape:

```cpp
const std::vector<GuardAnnotation> &GuardAnnotations(void) const noexcept;      // -> impl->guard_annotations
const std::vector<RecognizedSubgraph> &RecognizedSubgraphs(void) const noexcept;// -> impl->recognized_subgraphs
```

Both empty in every non-`-demand` build (populated only inside the pass).
`RecognizedSubgraphs()` is the census-recount source D1.b consumes (independent
of the DR mint loop — the Aggregates() independence pattern, §A.1.5).

======================================================================
## §3. HP-9 COMPLIANCE ((F)-law walk-order pin)
======================================================================

HP-9: BOTH the ApplyDemandTransform stamp loop AND every emission/dump-time
consumption must be driven by the DefList/det_seq walk order — NEVER by
`guard_annotations` vector order, NEVER by unordered_map iteration.

1. **Stamp-loop order source = `sites`, itself a DefList walk.** The stamp loop
   is `for (const GuardSite &site : sites)` at `Demand.cpp:957`. `sites` is
   built in STEP 3 by iterating `p_merge->merged_views` (`Demand.cpp:564`,
   `sites.push_back` at :734) — `merged_views` is a `UseList` with deterministic
   insertion order, NOT a hash container. The single query-projection stamp
   (STEP 8) is always appended last, after the loop. So the append order of
   `guard_annotations` is a pure function of the deterministic merged_views walk
   + the fixed STEP-7-then-STEP-8 sequence. No pointer ordering enters.

2. **No sort / no range-for over the metadata.** `guard_annotations` is appended
   in walk order and NEVER re-sorted (unlike `group_ids`, which IS sorted
   :562 — the annotation vector must not copy that). `guarded_read` /
   `demanded_view` are opaque `QueryView` handles whose `UniqueId()` is a pointer
   (Node.h:31-33) — they are used for equality/lookup ONLY and NEVER sorted or
   iterated into any ordered/emission-visible consumption.

3. **`guard_annotation_of` is lookup-only — and here, absent.** Per §2.3 the
   association is a per-view field (`guard_annotation_index`), and I recommend
   NOT materializing the map at all. This removes HP-9's only "map iteration"
   hazard *by construction*: there is no map to accidentally range-for. Any
   ordered consumption (census §2.6; the D1.b recount; the D2.b spine builder)
   walks the DefList/det_seq view order and READS `view.guard_annotation_index`
   / `RecognizedSubgraphs()` (append = forcing order) per view — the order always
   comes from the walk, never from the container.

4. **D1.a checklist re-grep (HP-9 review item):** `grep -rn 'guard_annotation'`
   over lib/ must show ZERO `std::sort(...guard_annotation...)` and ZERO
   `for (... : ...guard_annotation...)` at any emission-or-dump site; the only
   iteration is the append at the stamp sites and (if a map is ever added) a
   lookup-only `.find`. The stamp loop's `merged_views`-derived order is an
   explicit review line (d1-pinned §3).

======================================================================
## §4. DEMAND_SIDE AT STAMP TIME (GT-3) — why demand_side is recorded pre-CSE
======================================================================

**Ordering (Build.cpp).** `ApplyDemandTransform` runs at `Build.cpp:2587`;
`impl->Optimize` (Simplify → Canonicalize fixpoint → **CSE** → dead-flow) runs
at `Build.cpp:2599`, gated by `AnyBodyOptionalEnabled(kDataFlow)` (:2598). So
**both stamps are written strictly BEFORE CSE runs.** At stamp time `d_reader`
(STEP 6, :944) and `raw_seed` (STEP 8, :990) are two DISTINCT freshly-minted
TUPLEs, and each stamp records its `demand_side` literally (kDReader at :959,
kRawSeed at :1001).

**The fold (empirically confirmed at tip — nbhd.df).** On
`demand_neighborhood_witness` the d_p relation has ONLY the root member (no
recursive propagation member), so `d_reader` (TUPLE over d_merge) and `raw_seed`
(TUPLE over recv) become structurally equal and **CSE folds raw_seed into
d_reader**. The optimized dump shows it directly: BOTH guard JOINs share the
same demand-side child —

```
tuple ^tuple.4 (c8:u64)   [%table:8, the demand reader]
  => ^join.6 .in0(c8)     ; body guard  (kBaseAtom)
  => ^join.7 .in0(c8)     ; query-projection guard   <- was raw_seed, now folded
```

(nbhd.df:19-22,28-40.) After CSE you can no longer tell join.6's demand side
from join.7's by inspecting the child — both point at tuple.4. **This is exactly
why demand_side must be a recorded FIELD (pre-CSE), not something recovered from
the graph later.** GT-3 verified.

**Which transfer case fires when CSE folds raw_seed.** The CSE merge does
`raw_seed.SubstituteAllUsesWith(d_reader)` → `CopyDifferentialAndGroupIdsTo`
(View.cpp:588) → `TransferGuardAnnotation(loser=raw_seed, survivor=d_reader)`.
Because the annotation is keyed on the **guard JOIN** (§2.4), NOT on the
demand-side TUPLE, `raw_seed->guard_annotation_index == ~0u` — so the
**loser-unannotated NO-OP case fires** (§2.5). The two guard JOINs (join.6,
join.7) are structurally distinct (different pivots + different in1 reads) and
CSE-STABLE — they do NOT fold — so both annotations survive intact on their own
JOINs, each carrying its recorded `demand_side` (kDReader / kRawSeed). This is
precisely the pinned P-D1a.3 wording "both survive distinct (GT-3 folds only the
demand TUPLE)". Zero orphans.

> Corollary: on the RECURSIVE witness `demand_tc_witness` the fold does NOT
> happen — d_reader's d_merge has propagation members, so raw_seed stays a
> distinct node (tc.df: join.16.in0 = tuple.8 %table:23 = raw_seed, distinct from
> the d_reader tuple.6 %table:12). Recording demand_side at stamp time is correct
> in both regimes; the field is the single source of truth regardless of whether
> the child folded.

======================================================================
## §5. PRE-DERIVED PREDICTIONS (P-D1a.1..4) + code-derived counts
======================================================================

**P-D1a.1 (flag-off).** `demand_mode==false` returns at Demand.cpp:370 before
minting anything → `guard_annotations` / `recognized_subgraphs` EMPTY; every
view's `guard_annotation_index == ~0u`. [G2] 676-row corpus byte-identical vs
frozen e6264b54; suite 169 [G1]. HOLDS — the stamp is unreachable flag-off and
the new field's default is inert.

**P-D1a.2 (demand_tc_witness, LIVE — code-derived at tip from tc.df).** Bound
query `reachable_from(bound From, free To) : path(From,To)`; p = path; p_bound =
[0]. path's MERGE has TWO rule bodies → STEP 3 produces 2 GuardSites; STEP 8 adds
the query-projection guard. **3 stamps total:**

| stamp   | guard JOIN (tc.df) | kind        | role            | demand_side |
|---------|--------------------|-------------|-----------------|-------------|
| body 1  | `join.14` (edge_2 read ⋈ d_reader on F) | **kBaseAtom**   | kBody           | kDReader |
| body 2  | `join.15` (path read ⋈ d_reader on From) | **kPushDown**   | kBody           | kDReader |
| q-proj  | `join.16` (path read ⋈ raw_seed on From) | **kReadAtTuple**| kQueryProjection| kRawSeed |

Derivation: base rule `path : edge_2(F,T)` — F traces to the edge_2 message-atom
read → `kBaseAtom` (Demand.cpp:659); recursive rule `path : path(F,M),edge_2(M,T)`
— F emerges from the body JOIN with a full-width path read at pos 0 → `kPushDown`
(Demand.cpp:704); the query projection reads q_read (path reader) directly →
kReadAtTuple SHAPE, stamped kReadAtTuple by §2.4. The witness's OWN committed
comment agrees ("the base body at its bound-column source atom; the recursive
body by the push-down edge_2 JOIN"). `|RecognizedSubgraphs()| ==
|DemandForcings()| == 1`; zero orphans; the census (debug-severity flat, §2.6) is
green. **kind multiset = {kBaseAtom:1, kPushDown:1, kReadAtTuple:1}.**

> **FLAG-2 (LOUD).** d1-design-consolidated §0.1 F6-annot says demand_tc_witness
> "carries **kReadAtTuple guard sites**" and glosses it as coming from the
> "recursive body under -demand". **That attribution is wrong.** Empirically
> (tc.df) and per the classifier + the witness's own comment: the recursive body
> guard is **kPushDown**, the base body guard is **kBaseAtom**, and the ONLY
> kReadAtTuple stamp is the **query-projection** guard (join.16) — which is not a
> body site and is not even GuardSite-classified (§1.2/§2.4). The prediction
> P-D1a.2's literal text ("incl. kReadAtTuple sites") SURVIVES, but ONLY because
> the query-projection stamp is designed kReadAtTuple; any reading that expects a
> kReadAtTuple *body* site on this witness is false. This is why §2.4 pins the
> step-8 kind explicitly — the prediction's coverage claim rests on it.

**P-D1a.3 (demand_neighborhood_witness, probe-compile — code-derived at tip from
nbhd.df).** Bound query `neighborhood(bound Start, free Node) : edge(Start,Node)`;
p = edge; single base rule `edge : add_edge`. **2 stamps:**

| stamp  | guard JOIN (nbhd.df) | kind        | role             | demand_side |
|--------|----------------------|-------------|------------------|-------------|
| body   | `join.6` (add_edge read ⋈ demand on From) | **kBaseAtom**   | kBody            | kDReader |
| q-proj | `join.7` (edge read ⋈ demand on Start)    | **kReadAtTuple**| kQueryProjection | kRawSeed |

VERIFICATION of the pinned "join.6 kBaseAtom body" against E-46: the body `edge :
add_edge` is a BASE rule whose bound value's source is the add_edge message-atom
read → `AtomReceiveReadOf` true → **kBaseAtom** (Demand.cpp:659). It is NOT
kReadAtTuple (no recursive p read) and NOT kPushDown (no body JOIN carrying it).
**The pinned P-D1a.3 kinds are CORRECT — no flag.** Both stamps survive distinct
even though GT-3 folds raw_seed into d_reader (§4): the fold hits the demand
TUPLE, not the annotation-bearing guard JOINs. 1 recognized subgraph, key
{Start}, zero orphans. (`demand_neighborhood_witness` is NOT in the suite at
D1.a — it lands at D2.c — so this prediction is a probe-compile, not a gate.)

**P-D1a.4 (dump/determinism/bench).** [G3] zero-churn EXACT — D1.a touches no
dump emitter (the census counters + kInstance* rows are D1.b), so no golden byte
moves. [G5] 1-hash (the stamp order is merged_views-deterministic, §3, and the
new field is (F)-neutral). [G4] Q5 ABABAB neutral (DataFlow-only, no emission).
HOLDS.

======================================================================
## §6. GATES + D1.a CHECKLIST (d1-pinned §3; §19(J) ASAN cadence)
======================================================================

Standing-gate vocabulary (consolidated §B): [G1] SUITE PASS (169) all sidecar
arms; [G2] 676-row corpus A/B byte-identical vs frozen e6264b54 (knob-off);
[G3] irgold byte-compare both carriers; [G4] Q5 ABABAB bench neutral; [G5] 3-run
determinism + debug==release on all four demand-ON surfaces; [G6] E-62 tripwire
re-grep (DeltaRel-touching only); [G7] data/ clean + ctest 3/3.

D1.a-applicable rows (consolidated §B "Checklist: [G1][G2][G3][G4][G5][G7]" +
d1-pinned §3):

- [ ] **[G1]** `DR=build/debug/bin/drlojekyll runall.sh <wr>` → SUITE: PASS (169).
- [ ] **[G2]** 676-row corpus A/B byte-identical vs frozen e6264b54, knob-off
      (the stamp is demand-mode-gated; flag-off is P-D1a.1).
- [ ] **[G3]** irgold byte-compare: ZERO churn on both carriers (D1.a touches no
      dump emitter — P-D1a.4). demand_tc_witness.deltarel golden UNTOUCHED (the
      +3 census counters are D1.b, not here).
- [ ] **[G4]** Q5 ABABAB neutral.
- [ ] **[G5]** 3-run determinism (1 hash) + debug==release on demand_tc_witness's
      four surfaces (h/ir/df/deltarel), demand-ON — the permanent (F) gate.
- [ ] **[G7]** data/ examples compile in all 4 modes; ctest 3/3.
- [ ] **G6 N/A** — D1.a does NOT touch lib/DeltaRel (no E-62 re-grep required;
      the field lives entirely in DataFlow).
- [ ] **PassPolicy untouched** — the stamp is demand-mode-gated, NOT a pass
      (consolidated §B "PassPolicy untouched"; the P1 "demand is semantics" pin).
- [ ] **No Runtime edit** — DataFlow-only (InstanceStore is D2.a).
- [ ] **HP-9 re-grep** (d1-pinned §3): `grep -rn guard_annotation lib` shows NO
      `std::sort` over and NO emission/dump-site `for (... : ...)` over
      `guard_annotations` / `guard_annotation_of`; the stamp loop's order is the
      `sites`/`merged_views` DefList walk (explicit review item).
- [ ] **P-D1a.2 live check**: on demand_tc_witness, `GuardAnnotations().size()==3`
      with kinds {kBaseAtom, kPushDown, kReadAtTuple}, `RecognizedSubgraphs().size()
      == DemandForcings().size() == 1`, census green (add a throwaway debug print
      or DrTest read behind the debug assert; delete before commit — no env-gated
      scaffolding).

**ASAN sweep (§19(J) cadence, STANDING owner directive):** run BOTH surfaces per
diff (~4.5 min combined) BEFORE D1.a lands and after:
- Surface 1: `build/asan` (Debug + `-fsanitize=address -fno-omit-frame-pointer`,
  tests ON) → `ctest 3/3` under ASAN + full suite `DR=build/asan/bin/drlojekyll
  runall.sh` → SUITE: PASS (169).
- Surface 2 (emission/Runtime diffs; D1.a is DataFlow-only so surface 2 is
  optional but cheap): tip debug compiler + env CXX wrapper so generated code +
  drivers + Runtime are ASAN-compiled → SUITE: PASS (169).
ASAN runs are never timed as benchmarks; nothing enters FINDINGS.md absent a find.

**Per-diff design ritual (still gates the landing, §19(I)):** this pseudocode →
diff-on-pseudocode (this doc) → critique → desired IR states vs real dumps →
implement → Fable review → owner brief.

======================================================================
## FLAGS (pinned/consolidated claims I believe are wrong — see return payload)
======================================================================
- **FLAG-1** (§2.3/§2.5): d1-design-consolidated §A.1.2/§A.1.3's
  `guard_annotation_of` map + `impl->TransferGuardAnnotation(this,that)` INSIDE
  CopyDifferentialAndGroupIdsTo is NOT implementable as written — QueryViewImpl
  has no QueryImpl back-pointer and the function takes no QueryImpl arg (View.cpp:
  557/575/602). Fix: per-view field `guard_annotation_index` (the group_ids
  precedent); the map, if kept, must be lookup-only and derived. Architecture
  holds; mechanism corrected.
- **FLAG-2** (§5, P-D1a.2): d1-design-consolidated §0.1 F6-annot mis-attributes
  demand_tc_witness's kReadAtTuple to the "recursive body". Empirically (tc.df) +
  classifier + the witness's own comment: recursive body = kPushDown, base body =
  kBaseAtom; the sole kReadAtTuple is the query-projection stamp. P-D1a.2's
  literal text holds only because §2.4 pins the step-8 kind to kReadAtTuple.
- **NOT-A-FLAG** (verified per task): pinned P-D1a.3 "join.6 kBaseAtom body,
  join.7 query-projection" is CORRECT against E-46 and nbhd.df.

======================================================================
## ADJUDICATION LEDGER (XHIGH adjudicator, tip 0475306d — BINDING)
======================================================================
Every critic finding + designer flag re-verified AT THE CODE by the adjudicator.
Amendments folded above are marked `[ADJ:<id>]` at their insertion points.

**crit-correctness-1 [MED] — ACCEPT.** Copy-not-move: §2.5 migrated the scalar
index without clearing the loser. VERIFIED: group_ids is a monotone union
(View.cpp:560-562) so the "EXACTLY like group_ids" analogy fails for a unique
scalar; `SubstituteAllUsesWith` keeps `this` valid (View.cpp:573-574,588) and
Join.cpp:424 is a live surviving-`this` funnel. Fix folded: CLEAR-ON-MOVE
(`this->guard_annotation_index = ~0u`) in both transfer branches (§2.5). Inert at
D1.a but the mechanism is consumed by D1.b/D2, so it must land correct.

**crit-correctness-2 [MED] — ACCEPT.** Census correct only pre-Optimize.
VERIFIED: PrepareToDelete leaves the field set and the view in the DefList
(View.cpp:457-464); dead-flow elimination deletes outright with no orphan bucket
(DeadFlowElimination.cpp:273/276, Join.cpp:371, Select.cpp:196); census placed at
Build.cpp:2587 before Optimize :2599 is what saves shipped D1.a. Fix folded: add
`is_dead` filter to the walk, WITHDRAW the post-Optimize "debug validator"
variant for D1.a, pin census as pre-Optimize-once (§2.6 banner). Outright-deletion
orphan term deferred to D1.b/D2 (field not consumed post-Optimize at D1.a).

**crit-correctness-3 [LOW] — ACCEPT-AMENDED (merged with crit-pins-4).**
static_cast across two uncoupled enums. VERIFIED both are `{kReadAtTuple,kPushDown,
kBaseAtom}=0,1,2` today (Demand.cpp:127-132 vs §2.1). Fix folded: `static_assert`
coupling the three value-pairs beside the cast (§2.1 banner).

**crit-correctness-4 [LOW] — ACCEPT-AMENDED.** `forcing_index` local never
established before the STEP-10 emplace. VERIFIED: sole `demand_forcings.emplace_back`
at Demand.cpp:1051, after both stamps. Fix folded: introduce
`const unsigned forcing_index = demand_forcings.size();` before STEP 7 + pin the
one-forcing precondition (Demand.cpp:376/412-416) (§2.4 banner).

**crit-pins-1 [MED] — ACCEPT (owner-ratifiable).** Per-view field replaces the
pinned §A.1.2 map + §A.1.3 `impl->TransferGuardAnnotation`; d1-pinned §3 folds this
under "D1.a — unchanged." VERIFIED the pinned mechanism is un-implementable
(View.cpp:557/575/602 take only `that`; QueryViewImpl at Query.h:262 has no
QueryImpl member — confirmed by the adjudicator). The swap is forced and correct
(FLAG-1) but is a first-class mechanism substitution (OD-2 shape) — recorded as an
OWNER-RATIFIABLE swap (§2.3 banner); it may NOT ship under "unchanged."

**crit-pins-2 [MED] — ACCEPT-AMENDED.** Primary §2.5 code block dereferenced
`query->` inside CopyDifferentialAndGroupIdsTo, contradicting its own FLAG-1.
VERIFIED. Fix folded: the SHIPPABLE compilable field-only form is now the primary
block; the record-dereferencing fold-count/diagnostic is explicitly a D3
deliverable with a minimal QueryImpl-threading fallback documented (§2.5).

**crit-pins-3 [MED] — ACCEPT (owner-ratifiable).** STEP-8 kind=kReadAtTuple is a
design invention (no GuardSite → no classifier) that pinned P-D1a.2 depends on.
VERIFIED: on demand_tc_witness the sole kReadAtTuple is the query-projection stamp;
body sites are kBaseAtom + kPushDown (classifier + witness comment + tc.df).
Recorded as an OWNER-RATIFIABLE kind choice (§2.4 banner); kReadAtTuple endorsed as
shape-correct. FLAG-2 (consolidated §0.1 mis-attribution) confirmed → ledger erratum.

**crit-pins-4 [LOW] — ACCEPT-AMENDED (duplicate of crit-correctness-3).** Same
uncoupled-enum defect; closed by the same static_assert (§2.1).

**crit-pins-5 [LOW] — ACCEPT-AMENDED.** `guard_annotation_folded_count` consumed
but undeclared. VERIFIED. Fix folded: declared in the §2.3 storage block
(`unsigned guard_annotation_folded_count{0u};`). Note: under the recommended
crit-pins-2 option (1) the counter is deferred to D3 and the census term reads 0.

**crit-predictions.md — NO FINDING, CONCUR.** The adjudicator's own spot-check of
tc.df/nbhd.df + the classifier (Demand.cpp:650/659/704) reproduces P-D1a.1/.2/.3
exactly (3 stamps {kBaseAtom,kPushDown,kReadAtTuple} / 1 forcing on tc; 2 stamps /
1 forcing on nbhd). Predictions stand.

**Designer FLAG-1 — UPHELD.** The map + `impl->TransferGuardAnnotation` is
un-implementable at the choke point (verified). Resolution: per-view field is the
correct fix AND is recorded as an owner-ratifiable mechanism swap (crit-pins-1).

**Designer FLAG-2 — UPHELD (ledger erratum).** consolidated §0.1 F6-annot
mis-attributes demand_tc_witness's kReadAtTuple to the "recursive body"; it is the
query-projection stamp (recursive body = kPushDown). Route to the KeyedInstances
ledger as an erratum. P-D1a.2's literal text survives only via the §2.4 step-8 kind.

======================================================================
## FINAL VERDICT: GO-WITH-AMENDMENTS
======================================================================
The architecture is INERT as the D1.a pin requires (DataFlow-only; no
DR-IR/emission/Runtime/PassPolicy; predictions P-D1a.1..4 reproduce from code) and
is faithful to the big pins (§A.1.1 fields verbatim, X-9, is_instance_key=false,
HP-9 walk-order strengthened, E-46/E-52). It correctly catches two lower-stack
errata (FLAG-1, FLAG-2). The eight folded amendments make the pinned MECHANISM
(consumed by D1.b/D2) sound; NONE alter shipped-D1.a behavior. TWO items require
OWNER SIGN-OFF before landing: (a) the per-view-field mechanism swap [crit-pins-1],
and (b) the invented STEP-8 kReadAtTuple kind [crit-pins-3]. With the amendments
folded and those two ratified, this doc is the binding D1.a implementation contract.
