# P6 grounding — recursive regional execution (compile-time first cut: P6.1)

> **P6.1 LANDED (session 21, 2026-08-09).** Executed the v2 design (§6). `RecursiveComponent`
> gains `std::vector<RelationId> members`; `ComputeRecursiveComponents` (Planning.cpp, the SOLE
> populator) projects the DataFlow multi-view-stratum SCCs onto the frozen relations via
> `OriginDecls`; `Format.cpp` renders a gated own-width `recursive-component  C<k>  members=(…)`
> block (NO census count — P5 precedent). NEW carrier `corecursion_1` (co-recursion,
> `members=(ping, pong)`); `two_inductions`/`recursion` gained region `.irgold`s (two-independent-
> components + the mode-faithful 0-vs-5 witness); `tc_nonlinear_diff` region goldens moved by ONE
> line (`members=(tc)`). Gate GREEN: OptDiff **SUITE: PASS**, ctest **5/5**; 16 region goldens
> blessed; all `.rel`/`.h`/`.stdout`/behavioral/oracle + the 16 unrelated region goldens
> BYTE-IDENTICAL (codegen unchanged). P6.2 (rules/SymbolicFieldId) deferred to its own cut.

Session 21 (2026-08-09). Branch `keyed-instances`, tip `375e8713` (POST-P5). This is the
grounding+design record for **P6.1** — the first compile-time slice of P6. Method: the landed
grounding loop (ground the whole program → design-goal diffs → adversarial opus critique →
IR desired states), then owner-gated execution.

## §0. Owner decisions (session 21 open)

- **P6 scope = COMPILE-TIME first cut** (P3/P4/P5 cadence): P6.1 populates
  `RegionTemplate.recursive_components` from a query-independent SCC analysis; codegen
  BYTE-UNCHANGED; the M3 full-materialization backend still evaluates. Runtime evaluation
  (P6.5 EvaluateEpoch / per-fact DRed / cyclic activation) is a later, separately-gated cut.
- **Session mode = GROUND, THEN EXECUTE**: run the loop, and if the critique clears with 0
  blocking defects, LAND the code with the structural gate green.
- **Commit split (this doc's recommendation, settled by grounding — see §2.1):** with the
  Stratify-projection arm, `ComputeRecursiveComponents` reads `view->stratum` directly and
  does NOT need `rules`. So **P6.1 lands INDEPENDENTLY of P6.2** (co-recursion detection needs
  no field routing). Recommendation: **land P6.1 this session; P6.2 (rules / SymbolicFieldId
  promotion) is its own subsequent grounded cut** (§4). This honors "compile-time first cut"
  (P6.1 is that cut) while keeping the structural-gate discipline tight.

## §1. Grounding — anchors verified at tip `375e8713` (3 read-only agents)

### §1.1 The RESERVED-EMPTY P6 surface (CONFIRMED, with one drift)
- `Regional.h:155-160`: `struct RuleRoutingProjection {};` (P6.2 sole populator) and
  `struct RecursiveComponent {};` (P6.1 sole populator) — genuinely EMPTY stub structs,
  defined nowhere else.
- `Regional.h:167` `inherited_symbolic_fields` (empty), `:175` `rules` (empty), `:176`
  `recursive_components` (empty) — populated by NOBODY (grep-verified: only the decl +
  `Planning.cpp:469` comment).
- `RegionInstance.h:417` `activation_edges` reserved-empty; `RootedReachability` (`:492-507`)
  is a single topo sweep, `activation_edges is empty at P3 → no propagation`.
- `AddDerivation` (`:475`) / `RouteResults` (`:512`) have ZERO real-compile callers (only
  `tests/RegionInstance/RegionInstanceTest.cpp`). **DRIFT:** `DeriveActivationEdge` does NOT
  exist as a method — it is a prose comment (`:373-374`) reserved for a later (runtime) cut.
  Irrelevant to P6.1's scope.

### §1.2 Stratify — the SCC authority for the projection (CONFIRMED, F7 settled)
- `lib/DataFlow/Stratify.cpp:124` `QueryImpl::Stratify` is an **iterative Tarjan SCC
  condensation**; `views[i]->stratum = state[i].stratum` (`:237-240`); all views popped in one
  SCC get the SAME `next_stratum` (`:217-229`). Doc (`:38-42`): "a stratum is one SCC, so a
  recursive fixpoint (an induction) is exactly a multi-view stratum."
- Public accessor `QueryView::Stratum()` → `std::optional<unsigned>` at
  **`include/drlojekyll/DataFlow/Query.h:367`** (drift: NOT `lib/DataFlow/Query.h`), defined
  `lib/DataFlow/Query.cpp:254`. Doc: "Two views share a stratum id iff they are in the same
  SCC … a recursive fixpoint is exactly a multi-view stratum."
- **F7 (message-mediated recursion) is CLOSED by Stratify**: `INSERT→SELECT` decl seams —
  including message publish→receive pairs — are pushed straight into the `sources[]` adjacency
  the Tarjan walk consumes (`ForEachInsertToSelectSeam`, Stratify.cpp:158-172). So a projection
  off `view->stratum` sees message-mediated recursion. **`IdentifyInductions`/`InductionGroupId`
  is BLIND to seams** (the `stratum_has_io_seam` special-case at `:356-361` exists precisely
  for this). ⇒ **P6.1 MUST project from `Stratum()`, never `InductionGroupId()`.**
- Reusable relation-level projection ALREADY exists (private): `EquivalenceSet::stratum := max
  stratum over member views` (`Stratify.cpp:242-264`) + `QueryImpl::stratum_straddling_models`.
- Two SCC notions coexist: DataFlow `stratum` (query-independent, seam-closing — the P6.1
  authority) vs ControlFlow `RecursiveSccMap` by `InductionGroupId` (`Stratum.cpp:185-243`,
  a downstream evaluation grouping). P6.1 uses the DataFlow `stratum`.

### §1.3 Baseline dumps + census (CONFIRMED)
- `RegionalCensus` (Regional.h) = 7 counts: regions, child_calls, program_roots, request_ports,
  input_ports, result_ports, row_contracts. `DeriveRegionalCensus(query)` is a function of
  `query` (Planning.cpp:294-313); the H2 recount belt (Planning.cpp:477-502) re-counts the
  built `R` against it. Census render: `Format.cpp:382-388`.
- Render blocks: row-contract `Format.cpp:337-358`; the P5 `declared-key` block (gated, own
  width, "no golden moves for unkeyed programs") `:368-376`.
- `tc_nonlinear_diff.dr` = self-recursive TC; row-contracts E0=reachable, E1=edge, E2=tc (all
  `support=differential`); has `.irgold` + 4 committed `.region` goldens. **The natural
  self-recursion carrier.** Its `tc` is the recursive relation.
- 20 committed `.region` goldens: booleans, join_1, key_partial_1, merge_2, tc_nonlinear_diff
  (× 4 modes). Only tc_nonlinear_diff is recursive.
- **No co-recursion carrier exists** — P6.1 must author one.

## §2. P6.1 design — project the view-SCC onto relations

**Touches:** the recursive-analysis half of the model (`recursive_components`). Does NOT
fabricate routing/symbolic-field authorities (P6.2) or any physical/runtime structure.
**Codegen BYTE-UNCHANGED** (compile-time model + render only — the P3/P5 discipline).

### §2.1 The projection (public-API, query-independent)
```
RecursiveComponent { std::vector<RelationId> members; }   # replace the empty stub

ComputeRecursiveComponents(query, relation_schemas) -> vector<RecursiveComponent>:
    # (1) recursive strata = MULTI-VIEW strata (Stratify doc: multi-view stratum ⟺ recursion).
    views_per_stratum : map<unsigned,unsigned>
    query.ForEachView(V):  if s = V.Stratum():  views_per_stratum[s] += 1
    recursive(s) := views_per_stratum[s] > 1

    # (2) map each relation-schema to the stratum of its PRODUCING view; keep only recursive.
    #     Producer = the view that MATERIALIZES the relation's rows (in the SCC via the
    #     INSERT→SELECT seam), NOT a downstream consumer and NOT an upstream base whose rows
    #     merely flow through (the OriginDecls-membership over-inclusion trap — §3-Q1).
    bucket : ordered_map<unsigned, set<RelationId>>       # stratum -> relations
    for schema in relation_schemas:
        s = ProducerStratumOf(query, schema)              # §2.2
        if s and recursive(*s):  bucket[*s].insert(schema.id)

    # (3) one component per recursive stratum bucket, members sorted, components stratum-ordered.
    return [ RecursiveComponent{sorted(members)} for (s, members) in bucket ]   # ascending s
```
This is **query-independent by construction**: `Stratum()` is assigned in `Query::Build`
(Stratify), before any `#query` is consulted; queries never introduce dataflow cycles. Adding
or removing a `#query` cannot change `views_per_stratum` or any producer stratum.

### §2.2 ProducerStratumOf — the load-bearing mapping (the §3 critique target)
Two arms, mirroring the two schema-build arms (CollectContractInserts / CollectOriginInteriorDecls):
- **Insert-materialized (R-STORE):** the producing view is the relation's INSERT view. tc's
  INSERT is in tc's recursive SCC via the `INSERT→SELECT` seam (§1.2), so
  `QueryView(ins).Stratum()` IS the recursive stratum. → carry the `ins` view (already held by
  `CollectContractInserts`) onto the schema (or re-resolve) and read its `Stratum()`.
- **Tier-2 origin interior:** no INSERT. Producer = the highest-stratum LIVE carrier view whose
  `OriginDecls()` contains `decl` AND whose stratum is recursive **and native** — i.e. resolve
  as `ResolveOriginSupport` does (iterate carriers), but take the carrier whose own stratum is
  multi-view and that is not a strict downstream consumer. CONSERVATIVE first-cut rule
  (§3-Q2): if no such native recursive carrier is unambiguous, leave the Tier-2 relation OUT of
  a component (under-approximate) — the DEBUG cross-check belt (§2.4) catches any disagreement
  with the model-condensation projection, converting a silent miss into a tripwire. Every
  known recursion carrier in the corpus (tc self-recursion, the new co-recursion witness) is
  insert-materialized, so the conservative Tier-2 arm loses no corpus coverage today.

To hold the producing view for the insert arm, extend `RelationSchema` with a
`std::optional<unsigned> producer_stratum` computed at build (or thread the `ins`/carrier view
into `ComputeRecursiveComponents`). Prefer computing `producer_stratum` inside the existing
schema-build arms (they already hold the view) and reading it in ComputeRecursiveComponents —
keeps the projection a pure read over `R.relation_schemas`.

### §2.3 Census + render
- **Census:** add an 8th count `recursive_components` to `RegionalCensus`.
  `DeriveRegionalCensus(query)` computes it INDEPENDENTLY (calls `ComputeRecursiveComponents(...)
  .size()` over freshly-collected schemas) — the H2 anti-stub authority. The Build recount belt
  (`check_count`) gains `check_count("recursive-components", R.recursive_components.size(),
  census.recursive_components)` — a Build arm that forgets to populate the vector aborts.
- **Census render** (`Format.cpp:388`): append ` recursive-components=<N>` to the census line.
  → moves ALL 20 existing region goldens (uniform token append; the request-ports/row-contracts
  skeleton-count precedent, NOT the P5 declared-key no-census precedent — a recursive-SCC count
  is a first-class structural skeleton count).
- **Render block** (new, after the declared-key block, `Format.cpp:376`): gated on non-empty
  `recursive_components`, own width block (unkeyed/non-recursive programs emit nothing):
  ```
  recursive-component  C<k>  members=(rel, rel, …)
  ```
  Members rendered by relation NAME in the sorted-members order; `C<k>` = component index
  (stratum-ascending). Only tc_nonlinear_diff (among existing goldens) gains a block.

### §2.4 Belts
- **Recount belt** (H2): `R.recursive_components.size() == census.recursive_components`, abort
  on mismatch (added to the existing `check_count` battery).
- **DEBUG cross-check** (F19 positive assertion): re-derive the partition a SECOND way — group
  relations by a model-condensation projection — and assert the two partitions agree on WHICH
  relations are mutually recursive. Advisory (`#ifndef NDEBUG`), a tripwire for the §2.2 Tier-2
  conservative arm.

### §2.5 The discriminating gate (structural — a stub FAILS)
- **NEW carrier `corecursion_1.dr`** — mutual recursion, **NO `#query`** (publishes via a
  message), so detection is proven query-free:
  ```datalog
  #message add_edge(u64 From, u64 To).
  #message reachable_pair(u64 From, u64 To).
  #local ping(u64 A, u64 B).
  #local pong(u64 A, u64 B).
  ping(A, B) : add_edge(A, B).
  pong(A, B) : ping(A, B).
  ping(A, B) : pong(A, X), add_edge(X, B).      # mutual recursion: ping ← pong, pong ← ping
  reachable_pair(A, B) : ping(A, B).            # publish (no #query)
  ```
  `.irgold region` sidecar → 4 `.region` goldens pinning
  `recursive-component  C0  members=(ping, pong)` + `recursive-components=1`. A stub leaving
  `recursive_components` empty → no block + `recursive-components=0` → golden mismatch → FAIL.
  One component spanning the cycle (not two singletons) is the positive assertion.
- **tc_nonlinear_diff** (self-recursion): its 4 region goldens gain
  `recursive-component  C0  members=(tc)` — a second real-compile witness (self-recursion is a
  size-1 members list over a multi-view stratum).
- **Query-independence pin:** a sibling `corecursion_1_q.dr` = corecursion_1 + a bound
  `#query reach_q(bound A, free B) : ping(A, B).` (distinct name — a `#query` cannot redeclare a
  `#local`). Its `recursive-component` line must be byte-identical to corecursion_1's (only the
  request-port line differs). Enforced by a small check (ctest or a scripted grep-compare of the
  `recursive-component` lines across the two goldens).
- Extend the `RegionInstance` ctest with a `RecursiveComponentsP6` battery if
  `ComputeRecursiveComponents` can be exercised on a real compiled Query fixture; otherwise the
  golden + recount belt + query-independence pin ARE the discriminating gate (the projection
  needs a full Query, so a pure-unit test is not the natural anchor here — unlike P5's id-free
  render).
- **Codegen byte-stable:** `.rel` / `datalog.h` / `datalog.cpp` / `.stdout` / behavioral /
  oracle goldens BYTE-IDENTICAL for every existing case (predict-then-verify; pin this like P5).

## §3. Open sub-questions the critique must settle
- **Q1 (OriginDecls over-inclusion):** does `ProducerStratumOf` correctly EXCLUDE a base
  relation (`edge`/`add_edge`) whose rows flow THROUGH the recursive JOIN's OriginDecls? The
  insert-arm (INSERT view stratum) avoids this because edge's INSERT is a lower seed stratum —
  verify edge's insert view is NOT in tc's stratum.
- **Q2 (Tier-2 producer resolution):** is the conservative "native recursive carrier or omit"
  rule sound, and does the DEBUG cross-check actually catch a miss? Can a Tier-2 relation recurse
  in the corpus (if not, is the arm dead code that should abort-if-reached instead)?
- **Q3 (self-recursion detection):** is a single self-recursive relation ALWAYS a multi-view
  stratum (so `views_per_stratum>1` catches it)? Verify tc's SCC has ≥2 views
  (MERGE+JOIN+SELECT+INSERT). Is there any recursive shape that is a size-1 view SCC (a view
  with a literal self-edge)? If so, `views_per_stratum>1` misses it — need a self-edge check.
- **Q4 (census vs no-census):** is adding `recursive_components` to the census (moving all 20
  goldens) the right call vs the P5 declared-key no-census precedent? (§2.3 argues yes — it is a
  skeleton count.) Confirm the recount belt is not tautological (census + Build both call the
  same pure fn — the anti-stub value is that Build STORES and a stubbed store is caught).
- **Q5 (member ordering / determinism):** members sorted by RelationId vs by name vs by decl
  first-encounter? Must be a pure function of the graph (HP-9). Component ordering by ascending
  stratum id — is stratum id itself mode-stable across the 4 opt modes? (Optimize changes the
  view set → stratum ids may renumber; the region golden is pinned PER-MODE, so per-mode
  stability suffices, but the MEMBERS set must be mode-invariant.)
- **Q6 (false starts):** does this avoid "implement recursive keyed regions by extending only
  InstanceStore" (N/A — no InstanceStore), "let an internal activation edge become a request
  owner" (N/A — no activation edges minted), "reference counts to collect a cyclic graph" (N/A —
  no liveness/retirement at P6.1)? Confirm P6.1 mints NO activation edges and touches NO runtime.

## §4. P6.2 (DEFERRED — next grounded cut, not this session)
Populate `rules` (`RuleRoutingProjection`) + promote `SymbolicFieldId` classes: for each rule,
the body-pos↔head-pos field map; `PromoteSharedSymbolicField(f_a,f_b)` unions ONLY when EVERY
producer rule agrees (F16, both arms). `@key` paths (P5) seed the destination binding prefix.
Render a `rule`/`routing` block + census count. Exit gate: an all-producers-agree case promotes,
co-occurrence-only does not. **Independent of P6.1** (P6.1's projection does not read `rules`).
Grounded next session; NOT executed here.

## §5. Exit gate for P6.1 (structural, discriminating)
1. `recursive_components` populated correctly: corecursion_1 → ONE component `members=(ping,
   pong)`; tc_nonlinear_diff → `members=(tc)`; query-independent (corecursion_1_q identical
   block). A stub leaving it empty FAILS the golden + the recount belt.
2. DEBUG cross-check agrees with a model-condensation projection (F19).
3. Codegen byte-stable: only `.region` goldens move (all 20 gain the census token; tc + the 2
   new carriers gain the block); `.rel`/`.h`/`.cpp`/`.stdout`/behavioral/oracle byte-identical.
4. OptDiff `SUITE: PASS` (224 = 223 + corecursion_1; + corecursion_1_q if added), ctest green.
5. One coherent commit; goldens re-blessed via `runall.sh --bless` after review.

---

## §6. PANEL VERDICT + corrected design (v2 — the design that ships)

Three opus refuters (soundness / determinism+churn / invariants+false-starts) verified EMPIRICALLY
against live `-df`/`-origin-out`/`-region-out` dumps of the carriers in all 4 modes. Verdict:
**DIRECTION SOUND; one BLOCKING arm-inversion (B1) + two correctable defects (H1×2, M1); the
corrected formulation was empirically PRE-VERIFIED on tc/ping-pong. Execute v2.** Certified clean:
false-starts, dormant-half-untouched, RelationId identity, determinism/ordering (decl.Id golden-safe,
matches CollectOriginInteriorDecls), #query-independence, P6.1⊥P6.2.

### B1 (BLOCKING, unanimous) — the arm is inverted; use the ORIGIN projection, not the insert arm.
Empirical: in BOTH carriers the ONLY relation-INSERT materializes a NON-recursive relation
(tc_nonlinear_diff: `reachable` at stratum 4; corecursion_1: none — the sole insert is the
`reachable_pair` message TRANSMIT). Every recursive relation (`tc`, `ping`, `pong`) is a **Tier-2
origin-interior** (merge-materialized, `tag=connect/insert-union`, `origin=(…)`). The insert arm
NEVER sees recursion; my "omit-ambiguous Tier-2" default would drop `pong`. **v2 formulation
(empirically validated by refuters 1&2 to yield the exact goldens):**
```
ComputeRecursiveComponents(query, relation_schemas) -> vector<RecursiveComponent>:
    rel_ids = { schema.id : … for schema in relation_schemas }          # the frozen relations
    views_per_stratum : map<unsigned,unsigned> = {}
    query.ForEachView(V):  if s = V.Stratum():  views_per_stratum[s] += 1
    members : map<unsigned, set<RelationId>> = {}                        # stratum(ascending) -> rels
    query.ForEachView(V):                                               # ANY origin carrier, not just inserts
        s = V.Stratum()
        if !s or views_per_stratum[*s] <= 1:  continue                   # only MULTI-VIEW (recursive) strata
        for decl in V.OriginDecls():
            rid = RelationId{decl.Id()}
            if rid in rel_ids:  members[*s].insert(rid)
    return [ RecursiveComponent{sorted(mem)} for (s, mem) in members if !mem.empty() ]  # ascending s
```
Why sound (certified): (a) a MULTI-VIEW stratum IS an SCC cycle (Stratify: "recursive fixpoint ⟺
multi-view stratum") — so a size-1 members set from a multi-view stratum is genuine SELF-recursion,
no self-edge check needed (Q3 ✓); (b) `OriginDecls` is seeded at the insert-proxy and migrates ONLY
via CSE folds — strictly NARROWER than "rows flow through" — so a base relation (`edge`) whose rows
merely pass through the recursive JOIN never lands on the recursive UNION's origin (Q1 ✓, edge's
origin sits only on non-recursive strata); (c) all decls sharing one multi-view stratum are one SCC
⟹ one component. Preferred over the reconstruction-diffs §3-P6.1 relation-level clause-Tarjan
because the clause graph is BLIND to canonicalization and would over-report vacuous dead cycles in
EVERY mode; the DataFlow-graph projection is faithful to each mode's actual graph. `rules` is NOT
read ⟹ P6.1 stays independent of P6.2 (commit split preserved).

### H1a (HIGH, refuter 1) — MODE-FAITHFUL, not mode-invariant. Pin `recursion.dr`.
Empirical on `recursion.dr`: opt/nocf → 0 recursive components (canonicalization strips vacuous
`p:-p` self-loops + collects source-less cycles); nodf/none → 5 (`{direct_const}…{direct_and_join}`,
each a surviving multi-view UNION stratum). No mode-uniform strip exists short of dead-flow analysis
(opt-only). **Resolution:** `recursive_components` is a FAITHFUL per-compile observer of the
DataFlow SCC structure IN THAT MODE (like the whole Regional model). It is NOT claimed mode-invariant;
the exit gate's invariance is over add/remove `#query` (which holds — Stratify runs pre-query), never
over opt mode. Pin the split HONESTLY with a `recursion.dr` `.region` golden (opt/nocf: no block;
nodf/none: 5 blocks) so the property is TESTED, not latent. Real programs (tc/ping-pong/loop1-loop2)
have no vacuous cycles and ARE mode-invariant (certified).

### H1b (HIGH, refuter 2) — E-K5-PAD: render the block with INDEPENDENT width.
`"recursive-component"` is 19 > `kind_w`'s max 14. It must NOT feed the shared `kind_w`
(Format.cpp:276-298) or it re-pads every row-contract/port line in recursive dumps. Render each line
with its OWN literal spacing after the declared-key block; leave `kind_w` untouched.

### M1 (MED, both) — DROP the model-condensation cross-check (unsound).
It aborts on tc (reachable≡tc share `table:4` via CSE) and disagrees on nodf ping/pong (different
models, same stratum). No sound "second independent derivation" exists that isn't tautological. **The
region GOLDEN on the recursive carriers is the discriminating anti-stub authority** (a stub → empty →
no block → tc/corecursion/two_inductions goldens FAIL). Keep only a cheap Tigerstyle internal-
consistency assert (each component non-empty, members sorted-unique ⊆ rel_ids).

### Census (unforced judgment, M1 refuter 2) — NO census count (P5 precedent, minimal churn).
Follow the P5 declared-key pattern: a gated own-width render block, NO 8th `RegionalCensus` field.
⟹ 16 unrelated existing goldens BYTE-IDENTICAL; only tc_nonlinear_diff × 4 MOVE (gain the block).
(Drops the H2 recount belt — near-tautological anyway; the golden is the real gate.)

### v2 exit gate / golden ledger (STRUCTURAL)
- **MOVE (re-bless):** `tc_nonlinear_diff.region.*` × 4 gain `recursive-component  C0  members=(tc)`.
- **NEW goldens:** `corecursion_1.region.*` × 4 (`C0 members=(ping, pong)` — one component, two
  members, true co-recursion, NO `#query`); `two_inductions.region.*` × 4 (`C0 members=(loop1)`,
  `C1 members=(loop2)` — TWO independent components, discriminates a merge-all bug);
  `recursion.region.*` × 4 (the mode-split witness: 0 vs 5).
- **BYTE-IDENTICAL:** booleans/join_1/key_partial_1/merge_2 `.region.*` × 4 = 16 (no census token,
  non-recursive → no block).
- **Codegen byte-stable:** `.rel`/`datalog.h`/`.cpp`/`.stdout`/behavioral/oracle unchanged for all.
- #query-independence: CERTIFIED empirically (corecursion_1 vs +`#query` identical partition) +
  structurally (Stratify pre-query); WITNESSED by the contrast tc/two_inductions (recursion WITH a
  query) vs corecursion_1 (recursion WITHOUT). A dedicated corecursion_1_q byte-pin is deferred
  (marginal over the certification; cheap to add later).
- OptDiff `SUITE: PASS` (was 223; + corecursion_1 = 224), ctest 5/5. One coherent commit.
