# P9 — access-path inference (additive, logical-only) — grounding doc

> STATUS: **DOCS-ONLY GROUNDING — PRE-EXECUTION, awaiting owner go/no-go.** Branch
> `keyed-instances`, grounded at tip `99b91335`. Produced by the session-26 grounding loop
> (anchor sonnet → design opus → 6-concern opus refuter panel). **Refuter verdict: SOUND BUT
> SPECULATIVE** — 5 of 6 concerns refuted; the one surviving concern is load-bearing and
> decision-relevant: **P9 is genuinely CONSUMER-LESS at landing** (see §2 + §8). NOTHING built
> or edited yet.

## §0 STATUS + the one-line gap

P5 realized the logical-access-path authority: `@key` pragmas intern per-relation into
`RelationSchema::declared_access_paths` (`DeclaredAccessPathSet`, `RegionInstance.h:314-331`) and
render as a gated `declared-key` block in `-region-out` (`lib/Regional/Format.cpp:387-403`). `@key` is
otherwise **inert** (P1 deleted `-demand`, the nested lowering, and the `V-DECLARED-KEY` bijection;
`RecognizedSubgraphs` and the `Demand.cpp` `LiftBoundColumnsToRelation` algorithm are GONE).

**The one-line gap:** the region model records *declared* access paths but never *infers* the paths a
bound `#query` actually needs from a relation. P9 adds that inference — additively (declared always
wins), logical-only (drives no codegen).

## §1 What the cut IS (RECOMMENDED formulation — see §7 OQ1)

A **freeze-time, clause-source, read-only** analysis (the landed P6.2 pattern, **NOT** the seed doc's
deleted pre-Optimize DataFlow walk):

1. **New data:** a second `DeclaredAccessPathSet inferred_access_paths` on `RelationSchema`
   (`Regional.h:94`, beside `declared_access_paths`). Reusing the P5 struct means the declared field is
   **literally never touched** → anti-regression is structural-by-construction; provenance = *which
   field the path lives in*.
2. **New populator:** `InferAccessPaths(RegionTemplate &R, const Query &query)` in `Planning.cpp`, at
   the freeze tail (after `PromoteSharedSymbolicField` `:760`, before census `:763`). For each **bound**
   `#query` redecl, project the bound head vars onto positive **frozen body** predicates by
   `ParsedVariable::Id()` equality (the P6.2 `BuildRuleRoutingProjections` variable-matching pattern),
   order by adornment param order, intern via `InternDeclaredPaths`, dedup **exact-ordered** against the
   relation's declared paths, store residue into `schema.inferred_access_paths`.
3. **New render:** a gated own-width `inferred-key` block in `Format.cpp`, appended after the P5
   `declared-key` block. Empty ⇒ no line ⇒ no golden moves. **NO census count.**
4. **New referee `V-INFER-DEDUP`** (fprintf+abort, survives NDEBUG): no inferred path exact-ordered-equals
   a declared path of the same relation.

**Scope of the first cut: SOURCE 1 (query adornments) only.** Join-pivot (SOURCE 2) and contextual
(SOURCE 3) deferred to a P9.x — SOURCE 1 alone exercises the full additive/anti-regression story.

## §2 Why it is sound + (bluntly) how thin the value is

**Sound:** additive by construction (`declared_access_paths` never read-modified); Optimize-invariant
(clause-source, the P6.2-certified identity source — never the CSE-merged post-Optimize graph);
logical-only (chooses no physical structure, mints no edge, **never** fed to `SelectAccessPlan`, which
reads only the call-site's raw `available_bindings` — the four-authority firewall); answer-invariant (M3
answers correctly with `@key` inert; a no-op P9 passes every answer golden). **Every P9 pin is
structural.**

**Value (BLUNT — the load-bearing finding):** P9 is **CONSUMER-LESS at landing**. `SelectAccessPlan`
(`RegionInstance.h:293-301`) structurally cannot read `declared_access_paths` OR an inferred twin, and
P7's seek already fires from the RAW bound subset (not `@key`/path-gated), so even the physical layer
never needs P9. It drives **zero** codegen and zero answers; its only surface is `-region-out`. A
consumer requires an **explicit four-authority-firewall relaxation** — it does NOT fall out of "a P8
arriving." Honest deliverable: an additive observability surface + a discriminating structural test that
re-exercises landed P6.2 machinery.

> The critique flagged the earlier "anti-regression contract replacing V-DECLARED-KEY" framing as a
> **false equivalence**: V-DECLARED-KEY was a *semantic* bound-set-vs-SIP bijection inside the deleted
> demand transform; `declared_access_paths` is already written-once/never-mutated, so a separate
> never-read field grants no new protection. `V-INFER-DEDUP` is render hygiene, not a safety contract.
> Do not sell P9 as a codegen/query-performance win.

## §3 The diff at hunk grain

| # | File:anchor | Change |
|---|---|---|
| H1 | `Regional.h:94` (after `declared_access_paths`) | Add `DeclaredAccessPathSet inferred_access_paths;` to `RelationSchema`; reuse the existing struct — no new type. |
| H2 | `Planning.cpp` (new static near P6.2 populators) | `InferAccessPaths(R, query)`: walk bound `#query` redecls (dedup like `CountBoundQueryRedecls` `:270-292`); per positive body pred with `Of(pred).Id() ∈ relation_schemas`, collect bound-head-var → body-ordinal by `ParsedVariable::Id()`; order by adornment param; `InternDeclaredPaths`; dedup exact-ordered vs declared; append residue. Pure, freeze-time. |
| H3 | `Planning.cpp:760` (after `PromoteSharedSymbolicField`, before census) | `InferAccessPaths(R, query);` — model+render only; no census count. |
| H4 | `Format.cpp` after `:403` | Symmetric loop: per `schema.inferred_access_paths.paths`, emit `inferred-key  E<k>  rel=<name>  path=(names…)` via `RenderDeclaredPathText`. Own-block, gated, reuses declared-key's `E<k>`/`rel=` columns. |
| H5 | `Planning.cpp` freeze tail | `V-INFER-DEDUP`: ∀ schema, no inferred path exact-ordered-equals a declared path; else fprintf+abort. |
| H6 | `tests/OptDiff/cases/key_infer_1.{dr,main.cpp,irgold}` | NEW witness (§4/§5) — the ONLY case pinning declared∪inferred coexistence. |

No change to: `SelectAccessPlan`, the `DeclaredAccessPath`/`DeclaredAccessPathSet` structs, `-contract-out`
(the Stage-A row-contract dump — **unrelated**, do NOT touch), any DataFlow-layer renderer
(`RecognizedSubgraphs` is deleted), any `QueryImpl` member, any codegen/`.rel`/`.df` surface.

## §4 IR desired states (predict-then-verify, STRUCTURAL)

Smallest carrier `key_infer_1`:
```
#message edge_2(u64 From, u64 To).
#local path(u64 From, u64 To) @key(From).
path(F, T) : edge_2(F, T).
#query to_via(free u64 From, bound u64 To) : path(From, To).
```
`@key(From)` ⇒ declared `[0]=(From)` on `path`. Query binds `To` → `path` ordinal 1 ⇒ inferred
`[1]=(To)`. `(From) != (To)` ⇒ survives dedup ⇒ two coexisting lines. AFTER appends exactly one line
before `}`:
```
  declared-key  E1  rel=path    path=(From)
  inferred-key  E1  rel=path    path=(To)
}
census: … row-contracts=2   ← census BYTE-UNCHANGED (inferred-key carries no count)
```
`inferred-key` is 12 chars ≤ existing `kind_w` max ⇒ **no padding re-flow**, pure byte-additive.

Dedup-suppression pin (existing golden, must NOT move): `key_partial_1` (`@key(From)`, query binds
`From`) → inferred `(From)` == declared `(From)` → **suppressed** → `key_partial_1.region.*` stays
**byte-identical**. Same for `key_corecursion_1` (`@key(K)`, query binds `K`).

## §5 Which goldens MOVE (exact blast radius — only 4 corpus cases carry a real bound-query request-port)

| Case | Bound query? | Inference | Golden effect |
|---|---|---|---|
| **key_infer_1** (NEW) | binds `To` | `(To)` ≠ declared `(From)` | **NEW** `.region.{4}` + `.stdout` + `.main.cpp` + `.irgold`. Pins additive-not-merge. |
| **booleans** | binds `UserId` | `(UserID)` on frozen `user`, no declared | **MOVE** additive `inferred-key` suffix. Proves inference fires on an *undeclared* relation. |
| **two_inductions** | `output(bound A)` | `(A)`, no declared | **MOVE** additive `inferred-key` suffix. |
| key_partial_1 | binds `From` | `(From)` == declared → suppressed | **NO MOVE** (dedup-suppression anti-regression pin). |
| key_corecursion_1 | binds `K` | `(K)` == declared → suppressed | **NO MOVE**. |
| join_1, merge_2, recursion, tc_nonlinear_diff, corecursion_1, fixpoint_force | all-free / no bound query | empty bound set → nothing | **NO MOVE** (all-free inertness). |

Every moved case = additive suffix, never padding re-flow, never census delta. **Honesty net:** all
`.h`/`.ir`/`.rel`/`.df`/`.stdout`/`.oracle`/`.monotone`/`.behavioral`/`.contract` goldens across all 226
cases × 4 modes stay byte-identical. P9 moves ONLY `.region` goldens (2 existing + 1 new case). Any
other golden moving is a bug. (The exact body relation + ordinal for booleans/two_inductions is confirmed
by the implementer's clause walk at bless; the *shape* — one additive `inferred-key` line — is the pin.)

## §6 The DISCRIMINATING STRUCTURAL exit gate

1. **ADDITIVE-NOT-MERGE** (`key_infer_1`): `path` renders BOTH `declared-key … (From)` AND `inferred-key
   … (To)` — two lines. A merge impl collapses to one; a provenance-loser mislabels one.
2. **DEDUP-SUPPRESSION / ANTI-REGRESSION** (`key_partial_1`, `key_corecursion_1`): `.region` byte-identical.
   A no-dedup bug adds a spurious `inferred-key (From)`; a drop-declared bug removes the `declared-key`
   line. Both → suite red.
3. **INFERENCE-FIRES-UNDECLARED** (`booleans`, `two_inductions`): an `inferred-key` line appears on an
   UNKEYED relation. A no-op P9 leaves these byte-identical → the required MOVE is the proof inference ran.
4. **ALL-FREE-INERT** (6 cases): `.region` byte-identical — a spurious-inference bug moves them.
5. **LOGICAL-ONLY / CONSUMER-LESS HONESTY** (whole suite): every non-`.region` golden byte-identical; no
   physical token (`plan=`, `Index`, `First`/`Next`, `NumRows`) ever appears on an `inferred-key` line;
   `V-INFER-DEDUP` never fires. The four-authority firewall pin.

Gate summary: **OptDiff SUITE PASS (226)**; **ctest 5/5**; exactly 2 existing `.region` cases move
(additive suffix) + 1 new case; every other golden byte-identical.

## §7 Open questions (owner)

- **OQ1 (owner call, recommendation FIRM): clause-source-at-freeze** (this doc — P6.2-certified, no
  QueryImpl churn, sidesteps the CSE-identity-loss + out_to_in-ordering hazards) **vs the seed doc's
  pre-Optimize DataFlow walk + new `QueryImpl::inferred_access_paths` member** (heavier, re-derives ~90
  deleted lines, hits the identity loss P6.2 already proved). Recommend ratifying clause-source as the
  design decision.
- **OQ2:** multi-column inferred ORDER = adornment-param order for SOURCE 1 (recommended — the query's
  stated calling convention). Ratify before adding a multi-bound witness.
- **OQ3:** land inferred paths on the query's OWN head relation, or ONLY on frozen BODY predicates? This
  doc scopes to body predicates (the seek target; head path is tautological). Confirm.
- **OQ4:** SOURCE 2 (join pivots) / SOURCE 3 (contextual) — deferred; decide P9 vs P9.x.
- **OQ5:** render target = `inferred-key` block beside `declared-key` in `-region-out` (the landed P5
  location; the seed doc's `-contract-out` split is wrong at tip). Confirm the naming (`inferred-key` vs
  `inferred-path`).

**Stale-anchor drift surfaced:** design doc is `keyed-rewrite-p7p9-diffs.md`; its §3.5
`RecognizedSubgraphs` renderer premise is DEAD; `proxy_view_to_decl`-extension is dead (Build-scoped,
dangling keys); `key_multi_adorn_witness` does NOT exist (deleted `-demand-instance` infra). Treat §3 as
historical intent, not a patch target.

## §8 Refuter panel — verdict SOUND BUT SPECULATIVE

Refuted (5): the recommended clause-source formulation sidesteps the pre-Optimize-walk hazard; adds no
dangling QueryImpl satellite; is byte-additive with the correct blast radius and a real anti-regression
pin (verified against tip goldens — exactly 4 bound-query carriers, all predictions reproduce by hand);
never leaks a physical token or duplicates the AccessPlan authority; its exit gate is structural and
discriminates a stub. **Surviving (1, major, NOT refuted):** P9 is consumer-less; the value framing is
thinner than the design's original prose implied (folded into §2). **Panel recommendation:** technically
low-risk and correct to build, but it is infrastructure-ahead-of-a-consumer — sequence it AFTER the
firewall-relaxation decision that would give it a reader, and one load-bearing implementation
pre-check remains: confirm a `#query` decl's `.Clauses()` returns its defining clause and the redecl
binding ordinal aligns positionally with the clause head param (`booleans` is the test — its query decl
is declared separately from its clause). This is the one step not already demonstrated by landed P6.2
code.
