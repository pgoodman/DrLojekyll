# D3.a.3 STAGE (b) — DESIGN LANE b2: g1 THE FOLD-ARM SURVIVOR POLICY + PREDICATE JUSTIFICATION

Repo `/Users/pag/Code/DrLojekyll`, branch `keyed-instances`, tip b65e7668 (docs-only
atop D3.a.2 landing bfc068d1; **code bytes == the landing**). Repo READ-ONLY; every
anchor below re-read at code this session. Binding inputs: `d3a3-substrate.md` §3
(g1 verdict — arm DORMANT, install the survivor policy, KEEP the predicate), §6
(O1 per-pub, not re-litigated), §7-g1, §9 anchors 7/9.

SCOPE: `lib/DataFlow/View.cpp` (fold arm `CopyDifferentialAndGroupIdsTo` :624-683 +
`GuardAnnotationsCompatible` :584-588) + `lib/DataFlow/Query.h` (the `GuardAnnotation`
struct + pure-fn decls) + `lib/DataFlow/Demand.cpp` (the g8 census belt) + the two
directed witnesses (DataFlowValidators, riding the existing `GuardAnnotationFoldTest`
mold + the b3/g2 design-1 harness).

===============================================================================
## §0 CODE-VERIFICATION OF THE SUBSTRATE PREMISES (deliverable 1 asks)

All three verify-at-code asks CONFIRMED verbatim:

**(V1) `GuardAnnotation` field names** — `include/drlojekyll/DataFlow/Query.h:988-1019`.
The struct in declaration order:
```
enum Kind : uint8_t { kReadAtTuple, kPushDown, kBaseAtom };   // :993
enum DemandSide : uint8_t { kDReader, kRawSeed };             // :996
enum Role : uint8_t { kBody, kQueryProjection };              // :998
Kind kind;                          // :1000
DemandSide demand_side;             // :1001
Role role;                          // :1002
bool is_instance_key;               // :1006  (always false this slice)
std::vector<unsigned> instance_key; // :1010  (pivot positions, adornment order)
QueryView guarded_read;             // :1014  (opaque handle, eq/lookup only)
QueryView demanded_view;            // :1015  (opaque handle)
unsigned forcing_index;             // :1018  (index into Query::DemandForcings())
```
The §3.3 pseudocode's field names (`role`/`demand_side`/`kind`/`guarded_read`/
`instance_key`/`forcing_index`) all match. NOTE the pseudocode's `surv.role = kBody`
uses the `GuardAnnotation::Role` enum (`kBody`/`kQueryProjection`), not a free name.

**(V2) The survivor is `that`, chosen by CSE det-order NOT role.** In
`CopyDifferentialAndGroupIdsTo` the header comment (`View.cpp:641-642`) states
`"this = loser (being replaced), that = survivor"`; the fold arm fetches the survivor
record with the SURVIVOR's index `that->guard_annotation_index` (`View.cpp:676`,
`query->guard_annotations[that->guard_annotation_index]`). Which of two Equals views
becomes `that` is decided by CSE's replacement sort at `Optimize.cpp:355-378`: key =
`min(Depth)` then `det_seq` (a deterministic pointer-order-independent tie-break),
verbatim:
```
const auto a_depth = std::min(a_v1->Depth(), a_v2->Depth());   // :365
...
if (a_v1->det_seq != b_v1->det_seq) return ...                 // :375
```
**Role is NOT consulted.** ⇒ a `kQueryProjection` guard can legally become the survivor
of a `kBody` loser. THIS is the latent cliff the policy repairs.

**(V3) Input derived ONLY from `role == kBody`** — `Rel.cpp:976-982`:
```
if (annots[ai].role == GuardAnnotation::kBody && !ri.input_table) {   // :976
  if (TABLE *in = model_table(jl[1])) {
    ri.input_table = in; ri.input_view = jl[1];
    ri.input_key_cols = annots[ai].instance_key; ...                  // :980
```
A survivor stamped `kQueryProjection` ⇒ `ri.input_table` stays null ⇒ `ri.ok` false
(`Rel.cpp:1016-1018`) ⇒ the instance is SILENTLY skipped at mint (`Rel.cpp:1053-1055`,
`"fully-dead forcing ... ABA-safe skip"`) ⇒ missing answer rows. CONFIRMED.

**(V3b — NEW, strengthens the justification)** `role == kBody` is read by a SECOND
downstream consumer the substrate does not name: the nested pre-pass recursive-content
fence at `Build.cpp:1528`:
```
if (annots[ai].role == GuardAnnotation::kBody) {          // :1528
  const QueryView in = jl[1];
  if (in.InductionGroupId().has_value() || ViewSelfReachable(in)) recursive_content = true;
```
A role-losing fold that dropped the last `kBody` for a forcing would make
`recursive_content` stay false ⇒ a recursive-content demanded body slips the
`-demand-instance` fence (`Build.cpp:1546-1549`). So the survivor policy protects TWO
consumers (input resolution AND the recursive-content fence), not one. Both key on the
IDENTICAL `role == kBody` test — the policy's single fixup (`surv.role = kBody`) heals
both.

**Fields NOT read post-CSE** (grep of `annots[...]` reads across `Rel.cpp` +
`Build.cpp`): only `forcing_index` (bucket key, `Rel.cpp:953` / `Build.cpp:1513`),
`role` (`Rel.cpp:976` / `Build.cpp:1528`), and `instance_key` (`Rel.cpp:980`) are ever
read after CSE. `demand_side` / `kind` / `guarded_read` / `demanded_view` are PRE-CSE
diagnostic carriers / opaque handles, read by NOTHING downstream. ⇒ **only `role` is
strictly load-bearing to fix**; adopting `demand_side`/`kind`/`guarded_read` is
defensive record-coherence (keeps the mutated record self-consistent for the OWN-3
diagnostic prints). `forcing_index` + `instance_key` are already equal by the compat
gate (see §3), so the policy touches neither.

===============================================================================
## §1 DELIVERABLE 1 — THE SURVIVOR-RECORD POLICY (concrete diff at View.cpp)

### 1.1 Factor the policy PURE (mirrors the CheckGuardAnnotationFold / RAT-3 idiom)

The substrate §3.3 inlines the policy in the fold arm. I FACTOR it into a pure free
function, exactly as `GuardAnnotationsCompatible` / `CheckGuardAnnotationFold` are
factored pure (`Query.h:1181-1189`, `"so a death test can hand-build inputs"`). This
makes the directed witness (§4-a) a hand-built-record unit with no view graph — the
same DeathHarness/`MakeAnn` mold the existing `GuardAnnotationFoldTest.cpp` already uses.

**`Query.h`, ADD beside the OWN-3 decls (after :1189):**
```cpp
// OWN-3 (g1, D3.a.3): the SURVIVOR-RECORD POLICY. When two COMPATIBLE guard
// annotations fold, ResolveLiveRecognition (Rel.cpp:976) AND the nested pre-pass
// recursive-content fence (Build.cpp:1528) derive/gate ONLY off a role==kBody
// stamp -- but CSE picks the fold survivor by depth/det_seq (Optimize.cpp:365),
// NOT by role, so a kQueryProjection survivor can shadow a kBody loser and
// silently drop the input (Rel.cpp:1053 skip) + blind the recursive-content
// fence. This forces the SURVIVING record to carry kBody whenever the loser did.
// PURE (mutates only `surv` from `loser`); GuardAnnotationsCompatible is the
// precondition (already proved forcing_index + instance_key equal). Defined in
// View.cpp; called from the CopyDifferentialAndGroupIdsTo both-set fold arm.
void PromoteSurvivorToBody(GuardAnnotation &surv, const GuardAnnotation &loser);
```

**`View.cpp`, ADD the definition (beside GuardAnnotationsCompatible, ~:588):**
```cpp
void PromoteSurvivorToBody(GuardAnnotation &surv, const GuardAnnotation &loser) {
  // Only a kBody loser carries a recoverable body input. Cases:
  //   loser=kBody, surv=kQueryProjection -> REPAIR (adopt kBody + loser stamps).
  //   surv=kBody (either loser)          -> no-op (kBody already survives).
  //   both=kQueryProjection              -> no-op (no body input to recover;
  //                                         the g8 stamp-time belt catches a
  //                                         forcing that minted ZERO kBody).
  if (loser.role == GuardAnnotation::kBody &&
      surv.role == GuardAnnotation::kQueryProjection) {
    surv.role = GuardAnnotation::kBody;  // the ONLY field read post-CSE (V3/V3b)
    // Adopt the loser's PRE-CSE site stamps so the promoted record stays
    // self-consistent for the OWN-3 diagnostic prints. These are read by NOTHING
    // downstream (defensive). instance_key + forcing_index are already equal by
    // the compat gate; demanded_view is an opaque handle, left as-is.
    surv.demand_side  = loser.demand_side;   // kDReader (the real body read side)
    surv.kind         = loser.kind;
    surv.guarded_read = loser.guarded_read;
  }
}
```

### 1.2 The call site — after the compat gate, before the loser index clears

`CheckGuardAnnotationFold` (`View.cpp:675-677`) IS the compat gate: it calls
`GuardAnnotationsCompatible` and aborts on incompatible, RETURNS on compatible. So the
policy is inserted immediately after it returns and before `++...folded_count` (:678).
The two records are already fetched inline there.

**`View.cpp`, the both-set arm (:675-679), before/after:**
```cpp
// BEFORE (:675-678):
      CheckGuardAnnotationFold(
          query->guard_annotations[guard_annotation_index],
          query->guard_annotations[that->guard_annotation_index]);
      ++query->guard_annotation_folded_count;  // the SOLE writer

// AFTER:
      CheckGuardAnnotationFold(
          query->guard_annotations[guard_annotation_index],
          query->guard_annotations[that->guard_annotation_index]);
      // SURVIVOR-RECORD POLICY (g1, D3.a.3): the compat gate above proved the
      // fold well-keyed; now force kBody to survive so ResolveLiveRecognition
      // (Rel.cpp:976) + the nested pre-pass fence (Build.cpp:1528) still see the
      // body input. loser = `this`'s record, survivor = `that`'s record.
      PromoteSurvivorToBody(
          query->guard_annotations[that->guard_annotation_index],  // surv
          query->guard_annotations[guard_annotation_index]);       // loser
      ++query->guard_annotation_folded_count;  // the SOLE writer
```

`guarded_read`/`query`/`guard_annotation_index` fetch semantics are unchanged; the two
`operator[]` lookups are already null/OOB-guarded by the INV-OWN3-Q checks at
`View.cpp:658-674` (which run BEFORE this point), so the policy adds no new deref risk.

### 1.3 The no-op cases (verified against the enum + the compat gate)

| loser.role | surv.role | policy | why |
|---|---|---|---|
| kBody | kQueryProjection | **REPAIR** (surv→kBody + stamps) | CSE made the qp guard the survivor; recover the body input |
| kQueryProjection | kBody | no-op | kBody already survives (the desired end state) |
| kBody | kBody | no-op | already kBody |
| kQueryProjection | kQueryProjection | no-op | no body input exists to recover — the g8 belt (§2) is the upstream teeth for "forcing minted zero kBody" |

`forcing_index` + `instance_key` are NEVER touched: the compat gate
(`GuardAnnotationsCompatible`, :586-587) already proved both equal, so there is nothing
to reconcile. `demanded_view` is left untouched (opaque, unread post-CSE).

===============================================================================
## §2 DELIVERABLE 2 — THE BELT (g8): stamp-time kBody-presence census

### 2.1 Site + predicate (Demand.cpp census, inside the once-per-module block)

The belt rides the existing Step-11 annotation census block (`Demand.cpp:1140-1163`),
appended AFTER the two existing count equalities (`n_stamped + folded == total` :1147;
`recognized_subgraphs == demand_forcings` :1156), still inside the `{ }`.

**SIBLING DEPENDENCY (b1/g3):** g3 moves this Step-11 census BELOW the per-adornment
loop (substrate §4.1 "below": once-per-module, after the last adornment). My belt sits
in that same relocated once-per-module block; under N adornments `forcings_with_guard`
holds N keys and each is checked. I rely on g3 keeping the census once-per-module (not
per-iteration) so the belt sees the FULL stamped graph. No other g3 dependency.

**`Demand.cpp`, ADD after :1162 (before the census block closes at :1163):**
```cpp
    // g8 BELT (D3.a.3, PRE-Optimize, STAMP-TIME): every forcing that stamped ANY
    // guard MUST have stamped >=1 kBody guard. ResolveLiveRecognition (Rel.cpp:976)
    // AND the nested recursive-content fence (Build.cpp:1528) derive/gate ONLY off
    // a role==kBody stamp; a forcing minted with zero body guards resolves
    // input_table==null -> the instance is SILENTLY skipped at mint (Rel.cpp:1053)
    // -> missing answer rows. This is the STAMP-TIME half of the survivor-policy
    // invariant. HONESTY (L15): the FOLD-TIME half (a CSE fold that drops the last
    // kBody) is NOT visible here -- CSE runs AFTER this pre-Optimize census; it is
    // enforced by PromoteSurvivorToBody (View.cpp) + its directed unit (§4-a), and
    // in production by the recommended mint-skip sharpening (§2.3). ALWAYS-ON.
    std::unordered_set<unsigned> forcings_with_guard, forcings_with_body;
    ForEachView([&](VIEW *v) {
      if (v->guard_annotation_index == ~0u) {
        return;
      }
      const GuardAnnotation &g = guard_annotations[v->guard_annotation_index];
      forcings_with_guard.insert(g.forcing_index);
      if (g.role == GuardAnnotation::kBody) {
        forcings_with_body.insert(g.forcing_index);
      }
    });
    for (unsigned fidx : forcings_with_guard) {
      if (!forcings_with_body.count(fidx)) {
        fprintf(stderr,
                "OWN-3/g8: forcing %u stamped guards but NONE is kBody -- "
                "ResolveLiveRecognition resolves a null input_table and silently "
                "skips the instance (a Step-7 guard-mint bug)\n",
                fidx);
        abort();
      }
    }
```

No new include: `<unordered_set>` is already present (`Demand.cpp:81`); `GuardAnnotation`
via `Query.h` (:72); `ForEachView`/`VIEW`/`guard_annotations` are the SAME members the
existing census at :1142 already uses. `abort()`/`fprintf` match the existing OWN-3 idiom
(:1148-1154). ALWAYS-ON, NDEBUG-surviving.

### 2.2 What this belt catches, honestly (the L15 discipline)

The Demand census runs at `Build.cpp:2587`, BEFORE `Optimize` at `Build.cpp:2599` — so
NO fold has happened when it fires (`guard_annotation_folded_count == 0`, every guard
still live). Therefore this belt catches **a MINT bug** (Step-7 produced a forcing with
zero body guards), NOT **a fold regression** (CSE dropped the last kBody). It has REAL
teeth for the former (it would fire loudly if a future body-walk change minted a
kQueryProjection-only forcing); it is SILENT on the corpus (every forcing today mints ≥1
body guard). A masked-negative that red-teams nothing would be a lie (L15) — so I state
the fold-time teeth explicitly: §4-a's `PromoteSurvivorToBody` unit is the fold-time
teeth, and §2.3 is the recommended production teeth.

### 2.3 RECOMMENDED companion — the fold-time production teeth (Rel.cpp mint skip)

The task pins deliverable 2 to the Demand census (§2.1, in-scope). But the phrase
"aborts loudly, not silently skips" is fundamentally about the POST-fold state, and the
Demand census cannot see it. The natural production site is the mint skip
`Rel.cpp:1053-1055`, which today lumps "fully-dead forcing" together with "live but
role-lost." They ARE distinguishable: a forcing gets a `by_forcing` entry iff it has ≥1
LIVE guard (`Rel.cpp:947-954,1019`), so `git != end` ⟺ live guards exist; the role-loss
signature is specifically `demand_table && pub_table && !input_table`.

**RECOMMENDED (coordinate with g5/b1 — Rel.cpp is their surface):** sharpen the skip:
```cpp
    auto git = lr.by_forcing.find(rs.forcing_index);
    if (git == lr.by_forcing.end()) {
      continue;  // fully-dead forcing (NO live guard) — ABA-safe skip
    }
    const ResolvedInstance &ri = git->second;
    if (!ri.ok) {
      // Live guards resolved (git found) but the instance failed: if demand+pub
      // resolved yet input_table is null, a role-losing fold shadowed the kBody
      // stamp (g1 survivor policy regressed). Abort loudly -- never silently skip
      // live rows.
      if (ri.demand_table && ri.pub_table && !ri.input_table) {
        ValidatorFail("BuildSubgraphInstanceOps: live forcing resolved demand+pub "
                      "but null input_table (g1 survivor-policy regression -- "
                      "kBody stamp lost to a fold)");
      }
      continue;  // genuinely unresolved (dead demand/pub) — skip
    }
```
This is BYTE-neutral on the corpus (a validator emits nothing; the arm is dormant so it
never fires) and is the true fold-time teeth. It is OUT of my strict View.cpp scope; I
flag it as the honest completion and defer the one-line ownership to g5/b1. If g5
declines it, the fold-time teeth reduces to §4-a's unit (acceptable — the arm is
dormant, §4-a forces the exact shape), but the census belt alone must NOT be presented
as fold-regression protection.

===============================================================================
## §3 DELIVERABLE 3 — THE PREDICATE JUSTIFICATION (keep the predicate, re-derive it)

**DISPOSITION (binding, substrate §3.4): KEEP `GuardAnnotationsCompatible` unchanged**
(`View.cpp:586-587`, `forcing_index == && instance_key ==`). Only the JUSTIFICATION
comment (the "seated while dormant / single-adornment" residual, `View.cpp:558-583`)
is rewritten for the multi-adornment reality.

### 3.1 The re-derivation, precisely

- `forcing_index` is the LOAD-BEARING identity. Two annotations may legally fold only
  within one forcing; a cross-forcing fold is a genuine mis-key and MUST abort
  (`CheckGuardAnnotationFold`). Under N adornments this is exactly what keeps forcing A's
  guards from collapsing into forcing B's (distinct fabricated demand relations ⇒
  distinct `joined_views[0]` ⇒ never Equals, §3.2 — but the belt stays armed).
- `instance_key` is a fold-INVARIANT BELT, not an identity. Within a single forcing every
  guard shares the adornment's pivot positions, so same-forcing guards carry the SAME
  `instance_key` — the equality is trivially satisfied on the legal fold and the term
  never rejects a legal same-forcing fold. It CAN only ever mismatch ACROSS forcings
  (different adornments ⇒ different pivots), which `forcing_index` already rejects. So
  `instance_key` is stale-but-harmless: fold-invariant on JOIN carriers (`Join::Equals`
  compares pivot vectors, so two Equals JOINs cannot carry divergent keys), and
  stale-but-harmless on proxy-TUPLE carriers (`Tuple::Equals` ignores pivots — but two
  same-forcing proxy TUPLEs forward DIFFERENT incoming views ⇒ `ColumnsEq` fails ⇒ never
  Equals, so the false-abort is unreachable, substrate §3.4).

### 3.2 The exact comment rewrite

**`View.cpp`, REPLACE the LABELED RESIDUAL block (:571-583)** — the D3.a.0 residual that
binds g1 to re-derive the predicate before multi-guard folds go live — with the
discharged multi-adornment justification:
```cpp
// MULTI-ADORNMENT JUSTIFICATION (D3.a.3, discharged; supersedes the D3.a.0
// LABELED RESIDUAL): fold-eligible annotated views of ONE forcing forward the
// SAME incoming view, so they carry an IDENTICAL instance_key -- the predicate
// keys on the two fold-INVARIANT identity fields. `forcing_index` is the
// LOAD-BEARING identity: a cross-forcing collapse is a mis-keyed instance and
// CheckGuardAnnotationFold aborts. `instance_key` is a defensive BELT: within a
// forcing every guard shares the adornment's pivot positions (equal, so no legal
// same-forcing fold is ever rejected); it can differ ONLY across forcings, which
// forcing_index already rejects. It is fold-invariant on JOIN carriers
// (Join::Equals compares pivot vectors) and stale-but-harmless on proxy-TUPLE
// carriers (Tuple::Equals ignores pivots, but two same-forcing proxy TUPLEs
// forward different incoming views => ColumnsEq fails => never Equals, so the
// false-abort is unreachable). direction (a) SURVIVORSHIP is handled by
// PromoteSurvivorToBody (the surviving record's role is load-bearing:
// ResolveLiveRecognition + the nested pre-pass derive input ONLY from a kBody
// stamp). `is_instance_key` is always false this slice (a future recursive-
// subgoal slice revisits it). PURE: no views, no QueryImpl.
```
(Line-count is not pinned for this comment — but keep it near the original length; no
E-71-style residual token is created.)

===============================================================================
## §4 DELIVERABLE 4 — THE TWO DIRECTED WITNESSES

Both extend the EXISTING `tests/DataFlowValidators/GuardAnnotationFoldTest.cpp` mold
(`MakeAnn` + the DeathHarness fork/waitpid, :38-60). The pure-record fixture needs NO
view graph — the same reason the existing OWN-3 death test hand-builds records.

### 4.1 The `MakeAnn` extension (both witnesses need a role parameter)

`MakeAnn` (`GuardAnnotationFoldTest.cpp:38-50`) hardcodes `kBody` at struct position 3.
Add a trailing `role` parameter DEFAULTED to `kBody` (keeps the two existing callers
compiling unchanged):
```cpp
static hyde::GuardAnnotation MakeAnn(
    unsigned forcing_index, std::vector<unsigned> instance_key,
    hyde::GuardAnnotation::DemandSide side,
    hyde::GuardAnnotation::Role role = hyde::GuardAnnotation::kBody) {
  return hyde::GuardAnnotation{
      hyde::GuardAnnotation::kReadAtTuple, side, role, false,
      std::move(instance_key), hyde::QueryView(nullptr),
      hyde::QueryView(nullptr), forcing_index};
}
```

### 4.2 Witness (a) — the survivor policy REPAIRS a qp-survivor fold (pure, no fork)

`PromoteSurvivorToBody` never aborts, so this is plain `ASSERT_EQ` (no DeathHarness).
This is the FOLD-TIME teeth: remove/regress the policy ⇒ the repair assert fails at
ctest.
```cpp
// The g1 survivor policy: CSE can make a kQueryProjection guard the survivor of a
// kBody loser (Optimize.cpp:365 orders by depth/det_seq, not role). The policy must
// promote the survivor back to kBody so ResolveLiveRecognition (Rel.cpp:976) still
// finds the body input. Directed witness (a): force the exact qp-survivor shape.
TEST(DataFlowValidators, SurvivorPolicyRepairsQueryProjectionSurvivor) {
  const hyde::GuardAnnotation loser =
      MakeAnn(3u, {1u}, hyde::GuardAnnotation::kDReader, hyde::GuardAnnotation::kBody);
  hyde::GuardAnnotation surv =
      MakeAnn(3u, {1u}, hyde::GuardAnnotation::kRawSeed,
              hyde::GuardAnnotation::kQueryProjection);
  hyde::PromoteSurvivorToBody(surv, loser);
  ASSERT_EQ(static_cast<int>(surv.role), static_cast<int>(hyde::GuardAnnotation::kBody));
  ASSERT_EQ(static_cast<int>(surv.demand_side),
            static_cast<int>(hyde::GuardAnnotation::kDReader));  // adopted loser stamp
  ASSERT_EQ(static_cast<int>(surv.kind), static_cast<int>(loser.kind));
}

// No-op arms: the policy must NOT disturb a survivor that already carries kBody, nor
// invent a body where neither side had one.
TEST(DataFlowValidators, SurvivorPolicyNoOpWhenSurvivorAlreadyBody) {
  const hyde::GuardAnnotation loser =
      MakeAnn(3u, {1u}, hyde::GuardAnnotation::kRawSeed,
              hyde::GuardAnnotation::kQueryProjection);
  hyde::GuardAnnotation surv =
      MakeAnn(3u, {1u}, hyde::GuardAnnotation::kDReader, hyde::GuardAnnotation::kBody);
  hyde::PromoteSurvivorToBody(surv, loser);
  ASSERT_EQ(static_cast<int>(surv.role), static_cast<int>(hyde::GuardAnnotation::kBody));
}
TEST(DataFlowValidators, SurvivorPolicyNoOpWhenBothQueryProjection) {
  const hyde::GuardAnnotation loser =
      MakeAnn(3u, {1u}, hyde::GuardAnnotation::kDReader,
              hyde::GuardAnnotation::kQueryProjection);
  hyde::GuardAnnotation surv =
      MakeAnn(3u, {1u}, hyde::GuardAnnotation::kRawSeed,
              hyde::GuardAnnotation::kQueryProjection);
  hyde::PromoteSurvivorToBody(surv, loser);
  ASSERT_EQ(static_cast<int>(surv.role),
            static_cast<int>(hyde::GuardAnnotation::kQueryProjection));  // unchanged
}
```
**"ri.ok holds" (the substrate's second clause):** proving `ri.ok` end-to-end needs a
real Query graph + `ResolveLiveRecognition`, which the DataFlowValidators fixture cannot
build. I DEFER that half to b3/g2 design-1 IF that lane builds a `Query`/`DRFlowGraph`
fixture that reaches `ResolveLiveRecognition` — spec for them: build a two-guard
forcing, mutate the survivor to `kQueryProjection`, assert `ResolveLiveRecognition`
yields `!ri.ok` (`input_table==null`, the cliff); apply `PromoteSurvivorToBody`; assert
`ri.ok` with `input_table` resolved. **HONESTY:** the design-1 harness is DRFlowGraph-
based (`ValidateDROps`), NOT Query-based, so this half may not be cheaply reachable; if
not, the fold-time teeth is the pure unit above (which forces the exact mutation) plus
§2.3's production abort. I do NOT claim g6's compile witness exercises the policy — §3.2
proves the arm stays DORMANT in the g6 shape, so g6 is baseline coverage, not policy
teeth.

### 4.3 Witness (b) — the instance_key belt still has TEETH + no spurious live fold

**(b-unit) same-forcing / DIFFERENT-key ⇒ `CheckGuardAnnotationFold` aborts.** This
extends the existing death test (which today covers distinct-forcing abort + same-
forcing-same-key accept). It proves the KEPT `instance_key` conjunct is armed under
multi-adornment. Uses the existing `RunCheckInChild` fork harness (:56-60):
```cpp
// The instance_key belt is KEPT (D3.a.3): two guards of the SAME forcing but
// DIFFERENT instance_key are a mis-keyed instance and MUST abort (SIGABRT).
TEST(DataFlowValidators, GuardFoldTripsOnSameForcingDifferentKey) {
  const hyde::GuardAnnotation loser =
      MakeAnn(7u, {2u, 3u}, hyde::GuardAnnotation::kDReader);
  const hyde::GuardAnnotation survivor =
      MakeAnn(7u, {2u, 4u}, hyde::GuardAnnotation::kRawSeed);  // key differs at pos 1
  ASSERT_EQ(static_cast<int>(ChildOutcome::kSigAbrt),
            static_cast<int>(RunCheckInChild(loser, survivor)));
}
```

**(b-live) the two-adornment shape asserts census `folded == 0`.** COORDINATE with
b4/g6: the g6 From-preserving two-adornment success witness (substrate §2.3 seed — bf
binds col 0, fb binds col 1) compiles to N=2 disjoint stores with NO spurious guard
fold. The observable is the census: `kSubgraphInstantiate == 2` in the g6 `.rel`/census
golden ⇒ both forcings resolved `ri.ok` ⇒ no forcing lost its guards to a collapse. The
INTERNAL `guard_annotation_folded_count` is not currently rendered in any dump, so a
direct `folded == 0` assertion is not observable today; the equivalent teeth is the
census `n_stamped + folded == total` (which passes without abort) + `kSubgraphInstantiate
== 2`. **Optional (flag to b4/orchestrator):** render `folded=` in the census/`.rel`
line if a first-class `folded == 0` pin is wanted. I mark (b-live) as a g6/b4-carried
end-to-end observation, not a b2-owned test.

### 4.4 Test-file placement

Witnesses (a) + (b-unit) land in `tests/DataFlowValidators/GuardAnnotationFoldTest.cpp`
(the natural home — pure records, existing mold, existing `MakeAnn`/DeathHarness). No
new CMake target. The deferred "ri.ok holds" half (§4.2) rides b3/g2 design-1 IF
reachable; (b-live) rides b4/g6.

===============================================================================
## §5 DELIVERABLE 5 — THE CO-LAND CONSTRAINT + GATES

### 5.1 g1 lands FIRST/BINDING and MUST co-land with b1's g3 enable

**The binding co-land (substrate §3.5, §7-g1):** g1 (the survivor policy + the g8 belt +
the predicate justification) MUST land in the SAME commit that ENABLES multi-adornment —
b1/g3's lift of the per-name multi-adornment reject (`Demand.cpp:444-462`) plus the
two-phase per-adornment loop. Precisely why they are inseparable:

- **g1 WITHOUT the g3 enable = untestable dead defensive code.** With `:444-462` still
  rejecting, NO program ever mints a second forcing for one query name; the fold arm is
  provably dormant and the survivor policy has no forcing to protect. (The pure unit
  §4-a still passes — it hand-builds records — but there is no live surface.)
- **the g3 enable WITHOUT g1 = a re-opened silent-skip cliff.** The `:444-462` lift is
  what first creates N>1 forcings sharing ONE pub table `p` and its readers (`Rel.cpp:992`
  keys pub by `q_decl.Id()` = name+arity — the N adornments SHARE it). The moment two
  forcings' guards can sit on shared readers, a role-losing collapse (or a body-guard-
  dropping mint) silently nulls `input_table` (`Rel.cpp:976`) AND blinds the recursive-
  content fence (`Build.cpp:1528`) — the exact failures the belt + policy guard. Even
  though §3.2 proves the fold is dormant for the CURRENT shapes, landing the enable
  without the guard means the cliff has zero protection the instant any later widening
  makes a fold reachable.

⇒ g1 is FIRST in the diff (policy + belt + justification in place) and the g3 enable
rides the same commit.

**The task's `instance_key`-divergence framing, reconciled with §4.4.** The task states
"the fence-lift is the only removal that could make `instance_key` diverge within a
forcing." Precise reading: within ONE forcing `instance_key` is uniform (all guards share
the adornment's pivot positions), so the ONLY thing that could make two same-forcing
guards carry DIFFERENT keys is a relaxation of the From-preservation fences
(`Demand.cpp:668-672 / :723-726`) that let body-pivot ≠ query-proj-pivot. **D3.a.3's g3
does NOT lift those — it RE-MESSAGES them, condition unchanged (substrate §4.4).** So
within D3.a.3 no removal makes `instance_key` diverge within a forcing; the predicate
stays trivially satisfied on every legal same-forcing fold and never false-aborts. The
`instance_key` conjunct is KEPT ARMED as a defensive belt for the FUTURE From-relaxation
/ recursive-subgoal slice that DOES relax `:668/:723` (the `is_instance_key` reviser). I
carry this as the honest statement: the co-land is load-bearing because of the `:444-462`
multi-adornment ENABLE (N forcings share `p`), and the `instance_key`-divergence removal
the task names is a NON-event this slice (the fences are only re-messaged). g1 must NOT
be presented as guarding an instance_key divergence that D3.a.3 cannot produce.

### 5.2 PRE-REGISTERED GATE PREDICTIONS

The whole g1 sub-diff is **[BYTE]** on the entire emission corpus — the fold arm is
DORMANT (§3.2: no both-set fold occurs for any corpus shape, single- OR multi-adornment),
so `PromoteSurvivorToBody` NEVER fires on a real program; the belt + predicate are
validators/comments that emit nothing.

| touched surface | change | gate | prediction |
|---|---|---|---|
| `View.cpp` fold arm (`PromoteSurvivorToBody` call) | new pure call, only fires on a fold that never happens | SUITE (178) + all 11 `.irgold` pins + nested witness `.rel`/`.df` dumps + eqgate (4 carriers) + config-invariance | **[BYTE]** — zero emission change |
| `View.cpp` `PromoteSurvivorToBody` defn (factored) | refactor of §3.3 inline into a pure fn | same | **[BYTE]** |
| `View.cpp` predicate JUSTIFICATION comment (:571-583) | comment-only | any | **[BYTE]** |
| `Query.h` `PromoteSurvivorToBody` decl | header decl, no codegen | any | **[BYTE]** |
| `Demand.cpp` g8 census belt | always-on validator, silent on corpus | SUITE + census goldens | **[BYTE]** on emission; census counts unchanged (`folded==0`, `kSubgraphInstantiate` per pin unchanged) |
| `tests/DataFlowValidators/GuardAnnotationFoldTest.cpp` | +3 policy `ASSERT_EQ` tests + 1 same-forcing-diff-key death test + `MakeAnn` role param (defaulted) | **ctest** (DataFlowValidators) | test count **+4**; existing 2 tests unchanged (defaulted param) |
| `Rel.cpp` mint-skip sharpening (§2.3, RECOMMENDED, g5/b1-owned) | validator, silent on corpus | SUITE + RelValidators | **[BYTE]** on emission; +1 RelValidators death test if authored (rides g2 design-1) |

- **Single-adornment corpus:** every pin **[BYTE]** — no golden moves, `folded_count`
  stays 0, the belt never fires (every forcing mints ≥1 kBody).
- **Multi-adornment (the g6/b4 witness):** NOT a g1 gate — g1 contributes no golden of
  its own; g6's `kSubgraphInstantiate==2` census (b4) transitively confirms both
  forcings resolved `ri.ok` (no role-loss), which is the only end-to-end evidence the
  policy's invariant holds under N=2. HONESTLY, the g6 shape does NOT fire the policy
  (dormant arm), so it is baseline evidence, not policy teeth; §4-a is the policy teeth.

===============================================================================
## §6 SIBLING-LANE EXPECTATIONS I RELY ON (merge contract)

1. **b1/g3** moves the Step-11 census (my belt's host block) BELOW the per-adornment
   loop, ONCE per module, over the full stamped graph (§4.1 "below"). My belt appends to
   that relocated block. b1/g3 also lifts `Demand.cpp:444-462` (the multi-adornment
   enable) — the co-land trigger (§5.1). b1/g3 RE-MESSAGES (does NOT lift) `:668/:723`, so
   `instance_key` stays uniform within a forcing (§5.1 reconciliation).
2. **b3/g2 design-1** provides the forked RelValidators harness; IF it reaches a
   `Query`/`ResolveLiveRecognition` fixture, it carries the §4.2 "ri.ok holds" half
   (spec given). It also owns the E2c gate-clone dedup + the `[F]` always-on fence
   (out of b2 scope).
3. **b4/g6** authors the §2.3 From-preserving two-adornment SUCCESS witness (driver +
   probes + `.eqgate`); its census golden carries `kSubgraphInstantiate==2`, the
   end-to-end no-role-loss evidence (§4.3 b-live). Optional `folded=` render if a
   first-class `folded==0` pin is wanted.
4. **g5** owns the `Rel.cpp` V-INST-SOLE O1 per-pub relaxation AND (recommended) the
   §2.3 mint-skip sharpening (both `Rel.cpp` = their surface).

## §7 OPEN ITEMS / HONEST RESIDUALS

- **R-b2-1 (fold-time teeth placement).** The Demand census belt is stamp-time only; the
  fold-time production teeth is §2.3's `Rel.cpp:1053` sharpening, which is g5/b1-owned.
  If declined, fold-time teeth = §4-a's pure unit only (acceptable — dormant arm; but the
  census belt must not be sold as fold-regression protection).
- **R-b2-2 (ri.ok half).** §4.2's "ri.ok holds" end-to-end assertion depends on whether
  b3/g2 design-1 exposes a `ResolveLiveRecognition`-reaching fixture; it may reduce to
  the pure-unit + §2.3 abort. Flagged, not assumed.
- **R-b2-3 (folded==0 observability).** No dump renders `guard_annotation_folded_count`
  today; the §4.3 (b-live) `folded==0` is asserted transitively via the census pass +
  `kSubgraphInstantiate==2`. First-class pin needs a b4-owned render addition (optional).
- **NO owner brief triggered.** Both g1 changes are pure internal correctness while the
  arm is dormant (substrate §3.5, §8-Q6); no admitted program's answers or deltas move.
  The contingent owner-brief trigger (a DEMONSTRATED live role-divergent fold) was not
  produced by any lane and is not produced here.
