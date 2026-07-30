# D3.a.3 STAGE (b) — DESIGN LANE b1: g3 THE PASS LOOP

Target: `lib/DataFlow/Demand.cpp` `QueryImpl::ApplyDemandTransform` (:385-1167).
Tip b65e7668 (docs-only atop D3.a.2 landing bfc068d1; **code bytes == the
landing**). Repo READ-ONLY; every line number below re-read at code this session
against the file dumped in-thread. Two scratch compiles run against
`build/debug/bin/drlojekyll` (§3 grounding).

BINDING inputs (not re-litigated): substrate §4 (two-phase locate/check/mint
loop; RE-MESSAGE :668/:723; lift :444-462; snapshots-in-loop; stray-union;
once-per-module census); §6 O1 (V-INST-SOLE per-pub relaxation — b3's edit, this
lane produces the N forcings that arm it); §2 (`demand_multi_adorn_1` STAYS
diagnostic; a NEW From-preserving success witness is b4's). SCOPE = OD-15 (N
disjoint stores, one per (query, BindingPattern)).

===============================================================================
## §0 HEADLINE (most-load-bearing first)

1. **The rewrite is a TWO-PHASE split of one straight-line function.** Phase-1
   (Loop 1) does Steps 2+3 (trace + SIP locate) per adornment, NO minting,
   accumulating a `plan` vector + a pass-level `known_consumers` union. Step 4
   (stray) runs ONCE over the union on the pre-mint graph. Phase-2 (Loop 2) does
   Steps 5-10 per adornment (fabricate/mint/guard/register), snapshotting
   `forcing_index`/`first_annotation` INSIDE each iteration. Step 11 census +
   `MarkDemandFabricated` run ONCE below.

2. **ONE CORRECTNESS ADDITION beyond the substrate's plain loop: the loop MUST
   dedup by BindingPattern string** (mirror `BuildQueryEntryPoint`
   Build.cpp:678-687 `seen_variants`). `UniqueRedeclarations()` can return
   DUPLICATE redeclarations sharing a binding pattern — that is *why* the old
   belt built a `std::unordered_set<std::string> patterns` (:453-456) instead of
   just counting the range, and why Build.cpp:678 dedups. Without the dedup, two
   duplicate `bf` redecls both fabricate `demand__q_bf` → the SECOND hits
   `assert(!io_slot)` (:860). The dedup keeps the DataFlow loop and the
   ControlFlow entry-point fan-out (Build.cpp:676-689) enumerating the SAME set
   — the g4 alignment. **This is a required refinement I am folding into the
   diff.**

3. **[BYTE]-neutral for the whole single-adornment corpus** (|plan|==1): emission
   order 2,3→4→5..10→11,mark is identical to today at identical graph states;
   `redecl`-derived locals == `q_decl`-derived (the sole `UniqueRedeclarations()`
   element IS the stored decl); `ParsedQuery::From(redecl)`==`From(q_decl)`;
   union==today's single set. The two RE-MESSAGED rejects are text-only exit
   paths no corpus program reaches.

4. **`demand_multi_adorn_1` has NO stdout golden** (confirmed: `ls
   tests/OptDiff/goldens/ | grep multi_adorn` = empty). It is handled by
   `expect_diagnostic` (runall.sh :169-181), which asserts ONLY `exit==1`. Its
   reject MOVES :457→:717-722 but STAYS exit 1. → **the golden does NOT churn;
   runall.sh needs NO edit** (the case stays in the :364 all-modes-diagnostic
   list verbatim). No bless.

===============================================================================
## §1 DELIVERABLE 1 — THE EXACT DIFF SHAPE

I give the four sub-diffs (a)-(d) plus the folded-in dedup (b'). Line spans are
CURRENT (the in-thread dump). Real locals named throughout.

### 1.0 The `plan` record (NEW type, function-local, above Loop 1)

```cpp
// One traced-but-not-yet-minted adornment. Carries everything Loop 2 needs so
// Phase-1 can complete for ALL adornments before ANY minting (ADV-3).
struct PerAdornment {
  ParsedDeclaration redecl;          // the declared binding pattern
  std::vector<unsigned> bound_indices;  // Step-1 (redecl-derived)
  std::vector<unsigned> p_bound;     // the adornment as p-column positions
  TUPLE *q_read{nullptr};            // Step-2 outputs
  VIEW *q_consumer{nullptr};
  MERGE *p_merge{nullptr};
  std::vector<GuardSite> sites;      // Step-3 outputs
  std::vector<TUPLE *> pushdown_reads;
};
```
`GuardSite`/`PerAdornment` copy fine (`GuardSite` is trivially copyable except
its `std::vector<unsigned> pivot_pos`; move `sites`/`pushdown_reads` into the
record). Declared just after `q_insert` is pinned (:482), before Loop 1.

### 1.1 (b') — the enumeration source, with the binding-pattern dedup

REPLACING the DELETED :444-462 belt (see 1.2) as the head of **Loop 1**:

```cpp
std::unordered_set<VIEW *> known_consumers;   // ADV-3 pass-level union
std::vector<PerAdornment> plan;
std::unordered_set<std::string> seen_variants;  // mirror Build.cpp:678

for (ParsedDeclaration redecl : q_decl.UniqueRedeclarations()) {
  std::string binding(redecl.BindingPattern());
  if (!seen_variants.insert(std::move(binding)).second) {
    continue;                       // duplicate redecl of an already-planned
                                    // adornment — Build.cpp:681-685 does the same
  }
  … Step 1b + Step 2 + Step 3 (per-adornment) …
}
```

WHY: `UniqueRedeclarations()` (Parse.cpp:942, `context->unique_redeclarations`)
is unique by full declaration identity, not by binding pattern; Build.cpp:678-687
proves duplicates-per-pattern are reachable (it re-dedups with the identical
`seen_variants` idiom). The old `patterns`-SET belt (:453) counted PATTERNS, so
this dedup is the faithful inversion of the lifted fence, not a new policy.

### 1.2 (a) — DELETE the :444-462 per-name multi-adornment belt

DELETE verbatim (19 lines, the whole `{ std::unordered_set<std::string>
patterns; … return reject("Multi-adornment demand …"); }` block, :444-462):
```cpp
// MULTI-ADORNMENT reject (the FIRST belt …):
  { std::unordered_set<std::string> patterns;
    for (ParsedDeclaration redecl : q_decl.UniqueRedeclarations())
      patterns.insert(std::string(redecl.BindingPattern()));
    if (patterns.size() != 1u) return reject("Multi-adornment demand …"); }
```
WHY: the loop (1.1) enumerates the adornments the belt used to forbid. Its
`seen_variants` dedup subsumes the `patterns`-set uniqueness. This is the SINGLE
lift point (substrate §1.2; confirmed at code §3: `demand_multi_adorn_1` fires
here today).

### 1.3 (b) — the STEP relocation into the two-phase loop

**(M) STAYS ABOVE both loops, byte-order-unchanged** (:393-482): mode gate,
G2 reject, `reject` lambda, bound-query collection, empty check, **`>1 bound
QUERY` reject (:435-439) STAYS** (different axis — multiple query NAMES; two
adornments of one name = ONE (name,arity) REL — confirmed by the belt's own
comment :446-448), `q_rel`/`q_decl` (:441-442), single-materialization check +
`q_insert` (:477-482).

**Loop 1 body — Step 1b + Step 2 + Step 3, per adornment, NO minting:**
- Step 1b `bound_indices` (:464-469): source `redecl.Parameters()`, NOT
  `q_decl.Parameters()`. This is the ONE change to the Step-1b text (the loop
  variable's name is `redecl`).
- Step 2 trace (:484-577): locals `q_consumer/q_read/p_merge/p_bound` (:484-487)
  become per-iteration; rejects :491/:498/:507-512/:514/:524/:533-537/:545-549/
  :559-560 unchanged; `assert(p_merge&&q_read&&q_consumer)` (:567) per-iter;
  distinct-bound-positions reject (:570-577) per-iter. **Move `bound_indices` and
  Step 2 verbatim; only the enclosing loop and the `redecl` source change.**
- Step 3 SIP locate (:584-759): `sites`/`pushdown_reads` (:584-585) per-iter;
  the per-MERGE-member walk (:587-759) verbatim except the two RE-MESSAGES (1.4).
- Loop-1 tail (NEW, after :759, before the member loop closes into the plan):
```cpp
  known_consumers.insert(q_consumer);
  for (const GuardSite &s : sites) known_consumers.insert(s.consumer);
  assert(plan.empty() || p_merge == plan.front().p_merge);  // one relation p
  plan.push_back(PerAdornment{redecl, std::move(bound_indices), std::move(p_bound),
                              q_read, q_consumer, p_merge,
                              std::move(sites), std::move(pushdown_reads)});
```

**Step 4 stray-consumer check — ONCE, between the loops** (1.5 gives the diff).

**Loop 2 body — Steps 5-10, per adornment, minting:**
```cpp
for (PerAdornment &a : plan) {
  const std::vector<unsigned> &bound_indices = a.bound_indices;   // rebind names
  const std::vector<unsigned> &p_bound = a.p_bound;
  MERGE *const p_merge = a.p_merge; TUPLE *const q_read = a.q_read;
  VIEW *const q_consumer = a.q_consumer;
  const std::vector<GuardSite> &sites = a.sites;
  const std::vector<TUPLE *> &pushdown_reads = a.pushdown_reads;
  … Step 5 (:797-849) … Step 6 (:858-985) … Step 7 (:987-1027) …
  … Step 8 (:1029-1066) … Step 8b (:1068-1083) … Step 9 (:1085-1121) …
  … Step 10 (:1123-1130) …
}
```
The Step-5/6/7/8/8b/9/10 bodies move VERBATIM under this loop, with exactly
these text changes:
- Step 5 `adorn` (:797-800): iterate `a.redecl.Parameters()`, not
  `q_decl.Parameters()`. `base_name` (:807-810) keeps `q_decl.NameAsString()`
  (name is shared) — `adorn` differs → distinct `demand__q_bf`/`demand__q_fb`
  (ADV-7).
- Step 5 `bound_types` (:812-815): `a.redecl.NthParameter(bi).Type()`, not
  `q_decl.NthParameter(bi)`.
- Step 6 snapshots (:983-985) STAY inside the loop, re-read per iteration
  (ADV-2 — the bucket key `forcing_index` at Rel.cpp:953 / Build.cpp:1513).
- Step 10 (:1129): `ParsedQuery::From(a.redecl)`, not `From(q_decl)` (g4 — the
  Build.cpp:469-471 registry match is (query, BindingPattern)-keyed).
- Everything else (`decl_to_input`/`decl_to_relation` maps with their
  `assert(!io_slot)`/`assert(!rel_slot)`, the graph mint, the tripwire) is
  UNCHANGED — decl-keyed, distinct-suffix (ADV-7).

**(below) ONCE after the last adornment** (:1132-1165): Step 11 census (both
count equalities are module-global sums — :1147 `n_stamped +
guard_annotation_folded_count == guard_annotations.size()`, :1156
`recognized_subgraphs.size() == demand_forcings.size()`) + `MarkDemandFabricated`
(:1165). Move OUT of any loop; run once. Each adornment appends exactly one
forcing + one subgraph + its (1 + |sites|) annotations, so both counts stay
balanced at every loop boundary and the once-at-tail check is sound.

### 1.4 (c) — RE-MESSAGE :668-672 and :723-726 (text only; condition unchanged)

At :668-672 (read-at-tuple `in_col->Index() != pos`) and :723-726 (push-down
`found_col->Index() != pos`), the CONDITION `Index() != pos` STAYS (it fences a
SIP-derived sideways / non-From-preserving adornment — a recursion-shape artifact,
NOT a second DECLARED binding pattern; confirmed at code — a swap recursion with
ONE declared `bf` adornment fires :723, substrate §4/O-1). Only the message text
changes, mirroring the :717-722 left-linear wording:

:668-672 becomes:
```cpp
return reject(
    "Sideways (non-From-preserving) demand propagation is not yet "
    "supported under -demand");
```
:723-726 becomes the same string. WHY: "Multi-adornment demand is not yet
supported" becomes FALSE the moment multi-DECLARED-adornment lands — it is a
MISNOMER at the body-walk. The re-message names the real unsupported shape
(sideways SIP propagation to a DIFFERENT adornment, which the single-adornment
propagation member Step-6 :926-927 cannot produce and which the N-declared model
does not supply — a g7 fence). All OTHER body-walk rejects STAY verbatim,
unconditionally: NEGATE/AGG (:625-628), self-join (:633-637), left-linear
(:717-722), and the malformed-shape belts (:590/:622/:630/:647/:657/:692/:705/
:733-734/:745-748).

### 1.5 (d) — the stray-consumer UNION (substrate §4.3 / ADV-3)

The Step-4 block (:761-791) MOVES out of the per-adornment body to run ONCE
between Loop 1 and Loop 2, over the pass-level `known_consumers` union built in
Loop 1's tail (1.3). The block's INTERNALS are byte-identical to today; only
(i) `known_consumers` is now the pre-built union (the `.insert(q_consumer)` +
site loop at :770-774 move to Loop 1's tail), and (ii) `p_merge` reads
`plan.front().p_merge`:

```cpp
// Step 4 ONCE, on the PRE-MINT graph (every adornment's p_merge is identical).
{
  MERGE *const p_merge = plan.front().p_merge;
  for (VIEW *user : CollectColUsers(this, p_merge)) {         // :776
    TUPLE *const t = user->AsTuple();
    if (!t || !IsFullWidthReaderOf(t, p_merge))
      return reject("The demanded relation has a consumer shape not yet "
                    "supported under -demand");               // :779-781
    for (VIEW *ruser : CollectColUsers(this, t))              // :783
      if (!known_consumers.count(ruser))
        return reject("The demanded relation is read by a consumer demand "
                      "cannot guard (a sibling query or another rule) "
                      "under -demand");                       // :785-788
  }
}
```

WHY (the false-reject it prevents): both adornments demand the SAME `p` → share
`p_merge` and its readers. If adornment A completed Step 7 (minting A's guard
JOINs, which `MintGuardJoin({d_reader, read})` makes NEW consumers of the shared
reader `t`) before B's Step 4 ran, B's Step 4 would see A's guards as untraced →
false reject at :785-788. Two-phase (locate-all / check-once-pre-mint / mint-all)
is the fix; deferring the check to AFTER a single Steps-2-10 loop is WRONG
(minted guards become untraced consumers — substrate §4.3, lane §3 "Alternative
rejected").

===============================================================================
## §2 DELIVERABLE 2 — THE [BYTE]-NEUTRALITY PROOF (|plan|==1)

Claim: for every existing single-adornment corpus program (`demand_tc_witness`,
`demand_neighborhood_witness`, `demand_neighborhood_mono_witness`,
`demand_diff_input_1`, `demand_diff_neighborhood_witness`, and the flag-off
166-case net which never enters the pass), the g3 restructure emits byte-identical
output. Proof, point by point at code:

1. **The mode gate is untouched** (:393-395): flag-off returns before any of this
   — the 166 non-demand goldens are trivially [BYTE].

2. **`|plan|==1`** for every single-adornment demand case. `q_decl` for a
   `demand_tc_witness`-shaped program is a `#query` with ONE binding pattern.
   `UniqueRedeclarations()` yields the sole stored declaration; `seen_variants`
   admits it once → `plan` has exactly one element. (The dedup is a no-op for
   one pattern.)

3. **`redecl`-derived == `q_decl`-derived.** For |plan|==1 the loop variable
   `redecl` IS `q_decl`'s stored declaration (the REL's `q_rel->declaration` is
   the sole `UniqueRedeclarations()` element). So `bound_indices` (:464-469),
   `adorn` (:797-800), and `bound_types` (:812-815) computed off `redecl` equal
   today's off-`q_decl` values byte-for-byte.

4. **`ParsedQuery::From(redecl) == ParsedQuery::From(q_decl)`** at Step 10
   (:1129). Both resolve the same underlying `ParsedQueryImpl` (Parse.cpp:1118,
   keyed on the declaration's context + binding pattern); for the sole redecl
   they are the same object → identical `demand_forcings` entry.

5. **Emission order + graph states identical.** The two-phase split visits, for
   |plan|==1: Step 2,3 (Loop 1) → Step 4 (between) → Step 5..10 (Loop 2) → Step
   11, mark (below) — the EXACT sequence of today's straight-line 2→3→4→5→…→10→
   11→mark. Step 4 still runs pre-mint (no guard exists yet with one adornment).
   No view is minted earlier or later; the id stream is identical; the snapshots
   (:983-985) read `demand_forcings.size()==0`/`guard_annotations.size()==0`
   exactly as today.

6. **The stray union == today's single set.** With one adornment,
   `known_consumers` = `{q_consumer}` ∪ site consumers — the identical set Step 4
   built inline today (:770-774). Same graph (pre-mint), same walk, same verdict.

7. **The two RE-MESSAGED rejects are unreachable on the corpus.** They are exit
   paths (`return reject(...)`); a program that reaches them fails to compile. The
   corpus compiles → no corpus program reaches :668-672/:723-726. Text-only, zero
   stdout impact. (No diagnostic golden pins these two messages — the only
   multi-adornment diagnostic, `demand_multi_adorn_1`, fires the LIFTED :457, not
   these; §3.)

**Surfaces the [BYTE] gate covers (all must be byte-identical):**
- All 176 non-witness stdouts × 4 modes (SUITE, `run_vs_golden` / `diffrun.sh`).
- `demand_tc_witness` `.irgold` sidecars: the h/ir/df/**rel** dumps
  (`tests/OptDiff/goldens/demand_tc_witness.{h,ir,df,rel}gold` per the CLAUDE.md
  IR-golden policy). The demand pass's minted graph feeds all four; |plan|==1
  keeps the mint order + ids identical → [BYTE] on each.
- The four FROZEN nested witness dumps: `demand_neighborhood_witness`,
  `demand_neighborhood_mono_witness`, `demand_diff_input_1`,
  `demand_diff_neighborhood_witness` (their `.eqgate`-driven nested-arm stdouts +
  any pinned nested census/dumps). All single-adornment → [BYTE].
- Config-invariance: the single-hash-across-knobs check (release==debug) — a
  validator/loop-structure change emits nothing, so the hash is unmoved.
- ctest RelValidators / DataFlowValidators: g3 alone changes no validator; the
  g1/g2 death tests are sibling-owned (co-land, §4).

===============================================================================
## §3 DELIVERABLE 3 — THE GOLDEN CHURN

**`demand_multi_adorn_1` STAYS diagnostic; its committed golden does NOT churn;
runall.sh needs NO edit.** Grounding at code:

- **No stdout golden exists.** `ls tests/OptDiff/goldens/ | grep -i multi_adorn`
  → empty. The case files are only `demand_multi_adorn_1.dr` +
  `.drflags` (`-demand`) + an inert `.main.cpp` (`int main(){return 0;}`, never
  compiled — its header says so).
- **It is an all-4-modes-diagnostic case** (runall.sh :364 list contains
  `demand_multi_adorn_1`). Handling: `expect_diagnostic $mode` (:169-181), which
  asserts ONLY `rc == 1` (`if [ $rc -ne 1 ]; then … EXPECT-ERROR-GOT`). It does
  NOT compare stderr text against any golden.
- **The reject MOVES but the exit code is invariant.** Observed at code (§ below):
  today the case exits 1 at :457 ("Multi-adornment demand is not yet supported (a
  demanded query name with more than one binding pattern)"). Post-lift, its `bf`
  half compiles and its `fb` half fires the per-adornment left-linear reject
  :717-722 ("The demanded relation's bound columns do not trace to a recursive
  read or a source atom") — I verified the fb-half-in-isolation exits 1 with
  exactly that message. So all 4 modes still exit 1. `expect_diagnostic` passes
  unchanged.

**OBSERVED (scratch, `build/debug/bin/drlojekyll`):**
- `demand_multi_adorn_1.dr -demand` → `exit=1`, message
  "Multi-adornment demand is not yet supported (…more than one binding pattern)…"
  at `:1:1` (the :457 belt).
- fb-half in isolation `-demand` → `exit=1`, message
  "The demanded relation's bound columns do not trace to a recursive read or a
  source atom (unsupported demand propagation shape)…" (the :717-722 left-linear
  reject) — the post-lift landing site.

**Pre-registration:**
- runall.sh: **NO edit.** `demand_multi_adorn_1` STAYS in the :364
  all-modes-diagnostic list; `demand_cyclic_1`/`demand_recursive_content_1` also
  stay (untouched by this lane).
- Golden churn from THIS lane: **NONE.** `demand_multi_adorn_1` has no stdout
  golden to churn; every single-adornment `.irgold`/witness golden is [BYTE].
- The ONE genuinely-new golden (the §2.3-seed From-preserving two-adornment
  SUCCESS witness + its `.eqgate`) is **b4/g6's** deliverable, NOT this lane. It
  is a NEW case, not a churn of an existing golden. (Flagged as a sibling
  dependency, §4.)

===============================================================================
## §4 DELIVERABLE 4 — RISKS + SIBLING DEPENDENCIES

### 4.1 Could any single-adornment program's emission MOVE?

Probed at code. The only path the deleted :444-462 belt guarded is
`patterns.size() != 1u`; a single-adornment program has `patterns.size()==1` and
NEVER entered the reject — deleting the belt changes nothing on its path (the
belt was a pure pass-through for one adornment). Every other change is either
(i) a rename of the enclosing loop variable (`redecl` for `q_decl`, byte-equal
for |plan|==1, §2.3), or (ii) a relocation preserving emission order (§2.5). **No
single-adornment program reaches new code; none can move.** RISK: NONE for the
existing corpus, contingent on the §2 proof holding (a mechanical obligation the
[BYTE] gate enforces, not a judgment call).

### 4.2 The dedup omission risk (the one I folded in)

If the loop is written as the substrate's bare `for redecl in
UniqueRedeclarations()` WITHOUT the `seen_variants` dedup (1.1), a program whose
`#query` is declared twice at the SAME binding pattern would fabricate
`demand__q_<adorn>` twice → the SECOND `decl_to_input[d_msg_decl] = d_io` hits
`assert(!io_slot)` (:860) → debug abort (exit 134). The dedup (mirroring
Build.cpp:678) closes this. **This is a REQUIRED refinement to the substrate §4
loop text; b2 (builder) and b4 (witness) must assume the dedup is present.**
Single-adornment corpus is unaffected (one pattern → dedup is a no-op → [BYTE]).

### 4.3 The `p_merge`-divergence assert (new, defensive)

`assert(plan.empty() || p_merge == plan.front().p_merge)` (1.3) encodes the
one-relation-`p` invariant. Under the one-query ruling (OD-15) all adornments
demand the same `p`, so it never fires; a divergence would mean the adornments
demand different relations (out of scope). Belt, not behavior. [BYTE] (asserts
emit nothing).

### 4.4 SIBLING DEPENDENCIES (named)

- **b1 → g1 (View.cpp survivor policy) CO-LAND, g1 FIRST.** This loop is what
  FIRST puts two forcings' guards on the shared readers of one `p`, arming the
  CSE both-set fold arm (View.cpp:651-679). Per the charter g1 is BINDING/FIRST
  and MUST land in the SAME slice, before this loop goes live (substrate §3.5
  binding co-land: the :668/:723 fence-lift here is the only removal that could
  make `instance_key` diverge within a forcing). **I rely on g1's survivor-record
  policy + "≥1 kBody survivor" belt being present.**
- **b1 → b3 (g5 V-INST-SOLE per-pub, Rel.cpp:4406) CO-LAND.** This loop produces
  N forcings sharing ONE pub table (`q_decl.Id()`), which trips today's
  `inst_per_pub != 1u` abort. **I rely on b3 relaxing the key to
  `(pub_table, forcing_index)` (§6 O1).** Without it, the first two-adornment
  program aborts at mint. (This lane does not touch Rel.cpp.)
- **b1 → b4 (g6 witness) NEEDS this fence-lift.** b4's From-preserving
  two-adornment SUCCESS witness (the §2.3 seed) can only compile once :444-462 is
  lifted and the loop mints per adornment. **b4 depends on b1**; b4 must assume
  the §4.2 dedup (so its witness's declared adornments each fabricate once) and
  the RE-MESSAGED :668/:723 (so a From-preserving second adornment does NOT trip
  a misnamed multi-adornment reject).
- **b1 → g2 (builder + [F] guard) INDEPENDENT of this lane's text**, but g2
  carries the g1 RelValidators death TEST that the co-land needs. Named for
  completeness; no Demand.cpp coupling.
- **g4 alignment:** Step 10's `ParsedQuery::From(a.redecl)` + `a.bound_indices`
  carry the per-adornment binding pattern so the Build.cpp:469-471 forcer /
  :571-574 retract registry matches key correctly. The `seen_variants` dedup
  (1.1) makes the DataFlow enumeration set-equal to Build.cpp:676-689's — the two
  sides align by construction.

===============================================================================
## §5 PRE-REGISTERED GATE PREDICTIONS (per touched surface)

| surface | gate | prediction |
|---|---|---|
| 176 non-witness stdouts × 4 modes | SUITE (`runall.sh`) | **[BYTE]** — flag-off no-op + |plan|==1 (§2) |
| `demand_tc_witness.{h,ir,df,rel}gold` | pinned `.irgold` | **[BYTE]** — single adornment, identical mint order/ids (§2.5) |
| 4 nested witnesses (`.eqgate` arms + frozen dumps) | eqgate + frozen nested dumps | **[BYTE]** — all single-adornment |
| `demand_multi_adorn_1` (4 modes) | `expect_diagnostic` (exit==1) | **[BYTE]** on the gate signal (exit 1); reject text moves :457→:717-722, but no text golden → no churn (§3) |
| runall.sh diagnostic list | file diff | **NO EDIT** (§3) |
| RelValidators / DataFlowValidators | ctest | **[BYTE]** for g3-alone; the g1/g2 death tests are sibling-owned co-land additions (NEW tests, not churn) |
| config-invariance (release==debug single hash) | hash | **[BYTE]** — no emission change |
| NEW g6 success witness + `.eqgate` | (b4-owned) | **[STRUCT]/new golden** — NEW case, N=2 stores; NOT this lane's churn |

**No [STRUCT] surface is owned by b1.** Every surface this lane touches is
[BYTE]; the only structural/new golden (the g6 witness) belongs to b4. The
co-land (g1 survivor policy, b3 per-pub relaxation) adds NEW RelValidators death
tests but churns no existing golden.

===============================================================================
## §6 SUMMARY OF b1's EDITS TO `lib/DataFlow/Demand.cpp`

1. Declare `struct PerAdornment` + `std::vector<PerAdornment> plan` +
   `std::unordered_set<VIEW *> known_consumers` + `std::unordered_set<std::string>
   seen_variants` after `q_insert` (:482).
2. DELETE the :444-462 per-name multi-adornment belt.
3. Wrap Steps 1b+2+3 (:464-759) in **Loop 1** over `q_decl.UniqueRedeclarations()`
   with the `seen_variants` binding-pattern dedup (mirror Build.cpp:678);
   `bound_indices`/Step-2 sourced from `redecl`; Loop-1 tail builds
   `known_consumers` + asserts one `p_merge` + `plan.push_back`.
4. Run Step 4 (:761-791) ONCE between the loops over `known_consumers` /
   `plan.front().p_merge`.
5. Wrap Steps 5-10 (:797-1130) in **Loop 2** over `plan`; `adorn`/`bound_types`
   from `a.redecl`; snapshots (:983-985) stay per-iteration; Step 10 uses
   `ParsedQuery::From(a.redecl)`.
6. RE-MESSAGE :668-672 and :723-726 to the "Sideways (non-From-preserving) demand
   propagation…" string (condition unchanged).
7. Keep Step 11 census (:1132-1163) + `MarkDemandFabricated` (:1165) ONCE below
   both loops.

Net: one function, two loops, one lifted belt, two re-messages, one folded-in
dedup. [BYTE] for the single-adornment corpus; the first two-adornment program
compiles to N=2 disjoint stores (co-landing with g1 survivor policy + b3 per-pub
relaxation).
