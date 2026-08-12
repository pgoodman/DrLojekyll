I have decisive material. Compiling findings.

## REFUTER #2 — Coverage & Emission Soundness: findings

The flat grove does **not** survive V-IF-COVERAGE / V-IF-EMISSION as framed. Three of the five attack vectors yield concrete violations; the headline one fires on the `transitive_closure` witness the design itself nominates.

---

### F1 — BLOCKING. Authority-per-use is undefined (and coverage is multiply-counted) whenever a shared subexpression feeds ≥2 INSERTs. Fires on `transitive_closure`.

**The design says**, for *every* use: `UseCoverage.authority = "the terminal INSERT reached along its chain"` (§3 model + §3.1 step 5 + §5 V-IF-EMISSION "each coverage record's authority resolves to a live authority whose site's collection matches **the use's terminal collection**" — singular). The join_1 hand-derivation only works because `q` and `never` have *disjoint* downstream chains.

**The graph refutes the singular assumption.** From the real optimized dump (`build/debug/bin/drlojekyll tests/OptDiff/cases/transitive_closure.dr -df-out`), `merge.11` (the `tc` model) fans out to six consumers:

```
merge ^merge.11 (From,To)  ; callers: ^tuple.1, ^tuple.9
  => ^tuple.2 (cycle→join.10)      => ^tuple.3 (cycle→join.10)
  => ^tuple.4 (→merge.12→insert.15)  => ^tuple.5 (→merge.12→insert.15)
  => ^tuple.6 (→insert.13)           => ^tuple.7 (→insert.14)
insert ^insert.13 into %table:4   insert ^insert.14 into %table:4   insert ^insert.15 into %table:8
```

Every use **upstream of `merge.11`'s fan-out** — e.g. the SCC-internal `use(consumer=merge.11, producer=tuple.1)` and `use(consumer=join.10, producer=tuple.2/tuple.3)` — has a downstream chain that reaches **three** terminal INSERTs across **two** collections (`tc` via 13+14, `is_node` via 15). The function `use → single authority` therefore does not exist, and `UseCoverage.authority` (a single `EmissionAuthorityId` field, `InstanceFlow.h` struct in §3) cannot represent it.

Both readings of V-IF-COVERAGE's "exactly once" break:
- **use-drives-authority**: the builder must pick 1 of 3 terminals — non-deterministic, and V-IF-EMISSION's "site's collection matches the use's terminal collection" has no single collection to match.
- **authority-drives-coverage** (each INSERT claims its backward cone): the SCC-internal uses are claimed by `insert.13`, `insert.14` **and** `insert.15` → **covered 3×**, a direct violation of "every OriginUse domain covered EXACTLY once."

**Anchor**: `lib/DataFlow/Query.cpp:1204-1223` (`QueryMerge::ForEachUse` — the fan-out node); dump above. This is not exotic: it is *every* shared/CSE'd subexpression feeding more than one collection; the recursive SCC guarantees it in `transitive_closure`.

**FIX**: Drop the "single terminal authority per use" model. Authority must be defined **only** on root/terminal uses (the `out==nullopt` INSERT boundary uses), and V-IF-EMISSION reduced to "bijection `authorities ↔ sites`, each writer an INSERT origin" *without* the "coverage record's authority resolves to the use's terminal collection" clause. Non-root uses get **no** authority field (or a set). V-IF-COVERAGE must be reframed as coverage of *consumer obligations* (each `(consumer, producer_col, role)` edge covered once by exactly one FamilyNode occurrence), fully decoupled from emission authority.

---

### F2 — BLOCKING. Live dead-end views generate consumer-side uses with **no reachable terminal INSERT** → V-IF-EMISSION cannot resolve an authority. Fires on `join_1`. The "orphan ⇒ vacuously covered" claim (IF4 stress #2) is wrong.

IF4 asserts `compare.14`/`compare.15` "have zero live consumers — orphan origins [that] trivially satisfy V-IF-COVERAGE (empty use-set)." That only inspects *outgoing* uses. Confirmed from the dump they are **live** (not `is_dead`) with **zero successors**:

```
compare ^compare.14 (c22:i32) ; eq   tag=build/constant-assign-guard   [no => lines]
compare ^compare.15 (c23:i32) ; eq   tag=build/constant-assign-guard   [no => lines]
```

But the builder walks *every* live view as a **consumer** (`for_each_view × ForEachUse`, §2.2 item 4). `QueryCompare::ForEachUse` (`lib/DataFlow/Query.cpp:1369+`) emits input uses for `compare.14` reading `select.1` and its constant. So `use(consumer=compare.14, producer=select.1)` **exists**, is non-orphan, and its downstream chain terminates at **nothing**. The design mandates a coverage record with an authority = "terminal INSERT reached along its chain" — there is none. V-IF-EMISSION ("each coverage record's authority resolves to a live authority") aborts, or the builder must silently special-case dead-end uses (undocumented in §3/§5).

**Anchor**: `lib/DataFlow/Query.cpp:481-525` (walker visits all kinds incl. dead-ends); dump above. `compare.14/15` are exactly the "unsat-but-uneliminated" residue the `@never` arm leaves — a per-witness, not hypothetical, hit.

**FIX**: Decouple coverage from authority (as F1). A dead-end use is a legitimately covered consumer obligation with **no** emission authority; V-IF-EMISSION must not require every use to reach an INSERT. Correct IF4's premise: orphan-ness is about the *incoming* (fan-out) set; a dead-end producer still has outgoing consumer-side uses.

---

### F3 — MAJOR. The canonical use-ordering key `(consumer det_seq, producer_col index, role)` is **not a total order** → non-deterministic dump / golden instability, and an undercount if reused as a dedup key.

`QuerySelect::ForEachUse` (`lib/DataFlow/Query.cpp:757-777`) loops **`impl->inserts`** and, for each live INSERT, emits `with_col(insert->input_columns[i], kCopied, select->columns[i])`. For a relation with **≥2 live INSERTs** (e.g. `tc`, which has `insert.13` + `insert.14`), a SELECT on it emits, for column `i`, two uses with **identical** `(consumer=select, producer_col=i, role=kCopied)` but **different producer views**. The §2.2 canonical key omits the producer `QueryOriginId`, so these tie: the "no `unordered_map` iteration is ever emitted / order is a pure function" determinism guarantee (§2.2) fails, and the `-instanceflow-out` golden is unstable. Worse, if any code keys OriginUse identity on that tuple (the design conflates the sort key with per-use identity), the two materialization edges collapse to one → an INSERT's read is **dropped** → V-IF-COVERAGE undercount.

**Anchor**: `lib/DataFlow/Query.cpp:757-777`; the multi-INSERT relation shape is corpus-real (`tc` above; any multi-rule non-recursive `#local` re-read by a SELECT).

**FIX**: Include the producer `QueryOriginId` (and `out_col` index) in the canonical key so it is a strict total order and a faithful per-edge identity.

---

### F4 — MAJOR. INSERT-with-successors double-represents the materialization edge (insert-side `kMaterialized` + select/merge-side `kCopied`), yielding two OriginUses for one physical column flow with conflicting authorities.

`QueryInsert::ForEachUse` relation-with-successors arm (`lib/DataFlow/Query.cpp:1571-1581`) emits `use(consumer=INSERT, in=input_columns[out->index], out=successor.columns[k], role=kMaterialized)` for each MERGE/SELECT successor. Independently, that successor's own `ForEachUse` emits `use(consumer=successor, in=insert->input_columns[i], role=kCopied)` (`QuerySelect::ForEachUse` reaches through `impl->inserts`). One materialization is thus modeled from **both** directions. The insert-side use terminates *at* the INSERT (authority = that INSERT); the select-side use flows onward (authority = wherever the successor goes). Under F1's "authority per use" this is a benign redundancy only if coverage is consumer-keyed; under the design's terminal-chain authority it is an inconsistent double-authority for one data flow.

Not exercised by the 3 nominated witnesses (their INSERTs are all terminal into distinct query tables), but it is the standard materialize-then-reread shape (any derived `#local`/`#export` consumed downstream). **Residual risk: untested at Phase A landing.**

**Anchor**: `lib/DataFlow/Query.cpp:1571-1590` vs `757-777`.

**FIX**: Pick one canonical direction for the materialization edge (recommend the consumer/SELECT side; treat the INSERT-successor arm as a root/boundary obligation only when `out==nullopt`), and document it in §8 so coverage counts each physical flow once.

---

### F5 — MINOR / residual. Condition (zero-arity/unit) relations (§14.9): `by_decl_id[ins.Declaration().Id()]` is a map lookup that must not miss, and a zero-column INSERT is a DerivationSite/authority with no column uses.

A condition relation surfaces as a real `QueryRelation` with a synthesized decl (`relation pred_18446742974197924863/0`, `tag=build/condition-select`, seen in `barrier_neck_1`) and a real `insert.21 (c13:bool) into %table:4`. So the LC interner (walking `query.Relations()`) plausibly covers it and `insert.Declaration().Id()` resolves — **but the design uses `p->collections.by_decl_id[ins.Declaration().Id()]` (§3.1 step 3) as a bare `operator[]`**: a miss silently default-constructs `LogicalCollectionId{0}`, **aliasing collection #0** and corrupting `DerivationSiteCatalog.collection` without tripping any validator. This is unverified for every synthesized-relation path (conditions, fabricated demand relations if `-demand` ever composes).

**Anchor**: `lib/DataFlow/Build.cpp:611,1414` (condition relations are real); §3.1 step 3 uses unchecked `operator[]`.

**FIX**: Replace `by_decl_id[...]` with a checked `.at(...)` / find-or-abort in a V-IF-ORIGIN-adjacent belt, and add a condition-bearing case (`barrier_neck_1`) to the golden set before claiming corpus totality.

---

### Vector explicitly **not** refuted
- **merge_2 shared origins** (`select.1 → {tuple.4, tuple.7}`, `select.2 → {tuple.3, tuple.8}`): confirmed sound. Because uses are keyed by **consumer**, a shared producer yields two distinct consumer-keyed uses with **disjoint** downstream chains (tuple.4→insert.13 `proj`; tuple.7→insert.12 `outer`), so each resolves to exactly one authority and is covered once. This is the one case the "maximal sharing" claim genuinely holds — the fork is *at* the origin, not downstream of a use (contrast F1, where the fork is downstream).
- **@inline elision** (`inner` in merge_2): sound — `inner` is gone before the pass runs and nothing references a non-existent view, so no dangling use. Residual risk only if a future partial-inline leaves a half-elided consumer.

---

**VERDICT: The flat slice does NOT survive as framed** — F1 (authority undefined + coverage multiply-counted under downstream fan-out to ≥2 INSERTs) fires directly on the `transitive_closure` witness, and F2 (dead-end live views have no resolvable authority) fires on `join_1`; both are structural, not tuning. The fix is the same for both: **decouple V-IF-COVERAGE (consumer-obligation, per-edge) from V-IF-EMISSION (authority defined only on terminal/root uses, bijection onto sites)** — do not tie non-root use coverage to a single terminal INSERT. F3/F4/F5 must also land for a stable, corpus-total dump.