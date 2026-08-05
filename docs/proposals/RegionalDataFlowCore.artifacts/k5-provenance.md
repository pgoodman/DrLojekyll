# K5 / DIFF-NEXT-S1 — Tier-2 origin-decl provenance: the design brief (session 8, 2026-08-04)

Purpose: scope the K5 slice — "origin decl-sets on models (Connect erasure
site, union-only on CSE folds) + D2.9 proxy-role inheritance as ONE slice;
unblocks the ADJ-R3-C column-survival belt + undemanded interior naming; the
FIRST sanctioned post-F1 maintained satellite, which the panel MUST litigate"
(`k6-landed-seed.md:146-150`).

SEED DISCIPLINE INVERSION: this is a DESIGN brief, not a landed seed. It grounds
the one-line seed against tip code (four verified extraction reports + spot-
checked anchors), lays out the DESIGN SPACE as alternatives with trade-offs (NOT
a chosen design), and hands the owner a numbered decision list. ONE-AUTHORITY:
the normative ratifications remain `region-model-diffs.md` (F1 discipline, D2.9,
ADJ-R3-C / R3A-IMPL-2, the SUBGRAPH-AUTHORITY FRAMING); this brief does not
ratify — it litigates.

COMPLEXITY IS GUILTY. The seed's own framing ("on models", "union-only on CSE
folds") is a POLICY sketch, not a mechanism; Part A shows the grounded code
makes two of its phrasings imprecise (§A.4) and Part B derives the smallest
sound shape from the ONE choke point the codebase already maintains. Both
near-term unblocks (ADJ-R3-C, tc naming) need EMPIRICAL re-confirmation before a
line is written (K5-Q1, K5-Q7) — they may be narrower than the seed asserts, or
already closed by RP-6.

---

## Part A — GROUNDED CURRENT STATE

### A.1 The pipeline region, as pseudocode at tip

```
Query::Build(module, log, policy, demand_mode, ...)          # lib/DataFlow/Build.cpp
  proxy_view_to_decl := {}                                   # Build-SCOPED local (Build.cpp:2577-2584)
  ConnectInsertsToSelects(log, proxy_view_to_decl)           # Connect.cpp:162-299
    for REL *rel : relations:                                # Connect.cpp:230
      insert_proxy := CreateProxyForMutableParams(           # Connect.cpp:262-263
                        CreateProxyOfInserts(rel->inserts), rel->declaration)
        # CreateProxyOfInserts (Connect.cpp:11-71): 1 TUPLE per clause-INSERT,
        #   each insert->CopyDifferentialAndGroupIdsTo(proxy) (Connect.cpp:38),
        #   insert->PrepareToDelete(). ALWAYS wraps in a MERGE — the
        #   `has_one_insert` early-return is DEAD (§A.3). node-kind = MERGE*
        #   (no mutable) | TUPLE*-over-KVINDEX (mutable).
      proxy_view_to_decl.emplace(insert_proxy, rel->declaration)   # Connect.cpp:269 — THE STAMP
      rel->inserts.Clear()                                   # Connect.cpp:271 — SEVERS REL->proxy
      if rel->selects.Empty() and not decl.IsQuery(): continue   # Connect.cpp:278-280 (dangling; culled)
      ProxySelects(rel->selects, insert_proxy)               # Connect.cpp:282; :127-156
      if decl.IsQuery(): re-mint terminal INSERT reading insert_proxy   # Connect.cpp:285-292
    RemoveUnusedViews(); TrackDifferentialUpdates(log, true) # Connect.cpp:295-296
  ApplyDemandTransform(...)                                  # Demand.cpp — SOLE reader of the map
    # resolves p_merge -> proxy_view_to_decl.at(...) -> p_demanded_decl (Demand.cpp:857;
    #   T1-DECL-MISS abort on miss). Reads the map, then the local dies.        <-- map end-of-life
  if policy.AnyBodyOptionalEnabled: Optimize(log, policy)    # Build.cpp:2621-2622
  ConvertConstantInputsToTuples(); RemoveUnusedViews()
  ProxyInsertsWithTuples(); LinkViews()                      # Build.cpp:2630-2631 (ProxyMerged/Joined/Negated)
  IdentifyInductions(); FinalizeDepths(); FinalizeColumnIDs()
  TrackDifferentialUpdates(); TrackConstAfterInit()
  BuildEquivalenceSets(impl)                                 # Build.cpp:2646 — DECIDES storage sharing
  Stratify(log)                                              # Build.cpp:2647
  row_contracts := InferConservativeRowContracts(impl)       # Build.cpp:2655 — per-LIVE-VIEW, PURE, once
  ValidateRowContracts; return Query(impl)

FrozenRegionalProgram::Build(query, log)                     # lib/Regional/Planning.cpp — BEFORE ControlFlow
  row-contracts := CollectContractInserts(query)             # Planning.cpp:178-196 (R-STORE: distinct
    #   non-demand__ relation-INSERT decls, first-insert-wins)
                 ++ CollectDemandInteriorDecls(query)         # Planning.cpp:207-226 (Tier-1: rs.demanded_decl
    #   not already insert-named — the ONLY other nameable surface)
  # reads query.impl->row_contracts via "the ONE friend leak" (Planning.cpp:28-30)

Program::Build(frozen, ...)  query = frozen.Query()          # ControlFlow — AFTER freeze
  BuildDataModel(query, program)                             # Build.cpp:253-268
    per view: new DataModel; view_to_model[view]=model; eq_classes[EquivalenceSetId()]=model
    per view: DisjointSet::Union(view_to_model[view], eq_classes[EquivalenceSetId()])  # Build.cpp:266
  FillDataModel(...)                                          # Build.cpp:37-205 — decides which models
    #   get a TABLE; DataTableImpl::GetOrCreate lazily realizes model->table (Data.cpp:147-344)
```

### A.2 Three lifetimes, three sharing layers (the structural terrain K5 lands in)

| Structure | Owner / lifetime | Sharing role | Carries a decl? |
|---|---|---|---|
| `proxy_view_to_decl` | `Query::Build` STACK local, dies before `Optimize` (Build.cpp:2577-2584) | none — a snapshot | ONE decl/proxy, singular; T1 side-channel |
| `RecognizedSubgraph::demanded_decl` | `QueryImpl` member, survives to freeze (Query.h:1028-1041) | per-forcing | ONE `ParsedDeclaration` snapshot, demand-scoped |
| `RowContract` | `QueryImpl::row_contracts`, built once at Build tail (Build.cpp:2655), per-LIVE-VIEW | none — per view, no cross-view merge | no decl; visible_fields/member_key only (RowContract.h:38-51) |
| `EquivalenceSet` | `QueryViewImpl::equivalence_set`, built pre-Stratify (Build.cpp:2646) | THE storage-sharing decision (`EquivalenceSetId()`) | no — views_in_set/induction_view/stratum only |
| `DataModel` | `ProgramImpl::models`, ControlFlow, built AFTER freeze (Build.cpp:253-268) | re-expresses EquivalenceSetId sharing | no — `table` only; NO decl field anywhere in Program.h |

CRITICAL ORDERING (E3): freeze (`Regional/Planning.cpp`) runs BEFORE ControlFlow
`BuildDataModel`. A field on the ControlFlow `DataModel` is INVISIBLE at freeze.
Any provenance a freeze consumer (Tier-1 naming, ADJ-R3-C) reads MUST be
DataFlow-side (QueryImpl-owned), read via the friend leak — exactly like
`row_contracts`. "Origin decl-sets ON MODELS" (the seed phrase) can only be the
ControlFlow-tier READ surface (cost/D5); it cannot be the freeze mechanism.

### A.3 Two grounded facts the seed's one-liner does not encode

- **`CreateProxyOfInserts` ALWAYS returns a MERGE** for the `rel->inserts` path
  (single-member for a 1-clause relation). `has_one_insert` is read from
  `inserts.Size()` AFTER `old_inserts.Swap(inserts)` emptied it (Connect.cpp:17
  vs DefUse.h:270-276) → always `0==1u` → the bare-TUPLE early return
  (Connect.cpp:50-52) is DEAD. Corroborated by Demand.cpp:579-583 ("the
  empirically real Connect shape") and `p_merge`'s static `MERGE*` type
  (Demand.cpp:563). Any K5 stamp keyed on the mint node-kind must NOT assume
  TUPLE for single-clause; a later "fix" of this quirk would flip it (K5-Q6).
- **`CreateProxyForMutableParams` does NOT call `CopyDifferentialAndGroupIdsTo`**
  (Connect.cpp:73-125) — the KV-wrapping TUPLE gets no satellite migration. If
  the K5 mechanism rides that choke point (§B2), the mutable-param arm is a
  residual gap (K5-Q3).

### A.4 Where the seed's phrasing is imprecise against the code

1. "origin decl-sets ON MODELS" — models (`DataModel`) don't exist at freeze
   (§A.2). The WRITE must be DataFlow-side and cross `Optimize`; "on models" is
   at best a second, ControlFlow-tier READ derived at `BuildDataModel`. Two
   consumers of one DataFlow-side write, not one structure spanning layers.
2. "union-only on CSE folds" — CSE is only ONE of ~12 view-replacement classes
   (§A.5). But nearly all route through the SINGLE choke point
   `CopyDifferentialAndGroupIdsTo` (View.cpp:662-727), which is exactly where
   `group_ids` already rides a monotone set-union (View.cpp:664-667). The
   correct mechanism target is "the choke point", not "CSE" — which SHRINKS the
   census to the handful of sites that bypass it.

### A.5 REPLACEMENT-SITE CENSUS — every site an origin-decl-set must account for

The heart of the correctness obligation. "Routes CDaGI" = the site reaches
`CopyDifferentialAndGroupIdsTo` (View.cpp:662-727), so a union placed THERE
(the group_ids precedent) covers it for free. "Survivor/Loser" names which view
keeps identity. "Missed-union effect" is what a bug does — under the union-only
policy every effect is an UNDER-NAME (safe), never a miscompile.

| # | Site (file:line) | Primitive | Routes CDaGI? | Survivor / Loser | Missed-union effect |
|---|---|---|---|---|---|
| 1 | **Connect.cpp:38** CreateProxyOfInserts | CDaGI + PrepareToDelete | YES | proxy / INSERT | STAMP site — the decl SOURCE, not a fold |
| 2 | **Connect.cpp:143,154** ProxySelects | CDaGI + ReplaceAllUsesWith | YES | proxy / SELECT | select-proxy under-names |
| 3 | Connect.cpp:73-125 CreateProxyForMutableParams | fresh TUPLE, NO CDaGI | **NO** | proxy (KV wrap) / — | mutable-param interior loses decl |
| 4 | **Optimize.cpp:406** CSE choke | ReplaceAllUsesWith | YES | v2 / v1 | survivor loses v1's decl (THE canonical fold) |
| 5 | Compare.cpp:171,216 trivial eq/ne | fresh TUPLE + RAUW | YES | tuple / CMP | — |
| 6 | Compare.cpp:557 TrySinkThroughMerge | RAUW(lifted_merge) | YES (final only) | lifted_merge / CMP | N sunk_cmp replicas get NO decl |
| 7 | Compare.cpp:948 TrySinkThroughNegate | RAUW(lifted_tuple) | YES (final only) | lifted_tuple / CMP | lowered_cmp/lifted_negate get NO decl |
| 8 | Negate.cpp:163 unsat negated-view | fresh TUPLE + RAUW | YES | tuple / NEGATE | — |
| 9 | KVIndex.cpp:242 no-used-values | fresh TUPLE + RAUW | YES | tuple / KVINDEX | — |
| 10 | Join.cpp:158 ConvertTrivialJoinToTuple | fresh TUPLE + RAUW | YES | tuple / JOIN | — |
| 11 | Join.cpp:424 dup-output | SubstituteAllUsesWith | YES | facade tuple / JOIN kept alive | — |
| 12 | **IdentityJoin.cpp:160-188** | RAUW, with **M1 guard-fold** | YES | tuple / JOIN | precedent: annotation FOLDED not migrated (§B3) |
| 13 | Merge.cpp:175 1-arm elim (F27 site) | RAUW(source) | YES | source view / MERGE | survivor loses MERGE's decl |
| 14 | Merge.cpp:272 N→1 flatten | fresh TUPLE + RAUW | YES | tuple / MERGE | dropped unsat/dup arms' decls vanish |
| 15 | Merge.cpp:296-343 unused-col guard | fresh guard TUPLE, **NO CDaGI** | **NO** | guard tuple / arm kept alive | per-arm decl not carried (inconsistent w/ #17) |
| 16 | Induction.cpp:439-444 inductive leave | RAW Def-RAUW + explicit CDaGI:444 | YES (explicit) | new_union / view kept alive | covered IF union lives in CDaGI |
| 17 | Join.cpp:266-280 ProxyUnusedInputColumns | fresh TUPLE + explicit CDaGI:279 | YES | proxy / joined_view kept alive | — |
| 18 | **Link.cpp:232** ProxyMergedViews | CDaGI + guard save/restore | YES | proxy / arm kept alive | D2.9-adjacent precedent (§B3) |
| 19 | Link.cpp:70,98,163 ProxyNegated/Joined | CDaGI | YES | proxy / original kept alive | — |
| 20 | **Link.cpp:11-53** ProxyInsertWithTuple | NO CDaGI (only diff-flags) | **NO** | proxy / INSERT reads it | INSERT-adjacent decl not carried |
| 21 | DeadFlowElim 281/515; RemoveUnusedViews | PrepareToDelete | n/a | — / deleted | decl dies with a genuinely-dead view (safe) |
| 22 | Merge.cpp sinking family (SSinkThrough*) | CDaGI at 647/865/888/924 | n/a — `can_sink_unions` DEAD (Optimize.h:24) | inert | UNREACHABLE; ignore unless revived |
| 23 | BuildDataModel Build.cpp:266 | DisjointSet::Union (NO hook) | n/a — ControlFlow tier | root/child | per-MODEL union needs OWN logic |

READING: if the union rides `CopyDifferentialAndGroupIdsTo` (sites marked YES,
i.e. the choke point group_ids already uses), the ONLY residual gaps are #3, #6,
#7, #15, #20 (five sites) plus the ControlFlow-tier #23 (if a per-model set is
also wanted). Under monotone under-naming safety (§B2) those five can be
deferred or closed on demand. This is the census's payload: **the seed's "union
at CSE folds" is really "one union statement inside CDaGI + a five-site residual
list."**

---

## Part B — THE DESIGN SPACE (alternatives, not a decision)

### B1 — WHERE the origin set lives

The elements are `ParsedDeclaration` values (parse-identity, immutable — the T1
snapshot property, `region-model-diffs.md:1958-1962`). The set GROWS
(union-only) but never mutates an element. Three homes:

- **B1-a — per-VIEW field on `QueryViewImpl`** (`std::vector<ParsedDeclaration>`
  or a small sorted set), unioned inside `CopyDifferentialAndGroupIdsTo`
  alongside `group_ids` (View.cpp:664-667). PRO: rides the ONE choke point →
  census sites 2,4,5,8-14,16-19 covered by a single added statement; DataFlow-
  side so freeze reads it via the friend leak; exact `group_ids` precedent
  (already-sanctioned union-only satellite). CON: a new field on EVERY view
  (memory on views that never carry a decl); the five §A.5 gap sites; the
  maintained-satellite F1 question (§B2).
- **B1-b — `QueryImpl` side-map `unordered_map<QueryViewImpl*, set<decl>>`**
  (the `proxy_view_to_decl` shape, but persistent + union-capable). CON:
  **UNSOUND as written** — VIEW* keys DANGLE across `Optimize` (the documented
  reason `proxy_view_to_decl` is Build-scoped and discarded before Optimize,
  Build.cpp:2578-2582); a map keyed by dead pointers after a CSE fold is a
  use-after-free unless every fold migrates the key — which is strictly MORE
  work than B1-a's single CDaGI statement. B1-b is dominated by B1-a; record it
  only to reject it.
- **B1-c — BOTH tiers.** B1-a per-view field (freeze-readable) + a per-`DataModel`
  set unioned at `BuildDataModel`'s `DisjointSet::Union` (Build.cpp:266; needs
  its OWN union hook — `DisjointSet::Union` is a bare parent swap, no payload
  merge, E3). PRO: serves BOTH the freeze consumer (per-view, DataFlow) AND the
  ControlFlow consumers (cost/D5, per-model). This is what the seed's "on
  models" literally wants for the ControlFlow half. CON: two write sites, two
  read surfaces; only justified if a ControlFlow-tier consumer actually exists
  (none does today — B5, K5-Q5).

Lifetime constraint (settles the layer): freeze needs it → DataFlow-side → it
MUST cross `Optimize` (K5 cannot use T1's "never crosses Optimize" property).
B1-a is the minimum that satisfies this; B1-c adds the ControlFlow read only if
a consumer materializes. B1-b is unsound.

### B2 — THE MAINTAINED-SATELLITE LITIGATION (mandatory, per all three seeds)

**The F1 hazard, precisely** (FINDINGS.md:195-201; `region-model-diffs.md:73`):
an invariant maintained by COORDINATION between passes, OWNED by none, ENFORCED
by a debug assert in a third. The `guard_annotation_index` satellite is the
in-tree instance CSE "must be TAUGHT to migrate" (`region-model-diffs.md:73`).
The danger is not the satellite per se — it is a satellite that some LOWERING
DECISION reads, creating a SECOND AUTHORITY over something an existing structure
already owns (`region-model-diffs.md:1620`, "two authorities").

**Why K5 cannot reuse T1's acquittal.** T1's `proxy_view_to_decl` passed F1
muster on FOUR properties (`region-model-diffs.md:1958-1962`): (1) short-lived
Connect→Demand, (2) never crosses Optimize, (3) snapshotted to parse-identity,
(4) a stack-scoped local, never a QueryImpl member. K5 BREAKS (1),(2),(4) by
construction — the set must survive to freeze, i.e. cross Optimize, i.e. live on
QueryImpl/view. It KEEPS ONLY (3): the elements are immutable `ParsedDeclaration`
values, so a fold has nothing to structurally migrate — only a SET to grow.

**The equivalent safety argument K5 must offer instead** (the litigation's
answer): a MONOTONE UNION of IMMUTABLE parse-identities that is (a) NEVER folded
into `Hash`/`Equals` (so it is never a CSE decision input — cannot become a
second authority over identity, the exact F1 fence; extends the D2.9/R1-a
"Equals-only, never Hash, zero-CSE-benefit" finding, owner-adjudication-
record.md:113-115), and (b) NEVER a lowering input, ONLY an observational /
planning / render surface. Under (a)+(b) the STRONGEST failure of the
satellite is an UNDER-NAME: a missed union means a model's origin set is a
strict subset of truth. Since no correctness-bearing decision reads it, an
under-name is a debuggability/observability regression, NEVER a miscompile
(`stage-b-landed-seed.md:102-103`, "monotone: a missed union under-names, never
miscompiles"). THIS is what makes it the "first SANCTIONED post-F1 satellite" —
sanctioned precisely because it is provably outside the correctness cone.

**What a missed union looks like, per census class, and the belt that could
catch it:**
- Sites routing CDaGI (census YES): a missed union here is impossible if the
  union statement lives IN CDaGI — they are covered structurally, not by
  coordination. This is the anti-F1 move: ONE owner (CDaGI), not N coordinating
  callers.
- The five bypass sites (#3,#6,#7,#15,#20): a missed union = the specific
  interior fed through that path under-names. All safe.
- Dead-view drops (#21): decl dies with a dead view — CORRECT, not a miss.
- **Belt options.** Because under-naming is safe, an ABORT belt would CONTRADICT
  the monotone-safe framing (an abort turns a safe under-name into a crash).
  Candidates, weakest-sufficient first: (i) NO belt — accept under-naming,
  document the five gaps; (ii) an ADVISORY census dump `-origin-out` reviewed
  like `-contract-out` (no abort); (iii) a DEBUG-ONLY conservation assert: every
  pre-Connect non-`demand__` relation-INSERT decl appears in ≥1 live view's
  origin set OR is provably dead-flow-eliminated — catches a GROSS drop (a whole
  decl lost) without punishing a partial under-name. Recommendation to litigate:
  (ii)+(iii), NEVER an always-on abort (K5-Q4).

### B3 — THE D2.9 PROXY-ROLE-INHERITANCE FOLD (is it genuinely same-slice?)

"D2.9" is OVERLOADED (E4): (α) the RATIFIED Stage-A rule — `ProjectionRole` /
`edge_kind` folded into `Equals` ONLY, build-stamped, immutable, EXCLUDED from
Hash (owner-adjudication-record.md:63-65,113-115); (β) the UNBUILT owner item
that merely INVOKES (α) as precedent — "should a proxy mint INHERIT its source's
`ProjectionRole` so set-boundary provenance survives onto the final graph? Zero
behavioral risk (role inert on survivors), changes ONLY dump semantics.
Recorded, not built." (stage-b-diff.md:737; owner-adjudication-record.md:170-175).

The seed bundles (β) with origin-decl-sets "as ONE slice" because both are
"erasure-site-family questions about what survives a fold." GROUNDED, they are
MECHANISTICALLY OPPOSITE:
- Origin-decl-set = a SATELLITE that MIGRATES via union (the group_ids shape).
- `ProjectionRole` = an IDENTITY field that NEVER migrates — it is const, set at
  the two `ConvertToClauseHead` mints (Build.cpp), folded into `Equals` to
  REFUSE cross-role folds (Tuple.cpp:308-310), and the (β) question is whether a
  PROXY MINT should COPY the source's role AT MINT TIME (Connect.cpp:32 et al.
  pass no role arg → default `kMember`, Query.h:736-738).

So (β) is a one-line change at the proxy-mint calls (pass the source role), NOT
a fold-time union; it shares NO code with the decl-set mechanism — only the
adjective "proxy". The precedents that DO bear on "does a facade inherit a
wrapped view's annotation" are IdentityJoin.cpp:160-188 (FOLD-and-discard the
guard, M1) and Link.cpp:230-238 (SAVE/RESTORE — proxy does NOT inherit, JOIN
stays authority). Both say: annotation inheritance is ANNOTATION-SPECIFIC, decided
by the downstream consumer. For origin-decl-sets the answer is clearly INHERIT
(union onto the facade — provenance has no "this fold makes it meaningless" case,
unlike the demand guard). For `ProjectionRole` the answer is the (β) question and
is INERT at Stage B (consumes nothing — E4). VERDICT to litigate: SEPARABLE.
Bundling is organizational (same review pass over the same mint sites), not
mechanistic; (β) can be DEFERRED at zero cost since it changes only dump bytes
and no consumer reads role (K5-Q2).

### B4 — THE ADJ-R3-C UNBLOCK (what `CheckDeclaredRegionKey` needs — and whether it still needs it)

ADJ-R3-C's scoped belt: "every declared-key column of a bracketed relation still
resolves to a LIVE visible column of that relation's canonical contract view in
the FINAL graph" (`region-model-diffs.md:1875-1880,2224-2261`). R3A-IMPL-2 then
found it UNIMPLEMENTABLE and DESCOPED it, blaming exactly the missing link K5
supplies: "a NON-demanded bracketed relation has NO decl→view link in the
post-Connect graph at all... the general link IS Tier 2's origin decl-sets... the
belt rides Tier 2" (`region-model-diffs.md:2537-2546`). So what it needs from K5:
given a `@key`'d relation's decl, FIND the live view(s) whose origin set contains
that decl, and check its `visible_fields` (RowContract) still covers the
declared-key columns.

**But guilty-until-proven (a finding):** the belt's ONE non-redundant scenario is
a NON-DEMANDED bracketed relation (a demanded one is covered by Step-2b). RP-6
made `@key` a FORCE-ACTIVATION: an `@key` on an UNDEMANDED relation now REJECTS
(`key_undemanded_1`, the realization reject — CLAUDE.md; the witness
`region_key_dead_relation_1` FLIPPED golden→diagnostic at RP-6,
`region-model-diffs.md:2607-2609`). If EVERY `@key`'d relation is therefore
demanded, the belt's motivating scenario may be UNREACHABLE and the belt fully
redundant with Step-2b — in which case K5 unblocks NOTHING here. This must be
re-confirmed post-RP-6 before citing ADJ-R3-C as a K5 justification (K5-Q1). If
confirmed moot, ADJ-R3-C drops off K5's consumer list entirely.

### B5 — CONSUMERS, RANKED (guilty-until-proven of existence)

The seeds list (identically): undemanded interior naming (`tc_nonlinear_diff`),
Stage-C demand-area member naming, hoist-vs-nest DeterminedBy, cost attribution,
D5 trie column order (stage-b-landed-seed.md:104-106). Ranked by whether a REAL
consumer exists at tip:

1. **Undemanded interior naming — the ONLY consumer with a witness + a live
   freeze reader.** `tc_nonlinear_diff`'s `tc`/`edge` are the cited case
   (stage-b-landed-seed.md:105). BUT (finding, K5-Q7): E3 reports `tc`/`edge`
   are `#local`s with real INSERT sinks and MAY already render `E0 rel=tc`/`E1
   rel=edge` under R-STORE (CollectContractInserts, Planning.cpp:178-196) — yet
   ConnectInsertsToSelects CLEARS `rel->inserts` for non-query rels and re-mints
   an INSERT only `if decl.IsQuery()` (Connect.cpp:285-292), which `tc` is not.
   Whether `tc` actually lacks a contract at tip is UNVERIFIED and decides
   whether the near-term unblock is real. MUST dump `-region-out
   tc_nonlinear_diff.dr` before building. If `tc` is already named, the
   undemanded-interior gap needs a DIFFERENT witness (a merge-materialized
   interior with NO surviving relation-INSERT).
2. **Stage-C demand-area member naming** — a FUTURE consumer; Stage C
   (DIFF-NEXT-S3/K3) is design-only, blocked on the D2.6 STOP and the ownership
   flip (`region-model-diffs.md:2688-2719`). No code to unblock now.
3. **Hoist-vs-nest DeterminedBy** — grounded as "a DeterminedBy question over
   the inner slice's ROW CONTRACT" (owner-adjudication-record.md:194-208);
   DeterminedBy/Minimize itself is the future WIDENING (O-R3.5), unbuilt.
4. **Cost attribution** — needs the (unbuilt) cost model; per-model set (B1-c).
5. **D5 trie column order** — the WCOJ/seekable-iterators direction, explicitly
   deferred (MEMORY: seekable-iterators-wcoj). Farthest out.

HONEST READING: only (1) is a near-term consumer, and (1)'s witness needs
empirical confirmation. (2)-(5) are all speculative — they must NOT inflate the
mechanism. Guilty-until-proven says: build the SMALLEST thing that names a
genuinely-unnameable undemanded interior at freeze (B1-a + one CDaGI union +
freeze read), and add B1-c/per-model only when a ControlFlow consumer lands.

---

## Part C — OPEN QUESTIONS FOR THE ORCHESTRATOR / OWNER

- **K5-Q1 — Is ADJ-R3-C still a real unblock post-RP-6?** RP-6 makes `@key` on an
  undemanded relation a hard reject (`key_undemanded_1`). Arm A (still needed):
  a non-demanded bracketed relation can exist via a route RP-6 does not catch
  (e.g. an `@key` whose relation IS demanded but whose declared-key column is
  canonicalization-dropped — but that is Step-2b's job). Arm B (moot): every
  `@key`'d relation is demanded ⇒ Step-2b covers column survival ⇒ the belt is
  fully redundant ⇒ K5 unblocks nothing here. EVIDENCE to decide: construct a
  program that reaches `CheckDeclaredRegionKey`'s one scenario post-RP-6; if
  none exists, drop ADJ-R3-C from K5's charter. DEFAULT LEAN: likely moot —
  confirm before citing it.
- **K5-Q2 — Bundle or split D2.9(β) proxy-role inheritance?** Arm A (bundle): one
  review pass over the shared proxy-mint sites; the seed says "ONE slice". Arm B
  (split/defer): mechanistically disjoint (identity-copy-at-mint vs satellite-
  union-at-fold, §B3), zero consumer reads role at Stage B, changes only dump
  bytes. EVIDENCE: E4 — "recorded, NOT built", "zero behavioral risk". DEFAULT
  LEAN: litigate together, IMPLEMENT decl-sets first and defer (β) unless a dump
  consumer is named.
- **K5-Q3 — Do the five CDaGI-bypass sites (#3,#6,#7,#15,#20) get instrumented
  now, or accepted as under-naming?** Arm A (close all five): full coverage, but
  five ad-hoc union calls off the choke point — re-introducing the coordination
  smell F1 warns against. Arm B (accept): monotone-safe (under-name only),
  document the gaps, close on demand. EVIDENCE: none of the five feeds the
  near-term undemanded-interior consumer (they are CMP-sink replicas, KV wrap,
  merge unused-col, insert proxy). DEFAULT LEAN: Arm B — one owner (CDaGI), a
  documented five-site gap list, close per real consumer.
- **K5-Q4 — Which belt (if any) guards the satellite?** (i) none; (ii) advisory
  `-origin-out` dump; (iii) debug-only conservation assert (every pre-Connect
  non-demand relation-INSERT decl reachable in some live set or provably dead).
  An always-on ABORT is ruled out — it would convert a safe under-name into a
  crash, contradicting the monotone-safe sanction. EVIDENCE: §B2. DEFAULT LEAN:
  (ii)+(iii).
- **K5-Q5 — One tier (per-view, B1-a) or two (B1-c, + per-model at
  BuildDataModel)?** Arm A (one): the only near-term consumer (freeze naming) is
  DataFlow-side; per-model buys nothing today and DisjointSet::Union has no merge
  hook to ride (E3). Arm B (two): future cost/D5 consumers want per-model. DEFAULT
  LEAN: B1-a only now; add the per-model union when a ControlFlow consumer lands
  (YAGNI — the union at `BuildDataModel` is a cheap add-on later).
- **K5-Q6 — May the design rely on the always-MERGE Connect quirk (§A.3)?** The
  `has_one_insert` dead-branch means single-clause relations mint a MERGE, not a
  TUPLE. Arm A (rely): it is the empirically-real shape Demand already hard-types
  as `MERGE*`. Arm B (don't): if K5 stamps in CDaGI (per-view, node-kind-blind),
  the quirk is IRRELEVANT — no dependence to fix later. DEFAULT LEAN: prefer Arm
  B (choke-point stamp is node-kind-agnostic), which incidentally immunizes K5
  against a future quirk fix.
- **K5-Q7 — Does `tc_nonlinear_diff`'s `tc` actually lack a row-contract at tip?**
  The named witness for the sole near-term consumer may already be nameable via
  R-STORE (§B5.1). EVIDENCE: dump `-region-out` on `tc_nonlinear_diff.dr` at tip.
  If `tc` IS already named, K5 needs a NEW purpose-built witness (a merge-
  materialized interior with no surviving relation-INSERT and no demand) or the
  undemanded-interior gap is narrower than the seed claims. This is a
  PRE-IMPLEMENTATION gate — the slice has no demonstrable consumer until answered.
- **K5-Q8 — Is K5 even ranked first this session?** Session-7 ratified only K1
  first; "K5/K4/K3 unranked" (`region-model-diffs.md:2666-2668`). K5-as-S1 is
  seed LIST ORDER, not a distinct owner ranking (E4). Given K5-Q1 and K5-Q7 both
  cast doubt on the near-term unblocks, confirm the owner still wants K5 first
  vs K4 (the fuzz arm, small/independent, no such doubt) or K3 design-only.

---

## ADJUDICATIONS (session 8, 2026-08-04 — probes + owner ratification)

- **K5-Q7 — PROBE-DECIDED, GAP REAL.** `-region-out` on `tc_nonlinear_diff.dr`
  at tip: census `row-contracts=1`, sole contract `E0 rel=reachable` (the
  query). Neither `tc` nor `edge` (both `#local`, no ABI surface) is nameable.
  The witness stands; the near-term consumer is real.
- **K5-Q1 — OWNER: DROP ADJ-R3-C from the charter.** Post-RP-6 every ACCEPTED
  `@key`'d relation is the demanded relation (unseeded reject +
  `key_undemanded_1` realization reject close the routes), so
  `CheckDeclaredRegionKey`'s one non-redundant scenario (a NON-demanded
  bracketed relation) is unreachable; zero code hits confirm design-only.
  Re-opens only if a non-reject `@key` activation route lands (K2 best-effort).
- **K5-Q2 — OWNER: DEFER D2.9(β).** Litigated here (§B3), implement origin
  decl-sets only; proxy-role-at-mint is a recorded follow-on rider, no consumer
  named. The seed's "as ONE slice" is superseded by the mechanistic-disjointness
  grounding.
- **K5-Q4 — OWNER: belt = (ii)+(iii).** Advisory `-origin-out` dump (reviewed
  like `-contract-out`, never goldened-by-default) + a DEBUG-ONLY conservation
  assert. NEVER an always-on abort. [SESSION-8 PANEL RESCOPE: (iii) is KEPT but
  NARROWED — the owner OVERRULED the panel's delete recommendation (K5P-simp-1)
  and re-scoped the assert to the one provably-non-empty class (every
  `RecognizedSubgraph.demanded_decl` origin-reachable, redundant-with-Tier-1
  defense-in-depth); the query-INSERT arm + `k5_seeded_decl_ids` machinery are
  dropped. See K5-D6 (rescoped) + the K5-D6b TIGERSTYLE battery.]
- **K5-Q5 — OWNER: per-view only (B1-a).** One field on `QueryViewImpl`, one
  union in `CopyDifferentialAndGroupIdsTo`, freeze reads via the friend leak.
  Per-model union at `BuildDataModel` deferred until a ControlFlow consumer
  lands.
- **K5-Q3 — ORCHESTRATOR: Arm B.** The five CDaGI-bypass sites
  (#3,#6,#7,#15,#20) are accepted as documented under-naming gaps, closed per
  real consumer — one owner (CDaGI), no coordination web.
- **K5-Q6 — ORCHESTRATOR: Arm B.** The stamp/union mechanism is
  node-kind-agnostic (choke-point, per-view); no reliance on the always-MERGE
  Connect quirk. The `has_one_insert` dead branch (§A.3, probe-verified:
  `Swap` at Connect.cpp:14 empties `inserts` before the `:17` read) is
  RECORDED as a standalone latent quirk, not a K5 dependency.
- **K5-Q8 — OWNER: S1/K5 ranked FIRST** (session-8 open, explicit).

---

## Part D — THE DIFFS (dated 2026-08-04, session 8)

Numbered dated diffs K5-D1..D8 on the Part A pseudocode, per the standing
dated-diff idiom of `region-model-diffs.md`: each diff states WHERE it lands
(pseudocode line + real tip file:line, every anchor re-read this session),
the exact mechanism, the pseudocode/code delta as a before/after fragment,
the obligations a panel will try to refute, and the blast radius. The charter
is the ADJUDICATIONS section above (BINDING): per-view origin decl-set (B1-a)
on `QueryViewImpl`; ONE union in `CopyDifferentialAndGroupIdsTo`; stamp at the
Connect decl SOURCE; NEVER Hash/Equals, NEVER a lowering input; the five
CDaGI-bypass sites documented, not instrumented; node-kind-agnostic; freeze
consumer = Tier-2 interior naming (generalize the CollectContractInserts +
CollectDemandInteriorDecls pair); belt = advisory `-origin-out` + DEBUG-only
conservation assert; D2.9(β)/ADJ-R3-C/per-model tier all deferred/dropped.

SMALLEST-SOUND-SHAPE throughout (complexity guilty until proven inherent).
The whole mechanism is FOUR landed statements (a field, a seed, a union, an
accessor) + one freeze collector + one dump + one debug assert. It rides the
`group_ids` precedent at every turn — where `group_ids` is a monotone
sorted-vector satellite unioned at the one CDaGI choke point, origin decls are
the same shape with a decl-Id key and a set-dedup.

### K5-D1 — the field (B1-a, per-view origin decl-set)

WHERE. Pseudocode §A.2 row "`RowContract` / `EquivalenceSet`" terrain — the
new satellite is a per-view DataFlow-side field. Real tip: `QueryViewImpl`,
`lib/DataFlow/Query.h:462` (`std::vector<unsigned> group_ids;`) is the sibling
satellite; land the new field immediately adjacent so the two monotone-union
satellites read as a pair.

MECHANISM. Elements are `ParsedDeclaration` VALUES (parse-identity, immutable —
the T1 snapshot property, `region-model-diffs.md:1958-1962`; already stored
by-value on `RecognizedSubgraph::demanded_decl` and in `proxy_view_to_decl`, so
by-value storage is proven). The set GROWS (union-only) and never mutates an
element. TYPE CHOICE: a plain `std::vector<ParsedDeclaration>` maintained
SORTED-UNIQUE BY `decl.Id()` — NOT `std::unordered_set` (iteration order of a
hash set is nondeterministic and this field is RENDERED by `-origin-out` and
drives the Tier-2 contract order, both of which must be a pure deterministic
function of the graph; §A.4/HP-9 determinism discipline). `decl.Id()` is the
canonical key everywhere in the freeze reader (Planning.cpp:190,211,217), is a
parse-identity (mode-stable, pointer-independent), and gives a total order.
Dedup is REQUIRED (unlike `group_ids`, which deliberately keeps duplicates for
the InsertSetsOverlap overlap count — Query.h:462 comment): a provenance set
renders one line per decl.

DELTA (code fragment, Query.h after :462):

```
  std::vector<unsigned> group_ids;                       // tip

+ // K5 (Tier-2 origin provenance): the monotone set of ORIGIN declarations
+ // whose rows flow through this view, sorted-unique by decl Id. Seeded at the
+ // Connect decl SOURCE (Connect.cpp:269), unioned at the ONE CDaGI choke point
+ // (View.cpp:667) exactly like `group_ids`. NEVER folded into Hash/Equals
+ // (never a CSE decision input — the anti-F1 fence, §B2), NEVER a lowering
+ // input; read ONLY by the freeze Tier-2 collector + the advisory -origin-out
+ // dump. Empty default (a view that carries no decl costs one idle vector).
+ std::vector<ParsedDeclaration> origin_decls;
```

Plus the accessor mirroring `QueryView::GuardAnnotationIndex` (Query.cpp:353):
`const std::vector<ParsedDeclaration> &QueryView::OriginDecls() const noexcept
{ return impl->origin_decls; }`, declared in `include/drlojekyll/DataFlow/
Query.h` beside :446 (the compiler-internal accessor band).

INIT. Empty at view mint (DefList `Create()` default-constructs the vector);
no explicit init, matching `group_ids`.

MEMORY. One `std::vector` header (24 B) per view, `nullptr`/empty for the vast
majority that never carry a decl (only insert-proxy descendants ever seed). No
per-view heap allocation until a seed lands. Same idle cost profile as
`group_ids`.

NECESSITY — load-bearing NOW (K5P-nec-1, K5P-note-1). The per-VIEW field (not a
flat decl-Id snapshot on QueryImpl) is REQUIRED today, independent of any
DCE-liveness witness, because K5-D4's `ResolveOriginSupport` resolves the Tier-2
`support=` byte by walking LIVE post-Optimize views `v` with `decl ∈
v.OriginDecls()` and OR-ing `v.CanReceiveDeletions()` — the EXACT Tier-1
`ResolveInteriorSupport` pattern (Planning.cpp:228-241 "NEC-1": support "resolves
off the LIVE post-Optimize graph", the one field the decl cannot supply). A
Connect-time decl-keyed SNAPSHOT cannot supply this: (a) its recorded proxy handle
DANGLES across Optimize (the B1-b use-after-free, §B1); (b) `CanReceiveDeletions`
is a POST-Optimize differential property, not snapshottable at Connect; (c) Tier-2
decls have no Stage-A `RowContract` to back-derive support from (K5-D4). So the
live per-view handle IS the field. The DCE-accurate naming-liveness the nec/note
findings flag (a filtered snapshot over-names a cascade-DCE'd interior) is a
SECONDARY, currently-unwitnessed benefit — NOT the field's justification, and
building the field is NOT gated on first producing a cascade-DCE witness.

OBLIGATIONS. (1) `ParsedDeclaration` has a stable `Id()` and is copyable — VERIFIED
(Planning.cpp uses both). (2) The field is NEVER read by `Hash()`/`Equals()`/
canonicalization — a panel will grep every `origin_decls` use to confirm the
Equals-only-never-Hash D2.9/R1-a fence extends here (it must have ZERO reads in
Tuple.cpp/Optimize.cpp CSE paths). (3) Determinism: sorted-by-Id, never by
pointer — the HP-9 belt.

### K5-D2 — the stamps (the decl SOURCE, one seed site)

WHERE. Pseudocode §A.1 line `proxy_view_to_decl.emplace(insert_proxy,
rel->declaration)  # Connect.cpp:269 — THE STAMP`; census §A.5 site #1
(Connect.cpp:38). Real tip: `lib/DataFlow/Connect.cpp:269`, co-located with the
existing T1 side-channel stamp, INSIDE the `for (REL *rel : relations)` loop
(Connect.cpp:231) after `insert_proxy` is fully formed (:262-263) and BEFORE
`rel->inserts.Clear()` (:271) severs the REL→proxy edge.

MECHANISM. EXACTLY ONE seed site. `insert_proxy` (the value returned by
`CreateProxyForMutableParams(CreateProxyOfInserts(...), decl)`) is the top of
the relation's definition — a MERGE (monotone/normal), or a TUPLE-over-KVINDEX
(mutable arm). Seed `insert_proxy->origin_decls = { rel->declaration }`
(node-kind-AGNOSTIC — K5-Q6 Arm B; the always-MERGE `has_one_insert` quirk
§A.3 is IRRELEVANT because we stamp the OUTER returned view, not a kind-typed
one). This is the decl SOURCE; every downstream fold carries it forward via
K5-D3's CDaGI union, so no other seed is needed.

WHY co-located at :269 and not inside `CreateProxyOfInserts` (#1/Connect.cpp:38).
The decl is not in scope inside `CreateProxyOfInserts` (it takes only
`UseList<QueryViewImpl> &inserts`); :269 is the first site the decl and the
final proxy coexist — precisely why T1 stamps there too. Stamping the OUTER
`insert_proxy` also SIDESTEPS census gap #3 (`CreateProxyForMutableParams`
omits CDaGI, Connect.cpp:73-125): the KV-wrap TUPLE it returns IS the seed
target, so the mutable arm gets a correct SEED regardless of the missing inner
migration (gap #3 only loses the MERGE's group_ids/diff-flags across the KV
wrap, a pre-existing concern orthogonal to the origin SEED).

STAMPS DELIBERATELY NOT PLACED (guilty-until-proven — no consumer need):
- **ProxySelects (#2, Connect.cpp:143)** — the SELECT-side reader proxies. The
  naming consumer wants the DEFINITION (merge-materialized model = the
  `insert_proxy` side), not the reader side; the select proxies READ
  `insert_proxy` (input_columns wired to its columns at :146-150) and carry no
  model. No seed.
- **Query decls, BOTH nodes (the insert_proxy AND the terminal re-minted query
  INSERT, Connect.cpp:285-292)** — query decls are already R-STORE-named via
  `CollectContractInserts` (Planning.cpp:182-193 walks `query.Inserts()`); Tier-2
  dedups against them (K5-D4). Naming their origin is redundant. NEITHER is
  seeded: the terminal INSERT is a different node the seed never touches, AND the
  insert_proxy is guard-excluded by `!decl.IsQuery()` (subsumed by `!IsInline()`
  today) — the prose asymmetry the K5P-corr-2 panel flagged is closed, both query
  nodes are unseeded.
- **@inline decls, and queries** — `decl.IsInline()` returns
  `IsQuery() || @inline` (Parse.cpp:998-1001), so a `#query` decl is ALREADY
  excluded by `!decl.IsInline()`. An @inline decl is DELIBERATELY
  non-materialized (spliced into readers; no table); a query is already
  R-STORE-named via `CollectContractInserts` (seeding its insert_proxy is dead
  weight K5-D4 dedups away). A row-contract implies a NAMED logical relation —
  several of which may model-SHARE one physical table, so the contract:table map
  is NOT 1:1 (demand_tc_witness renders `row-contracts=2` over 6 physical tables;
  merge_2's `outer` is model-shared into `q_outer`'s table:4, EQ SET 6 — it is
  STORED though it owns no own-named table, K5P-corr-1). GUARD the seed:
  `if (!decl.IsInline() && !decl.IsQuery() && decl.Arity())
  insert_proxy->origin_decls.push_back(rel->declaration);` (the `!decl.IsInline()`
  conjunct is the merge_2 blast-radius fix — see K5-D4; `!decl.IsQuery()` is
  REDUNDANT at tip — `IsInline()` already subsumes it, Parse.cpp:999 — but is
  written explicitly as FORWARD-LOOKING HYGIENE, K5P-corr-2: it is the exact
  complement of the Connect.cpp:285 terminal-INSERT re-mint condition and
  forecloses a future CCI-vs-COID decoupling from emitting a spurious per-mode
  Tier-2 line should `IsInline` ever drop its `IsQuery()` disjunct). Condition/unit
  relations (`decl.Arity()==0`) already `continue` at Connect.cpp:243-253 before
  reaching :269.

DELTA (Connect.cpp after :269):

```
  proxy_view_to_decl.emplace(insert_proxy, rel->declaration);   // tip (T1)
+ // K5 seed: the decl SOURCE for the per-view origin set. Skip @inline
+ // (non-materialized -> never a row-contract) AND queries (R-STORE-named;
+ // !IsQuery() redundant-but-explicit forward-looking hygiene, Parse.cpp:999).
+ // Node-kind-agnostic (K5-Q6 Arm B).
+ if (!decl.IsInline() && !decl.IsQuery()) {
+   assert(insert_proxy->origin_decls.empty());  // TIGERSTYLE: seed-once
+   insert_proxy->origin_decls.assign(1u, rel->declaration);
+ }
  rel->inserts.Clear();                                         // tip :271
```

OBLIGATIONS. (1) `insert_proxy` is the SAME view whose downstream survivors
carry the model at freeze — the panel will trace tc/edge/p/r from :269 through
Optimize to confirm the seed reaches the frozen model view (K5-D7 pins it). (2)
The @inline skip is SOUND: an @inline decl never gets a distinct DataModel table
(FillDataModel) — verify against merge_2's `inner`/`proj`. (3) One seed only:
no ProxySelects/terminal-INSERT seed — the panel will argue a reader-side
consumer might want it; today none does (ADJ-R3-C dropped, K5-Q1).

### K5-D3 — the union (one statement in the CDaGI choke point)

WHERE. Pseudocode §A.5 site #4 (the canonical CSE fold) + every YES-routing
site (2,5,8-14,16-19) collapse to ONE statement here; §A.4.2 "the correct
mechanism target is the choke point, not CSE". Real tip:
`QueryViewImpl::CopyDifferentialAndGroupIdsTo`, `lib/DataFlow/View.cpp:662-727`,
IMMEDIATELY after the `group_ids` union+sort (View.cpp:665-667), before the
`can_receive_deletions` block (:669).

MECHANISM. `this` = LOSER (being replaced), `that` = SURVIVOR (the standing
CDaGI contract, View.cpp:662 comment + :677-683). Union `this->origin_decls`
INTO `that->origin_decls`: append, sort by `decl.Id()`, unique by `decl.Id()`,
erase the tail. This is a monotone SET union — IDEMPOTENT, so a decl present on
both loser and survivor collapses (the exact reason dedup lives here, not in the
freeze reader). Loser-into-survivor DIRECTION matches `group_ids` (:665-666
appends `this`'s into `that`'s).

NO CLEAR-ON-`this` (the §B3 SEPARABILITY fact). `group_ids` does NOT clear
`this` (:665-667); `guard_annotation_index` DOES (:724, because it is a UNIQUE
scalar the demand census counts once — an uncleared index would double-count on
a `SubstituteAllUsesWith` survivor-both-live funnel, :681-683). `origin_decls`
is a SET, not a counted scalar: a decl lingering on a to-be-deleted `this` is
harmless (dead views are never read by the freeze collector), and on the
`SubstituteAllUsesWith` case where `this` legitimately stays live (#11
Join.cpp:424 dup-output; the JOIN self-canon funnel), leaving the set on both is
CORRECT — the set is additive and the freeze collector dedups by Id across all
live views. So: pure append+sort+unique into `that`, NO mutation of `this`.
This is the INHERIT-onto-facade answer §B3 predicts (provenance has no "this
fold makes it meaningless" case, unlike the demand guard's save/restore at
Link.cpp:230-238 or the fold-and-discard at IdentityJoin.cpp:160-188).

SAFETY INVARIANTS (litigated — K5P-note-3, K5P-corr-1). Two invariants, both
already true at tip, anchor the union's soundness; both must be PRESERVED by any
future CDaGI caller added off the choke point:

- **Survivor-DERIVES-FROM-loser (the anti-mis-attribution invariant, K5P-note-3).**
  The union is safe NOT because "a mis-attribution would require a pre-existing
  RAUW soundness bug" (too narrow — several CDaGI callers are non-RAUW proxies:
  ProxyUnusedInputColumns Join.cpp:279, ProxySelects Connect.cpp:143, the CMP
  sink replicas #6/#7, where the survivor is a fresh proxy that READS the loser).
  The load-bearing invariant is that at EVERY CDaGI call site the survivor
  DERIVES FROM the loser — RAUW-equal survivors OR fresh projection/proxy
  survivors reading the loser as an input (verified live-both cases:
  Join.cpp:447 dup-output facade reads the JOIN's own cols, Induction.cpp:446
  `new_union->merged_views.AddUse(view)`). So a loser's origins always
  legitimately flow into the survivor: over-naming at worst, NEVER attribution to
  a view whose rows do not flow through it. A future sideways copy would
  mis-attribute WITHOUT tripping any RAUW check — hence the invariant, not the
  RAUW framing, is the K5 correctness anchor.
- **Differentialness-migration (makes `support=` sound, K5P-corr-1).** CDaGI
  OR-propagates `can_receive_deletions` loser→survivor (View.cpp:669-674) IN
  LOCKSTEP with, and immediately after, this origin union; and CSE co-location is
  differentialness-GATED — `HashInit` folds `can_receive_deletions`/
  `can_produce_deletions` into the base hash (View.cpp:422-424, consumed by
  Merge/Join/Tuple/Select Hash), so views of differing differentialness never
  share a CSE bucket, never Equals-compare, never merge. The two no-clear RAUW
  funnels (Merge.cpp:175, Join.cpp:424) are intra-relation same-flow folds.
  THEREFORE every live view carrying origin decl D satisfies
  `CanReceiveDeletions() == D`'s true differentialness, and K5-D4's
  `ResolveOriginSupport(D)` yields D's true `support=` byte (the OR over carriers
  is redundant, never wrong). This is stated so a future fold that breaks the
  Hash-gate (a new cross-differentialness RAUW, or dropping
  `can_receive_deletions` from `HashInit`) is a KNOWN hazard — K5-D6b guards it.

DELTA (View.cpp after :667):

```
  that->group_ids.insert(that->group_ids.end(), group_ids.begin(),   // tip
                         group_ids.end());
  std::sort(that->group_ids.begin(), that->group_ids.end());

+ // K5 (Tier-2 origin provenance): union this (loser) -> that (survivor),
+ // sorted-UNIQUE by decl Id. A SET, not a counted scalar -> unlike
+ // guard_annotation_index (:724) it NEVER clears `this` (a live-both funnel
+ // keeps the set on both; the freeze collector dedups by Id). Monotone,
+ // Equals/Hash-blind (§B2): a missed union under-names, never miscompiles.
+ that->origin_decls.insert(that->origin_decls.end(),
+                           origin_decls.begin(), origin_decls.end());
+ std::sort(that->origin_decls.begin(), that->origin_decls.end(),
+           [](ParsedDeclaration a, ParsedDeclaration b) {
+             return a.Id() < b.Id();
+           });
+ that->origin_decls.erase(
+     std::unique(that->origin_decls.begin(), that->origin_decls.end(),
+                 [](ParsedDeclaration a, ParsedDeclaration b) {
+                   return a.Id() == b.Id();
+                 }),
+     that->origin_decls.end());
+ // TIGERSTYLE: the survivor set is strictly sorted-unique by decl Id post-union
+ // (unique() removed adjacent equals; strict-sorted <=> unique here).
+ assert(std::is_sorted(that->origin_decls.begin(), that->origin_decls.end(),
+                       [](ParsedDeclaration a, ParsedDeclaration b) {
+                         return a.Id() < b.Id();
+                       }));
```

COVERAGE. This single statement covers census §A.5 sites 2,4,5,8-14,16,17,18,19
(all marked "Routes CDaGI? YES") for free — including the two EXPLICIT-CDaGI
callers #16 (Induction.cpp:444, the inductive-leave leg) and #17
(Join.cpp:279, ProxyUnusedInputColumns) which already call CDaGI, and the
save/restore facade #18 (Link.cpp:232, ProxyMergedViews) — origin sets INHERIT
there with NO save/restore (§B3). RESIDUAL GAPS: #3 (KV wrap — but the SEED is
placed on its output, K5-D2), #6/#7 (CMP-sink replicas), #15 (Merge unused-col
guard, no CDaGI — K5-D8 rider), #20 (Link ProxyInsertWithTuple, INSERT-adjacent).
All five are documented under-name gaps (K5-Q3 Arm B), none feeds the near-term
undemanded-interior consumer.

OBLIGATIONS. (1) The NO-CLEAR decision under `SubstituteAllUsesWith`
survivor-both-live: a panel will construct the JOIN dup-output self-canon (#11)
and demand proof that leaving the set on both live views cannot MIS-ATTRIBUTE a
decl to a wrong interior — answer: over-naming at worst (a shared JOIN facade
lists both decls), which the monotone-safe framing sanctions; a decl is never
attributed to a relation whose rows do not flow through the view. (2)
Equals/Hash-blindness: the union runs in CDaGI, which is NOT on the Hash/Equals
path — the F1 fence holds. (3) Idempotence + Id-sort make repeated folds
(fixpoint canonicalization rounds) stable — no unbounded growth, no order drift.

### K5-D4 — the freeze read (Tier-2 interior contracts)

WHERE. Pseudocode §A.1 freeze block `row-contracts := CollectContractInserts ++
CollectDemandInteriorDecls`. Real tip: `lib/Regional/Planning.cpp` — generalize
the (CollectContractInserts :178-196, CollectDemandInteriorDecls :207-226)
PAIR with a THIRD collector `CollectOriginInteriorDecls`, append a THIRD contract
loop after the Tier-1 loop (:522-538), and add a THIRD term to
`DeriveRegionalCensus.row_contracts` (:297-299).

MECHANISM (the collector). Walk live views (`query.ForEachView`); accumulate the
union of `v.OriginDecls()` across all live views into a `decl.Id()`-keyed set;
DEDUP against (a) insert-named decls (`CollectContractInserts`) AND (b) Tier-1
demand-interior decls (`CollectDemandInteriorDecls`). The residual — decls
reachable ONLY via origin sets — are the Tier-2 contracts. Emit in ASCENDING
`decl.Id()` order (a pure, mode-stable total order; the per-view sets are already
Id-sorted, so the cross-view merge is a k-way dedup). This EXACTLY mirrors
`CollectDemandInteriorDecls`'s dedup-against-insert-names idiom (:209-219),
extended with the second dedup arm and the origin-set source in place of
`rs.demanded_decl`.

```
+ static std::vector<ParsedDeclaration>
+ CollectOriginInteriorDecls(const ::hyde::Query &query) {
+   std::unordered_set<uint64_t> named;            // insert-named U Tier-1
+   for (const auto &[d, ins] : CollectContractInserts(query)) named.insert(d.Id());
+   for (ParsedDeclaration d : CollectDemandInteriorDecls(query)) named.insert(d.Id());
+   std::map<uint64_t, ParsedDeclaration> out;     // Id-ordered, deduped
+   query.ForEachView([&](QueryView v) {
+     for (ParsedDeclaration d : v.OriginDecls())
+       if (!named.count(d.Id())) out.emplace(d.Id(), d);
+   });
+   return { values of out, in key order };
+ }
```

MEMBER-KEY. `AllParamNames(decl)` rendered positionally — the SAME call the
Tier-1 loop uses (Planning.cpp:533); an undemanded interior keeps the
AllFields/passthrough contract (the ORC-3 precedent). No RowContract lookup
needed (Tier-2 decls have no insert view, hence no Stage-A `row_contracts` entry
— exactly why R-STORE's `row_contracts.find` would abort on them; Tier-2 uses
the decl's declared params directly).

SUPPORT= (litigated). Tier-1's `ResolveInteriorSupport` ORs `CanReceiveDeletions`
over live annotated guard JOINs (Planning.cpp:246-282). A Tier-2 interior has NO
guard JOIN (no demand). The natural analog: `ResolveOriginSupport(query, decl)` =
OR over live views `v` with `decl ∈ v.OriginDecls()` of `v.CanReceiveDeletions()`
— the deletability of the interior's own flow. For tc_nonlinear_diff `tc`/`edge`
descend from a `@differential` message → `CanReceiveDeletions()` true →
`support=differential`; merge_2 `outer` is monotone. Existence is decl-counted
FROM live origin sets, so ≥1 carrier always exists; keep the RES-2 loud-abort
(mirrors :273-281) as a can't-happen belt so a resolve failure never silently
drops a counted line.

SUPPORT= SOUNDNESS (litigated — K5P-corr-1). The OR-over-carriers cannot
mis-attribute the `support=` byte because of the differentialness-migration
invariant (K5-D3): CDaGI OR-propagates `can_receive_deletions` loser→survivor in
lockstep with the origin union, and CSE co-location is differentialness-gated via
`HashInit` (View.cpp:422-424) — so every live view carrying decl D has
`CanReceiveDeletions() == D`'s true differentialness, and `ResolveOriginSupport(D)`
yields D's TRUE support in every mode. This makes `support=` a semantic
(mode-invariant) property of the relation's flow, not an artifact of which
carrier the OR happens to visit. K5-D6b turns the invariant into a DEBUG belt: for
each emitted Tier-2 decl D, assert all live views `v` with `D ∈ v.OriginDecls()`
AGREE on `v.CanReceiveDeletions()` (the OR result is not a mix) — a future
differentialness-crossing fold then trips a debug abort instead of silently
emitting a wrong/mode-split support golden byte.

CENSUS. `DeriveRegionalCensus.row_contracts` (Planning.cpp:297-299) gains the
third term `+ CollectOriginInteriorDecls(query).size()`. V-REGION-CENSUS
(stored-vs-rederived, always-on, :566) stays consistent BY CONSTRUCTION: the
build loop and `DeriveRegionalCensus` both call the SAME collector (the
CollectDemandInteriorDecls "consulted IDENTICALLY" precedent, :204-206). No
divergence possible.

DELTA (Planning.cpp after the Tier-1 loop :538):

```
+ // ---- Tier-2 ORIGIN-INTERIOR contracts (K5): undemanded #local/#export
+ // interiors nameable ONLY via origin decl-sets, appended after Tier-1 on the
+ // same dense `edge` counter, ascending decl Id. member-key = AllFields
+ // positional; support = OR over origin-carrying live views' CanReceiveDeletions.
+ for (ParsedDeclaration decl : CollectOriginInteriorDecls(query)) {
+   RegionalContract contract;
+   contract.edge_index = edge++;
+   contract.rel_name = std::string(decl.NameAsString());
+   contract.member_key_text = std::string(AllParamNames(decl));
+   contract.support_text =
+       ResolveOriginSupport(query, decl) ? "differential" : "monotone";
+   contract.declared_key = decl.HasInstanceKey();
+   out.contracts.push_back(std::move(contract));
+ }
```

MARKER CHOICE (litigated — the goldens are byte-law). Arm A: render Tier-2 as
plain `row-contract Ei rel=NAME ...`, INDISTINGUISHABLE from R-STORE/Tier-1.
Arm B: a distinguishing token (`origin=` in place of `rel=`, or a trailing
`via=origin`). VERDICT: Arm A. The adjudication says interiors "become
row-contracts" (same surface); the dropped ADJ-R3-C means no consumer needs the
tier; and the tier-1 vs tier-2 distinction is provenance-only, served by the
advisory `-origin-out` dump (K5-D5), not the contract line. Arm A is
LINE-ADDITIVE (Tier-2 appends E2+ and bumps the census count) but NOT always
byte-additive: the emitter's `member-key` column width is a PER-DUMP MAX over
all contract rows (Format.cpp:128, E-K5-PAD in Part E), so a new Tier-2 row
with a longer member-key RE-PADS existing lines' trailing alignment (join_1:
E0/E1 gain 3 trailing spaces). SEMANTICALLY additive, byte-diff-reviewed per
file at bless (Part E carries the exact diffs). This holds for
the `support=` token too: Tier-0/Tier-1 support is resolved by
`view.CanReceiveDeletions()` (Planning.cpp:516-517) and `ResolveInteriorSupport`
(:534-535), NEITHER of which reads `origin_decls` — so no EXISTING support byte
moves; only the NEW Tier-2 line's own `support=` byte is origin-derived
(K5P-corr-1). If a future consumer needs the tier, add a TRAILING advisory token
(never perturbs existing bytes).

BLAST RADIUS (the 16 .region goldens = 4 cases × 4 modes; key_tc_witness's 4 are
SYMLINKS to demand_tc_witness's). Per case, does a non-inline undemanded #local
interior exist?

- **demand_tc_witness** — sole #local `path` is the DEMANDED relation (Tier-1
  E1); `reachable_from` is the query (R-STORE E0); `edge_2` is a message. No
  undemanded non-inline interior. **UNCHANGED** (and key_tc_witness symlinks —
  UNCHANGED).
- **demand_multi_adorn_witness** — `rel` is the demanded relation (Tier-1 E1);
  `q` is the query (E0). No other #local. **UNCHANGED.**
- **join_1** — `p`, `r` are plain #locals (non-inline, undemanded, non-query),
  read by the `q`/`never` JOINs. Tier-2 NAMES both. **CHANGES**: appends
  `E2 rel=p member-key=(A, B) support=monotone` and `E3 rel=r member-key=(A)
  support=monotone`, census `row-contracts=4`. Id order `p` before `r` is
  correct but NOT because `p` is declared first — decl.Id() is dominated by the
  STRING-POOL INTERN OFFSET of the name (see K5-D7); `p`'s offset (24) < `r`'s
  (26), and neither suffix-aliases, so line order and Id order coincide here ONLY
  by luck. (Note join_1's `.contract.opt.golden` is a Stage-A dump, UNTOUCHED —
  different surface.)
- **merge_2** — `inner`/`proj` are `@inline` (SEED-skipped, K5-D2) → not named;
  `outer` is a non-inline undemanded #local read by `q_outer`. Tier-2 names
  `outer`. **CHANGES**: appends `E2 rel=outer member-key=(X, Y) support=monotone`,
  census `row-contracts=3`. HARDENING (K5P-corr-1 REFUTED evidence): naming
  `outer` does NOT violate "a row-contract implies a stored relation" even though
  no `outer`-NAMED table exists — `outer`'s UNION view is model-SHARED into
  `q_outer`'s DataModel (both in EQ SET 6 → one DataModel → physical table:4; the
  `outer`-arm `kEagerUnion` markers carry `table=%table:4`, E-107). Every LIVE
  non-inline relation's rows are materialized SOMEWHERE — own table, an F27 dedup
  table, or a model-shared table; `!IsInline()` discriminates
  spliced/never-materialized (which `outer` is not), it does NOT claim
  owns-a-table.

RE-BLESS SET (corrected per Part E / E-K5-PAD): `join_1.region.{opt,nodf,nocf,
none}` (4, NON-byte-additive — E0/E1 member-key column re-pads +3 spaces) +
`merge_2.region.{opt,nodf,nocf,none}` (4, byte-additive) + the NEW
`tc_nonlinear_diff.region.{opt,nodf,nocf,none}` (4, K5-D7) = **12 `.region`
golden files** total. Everything
outside `-region-out` is untouched (stdout/df/rel/ir/contract/behavioral
goldens do not carry region text). The default `-region-out` DOES grow
corpus-wide (every non-inline undemanded #local becomes a contract) — stated
plainly per the task — but only these two goldened cases realize it in the
suite; V-REGION-CENSUS stays green corpus-wide by the same-collector construction
above.

PREDICT-THEN-VERIFY GATE. The merge_2 line is the one to VERIFY at
implementation: the claim that `outer` is the SOLE Tier-2 add (i.e. `inner`/
`proj` are @inline-skipped AND no spliced-decl origin lands on a shared model
that over-names) must be confirmed by an actual `-region-out merge_2.dr` dump
BEFORE blessing. join_1 (`p`,`r`) and tc_nonlinear_diff (`tc`,`edge`) are the
robust predictions; merge_2 is the guilty-until-verified one.

OBLIGATIONS. (1) OVER-NAMING via CDaGI union onto a shared JOIN/MERGE facade
(§B3 / D3 no-clear) — could a decl land on a live view whose rows it does NOT
represent? The panel's sharpest attack; answer = ADDITIVE for the `rel=`/
`member-key` columns (monotone-safe over-naming), and for the `support=` byte
SOUND ONLY under the differentialness-migration invariant (K5-D3 / SUPPORT=
SOUNDNESS above, now DEBUG-asserted by K5-D6b) — the blanket "additive/monotone-
safe" of the first draft did NOT cover the support token, K5P-corr-1. @inline-skip
removes the known category error, but the exact merge_2 set is the verify gate.
(2) MODE-STABILITY of the Tier-2 set AND its support byte: does every mode keep
the interior materialized, and does the `support=` VALUE match across all 4 modes
(support is a semantic mode-invariant flow property — a per-mode split would be a
bug, K5P-corr-1)? A relation that is a real recursive/joined interior (tc, p, r,
outer) is materialized in ALL modes; the panel will probe a case where opt
eliminates an interior in one mode only (would split the per-mode goldens — none
of the 4 cases has one, but a corpus sweep at implementation must confirm no
OTHER region-goldened case gains a mode-split). (3) Dedup completeness: a decl
that is BOTH origin-carried AND demand-interior must appear ONCE (Tier-1 wins) —
the `named` set's second arm enforces it.

### K5-D5 — the `-origin-out` dump (advisory belt (ii))

WHERE. Pseudocode DOT-twin/dump family (§A.1 has none for origin). Real tip:
mirror the `-contract-out` sink (Main.cpp:406-421 arg parse; :86-93 the
`gRegionStream` top-level drain; the `SetRelDumpStream` install idiom :99). The
origin sets live on the DataFlow graph and are final after Query::Build, so the
drain sits RIGHT AFTER `Query::Build` returns (like `-dot-out`), BEFORE freeze.

MECHANISM. A new `-origin-out <PATH>` flag + `hyde::gOriginStream` global +
`FileStream origin_out`, byte-for-byte the `-contract-out` plumbing (Main.cpp:
407-421) and one usage line beside :240. Drain: `if (gOriginStream) {
(*gOriginStream) << QueryOriginDump{*query_opt}; gOriginStream->Flush(); }`.

FORMAT (litigated: per-VIEW, not per-decl — K5P-simp-2 refuted the per-decl
alternative). Per-decl would merely restate the Tier-2 contract list (already
golden-pinned) and LOSE the per-view "which view dropped the union mid-graph"
signal that is this belt's sole non-redundant value; the BELT's diagnostic value
is showing WHERE each decl's provenance landed (a missed union shows as a decl
absent from the view that should carry it). Header token `origin-sets`; one line
per live view with a NONEMPTY set. ROW ORDER keys on `(min decl.Id() in the
view's origin set, then det_seq as tie-break)` — a PAYLOAD-COVARYING primary key
(K5P-simp-2): since the dump is never goldened-by-default its only review is
manual/diff, and keying row order on the volatile `det_seq` insertion counter
alone (Query.h:469-476, re-stamped on any graph change) maximizes line-diff churn
even when NO origin set changed; a payload-stable primary sort keeps
same-provenance rows adjacent across compiler versions and the 4 modes, so a
genuine missed union is not buried under det_seq renumber churn. `det_seq` stays
PRINTED as the view label and as the tie-break (deterministic per mode; cross-mode
divergence is fine for an advisory dump):

```
origin-sets
  view=<det_seq>  kind=<MERGE|TUPLE|JOIN|...>  origin=(declA, declB, ...)
  ...
```

`origin=(...)` renders the decls in the view's stored Id order. ADVISORY,
NEVER-GOLDENED-BY-DEFAULT (K5-Q4 (ii); reviewed like `-contract-out`, not pinned).

GOLDEN PIN (litigated). NONE by default. `-contract-out` has 3 blessed pins
because it is the SOLE witness of the Stage-A contract layer; `-origin-out` is
NOT the sole witness of the origin mechanism — the Tier-2 `.region` goldens
(join_1, merge_2, tc_nonlinear_diff) pin the CONSUMER, which is the mechanism's
observable effect and is fail-closed on a dropped union (K5-D7). Adding a
`.origin` golden would additionally have to contend with `det_seq`'s per-mode
divergence (a 4-way-split pin). LEAN: no default pin; the belt stays advisory;
a pin can be added later if under-naming regressions recur.

SMOKE COVERAGE (K5P-test-3). The `-origin-out` flag is never passed by any suite
arm (like the three existing advisory DOT twins `-dot-out`/`-rel-dot-out`/
`-region-dot-out`, all un-wired), so `gOriginStream` is null in the suite and the
`QueryOriginDump::operator<<` path would ship UNEXERCISED (a crash-on-empty /
abort / format-rot would pass suite-green). CLOSE it with a PRODUCE-AND-EXIT-CHECK
clause: append `-origin-out "$iout/origin.$mode.out"` to the `run_irgold` compile
invocation (runall.sh:330-336, alongside the other `-*-out` flags) so the dump
path is PRODUCED on every `.irgold` case in each pinned mode — a crash/abort then
fails the compile as IRGOLD-FAIL (:338). Do NOT byte-compare and do NOT bless an
`.origin` golden — `det_seq`/min-Id ordering is per-mode divergent by design, so
any golden would 4-way split. Coverage falls out for free: tc_nonlinear_diff's
new `.irgold` (K5-D7) exercises the NONEMPTY-set path; the other irgold cases
(no origin sets) exercise the empty path, closing the crash-on-empty class. (The
three un-wired DOT twins warrant the same treatment as a separate hygiene item,
not gating K5.)

OBLIGATIONS. (1) The dump must be a PURE function of the frozen graph (min-Id
primary key, det_seq tie-break, Id-ordered origin lists) — no pointer iteration.
(2) Null-safe when unset (guarded no-op, the `SetRelDumpStream` idiom
Main.cpp:97-99). (3) It reads `OriginDecls()` only — never triggers a fold or
mutates state.

### K5-D6 — the DEBUG-only conservation assert (belt (iii)) — RESCOPED (session 8)

OWNER RULING (session 8): the panel (K5P-simp-1 CONFIRMED) recommended DELETING
this belt as near-vacuous. The owner OVERRULED the delete and RESCOPED it instead:
the assert is KEPT, narrowed to the one PROVABLY NON-EMPTY class, and explicitly
acknowledged as redundant-with-Tier-1 defense-in-depth. The vacuous query-INSERT
arm of `live_ref` and the `k5_seeded_decl_ids` snapshot machinery are DROPPED —
the rescoped assert derives the smallest form that still asserts something real.

WHY the first draft was over-built (K5P-corr-2 CONFIRMED / K5P-simp-1). The K5-D2
seed and its `k5_seeded_decl_ids` capture were BOTH `!decl.IsInline()`-guarded,
and `IsInline()` is true for every query (Parse.cpp:999), so the query class was
EMPTY of seeded ids and the query-INSERT arm of `live_ref` was DEAD CODE — it
could never cause a fire. The `provably_dead` predicate's query-INSERT disjunct
likewise gated nothing. What SURVIVES with real teeth is exactly the
DEMAND-FORCING class: a demanded interior (e.g. demand_tc_witness's `path`) is a
non-inline #local (hence seeded, hence origin-carried) AND is an
`rs.demanded_decl`, so its origin MUST reach a live view.

WHERE. Pseudocode §A.1 `Query::Build` tail, after Optimize and after origin sets
are final (the `InferConservativeRowContracts` slot, Build.cpp:2655). Real tip: a
`#ifndef NDEBUG` block at the Build tail. NO Connect-time snapshot is needed — the
rescoped assert reads only `recognized_subgraphs` (durable) and live-view origin
sets.

MECHANISM (rescoped). Compute `reachable` = union of `v.OriginDecls()` Ids over
LIVE views. For each `RecognizedSubgraph rs`, ASSERT `rs.demanded_decl.Id() ∈
reachable`. This asserts something REAL: a demanded interior is never DCE'd (it is
demanded), so it MUST have a live origin carrier; the assert independently checks
that the K5-D2 seed + K5-D3 union propagated its decl through Optimize to a live
view. It is redundant-with-Tier-1 (the Tier-1 collector already resolves
`rs.demanded_decl`), deliberately, as defense-in-depth on the K5 origin path.

DELTA (Build.cpp, #ifndef NDEBUG at the Build tail):

```
+ // K5 conservation belt (DEBUG-only, RESCOPED): every demanded interior's
+ // decl MUST be origin-reachable at a live view — the seed+union propagated it
+ // through Optimize. Redundant-with-Tier-1 defense-in-depth on the origin path.
+ // (No query arm: queries are !IsInline-skipped from the seed; the pure-interior
+ // class is delegated to the advisory -origin-out dump + the fail-closed
+ // opt/nocf .region goldens, K5-D7.)
+ std::unordered_set<uint64_t> reachable;
+ ForEachView([&](VIEW *v){ for (auto d : v->origin_decls) reachable.insert(d.Id()); });
+ for (RecognizedSubgraph &rs : recognized_subgraphs)
+   assert(reachable.count(rs.demanded_decl.Id()));  // demanded-interior origin belt
```

PRE-IMPLEMENTATION GATE (K5P-corr-2). Before landing, VERIFY that at least one
suite demand case's demanded interior actually undergoes a CDaGI FOLD between the
Connect seed and freeze such that a broken K5-D3 union would drop it from
`reachable` while the seed survived on the (now-dead) original view — otherwise
the assert has no EXECUTABLE positive test and must be documented as a
gross-drop tripwire with no live witness (or the demand corpus extended to
provide one). demand_tc_witness's `path` is the candidate; confirm its
insert_proxy is fold-migrated, not merely seed-carried, in the compiled mode.

OBLIGATIONS. (1) SCOPE is honest: this policies ONLY the demand-forcing class;
the pure-interior class (p/r/tc/edge/outer) is NOT policed here (no independent
liveness oracle survives Optimize) — delegated to the fail-closed opt/nocf
`.region` goldens (K5-D7) + the advisory `-origin-out` (K5-D5). (2) No spurious
fire: a demanded interior is never DCE'd, so `reachable` always contains it in
correct code; a fire is a genuine gross drop (seed omitted or union moved off the
choke point). (3) DEBUG-only — zero release cost; NO `#ifndef NDEBUG` QueryImpl
member is added (the F1-disfavored coordination satellite of the first draft is
eliminated by the rescope).

### K5-D6b — the TIGERSTYLE assert battery (invariant choke points)

Per the standing TigerBeetle-style directive (lots of assertions, positive AND
negative space, intent-communicating), the mechanism's invariant choke points
each carry a DEBUG assert. Some are placed inline at their diff; this subsection
is the roster (an assert that lives in another diff is cited, not duplicated):

- **Seed-once (K5-D2, positive).** `assert(insert_proxy->origin_decls.empty())`
  immediately before the seed `assign` — the decl SOURCE is stamped exactly once
  per relation; a second seed onto a non-empty set is a structural bug.
- **Post-union sorted-unique (K5-D3, positive).** `assert(std::is_sorted(...by
  Id...))` at the CDaGI union tail — the survivor set is strictly Id-ordered and
  deduped, the determinism precondition K5-D5/K5-D4 both rely on.
- **Support-agreement (this diff, positive — K5P-corr-1).** For each Tier-2 decl
  D emitted by K5-D4, `assert` that all live views `v` with `D ∈ v.OriginDecls()`
  agree on `v.CanReceiveDeletions()` (the OR is not a mix) — turns the
  differentialness-migration invariant (K5-D3) into a tripwire so a future
  cross-differentialness fold aborts instead of emitting a wrong/mode-split
  `support=` byte.
- **Freeze-collector postconditions (K5-D4, NEGATIVE space).** In
  `CollectOriginInteriorDecls`, for each decl D it is about to emit as a Tier-2
  contract, `assert` the negative space: `!D.IsInline()` (never spliced),
  `!D.IsQuery()` (never a query — R-STORE's job), `!named.count(D.Id())` (NOT
  insert-named AND NOT a Tier-1 demand-interior — the dedup arms held). A Tier-2
  decl is, by construction, exactly the residue outside all three prior naming
  tiers; asserting it makes a dedup regression (a decl double-counted across
  tiers) a loud abort, not a silent census inflation.
- **RES-2 loud-abort (K5-D4, retained).** `ResolveOriginSupport` keeps the RES-2
  can't-happen abort (mirrors Planning.cpp:273-281): a counted Tier-2 decl with
  ZERO live origin carrier aborts the freeze rather than silently dropping a
  census-counted line. Existence is decl-counted FROM live origin sets, so ≥1
  carrier always exists in correct code — the abort is the belt.

These are the mechanism's choke points; the conservation belt (K5-D6) and the
fail-closed consumer goldens (K5-D7) sit ABOVE them as the coarser referees.

### K5-D7 — witnesses

PRIMARY POSITIVE PIN: `tc_nonlinear_diff` gains a `.region` golden SET (4 modes)
via an `.irgold` sidecar carrying the `region` surface (the demand_tc_witness/
join_1/merge_2 precedent). `tc` is recursive (`tc(From,To):tc(From,X),tc(X,To)`),
so its model view is an INDUCTION/MERGE that folds through canonicalization (sites
#14/#16). `edge` (`edge(From,To):add_edge(From,To)`) is the seed-through-one-hop
witness. WHAT IT WITNESSES (corrected — K5P-nec-1): the bare `rel=` existence line
does NOT by itself prove the union rode a fold — a decl-Id set-difference snapshot
would emit the same `rel=` line. The FOLD-MIGRATION pin is the `support=differential`
BYTE: `ResolveOriginSupport(query, tc)` resolves support ONLY by finding a LIVE
post-Optimize view carrying `tc`'s decl in `OriginDecls()` (the recursive
INDUCTION/MERGE model reached through fold sites #14/#16), and under K5-D4's
live-view-walk existence collector the line's very presence also depends on that
reach. Deleting the K5-D3 union makes `tc`'s decl reach no live view → the line
drops AND/OR `ResolveOriginSupport` RES-2-aborts — the support byte, not the rel
token, is the fail-closed referee. (CAVEAT, K5P-oracle-1/2: `tc` is also
SEED-reachable in some modes via induction keep-alive — see MODE-PARTIAL below —
so `tc` is a weak union canary; `edge` and `p`/`r` in opt/nocf are the real ones.)

PREDICTED BYTES — CORRECTED ORDER (K5P-det-1, empirically re-verified this
session; tip dump: `row-contracts=1`, sole `E0 rel=reachable`):

```
  row-contract    E0  rel=reachable  member-key=(From, To)  support=differential
+ row-contract    E1  rel=edge       member-key=(From, To)  support=differential
+ row-contract    E2  rel=tc         member-key=(From, To)  support=differential
- census: ... row-contracts=1
+ census: ... row-contracts=3
```

ORDERING RATIONALE (corrected). Order is ASCENDING `decl.Id()` (K5-D4's `std::map`
key), and `decl.Id()` is DOMINATED by `atom_name_id = name.IdentifierId()`
(Parse.cpp:348, bits 16-39 of IdInterpreter, Parse.h:34) — the STRING-POOL INTERN
OFFSET, NOT declaration-line order. `edge` interns as the NUL-terminated suffix of
`add_edge` (appended first at line 5; StringPool.cpp:98-104), giving it a LOWER
offset than `tc` (freshly appended at line 7). Empirically verified this session:
`Id(edge) < Id(tc)` (atom-offset delta 23) → E1=edge, E2=tc. The FIRST-draft
prediction (E1=tc/E2=edge, rationale "tc declared line 7, edge line 8 → tc lower
Id") was EXACTLY BACKWARDS. `support=differential` because `add_edge` is
`@differential` and deletions propagate edge→tc → both models `CanReceiveDeletions`.
STANDING NOTE: Tier-2 contract order is a string-pool-offset function and MUST be
read from a LIVE `-region-out` dump before any bless — NEVER predicted from source
line order, since suffix aliasing (any name that is a NUL-terminated suffix of an
earlier-interned identifier) inverts line order. SECONDARY POSITIVE PINS: the
re-blessed `join_1` (`p`,`r` — non-recursive seed + JOIN-facade migration
witness) and `merge_2` (`outer` — the @inline-skip witness proving `inner`/`proj`
are NOT named).

NEGATIVE WITNESS (litigated honestly — MODE-PARTIAL, K5P-oracle-1/oracle-2). The
belt is ADVISORY, so no always-red case can be built AGAINST the advisory surface.
The mechanism's regression referee is the FAIL-CLOSED CONSUMER goldens — but the
first draft's blanket claim ("deleting the K5-D3 union makes all 12 re-blessed
`.region` files go byte-RED") is FALSE and is RETRACTED. The corrected normative
truth:

The frozen `.region` output is a pure function of the DATAFLOW-opt toggle ONLY
(nodf==none, opt==nocf). The folds the union exists to survive — CSE
(Optimize.cpp:406), 1-arm-merge-elim (Merge.cpp:175), N→1 flatten,
canonicalization — ALL live inside `QueryImpl::Optimize`, gated at
Build.cpp:2621-2622 and SKIPPED under `-disable-dataflow-opt`. In the
dataflow-opt-OFF modes (nodf, none) each interior survives as a DISTINCT un-fused
def-top MERGE carrying the K5-D2 seed (verified: join_1 `p`=merge.22 / `r`=merge.23;
tc `edge`=merge.14/table:12, `tc`=merge.15/table:4, `reachable`=merge.16/table:25),
and the K5-D4 collector (`ForEachView`) names it FROM THE SEED ALONE — no always-on
CDaGI site deletes a seeded def-top merge (ProxySelects keeps insert_proxy as
survivor; ProxyMergedViews wraps ARMS and skips tuple arms, Link.cpp:206).
Moreover EVERY mode-independent CDaGI fold KEEPS ITS SOURCE LIVE — induction-leave
`new_union->merged_views.AddUse(view)` (Induction.cpp:446), ProxyMergedViews
transparent forward (Link.cpp:225-238) — so the seed reaches a live view without
the union in those cases too. THEREFORE a union deletion leaves the 6 nodf/none
goldens byte-GREEN and all 4 merge_2 goldens byte-GREEN (`outer` is a stable
3-arm merge surviving identity in all modes — SEED-reachable); it reddens ONLY
`tc.{opt,nocf}` (via `edge`'s 1-arm-elim) + `join_1.{opt,nocf}` (via `p`/`r`) =
**4 of 12 files**. And even those 4 are CONTINGENT on 1-arm-elimination continuing
to fire (a canonicalization change that stopped folding `edge` into `tc` would
silently vaporize that coverage with zero golden movement).

CONSEQUENCES for the design: (1) the union's regression detector is the
**opt/nocf** `.region` set — the deliberate-break protocol MUST run in an OPT mode
or it observes a false GREEN; the union is STRUCTURALLY UNTESTABLE in nodf/none via
consumer goldens (no mode-independent fold deletes its source there). (2) DROP `tc`
and `merge_2` as claimed union witnesses — both are SEED-reachable; `tc`'s
fold-migration is witnessed by its `support=` byte (above), not by consumer-golden
divergence. (3) ADD A ROBUST CSE-BASED UNION WITNESS (new corpus case): two
STRUCTURALLY-IDENTICAL non-inline undemanded interiors that CSE-merge (census #4,
loser RAUW'd + deleted), so the loser's decl reaches the survivor ONLY via the
K5-D3 union; pin its opt/nocf `.region` goldens so the loser's Tier-2 line is
present IFF the union fired — a union-only regression flips exactly that line,
ISOLATED from any seed failure (which would instead drop BOTH lines). This is the
seed-vs-union failure-mode distinction the first draft lacked: a dropped SEED drops
both twins' lines and would redden across modes; a dropped UNION drops only the
CSE-loser's line and only in opt/nocf.

DELIBERATE-BREAK PROTOCOL (K5P-test-5 — MUST run in a DATAFLOW-OPT-ON compile: the
DEFAULT/opt or nocf mode, NEVER `-disable-dataflow-opt`). Comment out the K5-D3
CDaGI union → rebuild DEBUG → compile the witness in DEFAULT mode → confirm BOTH
(a) the K5-D6 assert fires FOR THE DEMAND-FORCING class (run on a DEMAND program —
candidate demand_tc_witness, forcing `path` through the broken union; a query-only
case like join_1 CANNOT trip the assert — its `.region` goldens diverge but the
assert stays silent, K5P-corr-2) AND (b) the opt/nocf `.region` goldens diverge;
then revert. MODE-DEPENDENCE (normative): under `-disable-dataflow-opt` every
relation's insert_proxy survives as its own live seeded view, so the seed alone
keeps every decl in `reachable`, the union is a no-op, and NEITHER half of the
protocol exercises the union — a break run in a dataflow-opt-off mode falsely shows
the tripwire dead. (No env-gated scaffolding — the break is a manual, reverted
probe, per MEMORY: no-env-gated-debug-scaffolding.)

SMOKE-COVERAGE ADD (K5P-test-3): tc_nonlinear_diff's new `.irgold` also drives the
`-origin-out` produce-and-exit-check (K5-D5) — its NONEMPTY origin sets exercise
the `QueryOriginDump` path in each pinned mode (a crash/abort → IRGOLD-FAIL), the
empty-set path covered by the other origin-less irgold cases. No `.origin` golden.

OBLIGATIONS. (1) The `.irgold` `region` surface must produce a mode-stable Tier-2
line SET for tc_nonlinear_diff across all 4 modes (the mode-stability obligation,
K5-D4-Obl-2) — `tc`/`edge` are present in every mode (seed-reachable in nodf/none,
union-migrated in opt/nocf), so the LINES do not split; what IS mode-partial is
the union's testability, not the line's presence (K5P-oracle-1). (2) The predicted
E1=edge/E2=tc order + `support=differential` are VERIFY-gated against a LIVE dump
(never source order — K5P-det-1); the `support=` value must additionally match
across all 4 modes (K5-D4-Obl-2, support extension). (3) merge_2's `outer`-only
prediction is the guilty-until-verified one. (4) The new CSE-based union witness
(above) is the mode-honest union canary; its opt/nocf loser line is the isolated
union-only regression detector.

### K5-D8 — the recorded riders (FINDINGS-style, fix-or-keep litigated separately)

RIDER-1 — the `has_one_insert` dead branch (§A.3, probe-verified this brief).
`CreateProxyOfInserts` (Connect.cpp:11-71): `old_inserts.Swap(inserts)` at :14
empties `inserts` BEFORE `const auto has_one_insert = inserts.Size() == 1u` at
:17 → always `0==1u` → false → the bare-TUPLE early return at :50-52 is DEAD;
single-clause relations ALWAYS mint a MERGE (corroborated by Demand.cpp:563
hard-typing `p_merge` as `MERGE*`). LATENT/HARMLESS (always builds a valid
single-member MERGE). K5 IMPACT: NONE — K5-D2 stamps the OUTER returned view,
node-kind-agnostic (K5-Q6 Arm B), so a future fix that flips single-clause to
TUPLE cannot regress K5. FIX-OR-KEEP: a separate one-line fix (read
`old_inserts.Size()`) is trivial but OUT OF K5 SCOPE (no K5 dependency); RECORD
as a standalone latent quirk for the K4 fuzz arm / a future Connect cleanup.

RIDER-2 — the Merge.cpp:296-343 vs Join.cpp:266-280 CDaGI inconsistency
(census #15 vs #17). Both build a GUARD TUPLE for unused columns, but
`Join::ProxyUnusedInputColumns` migrates the satellite —
`joined_view->CopyDifferentialAndGroupIdsTo(tuple)` at Join.cpp:279 (VERIFIED) —
while `Merge`'s unused-col guard (Merge.cpp:302 `guarded_view`) does NOT call
CDaGI at all. So the Merge guard under-migrates group_ids AND diff-flags AND
(post-K5) origin decls — a PRE-EXISTING inconsistency K5 merely inherits as
census gap #15. FIX-OR-KEEP: fixing (add `view->CopyDifferentialAndGroupIdsTo(
guarded_view)` in the Merge loop) would close gap #15 and align the twins, but
per K5-Q3 Arm B the five bypass sites are accepted under-name gaps closed per
real consumer — and no consumer needs the merge-unused-col interior named.
RECORD; note the group_ids/diff-flag under-migration is a pre-existing (non-K5)
latent that a Connect/Merge audit should weigh independently.

RIDER-3 — D2.9(β) deferral (K5-Q2 adjudicated DEFER). Proxy-role-at-mint
inheritance (does a proxy MINT copy its source's `ProjectionRole`, Connect.cpp:32
et al. pass no role arg → default kMember) is MECHANISTICALLY DISJOINT from origin
decl-sets (§B3: identity-copy-at-mint vs satellite-union-at-fold; shares only the
adjective "proxy"). Zero consumer reads role at Stage B (E4, "recorded, not
built"); changes only dump bytes. RECORD as a follow-on rider; the seed's "as ONE
slice" is superseded by the mechanistic-disjointness grounding. Re-opens only when
a dump consumer of `ProjectionRole` is named.

---

### Part D summary — the diff list + the three riskiest obligations

DIFFS (one line each):
- **K5-D1** — `std::vector<ParsedDeclaration> origin_decls` on QueryViewImpl
  (Query.h:462, sorted-unique by decl Id) + a `QueryView::OriginDecls()`
  accessor; empty-init, 24 B idle, Hash/Equals-blind.
- **K5-D2** — one SEED at Connect.cpp:269 (`insert_proxy->origin_decls =
  {rel->declaration}`), co-located with the T1 stamp,
  `!decl.IsInline() && !decl.IsQuery()`-guarded (the `!IsQuery` conjunct is
  redundant-but-explicit forward-looking hygiene, K5P-corr-2), node-kind-agnostic,
  seed-once asserted; no ProxySelects / terminal-INSERT / mutable-inner seed.
- **K5-D3** — one UNION in CopyDifferentialAndGroupIdsTo (View.cpp after :667),
  loser→survivor, sort+unique by Id, NO clear-on-`this` (a SET, not a counted
  scalar); covers all YES-routing census sites.
- **K5-D4** — freeze `CollectOriginInteriorDecls` + a third contract loop +
  a third census term (Planning.cpp), dedup vs insert-named ∪ Tier-1, Id-ordered,
  AllFields member-key, `ResolveOriginSupport`; Arm-A plain `rel=` render;
  re-bless join_1 (+p,+r) & merge_2 (+outer), tc_nonlinear_diff gains a set.
- **K5-D5** — advisory `-origin-out` per-view dump (det_seq-ordered, header
  `origin-sets`), `-contract-out` plumbing, never-goldened-by-default, no pin.
- **K5-D6** — DEBUG-only conservation assert, RESCOPED (session-8 panel): every
  `RecognizedSubgraph.demanded_decl` must be origin-reachable at a live view —
  redundant-with-Tier-1 defense-in-depth; the vacuous query arm +
  `k5_seeded_decl_ids` machinery DROPPED (owner overruled the panel's delete).
- **K5-D6b** — TIGERSTYLE assert battery at the invariant choke points: seed-once
  (D2), post-union sorted-unique (D3), support-agreement (D4), freeze-collector
  NEGATIVE-space postconditions (Tier-2 decl is non-inline/non-query/not-insert-
  named/not-Tier-1), RES-2 loud-abort retained.
- **K5-D7** — tc_nonlinear_diff `.region` set (predicted +E1 rel=EDGE/+E2 rel=TC —
  ascending string-pool-intern-offset, `edge` suffix-aliases `add_edge`, K5P-det-1
  corrected the first draft's backwards order; fold-migration witnessed by the
  `support=differential` BYTE not the `rel=` line, K5P-nec-1); join_1/merge_2
  re-bless; the union referee is MODE-PARTIAL — only opt/nocf (4 of 12 files)
  fail-close, nodf/none + merge_2 stay seed-reachable-GREEN (K5P-oracle-1/2), so
  a new CSE-based union canary + a dataflow-opt-ON-pinned break protocol are
  added; `-origin-out` smoke coverage folded into the `.irgold` compile.
- **K5-D8** — riders: (1) has_one_insert dead branch (latent, K5-immune),
  (2) Merge#15-vs-Join#17 CDaGI inconsistency (pre-existing, accepted gap),
  (3) D2.9(β) proxy-role deferral (mechanistically disjoint).

THE THREE RISKIEST OBLIGATIONS the panel will attack (post-panel, session 8):
1. **K5-D4 `support=` mis-attribution via the no-clear union onto shared
   facades** — over-naming is ADDITIVE-safe for `rel=`/`member-key`, but the
   `support=` byte is sound ONLY under the differentialness-migration invariant
   (K5-D3: CDaGI OR-propagates `can_receive_deletions` loser→survivor in lockstep
   + CSE co-location HashInit-gated). The first draft's blanket "monotone-safe"
   did not cover support (K5P-corr-1); it is now stated AND DEBUG-asserted
   (K5-D6b support-agreement). The exact merge_2 Tier-2 line set stays
   guilty-until-verified (predict-then-verify before bless; `outer` IS stored via
   model-sharing into table:4, K5P-corr-1 REFUTED).
2. **The union's referee is MODE-PARTIAL, not the claimed all-12-files fail-close**
   — the frozen `.region` output is a pure function of the dataflow-opt toggle;
   nodf/none reach every decl via the SEED alone (mode-independent CDaGI folds
   keep their source live), so a broken union reddens only opt/nocf (4 of 12) and
   NOT via tc/merge_2 (both seed-reachable), K5P-oracle-1/2. The mitigation is a
   purpose-built CSE-merge union canary + a dataflow-opt-ON-pinned deliberate-break
   protocol; the rescoped K5-D6 assert covers only the demand-forcing class.
3. **K5-D4 MODE-STABILITY of the Tier-2 set AND its `support=` value** — a Tier-2
   line (and its support byte, a semantic mode-invariant, K5P-corr-1) must appear
   identically in ALL 4 modes or the per-mode golden splits; the 4 named cases are
   safe (real materialized interiors present in every mode), but the corpus-wide
   `-region-out` growth obliges a sweep confirming no OTHER region-goldened case
   (present or future) gains a mode-split interior, and that V-REGION-CENSUS's
   same-collector construction holds corpus-wide.

---

### Part D panel record (session 8, 2026-08-04)

Adversarial panel: 4 lenses (correctness, oracle/testability, necessity/
simplicity, determinism/notes), 17 findings, each verdict independently
re-verified against tip (33547417). OWNER STANDING: (a) the K5-D6 delete
recommendation (K5P-simp-1) is OVERRULED — D6 is KEPT and RESCOPED to the
demand-forcing class as redundant-with-Tier-1 defense-in-depth; (b) a TIGERBEETLE-
style assert battery was directed at the mechanism's invariant choke points
(landed as K5-D6b + the inline seed-once/post-union asserts). One line per finding:

- **K5P-det-1** — CONFIRMED — Tier-2 order was backwards; corrected to
  E1=edge/E2=tc (ascending `decl.Id()` = string-pool INTERN OFFSET, `edge`
  suffix-aliases `add_edge`; empirically re-verified), rationale fixed in K5-D4/D7,
  order now verify-gated against a LIVE dump, never source line order.
- **K5P-oracle-1** — CONFIRMED — retracted the "all 12 `.region` files go
  byte-RED" claim; frozen `.region` is a pure function of the dataflow-opt toggle,
  nodf/none name every interior from the SEED alone, so the union referee is
  opt/nocf-ONLY (K5-D7 rewritten).
- **K5P-oracle-2** — PARTIAL — confirmed the 4-of-12 blast radius + the seed-vs-
  union failure-mode distinction and folded in the purpose-built CSE-merge
  mode-honest union canary; only the headline absolute ("no line requires the
  union") was overbroad (edge/p/r opt-nocf lines do).
- **K5P-test-5** — CONFIRMED — the deliberate-break protocol is now pinned to a
  DATAFLOW-OPT-ON compile; both halves (assert + `.region` divergence) are
  fold-dependent and show a false GREEN under `-disable-dataflow-opt`.
- **K5P-corr-2** (CONFIRMED lens) — CONFIRMED — the query-INSERT arm of `live_ref`
  is provably inert (queries are `!IsInline`-skipped from the seed); drove the D6
  rescope + re-pointing the break protocol at a DEMAND program.
- **K5P-corr-2** (PARTIAL lens, MINOR) — PARTIAL — added the explicit
  `!decl.IsQuery()` seed conjunct as redundant-but-explicit forward-looking hygiene
  and fixed the prose asymmetry; the spurious-Tier-2-contract failure is unreachable
  at tip (always-rooted query INSERTs).
- **K5P-corr-1** (REFUTED lens, MAJOR) — REFUTED — `outer` is NOT unstored: it is
  model-SHARED into `q_outer`'s table:4 (EQ SET 6); the EQ-set/rel-golden evidence
  is cited in K5-D4's blast-radius as hardening (contract:table is not 1:1).
- **K5P-corr-1** (PARTIAL lens, MINOR) — PARTIAL — stated the support-soundness
  (differentialness-migration) invariant in K5-D3/D4 and DEBUG-asserted it (K5-D6b);
  the mis-attribution failure is unreachable at tip (CDaGI lockstep + HashInit gate).
- **K5P-nec-1** — PARTIAL — re-located the load-bearing byte to `support=differential`
  (not `rel=` existence); the per-view field is independently REQUIRED as the live
  post-Optimize handle for `ResolveOriginSupport` (K5-D1 NECESSITY note, K5-D7).
- **K5P-note-3** — CONFIRMED — sharpened the mis-attribution safety anchor to the
  survivor-DERIVES-FROM-loser invariant (subsumes RAUW; covers the non-RAUW proxy
  callers) in K5-D3.
- **K5P-test-3** — PARTIAL — added the `run_irgold` `-origin-out` smoke-coverage
  clause (produce-and-exit-check, no golden) to K5-D5 + K5-D7; MAJOR overstated
  (advisory dump, matches the un-wired DOT-twin precedent).
- **K5P-simp-2** — PARTIAL — `-origin-out` row order → `(min decl.Id(), det_seq
  tie-break)` payload-covarying key, det_seq retained as label; the per-decl format
  and the delete-D5 conclusions were refuted.
- **K5P-note-1** — PARTIAL — dropped the build-gate recommendation: the field is
  NOT gated on a cascade-DCE witness, it independently serves `support=` resolution;
  the two liveness omissions are recorded as a scoped note.
- **K5P-simp-1** — CONFIRMED (finding) / OWNER-OVERRULED (disposition) — the panel
  proved D6's fire-set disjoint from the consequence-bearing under-name set and
  recommended DELETE; the owner OVERRULED and RESCOPED D6 to the demand-forcing
  class (defense-in-depth), dropping the query arm + `k5_seeded_decl_ids`.
- **K5P-mode-1** — REFUTED — no cross-mode `.region` identity guarantee exists to
  remove (pinned PER-MODE, CLAUDE.md:662); an opt-sensitive Tier-2 line is benign
  under per-mode pinning and is already the stated K5-D4-Obl-2.
- **K5P-oracle-4** — REFUTED — `p`/`r` are table-BACKED monotone in opt (table:12/
  16/19/23), not table-less; Tier-2 member-key is decl-driven and mode-independent,
  so join_1's prediction is robust (the brief's guilty/robust ranking is not
  inverted).
- **K5P-nec-2** — REFUTED — over-attribution (a monotone interior reporting
  `support=differential`) is unconstructible: CDaGI confines a decl to
  crd-AGREEING carriers, and `ResolveOriginSupport`'s reuse of the existing
  `can_receive_deletions` computation is the DRY choice, not a second authority.

---

## Part E — DESIRED OUTPUT STATES (byte diffs, session 8, 2026-08-04)

Predicted post-K5 bytes for every golden the slice touches, computed from the
`-region-out` EMITTER'S REAL padding rules (`lib/Regional/Format.cpp`, re-read
this session at tip 33547417 + the uncommitted k5-provenance.md), against
FRESHLY collected tip dumps. Every predicted file was generated by a faithful
byte reimplementation of the Format.cpp contract-block algorithm and diffed
against the committed golden; no spacing is hand-guessed. Tip dumps + predicted
goldens are under `scratchpad/phase-d/` (`*.region.<mode>.out` = tip,
`PRED.*.golden` = predicted).

TIP BASELINE (verified). All 8 committed `join_1`/`merge_2` `.region.<mode>`
goldens byte-MATCH a fresh tip `-region-out` dump in all 4 modes (`cmp -s`
green ×8) — the freshness gate passed, so the diffs below are against a
trustworthy baseline. Flag spellings confirmed against `runall.sh:206-218`
`flags_of`: `opt`="", `nodf`=`-disable-dataflow-opt`,
`nocf`=`-disable-controlflow-opt`, `none`=both (WATCH the zsh word-split
gotcha — the two `none` flags MUST be passed as separate argv words, not one
quoted string, or the driver rejects `'-disable-dataflow-opt
-disable-controlflow-opt'` as one unrecognized arg). For all three cases the
4 modes are byte-IDENTICAL at tip AND in prediction (region output is a pure
function of the dataflow-opt toggle, and here the two toggle-classes coincide),
but 4 pins are authored per case per the per-mode golden convention
(`CLAUDE.md`: pinned PER-MODE, cross-mode identity not claimed).

### E.1 — THE PADDING-RULE VERDICT (a) — NOT purely additive; Part D REFUTED for join_1

The row-contract block widths are computed PER-DUMP over ALL contract rows, NOT
fixed (`Format.cpp:123-139`):

```
size_t etok_w = 0u, rel_w = 0u, key_w = 0u;
for (const RegionalContract &contract : p.Contracts()) {
  etok_w = std::max(etok_w, 1u + std::to_string(contract.edge_index).size());
  rel_w  = std::max(rel_w,  4u + contract.rel_name.size());        // "rel="   = 4
  key_w  = std::max(key_w, 11u + contract.member_key_text.size()); // "member-key=" = 11
}
etok_w += 2u; rel_w += 2u; key_w += 2u;
for (...) os << "  " << Pad("row-contract", kind_w)
             << Pad("E"+..., etok_w) << Pad("rel="+rel_name, rel_w)
             << Pad("member-key="+member_key_text, key_w) << "support=" << support;
```

`Pad(s,w)` (`Format.cpp:21-26`) LEFT-justifies to `w` (trailing spaces),
no-op when already wider. Because `rel_w`/`key_w`/`etok_w` are per-case MAXes,
a NEW Tier-2 row whose `rel_name` or `member_key_text` is LONGER than every
existing row's re-pads the EXISTING E0/E1 lines (more trailing spaces in that
column). `kind_w` is stable (max is always `permanent-root`=14 → 16; row-contract
is 12), and `etok_w` is stable while every `edge_index` stays single-digit
(1+1+2=4; none of the three cases reaches E10). So only the `rel=`/`member-key=`
columns can move an existing line.

VERDICT: the re-bless is **NOT purely additive** — Part D's blanket claim
("existing E0/E1 lines are byte-identical", K5-D4 MARKER CHOICE l.850-857;
"= 8 golden files, ADDITIVE only", RE-BLESS SET l.891-893) is **REFUTED for
join_1** and holds only for merge_2 (and for the new tc file). Per case:

- **join_1 — NON-ADDITIVE.** Tier-2 adds `p` with `member-key=(A, B)` (len 6).
  Tip max member-key was `(B)` (len 3) → `key_w` grows 16→19. E0 and E1's
  `member-key=(B)` field therefore gains **+3 trailing spaces** (2→5 before
  `support=`). `rel_w` is UNCHANGED (new names `p`,`r` len 1 < existing max
  `never` len 5 → stays 11), so only the member-key column of E0/E1 moves. This
  contradicts Part D — see E.5(d).
- **merge_2 — ADDITIVE.** New `outer` (name len 5 < `q_outer` len 7 → `rel_w`
  stays 13) with `member-key=(X, Y)` (len 6 == existing max `(X, Y)`, so `key_w`
  stays 19). Neither column widens → E0/E1 byte-IDENTICAL, only E2 appended +
  census. Part D's merge_2 bullet holds.
- **tc_nonlinear_diff — ADDITIVE (new file).** New `edge`/`tc` both
  `member-key=(From, To)` (len 10 == E0 `reachable`'s key) and names shorter than
  `reachable` (len 9) → `rel_w`=15, `key_w`=23 both UNCHANGED. E0 byte-identical;
  E1/E2 appended.

### E.2 — INTERN-ORDER RECONCILIATION (b) — CONFIRMED, independently re-derived

`decl.Id()` (`ParsedDeclarationImpl::Id`, `Parse.cpp:321-352`) packs
`IdInterpreter` bitfields (`Parse.h:29-38`): `module_id`[0:10), `arity`[10:16),
`atom_name_id`[16:40). So `flat = module_id + (arity<<10) + (atom_name_id<<16)`.
An `atom_name_id` delta of 1 contributes 65536 > 65535 (the max of all
lower bits) → when names differ, `Id()` ordering IS `atom_name_id` =
`name.IdentifierId()` ordering (`Parse.cpp:348`), arity/module only break ties.
`IdentifierId()` = the STRING-POOL BYTE OFFSET (`StringPool.cpp:88-116`):
`InternString(data, force=false)` first searches the pool for `data` as a
**NUL-terminated suffix** of an already-interned string (`:97-105`, the
`!pool[pos+len]` trailing-NUL gate) and REUSES that offset; else appends
`data`+`\0` at the end. Identifiers are interned first-occurrence as the lexer
scans, so offset ≈ first-occurrence position EXCEPT suffix aliasing inverts it.

- **tc_nonlinear_diff.** First-occurrence decl-name order: `add_edge`(L5),
  `reachable`(L6), `tc`(L7), `edge`(L8). `add_edge` interns fresh (low offset,
  before `reachable`/`tc`). At L8, `edge` is the NUL-terminated suffix of
  `add_edge\0` → REUSES offset `Id(add_edge)+4`, which is LOWER than `tc`
  (freshly appended at L7, after `reachable`). So among the Tier-2 pair
  `{edge, tc}`: `Id(edge) < Id(tc)` → **E1=edge, E2=tc**. RECONCILES with the
  panel's empirical `Id(edge)<Id(tc)` (K5P-det-1) and refutes the first-draft
  line-order guess (`tc` before `edge`). `reachable`(E0) is fixed independently
  of Id — it is the sole query-INSERT decl, collected by CollectContractInserts
  BEFORE the Tier-2 loop.
- **join_1.** Decl names `t1,t2,p,r,q,never` all intern FRESH (none is a
  NUL-suffix of an earlier one — `p`/`r`/`q` are single chars not ending any
  prior string), so offset order = first-occurrence order. Tier-2 pair `{p,r}`:
  `p`(L9) before `r`(L10) → `Id(p)<Id(r)` → **E2=p, E3=r**. Coincides with
  declaration order here only because neither suffix-aliases (Part D's "by
  luck" caveat, K5-D4 join_1 bullet, is exactly this).

RECONCILIATION RESULT: both orders re-derive cleanly from the interning rule and
AGREE with the panel/empirical result. STANDING GATE (unchanged): Tier-2 order
is a string-pool-offset function; suffix aliasing can invert source-line order,
so any bless MUST read a LIVE `-region-out` dump, never predict from line order.

### E.3 — PREDICTED DIFFS (c) — one line per file

Real `diff -u` output (predicted vs committed golden) is in E.4. Summary:

- `join_1.region.opt.golden`   — **NON-ADDITIVE**: E0/E1 member-key +3 spaces; +E2 `rel=p`; +E3 `rel=r`; census 2→4.
- `join_1.region.nodf.golden`  — identical change to opt (byte-identical golden).
- `join_1.region.nocf.golden`  — identical change to opt.
- `join_1.region.none.golden`  — identical change to opt.
- `merge_2.region.opt.golden`  — ADDITIVE: +E2 `rel=outer member-key=(X, Y) support=monotone`; census 2→3; E0/E1 untouched.
- `merge_2.region.nodf.golden` — identical change to opt.
- `merge_2.region.nocf.golden` — identical change to opt.
- `merge_2.region.none.golden` — identical change to opt.
- `tc_nonlinear_diff.region.opt.golden`  — NEW FILE: E0 `reachable` unchanged; +E1 `rel=edge`; +E2 `rel=tc`; all `support=differential`; census 1→3.
- `tc_nonlinear_diff.region.nodf.golden` — NEW FILE, byte-identical to opt.
- `tc_nonlinear_diff.region.nocf.golden` — NEW FILE, byte-identical to opt.
- `tc_nonlinear_diff.region.none.golden` — NEW FILE, byte-identical to opt.

RE-BLESS COUNT: 8 EXISTING re-blesses (join_1 ×4 NON-ADDITIVE, merge_2 ×4
ADDITIVE) + 4 NEW files (tc_nonlinear_diff ×4) = **12 `.region` golden files**
(Part D counted only the 8; the 4 tc files are a NEW pin set K5-D7 already calls
for via the new `.irgold`). Everything outside `-region-out` is untouched:
`join_1.contract.opt.golden` (Stage-A `-contract-out`, different surface),
`*.rel.opt.golden`, all stdout/oracle/monotone/behavioral goldens.

### E.4 — THE BYTE DIFFS (unified, against tip goldens)

All four modes per case are byte-identical; ONE representative diff shown per
case, applies verbatim to `.{opt,nodf,nocf,none}`.

**join_1 (NON-ADDITIVE — E0/E1 change):**

```diff
@@ region R0 row-contract block + census @@
   permanent-root  q(B)
   permanent-root  never(B)
-  row-contract    E0  rel=q      member-key=(B)  support=monotone
-  row-contract    E1  rel=never  member-key=(B)  support=monotone
+  row-contract    E0  rel=q      member-key=(B)     support=monotone
+  row-contract    E1  rel=never  member-key=(B)     support=monotone
+  row-contract    E2  rel=p      member-key=(A, B)  support=monotone
+  row-contract    E3  rel=r      member-key=(A)     support=monotone
 }
-census: regions=1 child-calls=0 program-roots=1 request-ports=0 input-ports=2 result-ports=0 row-contracts=2
+census: regions=1 child-calls=0 program-roots=1 request-ports=0 input-ports=2 result-ports=0 row-contracts=4
```

Widths: `etok_w`=4, `rel_w`=11 (unchanged), `key_w`=**19** (was 16). E0/E1's
`member-key=(B)` (14 chars) now pads to 19 = +5 trailing spaces (was +2). E2's
`member-key=(A, B)` (17) pads to 19 = +2; E3's `member-key=(A)` (14) = +5.

**merge_2 (ADDITIVE):**

```diff
@@ region R0 row-contract block + census @@
   row-contract    E0  rel=q_outer  member-key=(X, Y)  support=monotone
   row-contract    E1  rel=q_proj   member-key=(X)     support=monotone
+  row-contract    E2  rel=outer    member-key=(X, Y)  support=monotone
 }
-census: regions=1 child-calls=0 program-roots=1 request-ports=0 input-ports=3 result-ports=0 row-contracts=2
+census: regions=1 child-calls=0 program-roots=1 request-ports=0 input-ports=3 result-ports=0 row-contracts=3
```

Widths: `rel_w`=13, `key_w`=19 both UNCHANGED (E0/E1 byte-identical). E2's
`rel=outer` (9) pads to 13 = +4; `member-key=(X, Y)` (17) pads to 19 = +2.

**tc_nonlinear_diff (NEW file — full predicted golden, all 4 modes identical):**

```
region-program
program-root {
  input-abi   add_edge/2(From:u64, To:u64)           -> R0 via P0
  query-abi   reachable(From:free u64, To:free u64)  -> permanent-root
  output-abi  <none>
}
region R0  owner=program-root  parents=()  children=() {
  input-port      P0  message=add_edge/2  fields=(From, To)
  permanent-root  reachable(From, To)
  row-contract    E0  rel=reachable  member-key=(From, To)  support=differential
  row-contract    E1  rel=edge       member-key=(From, To)  support=differential
  row-contract    E2  rel=tc         member-key=(From, To)  support=differential
}
census: regions=1 child-calls=0 program-roots=1 request-ports=0 input-ports=1 result-ports=0 row-contracts=3
```

Widths: `rel_w`=15 (`reachable` len 9 dominates; `rel=edge`=8→+7, `rel=tc`=6→+9),
`key_w`=23 (all keys `(From, To)` len 10; each `member-key=(From, To)`=21→+2).
E0 byte-identical to tip.

**tc_nonlinear_diff.irgold (NEW sidecar) — exact content:**

```
region opt
region nodf
region nocf
region none
```

(Format = `<surface> <mode>` per line, matching `join_1.irgold`/`merge_2.irgold`;
tc gains the `region` surface ×4 only — no `rel`/`contract` pin, per K5-D7. The
`.batches` sidecar it already carries is orthogonal.)

RUNALL.SH COMPOSITION (checked). `runall.sh:527-530` calls
`run_oracle`/`run_refinterp`/`run_eqgate`/`run_irgold` UNCONDITIONALLY and
independently for every case; each self-gates on its sidecar. tc_nonlinear_diff
runs through the default `*)` arm (`:523-524` diffrun.sh, an all-4-modes-clean
golden case), so it satisfies the run_irgold precondition ("permitted only on
all-4-modes-clean golden cases", `:319`). Adding `.irgold` to a case that
already has `.batches`+oracle/monotone/behavioral goldens COMPOSES CLEANLY —
run_oracle/run_refinterp (from `.batches`) and run_irgold (from `.irgold`) are
disjoint steps, no shared state. `bless_copy` writes the 4 new
`tc_nonlinear_diff.region.<mode>.golden` (none are symlinks → plain `cp`, no
BLESS-REFUSED risk). The ONLY runall.sh CODE edit K5 needs is the K5-D5
`-origin-out` produce-and-exit-check line appended to the run_irgold compile
(`:330-336`), which is orthogonal to golden bytes (never byte-compared).

### E.5 — DETERMINISM CRITIQUE + support= verify-gates

Every predicted byte and the emitter rule that determinizes it:

- **Line ORDER within the contract block** — the freeze collector append order:
  CollectContractInserts (R-STORE, query/insert decls) → CollectDemandInteriorDecls
  (Tier-1) → CollectOriginInteriorDecls (Tier-2), the last emitting ASCENDING
  `decl.Id()` (K5-D4 `std::map<uint64_t,...>`). E0 (query) is R-STORE; Tier-2
  appends by Id. DETERMINISTIC via the Id-keyed map. [VERIFY-GATE: the Tier-2
  Id order `{edge<tc}`, `{p<r}` rests on the string-pool intern offsets of
  E.2 — re-derived from code + panel-empirical, but MUST be read from a LIVE
  dump before bless, never source line order.]
- **`edge_index` values E0..E3** — the dense `edge++` counter (K5-D4), assigned
  in the append order above. DETERMINISTIC.
- **Per-column PADDING (the trailing-space counts)** — per-case `rel_w`/`key_w`
  MAX over the final contract list (`Format.cpp:123-129`), a pure function of the
  emitted `rel_name`/`member_key_text` byte lengths. DETERMINISTIC given the
  contract set. This is exactly why join_1 is non-additive (E.1) — the rule is
  deterministic, not additive.
- **`member-key=` text** — `AllParamNames(decl)` positional (K5-D4; the Tier-1
  precedent Planning.cpp:533), a parse-identity render `(Name, Name...)`.
  DETERMINISTIC, MODE-INVARIANT (decl-driven, not graph-shape-driven — so the
  member-key never splits across modes; confirms K5P-oracle-4).
- **census `row-contracts` count** — `DeriveRegionalCensus.row_contracts` +
  `CollectOriginInteriorDecls(query).size()` (K5-D4), same-collector as the build
  loop → V-REGION-CENSUS stays green by construction. Arithmetic: join_1 2+2=4,
  merge_2 2+1=3, tc 1+2=3. DETERMINISTIC.
- **`support=` byte** — `ResolveOriginSupport(query, decl)` = OR of
  `v.CanReceiveDeletions()` over live views carrying `decl` (K5-D4). Predictions:
  join_1 `p`/`r` = **monotone** (descend from `#message t1`/`t2`, NEITHER
  `@differential` → CanReceiveDeletions false); merge_2 `outer` = **monotone**
  (descends from `inner`←m1/m2 + m3, none `@differential`); tc `edge`/`tc` =
  **differential** (descend from `#message add_edge ... @differential` →
  deletions propagate edge→tc → both models CanReceiveDeletions true).
  [VERIFY-GATE, K5P-corr-1: soundness rests on the differentialness-migration
  invariant (CDaGI OR-propagates `can_receive_deletions` loser→survivor in
  lockstep with the origin union + CSE co-location HashInit-gated,
  View.cpp:422-424,669-674); the value must be re-read from a LIVE post-K5 dump
  and must AGREE across all 4 modes (a per-mode support split = the K5-D6b
  support-agreement bug). Cannot be dumped at tip since `origin_decls` does not
  exist yet — it is the single most implementation-contingent byte here.]

VERIFY-GATED BYTES (cannot be confirmed until K5 lands, distinct from the
padding/order rules which ARE tip-derivable): (1) that `p`,`r`,`outer`,`edge`,`tc`
are actually SEEDED and reach a live view in every mode (K5-D2/D3 mechanism —
Part D argues yes: seed-reachable in nodf/none, union-migrated in opt/nocf);
(2) that merge_2 names `outer` and ONLY `outer` (`inner`/`proj` @inline-skipped)
— Part D's own guilty-until-verified gate (K5-D4 PREDICT-THEN-VERIFY l.900-905);
(3) the `support=` values above. The PADDING, LINE-ORDER, member-key text, and
census arithmetic are pure functions of the (verify-gated) contract SET and are
computed here from the real emitter — they are correct IF the set is as predicted.

### E.5(d) — CONTRADICTIONS FOUND

**FINDING E-K5-PAD — Part D's "ADDITIVE only" blast-radius claim is REFUTED for
join_1.** Part D asserts (K5-D4 MARKER CHOICE l.850-857: "existing E0/E1 lines
are byte-identical ... the re-bless is purely new-line additions"; RE-BLESS SET
l.891-893: "8 golden files, ADDITIVE only"). The `-region-out` emitter computes
`key_w` as a per-dump MAX over all contract rows (`Format.cpp:128`:
`key_w = std::max(key_w, 11u + contract.member_key_text.size())`). join_1's new
Tier-2 row `p` carries `member-key=(A, B)` (len 6) > the prior max `(B)` (len 3),
growing `key_w` 16→19 and adding **+3 trailing spaces to E0's AND E1's
member-key column**. join_1's re-bless is therefore NON-ADDITIVE: 2 changed
lines + 2 added + 1 census, not 2 added + 1 census. The TRUE diff is E.4 above.
merge_2 and tc REMAIN additive only because their new rows' names/keys do not
exceed the existing per-column maxes (a coincidence of `outer`≤`q_outer` and
`(X, Y)`==`(X, Y)`; `edge`/`tc`≤`reachable` and keys all `(From, To)`) — so the
"additive" framing is a per-case ACCIDENT of name lengths, NOT a structural
property of Tier-2 append, and any future region-goldened case whose longest
member-key or rel-name arrives via Tier-2 will likewise re-pad its Tier-0/1
lines. RECOMMENDATION (per the task's directive not to silently adjust Part D):
amend the K5-D4 blast-radius + RE-BLESS SET prose to state "ADDITIVE for
merge_2/tc; NON-ADDITIVE for join_1 (Tier-2 `member-key=(A, B)` widens the E0/E1
member-key column by 3 spaces via the per-dump `key_w` max, Format.cpp:128)",
and restate the count as 12 files (8 re-bless incl. join_1's 4 line-moving,
+ 4 new tc files). No code or golden was modified by this analysis.

A SECOND, minor imprecision: Part D's RE-BLESS SET says "8 golden files" but
K5-D7 separately introduces the tc `.region` set (4 files) via the new
`.irgold`; the two counts should be unified to **12** in one place.
