# The (F) determinism fix — the post-verdict argument

Checkpoint step 3, artifact 2. Written against branch `keyed-instances`
tip `fa9a8cc2` (evidence gathered on the frozen baseline
`baseline-bin/drlojekyll.debug.60821adf`; the fix does not exist yet —
every prediction below is BY STRUCTURE, pre-registered before any edit).

Inputs consumed: `fleet-d0/verify-determinism.md`, the consolidated (F)
verdict (`fleet-d0/consolidated.md` §4), ledger §1 E-47..E-49 + the (F)
verdict fix-shape note, `fdet/{hdr.diff,ir1..ir6.ir}`, and a first-hand
end-to-end read of `lib/DataFlow/Induction.cpp:100-729`,
`lib/DataFlow/Query.{h,cpp}`, `lib/DataFlow/View.cpp`,
`lib/DataFlow/Columns.cpp`, `include/drlojekyll/Util/DefUse.h`,
`lib/ControlFlow/Build/Induction.cpp`, `lib/DataFlow/Stratify.cpp`.

This is the argument a reviewer reads to believe the emission is
deterministic BY CONSTRUCTION after the fix, not by allocation luck.

---

## 0. The defect in one sentence

`std::unordered_map<VIEW *, MergeSet> merge_sets`
(`Induction.cpp:108`) is iterated in pointer-hash-bucket order at
`:520`, and that single loop does the TWO order-sensitive things that
reach emission — it assigns each merge-set its `merge_set_id`/`group_id`
(`:530-531`, `++group_id`) and it populates each set's
`related_merges` list (`:536`, `AddUse(view)`). `VIEW` nodes are plain
`new` (`DefUse.h`), so `VIEW *` values reshuffle run-to-run
(malloc/ASLR), the buckets permute, and the induction machinery emits
its vectors and sibling regions in a run-varying order.

Everything downstream is deterministic *given* that list order:
`info->cyclic_views = merge_set->related_merges` (`:535`, an alias),
`QueryView::InductiveSet()` returns it verbatim
(`Query.cpp:1271-1278`), the control-flow builder walks it at
`Build/Induction.cpp:673` `for (auto other_view : view.InductiveSet())`
and calls `proc->VectorFor(...)` inside the loop — one `impl->next_id++`
per call (`Procedure.cpp`). So the list order IS the emission order. Fix
the list order once, at its source, and the whole chain is pinned.

---

## 1. The fix — concrete patch hunk against `Induction.cpp`

### 1.1 Adjudication: locus (i) beats locus (ii)

Two candidate loci were on the table:

- **(i) Deterministic ITERATION of `merge_sets` at `:520`.** Sort the
  keys by a stable structural view-key into a vector first, then drive
  the `:520` loop from that vector. This stabilizes BOTH order-sensitive
  outputs of the loop in one move: `merge_set_id`/`group_id` assignment
  (`:530-531`) AND `related_merges`/`cyclic_views` population (`:536`).
- **(ii) The consolidator/lane's original phrasing: a post-population
  `WeakUseList::Sort()` on each distinct `related_merges` list**, after
  the `:520-537` loop completes.

**Adjudicated: (i).** Reasoning, checked against the code:

1. **(ii) is INCOMPLETE.** The `:520` loop assigns `merge_set_id` via
   `++group_id` at `:530-531`, *inside the same pointer-ordered
   traversal*. A post-population list-sort fixes the list order but
   leaves the *integer value* of each set's `merge_set_id` still keyed
   to bucket order. Today that integer is only a debug comment
   (`Build/Induction.cpp:664` `"set " << merge_group`, `#ifndef NDEBUG`)
   and a Stratify partition-consistency ASSERT
   (`Stratify.cpp:404-413`), so it is not *currently* release-emission-
   visible — but leaving a pointer-ordered integer live in shared
   machinery is exactly the latent-landmine shape this epoch is closing,
   and directive 5's IR goldens will start printing the `.ir` `set N`
   annotation. (i) removes the landmine for free; (ii) leaves it armed.

2. **(i) fixes both axes with strictly less surface.** One sorted key
   vector, one loop rewrite. (ii) additionally has the "must Sort AFTER
   full population, once per distinct set, and the lists are aliased"
   footgun (verify-determinism.md sharpen-note 1 — the naive "Sort at
   `:529` where it's first created" is wrong; the list holds one element
   there). Driving the *outer* iteration from a sorted order sidesteps
   the footgun: `AddUse` is called in sorted order to begin with, so no
   post-hoc re-sort of an aliased shared list is needed.

3. **The key is proven run-stable. [AMENDED: CRITICAL — it is run-stable
   but NOT total; see the OWNER DECISION block below.]**
   `QueryViewImpl::Sort()`
   returns `Hash()` (`View.cpp:120-122`), which is memoized and purely
   structural: `HashInit()` folds kind-name + deletion flags +
   `columns.Size()` (`View.cpp:405-415`, NO pointer/id), and MERGE/JOIN
   `Hash()` XOR-fold their inputs' hashes recursively (`Merge.cpp:25-48`,
   `Join.cpp`) — pointer-independent. `Hash()` alone can collide, so a
   tie-break is required for a TOTAL order. The tie-break is the
   view's first column id: `(*view->columns.begin())->id`.

   **[AMENDED: MEDIUM — the tie-break integer is justified against the
   WRONG id below; corrected here.]** The `col->id` read at the `:520`
   loop is NOT the finalized `%col:N` value. This loop runs inside
   `IdentifyInductions` (`Build.cpp:2596`), which executes strictly
   BEFORE `FinalizeColumnIDs` (`Build.cpp:2602`). So the id read here is
   the EARLIER, VarId-derived `col->id` set in `BuildClause`
   (`CreateVarId`/`VarId`, from `ParsedVariable::Order()` through the
   deterministic DisjointSet var-merge) — source-lexical, not
   pointer-derived, and therefore still run-stable. `FinalizeColumnIDs`
   (`Columns.cpp:13-25`) later renumbers to the `%col:N` values in
   `ForEachView` order (per-kind `DefList` insertion order,
   `Query.h:1167-1205`), which are byte-stable across all six
   `fdet/ir*.ir` runs — but that finalized id is what the EMISSION reads,
   not what THIS comparator reads. The two are distinct; the comparator's
   run-stability rests on the VarId-derived source-lexical id, and a
   reviewer must check THAT invariant (source-lexical stability), not the
   `%col:N` byte-stability. If the fix locus were ever moved after
   `FinalizeColumnIDs`, the cited justification and the actual behavior
   would diverge — so the ordering constraint (`2596 < 2602`) is
   load-bearing and stated here.

   So `(Sort(), firstColId)` is a run-stable PARTIAL order — total on the
   committed corpus (measured: every merge_set has
   `distinct_hash == members`; see §5), but NOT total in general. The
   OWNER DECISION block below records the constructed counterexample and
   the open choice of a third, genuinely-unique final tie-break.

**[OWNER DECISION REQUIRED: the sort key `(Hash, first-col-id)` is not a
total order — pick the third tie-break key before landing the fix.]**

   A compiler-accepted program ties on BOTH `Hash` AND `first-col-id`,
   and the patch as written leaves it nondeterministic (`std::sort` is
   introsort — UNSTABLE — so the two tied elements keep their pre-sort
   pointer-bucket order). The counterexample (verified first-hand by the
   critique lane, probe at `:536`):

   ```
   #message edge(u64 A, u64 B).
   #local tc(u64 A, u64 B).
   #query out(free u64 A, free u64 B).
   tc(A,B) : edge(A,B).
   tc(A,B) : tc(A,X), edge(X,B).      ; right-recursive arm
   tc(A,B) : edge(A,X), tc(X,B).      ; left-recursive arm
   out(A,B) : tc(A,B).
   ```

   The two recursive arms are structurally symmetric, read the same base
   (`edge`), and produce the same output vars `(A,B)`. Probe:
   `members=3 distinct_keys=2 distinct_hash=2`, `DR_PROBE_TIE` fired —
   two members share both `Hash` and `first-col-id`. This is the ONLY
   merge_set in the program, so the patch does NOT stabilize it: measured
   2 distinct `-ir-out` hashes / 6 runs on the current binary AFTER the
   exact patch. Root cause of the collision: `HashInit` folds only
   (KindName, deletion-flags, column-count) and MERGE `Hash` XOR-folds
   arm hashes (`Merge.cpp:34,42-43`) — XOR is commutative AND
   self-cancelling, so symmetric arms actively collide.

   This is invisible to gates 1-4 (all corpus/witness-scoped): across all
   23 induction-bearing corpus cases, `distinct_hash == members` and
   cross-set `crossties == 0` — Hash alone is total on the committed
   corpus, so the tie-break is NEVER exercised today. The hole re-arms
   the exact bug this epoch closes the moment someone writes symmetric
   recursion.

   **The decision the owner must make: which third key.** The critique's
   first suggestion — `view->UniqueId()` (a DefList-assigned node id) —
   is DISQUALIFIED: I verified `DefList` (`include/drlojekyll/Util/DefUse.h`)
   assigns NO per-node sequence id (it is a `vector<unique_ptr<T>>` with
   no id field), and `QueryViewImpl` exposes no `UniqueId()`. There is no
   ready-made insertion-order node id to lean on. The candidate keys that
   ARE available, each needing an owner ruling:

   - **(a) A positional arm-content hash that folds the view's column
     VarIds in order** (the critique's own fallback). Distinguishes the
     two symmetric arms only if their per-column VarId sequences differ;
     for a genuinely symmetric `(A,B)`-over-`(A,B)` pair the top-level
     VarIds are identical, so this must fold the arm's INPUT column
     VarIds / predecessor structure, not just the output columns. Needs
     a proof that it separates the counterexample.
   - **(b) The existing `UpHash(depth)` upward-facing structural hash**
     (`View.cpp:419-441`) — folds each view's uses position-weighted.
     Distinguishes views by how they are CONSUMED. For two arms feeding
     the same MERGE the immediate up-use is symmetric, so `UpHash` at
     shallow depth may also collide; needs a depth/separation proof.
   - **(c) Mint a stable per-view sequence id at graph-build time** (a
     monotonic counter stamped in DefList insertion order at `Create`),
     making it the genuine total-order fallback the ledger names ("view
     id is the fallback key if Hash proves unsuitable",
     `KeyedInstances.md:206-207`). This is the most robust but touches
     the node-allocation surface.

   Until the owner rules, the patch in §1.2 stands as written (partial
   order, correct on the committed corpus) and MUST NOT be landed as
   "total by construction." The acceptance gate for whichever key is
   chosen is a NEW sub-gate (see gate 1b in §4): the counterexample above,
   committed as a directed determinism witness, must go 1 distinct `.ir`
   / N runs after the fix.

### 1.2 The patch

The change is confined to the `:518-537` block (the "label all merges
belonging to the same merge set" loop). Replace the direct
`for (auto &[view_, set] : merge_sets)` iteration with an iteration over
a sorted key vector built once. Concretely:

```diff
@@ lib/DataFlow/Induction.cpp  (the group_id / related_merges loop, ~:515-537)
   // We didn't inject any new UNIONs :-) Now we can label all the merges
   // belonging to the same merge set, and make all the merges in a set know
   // about all the other merges in that set.
+  //
+  // DETERMINISM (F): `merge_sets` is `unordered_map<VIEW *, ...>`, so its
+  // native iteration order is VIEW-pointer-bucket order and varies run to
+  // run (VIEW nodes are plain `new`).  Both outputs of this loop are
+  // order-sensitive and reach emission: `merge_set_id`/`group_id`
+  // assignment, and `related_merges`/`cyclic_views` population (that list
+  // is `InductiveSet()`, walked in `VectorFor` id-allocation order by the
+  // control-flow builder).  Drive the loop from a structurally-sorted key
+  // vector so both are canonical by construction.  Key: the view's content
+  // hash (`Sort()==Hash()`, pointer-independent, memoized) with the
+  // first-column-id as a total-order tie-break (column ids are assigned in
+  // deterministic `ForEachView` order by `FinalizeColumnIDs`).
   auto group_id = 0u;
 
-  for (auto &[view_, set] : merge_sets) {
-    VIEW *const view = view_;
+  std::vector<VIEW *> ordered_merge_views;
+  ordered_merge_views.reserve(merge_sets.size());
+  for (auto &[view_, set] : merge_sets) {
+    ordered_merge_views.push_back(view_);
+  }
+  std::sort(ordered_merge_views.begin(), ordered_merge_views.end(),
+            [](VIEW *a, VIEW *b) {
+              const auto a_key = a->Sort();
+              const auto b_key = b->Sort();
+              if (a_key != b_key) {
+                return a_key < b_key;
+              }
+              // Total-order tie-break: first column id (run-stable,
+              // assigned in ForEachView order by FinalizeColumnIDs).
+              assert(!a->columns.Empty() && !b->columns.Empty());
+              return (*a->columns.begin())->id < (*b->columns.begin())->id;
+            });
+
+  for (VIEW *const view : ordered_merge_views) {
     InductionInfo *const info = view->induction_info.get();
     if (!info) {
       continue;
     }
 
-    MergeSet *merge_set = set.FindAs<MergeSet>();
+    MergeSet *merge_set = merge_sets[view].FindAs<MergeSet>();
     if (!merge_set->related_merges) {
       merge_set->related_merges.reset(new WeakUseList<VIEW>(view));
       merge_set->merge_set_id = group_id;
       ++group_id;
     }
 
     info->merge_set_id = merge_set->merge_set_id;
     info->cyclic_views = merge_set->related_merges;
     merge_set->related_merges->AddUse(view);
   }
```

Notes on the hunk:

- **[AMENDED: CRITICAL — this comparator is a valid strict-weak-ordering
  but a PARTIAL order; a third final key is pending the OWNER DECISION in
  §1.1.]** As written (2-level, ties permitted) the comparator IS a valid
  strict-weak-ordering — ties are allowed by SWO — so `std::sort` is
  WELL-DEFINED here; there is no undefined behavior. **[AMENDED: NIT — do
  not conflate "not total" with "UB".]** The defect the CRITICAL finding
  identifies is non-determinism among EQUAL-key elements (introsort is
  unstable, so equal-key elements keep pre-sort pointer order), NOT
  std::sort UB. When the owner's third key lands, append it as the final
  `if`-level; the comparator then admits no ties and pins equal-key
  elements too.
- The sorted vector is built once from the map keys, then the loop body
  is otherwise **byte-identical** to the original (only the map lookup
  changes from the structured-binding `set` to `merge_sets[view]`). The
  union-find representative that `FindAs<MergeSet>()` returns is
  unchanged — sorting the *iteration* does not touch the disjoint-set
  forest, so which views share a `merge_set` is invariant; only the
  ORDER in which the shared list is filled and the ORDER of first-touch
  (hence `group_id` value) become canonical.
- Because the first view to touch a given `merge_set` now does so in
  sorted order, both `merge_set_id` (first-create at `:530`) and the
  `AddUse` sequence (`:536`) are canonical. This subsumes locus (ii)
  without a second pass over aliased lists.
- Column-emptiness: every VIEW reaching this loop is a MERGE/JOIN/NEGATE
  induction node with at least one output column, so the tie-break's
  `columns.begin()` is safe (asserted). The `columns.Empty()` asserts
  CANNOT fire — a JOIN asserts `input_columns` empty but always carries
  output columns; the assert is a documentation guard, not a live branch.

### 1.3 The secondary suspect and a full pointer-container sweep

Swept `Induction.cpp` for every pointer-ordered container and
classified each by whether its iteration order is emission-VALUE-visible:

| Site | Container | Order-visible? | Verdict |
|---|---|---|---|
| `:520` loop | `unordered_map<VIEW*> merge_sets` | **YES** — assigns `group_id` + fills `related_merges` (== `InductiveSet()`) | **THE FIX (this hunk)** |
| `:359` `for (VIEW *view : injection_sites)` | `std::set<VIEW*>` (`:128`) | Conditionally — mints new MERGEs (`merges.Create()`, `:379`) whose column/view ids follow mint order WHERE it fires | **Secondary, record-only — safety is `sites==1`, NOT non-firing. [AMENDED: MEDIUM — the original "does not fire" claim was factually wrong; corrected.]** Injection ACTUALLY FIRES (`changed=true`, `:378`) on FOUR corpus cases — `fibonacci_iterative`, `kcfa_tiny`, `kcfa_tiny_merged`, `transitive_closure5` (all in the §3.2 "WILL shift" set). The record-only safety is NOT that it stays dormant; it is that each fires with exactly `sites == 1` — a one-element `std::set` has no order to permute, so the new-MERGE mint order is trivially fixed. The earlier "no new-UNION churn observed" was true only for this reason, not because the path is dead. A program with ≥2 simultaneous injection sites WOULD re-introduce nondeterminism through the new MERGEs' node ids even after the `:520` fix, and no gate 1-5 would catch it (no corpus case has ≥2 sites; injection firing is asserted nowhere). Chosen remediation: **(b) an always-on guard** `assert(injection_sites.size() <= 1)` at `:378` so a future ≥2-site program trips LOUDLY instead of silently miscompiling — added to keep the (F) diff minimal while making the invariant enforced rather than merely observed. Option (a) (fold the same `(Sort(), firstColId, <owner-key>)` sort into the `:359` loop before minting) is strictly safer and CLOSES the second axis under the same "removes the landmine for free" reasoning used to prefer locus (i); it is deferred to the owner alongside the third-key decision, since it must use whatever total key the OWNER DECISION selects. |
| `:209`,`:325`,`:413`,`:448`,`:553`,`:628`,`:645` other `merge_sets` loops | `unordered_map<VIEW*>` | **NO** | Each sets per-view fields, accumulates into order-insensitive sets (`eventually_noninductive_successors`, `injection_sites` — both `std::set`), or builds `reached_ids` which is `std::sort`+`unique`'d at `:599-601`. Iteration order does not survive to emission. No change needed. |
| `:51`,`:70` `TransitivePredecessorsOf/SuccessorsOf` | `std::set<VIEW*>` (return) | NO | Used only for `.count()` membership tests, never iterated into an emission-ordered structure. No change. |
| `:109`,`:134` `std::set<pair<VIEW*,VIEW*>>` | pointer-pair-ordered | NO | `reached_inductions` drives union-find (`:195-203`, commutative — `DisjointSet::Union` is order-independent for the resulting partition); `eventually_noninductive_successors` seeds `injection_sites` (a `std::set`, dedup'd). No emission-order dependence. No change. |
| `:643` `unordered_map<ParsedClause, ...> bad_vars` | ParsedClause-keyed | NO | Feeds only the diagnostic loop `:685`; error text, not emission. And ParsedClause is source-position-keyed, not pointer-keyed. No change. |

Downstream (`lib/DR/*.cpp`) unordered_maps run AFTER vector ids are
assigned and are not implicated (consolidated.md §4). The fix is the
single `:520` hunk; the `:359` `std::set` is the lone recorded latent
sibling. **[AMENDED: MEDIUM — it DOES fire (4 cases, `sites==1`); safety
is the one-element invariant, not non-firing.]** It is guarded by the
always-on `assert(injection_sites.size() <= 1)` at `:378` (the second
axis is closed under a load-bearing assert, not left silent); folding the
same total sort into the `:359` loop is deferred to the OWNER DECISION
(it must use the chosen third key).

---

## 2. The canonical-order statement — what orders the emission

After the fix, here is what determines the order of each emitted
construct on the demand-ON witness path (`demand_tc_witness` under
`-demand`, and identically the flag-off inductive cases). The claim is
that every order below is a function of *structural content*, closing
the loop from source hash to emitted text with no pointer input.

| Emitted construct | Ordered by | Ultimately keyed to |
|---|---|---|
| **Merge-set numbering** (`merge_set_id` / `.ir` `set N`) | first-touch order of the `:520` loop | `(Sort()==Hash(), firstColId, <owner's 3rd key>)` sorted key vector — **the fix** (3rd key pending OWNER DECISION §1.1) |
| **`related_merges` / `cyclic_views` list** (== `InductiveSet()`) | `AddUse` order in the `:520` loop | same sorted key vector — **the fix** |
| **Induction member views** (`induction->views`, `Build/Induction.cpp:676`) | `for (auto other_view : view.InductiveSet())` (`:673`) | inherits `cyclic_views` order → the fix |
| **Induction vectors** (`$induction_in`, `$induction_pivots`, `$induction_swap`) | `proc->VectorFor(...)` call order inside the `:673` loop → `impl->next_id++` | inherits `InductiveSet()` order → the fix |
| **`^flow` proc parameter order** | the induction-vector list built above | inherits `InductiveSet()` order → the fix |
| **Sibling eager-drain / fixpoint-arm regions** (the `vec38`/`vec40` swap in `fdet/hdr.diff`; the two-column-JOIN arm vs the single-column direct arm in `fdet/ir1↔ir2`) | the per-member-view region built once per `InductiveSet()` element | inherits `InductiveSet()` order → the fix |
| **Column ids** (`%col:N`) | `FinalizeColumnIDs` → `ForEachView` per-kind `DefList` insertion order | already deterministic PRE-fix (byte-stable in all 6 `fdet` runs). NOTE: the emission reads `%col:N`, but the `:520` tie-break reads the EARLIER pre-`FinalizeColumnIDs` VarId-derived id — see §1.1 pt 3 [MEDIUM]; the tie-break does NOT ride on `%col:N` |
| **Table ids** (`%table:N`) | DataFlow node numbering / `DefList` order | already deterministic PRE-fix (byte-stable in all 6 `fdet` runs) |
| **Non-induction regions, join bodies, message handlers** | DataFlow depth-order / `DefList` order (`ForEachViewInDepthOrder` etc.) | already deterministic PRE-fix |

The single new dependency the fix introduces is the top two rows'
"sorted key vector," and the transitive closure of that key —
`Sort()==Hash()` (structural, memoized, pointer-free) with the
first-column-id tie-break — is closed under "no pointer input."
**[AMENDED: MEDIUM — the tie-break leans on the PRE-finalize VarId id,
not `%col:N`.]** The tie-break's `col->id` is the pre-`FinalizeColumnIDs`
VarId-derived id (source-lexical, run-stable via `ParsedVariable::Order()`
+ deterministic var-union), read inside `IdentifyInductions` (`:2596`)
BEFORE the `%col:N` renumber at `:2602` — so its run-stability comes from
source-lexical determinism, NOT from the `%col`/`%table` byte-stability
the repro artifacts show (that byte-stability is what the EMISSION reads
downstream; the two ids are distinct). **[AMENDED: CRITICAL — this key is
run-stable but PARTIAL, not total; a symmetric-recursion program ties on
it (see the OWNER DECISION in §1.1). The "pure function of content by
construction" statement below holds ONLY once the owner's third total
key lands.]**

**Reviewer's one-line belief statement:** *after the fix, the only
run-varying input to induction-vector numbering and sibling-region
order — the `unordered_map<VIEW*>` bucket order at `Induction.cpp:520`
— is replaced by a sort on `(content-hash, first-column-id, <owner's
third total key>)`, all of which are run-stable structural quantities;
therefore the emission is a pure function of the DataFlow graph's
content, by construction.* **[AMENDED: CRITICAL — the third key is
REQUIRED for this statement to be honest. With only `(hash, first-col-id)`
the order is total on the committed corpus but a symmetric-recursion
program ties and stays nondeterministic (§1.1 OWNER DECISION). The
belief statement is valid ONLY after the owner's third total-order key
lands.]*

---

## 3. Pre-registered blast-radius prediction (BY STRUCTURE, pre-fix)

The fix does not exist yet. This section is registered *before* the edit
so the shape-shift set can be checked against it after the fix lands.

### 3.1 What can shift

The fix imposes a NEW canonical order `(Hash, first-col-id)` on the
`:520` merge-set iteration. Any case whose *current* (pointer-bucket)
order for a merge-set already happens to equal the new canonical order
emits byte-identically; any case whose current order differs shifts
ONCE, to the canonical order, and then is frozen forever.

**Structural predictor:** a case can shift its emitted `.h`/`.ir`
**iff** it has a merge-set with **≥ 2 member views** (a multi-arm
induction) — only then is there an order to permute. A single-arm
induction (one view in its `cyclic_views`) has a one-element list, whose
order is trivially invariant. Cases with NO induction are untouched
(the loop never runs a non-trivial body).

### 3.2 The candidate universe (measured on the frozen baseline)

24 / 168 corpus cases are induction-bearing (emit at least one
`vector-define $induction`). Ranked by induction-vector count (a proxy
for merge-set arm count) with today's 6-run flag-off `.ir` hash spread:

| case | ind-vecs | 6-run flag-off hashes | shift class |
|---|---|---|---|
| `kcfa_tiny_merged` | 24 | **6** | WILL shift/stabilize (multi-arm, unstable today) |
| `kcfa_tiny` | 24 | **6** | WILL shift/stabilize |
| `cond_in_induction` | 21 | **6** | WILL shift/stabilize (ledger known-unstable) |
| `transitive_closure5` | 20 | **3** | WILL shift/stabilize |
| `transitive_closure2` | 12 | **2** | WILL shift/stabilize |
| `cond_in_induction_deep` | 12 | **4** | WILL shift/stabilize |
| `two_inductions` | 10 | **3** | WILL shift/stabilize |
| `cf14_2` | 10 | **3** | WILL shift/stabilize |
| `transitive_closure3` | 6 | **2** | WILL shift/stabilize |
| `transitive_closure_multiple_clause_bodies` | 6 | **2** | WILL shift/stabilize |
| `product_ind` | 6 | **2** | WILL shift/stabilize |
| `cf14_1` | 10 | 1 (but **3** in a separate batch) | MAY shift (stable-by-luck; ledger known-unstable) |
| `transitive_closure4` | 10 | 1 | MAY shift (multi-arm, stable-by-luck this batch) |
| `select_5` | 10 | 1 | MAY shift |
| `optimize_1` | 10 | 1 | MAY shift |
| `demand_tc_witness` (flag-off) | 10 | 1 | MAY shift flag-off; **WILL shift under `-demand`** (7/8 measured; 4 hash-distinct arms, up to 4! orderings) |
| `demand_multi_adorn_1` | 10 | 1 | MAY shift (it is a `-demand` REJECT — no emission under its drflags; flag-off emission may shift) |
| `cf13_4` | 10 | 1 | MAY shift |
| `transitive_closure_lazy` | 5 | 1 | MAY shift |
| `fibonacci_iterative` | 5 | 1 | MAY shift |
| `elim-cond-cycle-simple` | 5 | 1 | MAY shift |
| `deadflowelimination_5` | 5 | 1 | MAY shift |
| `transitive_closure` | 3 | 1 | MAY shift (likely single-arm — 3 vecs can be one JOIN arm) |
| `fibonacci` | 3 | 1 | MAY shift |

**The known-unstable pair the ledger names — `cf14_1` and
`cond_in_induction` — are in the MUST-shift/stabilize column** (a 6-run
batch shows cf14_1 at 3 distinct, cond_in_induction at 6 distinct;
allocation-batch variance is why one cf14_1 sweep read 1). They MUST go
to a single canonical shape after the fix.

Caveat on the "1-hash" rows: a 6-run reading of 1 distinct hash means
*stable in that batch*, NOT *equal to the post-fix canonical order*. A
multi-arm case can be pinned to a wrong-but-stable order by its fixed
`new`-call sequence and still shift once when the canonical sort lands.
So every multi-arm case above (≥2 real merge-set arms) is a legitimate
shape-shift candidate; the "MAY shift" rows are the ones whose current
frozen order is not yet known to equal canonical. The blast radius is
**bounded above by these 24 induction-bearing cases and by nothing
outside them** — no non-inductive case can change.

### 3.3 The expected stdout-golden churn: ZERO

The fix reorders *emission*, never *semantics*. Induction-vector
numbering, `cyclic_views` order, and sibling-region order are all
schedule/serialization choices over the SAME set of fixpoint arms
computing the SAME relations. The four optimization modes each still
byte-compare against the same stdout golden (CLAUDE.md golden policy),
and the derivation-counter oracle / monotone projection are semantic,
order-free. Therefore:

> **Predicted stdout-golden churn across all 168 cases: ZERO.**
> No `<name>.stdout`, `<name>.oracle.stdout`, or `<name>.monotone.stdout`
> changes. Only the (currently un-gated) emitted `.h`/`.ir` *shape* of
> the ≥2-arm induction cases moves, once, to canonical.

---

## 4. The acceptance gates (exactly)

The (F) fix is accepted iff ALL of the following hold. Gates 1, 1b, and 2
are the determinism proof (1b — the tie-break coverage gate — is
CONTINGENT on the §1.1 OWNER DECISION landing the third total key);
gate 3 is the semantic no-regression; gate 4 is the one-time shape-shift
bless; gate 5 is the durable gate the fix unlocks (ledger §0.5 directive
5 — the demand-ON IR goldens become *valid* only because this fix makes
them byte-stable).

1. **Demand-ON `.ir` + `.h` 20-run byte-stability on
   `demand_tc_witness`.** With its `-demand` drflags, 20 successive
   compiles of both the `-ir-out` ControlFlow dump AND the `-cpp-out`
   `datalog.h` each produce exactly ONE distinct hash / 20 runs.
   (Pre-fix baseline: `.ir` 7/8 under `-demand` — up to 4!-many orderings,
   4 hash-distinct arms; `.h` 10/20 — ledger §0's (F) repro. **[AMENDED:
   NIT — figure corrected from "3+/6" to the measured 7/8.]**)

1b. **[AMENDED: CRITICAL — new sub-gate; the tie-break path must be
   exercised, not merely asserted.]** **The symmetric-recursion
   counterexample (§1.1) as a committed determinism witness.** Once the
   owner's third total-order key lands (§1.1 OWNER DECISION), the
   `tietest` program — two structurally-symmetric recursive arms tying on
   `(Hash, first-col-id)` — is committed as a directed determinism witness
   and compiled N times: it must go 1 distinct `-ir-out` hash / N runs.
   This is the ONLY gate that exercises the tie-break path (gates 1-4
   cover only the Hash discriminator — §5). Pre-fix / partial-key
   baseline: 2 distinct `.ir` / 6 runs (the tie is unresolved by
   `(Hash, first-col-id)` alone). WITHOUT this gate the "total by
   construction" claim has zero coverage.

2. **Flag-off 12-run stability on `cf14_1` + `cond_in_induction`.**
   Each, compiled flag-off, yields ONE distinct `-ir-out` hash / 12
   runs (and one `.h` hash / 12). (Pre-fix baseline: cf14_1 3/12,
   cond_in_induction 11/12 — E-49.) This is the gate that proves the fix
   is in the SHARED machinery, not a demand-path patch: it must green
   two cases that never touch `-demand`.

3. **Suite 168 stdout zero churn.** `DR=<new-bin> runall.sh` ends
   `SUITE: PASS`, all 168 cases, every mode's stdout byte-equal to its
   committed golden, oracle + monotone sidecars unchanged. No
   `--bless` of any `.stdout`/`.oracle.stdout`/`.monotone.stdout`.

4. **The structural gate vs the frozen pre-fix snapshot.** For the
   shape-shift set (§3.2), the ONLY changes vs the frozen baseline
   `.h`/`.ir` are the induction-vector-id / sibling-region reorderings
   this fix causes — verified by the permutation-only referee
   (`permcheck.py`: published-delta / vec-id tokens compare order-free,
   all other lines byte-identical) against a snapshot taken from the
   frozen `baseline-bin`. No table-id, column-id, or non-induction
   region change is permitted. A change outside the induction machinery
   fails the gate (it would mean the fix perturbed something it must not).
   **[AMENDED: tightening per critique/ledger §1 — cite the 12-case
   floor.]** Additionally, per the ledger's stronger policy: any
   induction-bearing case OUTSIDE the ledger's named 12 known-unstable
   set that shifts shape gets a one-line written explanation appended
   before bless. §3.2 lists 24 candidates (a superset of the 12) — that
   superset is compatible with the ledger, but each of the non-12 that
   actually shifts must be explained, not merely permitted.

5. **The IR-golden sidecars `demand_tc_witness` then gains.** Per
   directive 5, once gates 1, 1b, 2-4 pass, `demand_tc_witness` is given its
   demand-ON `.ir`/`.h` (and, when the DeltaRel dump lands, its
   `.deltarel`) IR goldens — byte-compared by the harness, blessed via
   the standing `--bless` discipline. THIS FIX IS WHAT MAKES THEM VALID:
   before it, a demand-ON IR golden would be a coin-flip and could not
   be committed. The same sidecar mechanism re-arms IR goldens for the
   ≥2-arm flag-off cases (`cf14_1`, `cond_in_induction`, the
   `transitive_closure*` family) that were previously un-gate-able.

---

## 5. Evidence appendix — what was measured on the frozen baseline

All runs on `baseline-bin/drlojekyll.debug.60821adf` (the epoch-start
frozen bits). No repo file modified. This is the pre-fix ground truth
the post-fix gates diff against.

- **`-demand -ir-out demand_tc_witness`: up to 4!-many orderings, 7
  distinct / 8 runs measured** (UNSTABLE — contradicts the ledger's
  original ".ir stable"; the ledger E-47 figure was 3/8, but a re-sweep
  on the current binary reads 7/8). **[AMENDED: NIT — figure corrected
  from the artifact's earlier "3/6" to the measured 7/8.]** The
  program's merge_set has 4 hash-distinct members (probe:
  `members=4 distinct_keys=4`), so up to 4! orderings are possible and
  7/8 is expected. The fix pins all 4 by Hash ALONE — the tie-break is
  not needed here (all four arms are hash-distinct).
- **Flag-off induction-bearing corpus, 6-run `.ir` hash spread**
  (the §3.2 table): the multi-arm cases `kcfa_tiny{,_merged}` (6/6),
  `cond_in_induction` (6/6), `transitive_closure5` (3/6),
  `cond_in_induction_deep` (4/6), `transitive_closure2` (2/6),
  `two_inductions` (3/6), `cf14_2` (3/6),
  `transitive_closure3` (2/6),
  `transitive_closure_multiple_clause_bodies` (2/6),
  `product_ind` (2/6) are NONDETERMINISTIC FLAG-OFF today — direct
  confirmation that this is a pre-existing latent bug, not a demand
  regression (E-49 generalized: the instability is broad, not limited
  to the two cases the ledger names).
- **`cf14_1` flag-off: 3 distinct / 12** in one batch, 1/6 in another —
  allocation-batch-sensitive, which is exactly the ASLR/`new`-order
  signature of the `unordered_map<VIEW*>` bucket defect.
- **24 / 168 cases are induction-bearing** — the hard upper bound on the
  blast radius. 144 cases have no induction and cannot change.
- **[AMENDED: MEDIUM — the tie-break is DEAD WEIGHT on the current
  corpus; stated honestly.]** Across all 23 induction-bearing cases with
  a merge_set, every merge_set has `distinct_hash == members` and
  cross-set `crossties == 0` — **Hash ALONE is total on the committed
  corpus**, including the ir1↔ir2 witness (its two arms are hash-distinct
  `<u64>` vs `<u64,u64>`). Therefore gates 1-4 exercise ONLY the Hash
  discriminator and give the first-column-id tie-break — and any third
  key the owner adds — ZERO coverage. The tie-break path is currently
  UNTESTED by the acceptance gates; combined with the CRITICAL finding
  (it is also WRONG in the one symmetric-recursion case it targets),
  the ONLY way the "total by construction" claim earns coverage is to
  commit the §1.1 counterexample as a directed determinism witness and
  gate it (gate 1b, §4).
- **Mechanism, confirmed by first-hand read:** `HashInit`
  (`View.cpp:405-415`) and MERGE `Hash` (`Merge.cpp:25-48`) are purely
  structural (no pointer/id); `FinalizeColumnIDs` (`Columns.cpp:13-25`)
  walks `ForEachView` (`Query.h:1167-1205`) per-kind `DefList` insertion
  order (deterministic); the `%col`/`%table` ids are byte-identical
  across all six `fdet/ir*.ir` runs (only `$induction_*` vector ids
  permute). These three facts are what make the fix's sort key
  `(Hash, first-col-id)` run-stable. **[AMENDED: CRITICAL/MEDIUM — it is
  run-stable and total ON THE COMMITTED CORPUS, but NOT total in general
  (symmetric recursion ties, §1.1 OWNER DECISION); and the tie-break
  reads the pre-`FinalizeColumnIDs` VarId-derived id, so its stability
  comes from source-lexical determinism, not from the `%col:N`
  byte-stability cited just above (§1.1 pt 3 [MEDIUM]).]**

### The observed diff, pinned to the layer

`fdet/ir1 ↔ ir2` (two `-demand` runs of `demand_tc_witness`): the
single-column direct-recursion arm `$induction_in:33<u64>` and the
two-column JOIN arm `$induction_in:33<u64,u64>` **trade** ids 33/35, and
the entire fixpoint body reorders which arm emits first (the
`update-count +recursive` block and the `hash into @NN` /
`vector-append` regions swap). `fdet/hdr.diff` (`datalog.h`): the SAME
swap — the `vec38`/`vec40` drain loops (a JOIN over `table_15`/`idx_89`
vs a JOIN over `table_12`) exchange which vector they drain, and
`vec33`/`vec37`/`vec39` numbering follows. `%col`/`%table`/`v<N>` value
ids are identical across the two runs. This is the isolated signature of
induction-vector-id + sibling-region-order permutation and nothing else
— precisely what the `:520` fix pins.

Crucially, the two swapping arms are STRUCTURALLY DISTINCT (single-column
`<u64>` vs two-column `<u64,u64>`): their `HashInit` differs
(`columns.Size()` is folded in), so `Sort()==Hash()` alone already
discriminates them and the fix gives them a fixed relative order — the
tie-break is only needed for genuinely hash-equal arms (e.g. two
same-shape recursive clauses). **[AMENDED: CRITICAL — the
first-column-id does NOT always resolve hash-equal arms.]** For the
truly symmetric case (two same-shape recursive clauses over the same
base producing the same output vars — the §1.1 counterexample), the arms
tie on BOTH `Hash` AND `first-col-id`, and the tie-break FAILS to resolve
them; `std::sort` (unstable introsort) then leaves them in pointer-bucket
order and the program stays nondeterministic. This is exactly why the
third total-order key (§1.1 OWNER DECISION) is required — the observed
ir1↔ir2 witness is discriminated by Hash alone and is NOT evidence that
the tie-break works, because on this corpus the tie-break never fires
(§5, tie-break-dead-weight [MEDIUM]).
