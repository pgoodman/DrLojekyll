# Keyed-instance rewrite — adversarial critique of the P7–P9 physical-layer diffs (ranked)

Session 14 (2026-08-06, tip `46a404d4`). An independent opus refuter panel (4 refuters:
P7 / P8 / P9 phase critics + a cross-phase completeness critic) attacked
`keyed-rewrite-p7p9-diffs.md`. Every CONFIRMED finding was verified by OPENING the cited
`file:line`; PLAUSIBLE = reasoned, not fully code-verified. REFUTED / certified = the diff
held under attack.

**Tally: 15 surviving (3 blocking, 5 high, 5 medium, 2 low; 13 CONFIRMED, 2 PLAUSIBLE) +
18 certifications.** The DIRECTION holds — the load-bearing mechanisms are certified against
real code (the three-arm EmitScan taxonomy is accurate; the cursor-shape discriminator holds;
the trie is NOT a second fact owner; V-DECLARED-PATH-PRESERVED is a real anti-regression belt;
P9 SOURCE 1 treats a query adornment as ONE inferred source, never as the key). But three
findings force a real design correction each, and all are folded into
`keyed-rewrite-p7p9-diffs.md` §7.

## Owner headline

Three blocking corrections, each verified: (B-P7) the §1.5 V-PLAN-HONEST belt, placed
unconditionally at `EmitScan`'s head, ABORTS the corpus — `ProgramTableScanRegion` has a
PRE-EXISTING mint site (join pivots, Join.cpp:262-266, `index=Some`) that P7 never threads
`plan_kind` through, so the bijection assert fires `true==false` on every join. (B-P8) the
§2.1 trie is VALUE-keyed (`edges` on `(added_field_id, prefix_values_hash)`), so `[A,B]` and
`[B,A]` reach TWO physical leaves — refuting the §2.4 convergence + §2.7.1 "3-not-4" pin the
whole P8 exit gate rests on. (B-P9) §3.3 derives the inferred-path ORDER from
`SortedPredecessors`, a **file-static** ControlFlow heuristic (Join.cpp:142) that Regional
freeze provably cannot call AND that collapses the planning/logical authorities. Each has a
concrete fix; none invalidates the four-authority target or the phase sequence.

---

## BLOCKING

### B-P7 · CONFIRMED · P7 · soundness-gap — the V-PLAN-HONEST belt aborts every join scan
`P7-planhonest-belt-aborts-join-scans` (+ cross `P7-emitscan-belt-aborts-join-scans`).
The §1.5 belt `assert((plan_kind==kFullScanFilter)==(Index()==nullopt))` sits at `EmitScan`'s
arm-selection head (Database.cpp:3375-3395), but `EmitScan` serves ALL `ProgramTableScanRegion`s.
The SOLE existing mint site — join pivots, `Join.cpp:254-266` (`scan->index.Emplace(scan,
pred_index)`, `Index()==Some`) — never sets `plan_kind`; §1.4 threads it only at the new
`LowerAccessRequirement` site. With a `kFullScanFilter` default the belt evaluates `true==false`
and aborts on every join. **Fix (folded §7):** initialize `plan_kind` at EVERY
`ProgramTableScanRegion` mint (add `kFullKeyHashLookup` at Join.cpp:262), OR add a `kUnplanned`
sentinel default the belt explicitly skips, OR scope the belt to `LowerAccessRequirement`-origin
regions. No single silent default is safe across the mixed `index=Some`(join)/`nullopt`(P7) set.

### B-P8 · CONFIRMED · P8 · invariant-violation — the value-keyed trie does not converge
`P8-value-trie-noconverge`. §2.1 keys `edges` on `(added_field_id, prefix_values_hash)` and
mints children in a per-parent LOCAL map (`node.edges[key]=child`), so with `A=a,B=b`: `[A,B]`'s
terminal edge `(B,hash[a,b])` hangs off the node reached via `(A,hash[a])`; `[B,A]`'s terminal
edge `(A,hash[b,a])` hangs off a DIFFERENT node reached via `(B,hash[b])` — two leaf objects.
The `:248` "convergent: routes merge" comment is never executed by the pseudocode. This refutes
§2.4's one-to-one `BindingStateSchema↔TrieNode` and §2.7.1's "3 interior nodes not 4" — the
whole P8 structural exit gate. **Fix (folded §7):** intern nodes in a region-global pool keyed by
the canonical `(schema, sorted value-map)` = `BindingStateId` so convergent value maps share ONE
node (the P5 convergence contract), OR delete the §2.4/§2.7.1 convergence claims and admit the
trie is per-route. See also B-P8's sibling `P8-schemanode-vs-retrieval` (high).

### B-P9 · CONFIRMED · P9 · invariant-violation — inferred-path order derived from a ControlFlow heuristic
`P9-sortedpred-layer-and-authority`. §3.1 SOURCE 2 / §3.3 `OrderPivotsByInducedBinding` derive
the LOGICAL inferred-path order from `SortedPredecessors` — verified `static` (file-local) at
`lib/ControlFlow/Build/Join.cpp:142`, sorting on planning-cost signals (num_non_pivots / Depth /
Columns / EquivalenceSetId, :158-185). Two independent violations: (a) **layer** — P9 runs at
freeze (Regional; `Main.cpp` FrozenRegionalProgram::Build BEFORE Program::Build), and the
architecture is acyclic `{ControlFlow,Rel} → Regional → DataFlow`, so Regional calling a
ControlFlow (let alone file-static) function is a build cycle; the chosen probe order does not
even EXIST at freeze (`SortedPredecessors` is invoked during Program::Build at Join.cpp:217,360).
(b) **authority collapse** — the order becomes a function of a PLANNING decision, violating
goal-1 ("planning picks NO logical path; inference reads NO planning artifact"). **Fix (folded
§7):** derive path order from a DataFlow-visible logical signal reachable from Regional
(`out_to_in`, `lib/DataFlow/Query.h:832`, projected to a canonical field order — e.g. decl
parameter order / SIP arrival order over the frozen Query graph), never `SortedPredecessors`;
drop the "induced by the chosen probe order" framing entirely.

---

## HIGH

- **H-P7-inkey-order · CONFIRMED · invariant-violation** (`P7-in-vars-unsorted-miskeys-first`).
  §1.4 sets `in_vars` in the iteration order of the order-FREE `bound_field_positions` SET, but
  `EmitScan`'s keyed_chain emits `index.First(RowExpr(input_vars))` trusting `input_vars` in
  `KeyColumns` order; `GetOrCreateIndex` `SortAndUnique`s (Data.cpp:350) so `KeyColumns` are
  ascending and the emitted `Key` struct is laid out ascending (Database.cpp:1102-1105). An
  unsorted bound set builds a mis-ordered key tuple → wrong (usually empty) results. Join
  (Database.cpp:3172-3175) and the cursor factory (:1820-1824) explicitly rebuild `key_exprs` by
  iterating `KeyColumns`; `EmitScan` does NOT reorder (:3384). **Fix:** §1.4 must sort `bound_cols`
  ascending and build `in_vars` in that order (or state the alignment invariant EmitScan relies on).
- **H-P8-schema-retrieval · CONFIRMED · soundness-gap** (`P8-schemanode-vs-retrieval`). F11/§2.4
  ("node keys on SCHEMA; never store values") is incompatible with the head-chain retrieval: a
  schema-keyed node has ONE `head` (§2.1) into which `LinkRow` drops ALL rows of that schema, so
  `TrieFirst(full_key).head` returns rows of ALL values — violating the full-key-EXACT,
  no-per-row-recheck contract (Table.h:791-803,815) the reuse claim rests on. Also `Index::First`
  is `const noexcept` (Table.h:804) but `TrieFirst`→`Navigate` MINTS nodes (mutation), so it is
  NOT the "byte-identical 2-method reuse, no new dispatch arm" §2.2 claims. **Fix:** nodes must be
  value-partitioned for `TrieFirst` correctness; drop the F11 "schema-only node" framing for the
  RUNTIME trie (F11 is a P5 compile-lattice rule, not a P8 navigation-structure rule); specify
  `TrieFirst` as a mutating seek or pre-materialize before read. (Couples with B-P8 — one
  reconciliation: nodes intern on `BindingStateId` = schema + sorted values, satisfying BOTH the
  P5 convergence key AND value-partitioned retrieval.)
- **H-P8-dedup-kind-blind · CONFIRMED · soundness-gap** (`P8-getorcreateindex-kind-blind`). §2.3
  keeps `GetOrCreateIndex` "UNCHANGED", but its dedup compares ONLY `column_spec` (a colon-joined
  id string, Data.cpp:357-362) and is kind-BLIND, so an ascending declared `@key(A,B)` trie index
  (`"0:1"`) ALIASES a join-pivot HASH request (`SortAndUnique{0,1}→"0:1"`) — the pivot silently
  receives a `kTriePrefix` index. **Fix:** add a `kind` field to `DataIndexImpl` (Program.h:91-108)
  and dedup on `(column_spec, kind)` in BOTH `GetOrCreateIndex` and `GetOrCreateOrderedIndex`; §2.3
  cannot leave `GetOrCreateIndex` untouched.
- **H-P9-frozen-graph · CONFIRMED · soundness-gap** (`P9-lift-frozen-graph-demolished`). §3.2
  `LiftBoundColumnsToRelation` re-expresses the pre-Optimize SIP walk over the FROZEN
  (post-Optimize) graph, but Optimize DEMOLISHES exactly what the walk descends/anchors on.
  VERIFIED: `ApplyDemandTransform` runs at Build.cpp:2601 BEFORE `Optimize` at Build.cpp:2622;
  TUPLE canonicalization (Tuple.cpp:60-125) "hops over trivially forwarding TUPLEs and single-source
  UNIONs" and deletes them — precisely the forwarding TUPLEs the walk descends (Demand.cpp:768-773)
  and the per-relation single-member MERGE it anchors on (Demand.cpp:585-598). The frozen graph will
  not present the chain the algorithm walks. §6.5 understated this as mild "debt". **Fix:** either
  run inference over the pre-Optimize graph (as the deleted pass did) or rewrite §3.2 against the
  post-Optimize invariants (readers pull from as-far-up-as-possible; no forwarding chain; no
  single-source MERGE); predict-then-verify against a real post-Optimize `.df` dump; upgrade §6.5
  from "debt" to a **blocking prerequisite**.
- **H-P9-render-anchor · CONFIRMED · anchor-wrong** (`P9-contract-render-anchor-deleted`). §3.5
  names `DataFlow/Format.cpp:1745-1798` as the split-render target, but that block iterates
  `qc.query.RecognizedSubgraphs()` (Format.cpp:1747) — a **P1-DELETED** side-table — and lives in a
  DataFlow-layer dump that cannot read where P9 stores inferred paths (the Regional
  `schema.access_paths`). **Fix:** re-target the split render to a Regional/typed dump sourced from
  `schema.access_paths` (provenance-tagged), not an in-place edit of the deleted Format.cpp block;
  state the seam (that `-contract-out` moves DataFlow→Regional). This composes with the P2 mandate
  "formatting must derive from typed records."

---

## MEDIUM

- **M-P7-loopshape-nondiscriminating · CONFIRMED · silent-pass-test**
  (`P7-loopshape-gate-nondiscriminating-for-queries`). §1.6.1/§5's "no-op emits NumRows, real emits
  First/Next" is FALSE for the query path: the baseline bound `#query` ALREADY selects an index and
  its cursor emits `.First/.Next` (Build.cpp:558-566 index selection; Database.cpp:1762-1767 via_index
  emission) — verified empirically this session in `key_neighborhood_witness`'s generated header
  (`db.idx_41.First({Start})`). **Fix:** drop loop-shape as the query-path belt; rely on §1.6.2
  cursor-shape (`pos` vs `s<id>`); FIRST establish P7 actually relocates the query read to a region
  scan rather than the existing indexed cursor. (§1.6.1 stays valid for the interior/rule-body path.)
- **M-P7-exactkey-fiction · CONFIRMED · soundness-gap** (`P7-exact-full-key-hasharrangement-fiction`).
  `exact_full_key`→`keyed_probe(Find)` uses `member.Find` on the TABLE, not any index; §1.4's
  `GetOrCreateIndex` over ALL columns mints an index codegen EXCLUDES from `index_member`
  (`ValueColumns().empty()`, Database.cpp:501) — a dead structure consuming a `next_id` (perturbing
  `.ir` goldens) that exists only to keep the §1.5 assert true. **Fix:** represent exact-full-key as
  its OWN plan kind emitting `Find` with `index=nullopt`, and relax §1.5 to allow
  `kFullKeyExactProbe` with `Index()==nullopt`; OR document the dead all-column index explicitly.
  (The OUTCOME is nonetheless correct today — certification 3 — but via a mechanism the doc mis-states.)
- **M-P8-freejoin-no-emitter · CONFIRMED · false-start** (`P8-freejoin-no-emitter`). §2.5's
  `FreeJoinSpine` ("lock-step multi-trie descent, not nested First/Next") has NO codegen consumer:
  the join emitter emits per-pivot NESTED First/Next loops (Database.cpp:3169-3179); no lock-step mold
  exists; §6 omits the `EmitJoin` rewrite. Labeling it `[REUSE-P9 shape]` understates a wholesale
  `EmitJoin` rewrite — and an emitted plan the join emitter cannot honor is a `label!=emission`
  violation of the retained D4 invariant. **Fix:** add the `EmitJoin` lock-step rewrite to §6 as a
  gating deliverable and gate `FreeJoinSpine`/`BuildLazyOrdering` behind a caps handshake analogous
  to `kTriePrefixWalk`.
- **M-P8-nodecount-not-goldenable · CONFIRMED · silent-pass-test** (`P8-nodecount-not-stable` +
  cross `P8-nodecount-pin-not-goldenable`). §2.7.1's "3 not 4" is NOT compile-time-stable for a LAZY
  MINT-ON-VISIT value-keyed trie (runtime node count = distinct probed prefixes) AND has no
  observation surface (§6#2: "unpinned by any golden"). It actually counts the EAGER compile-time
  SCHEMA spine — the P5 deliverable — not the P8 runtime structure. Contrast: reconstruction-diffs
  §5.6 M7 REQUIRED exactly this declared-vs-visited scoping for the P5 sibling, and ir-desired-states
  §7A/§7B render the count to `-region-out`. **Fix:** scope the pin to the eager
  `MaterializePrefixChain` spine (declared-origin nodes, before any `EvaluateEpoch`); fold M7's
  declared-vs-visited origin tag into `TrieNode`; name the observation surface (reuse the P5
  `-region-out` binding-schema block, since §2.4 makes `TrieNode↔BindingStateSchema` one-to-one) —
  or cite the P5 logical pin instead of asserting a physical count.
- **M-cross-inducedordering-answer-gate · CONFIRMED · silent-pass-test**
  (`cross-inducedordering-answer-only-gate`). §5 declares answer-equality a LOST CHECK, yet §4
  Goals-4+7 offers ONLY an answer-equality belt for the InducedOrdering hazard ("published surface
  byte-identical on vs off") — which a no-op InducedOrdering trivially passes (off≡off). **Fix:**
  replace with a structural pin: assert InducedOrdering's chosen order is a permutation consistent
  with the P6.2 SymbolicField classes (a reorder that changes a co-recursive class aborts), AND pin
  that a shared trie prefix is actually emitted (spine count) — never published-surface equality.
  This mechanizes §6#6.

---

## LOW / PLAUSIBLE

- **L-P7-plankind-hasheq · PLAUSIBLE · invariant-violation** (`P7-plankind-hash-equals-unspecified`).
  §1.5 never states whether `plan_kind` participates in `Hash/Equals/MergeEqual`
  (Program.h:1601-1640, which drive region dedup). **Fix:** exclude it (the `mint_tag` S5′ precedent),
  and state the §1.5 assert tolerates whichever `plan_kind` survives a CSE merge (inert to emission).
- **L-P9-split-source3 · CONFIRMED · soundness-gap** (`P9-split-loses-source3-provenance`). The
  declared/inferred split drops the base→derived pairing SOURCE 3 itself generates
  (`ctx.ordered_fields ++ base.ordered_fields`). Rationale (b) "N+M not N×M" holds for SOURCES 1/2,
  not 3. **Fix:** add a provenance/base back-reference token on contextual `inferred-key` lines, or
  scope the "strict superset" claim to union sources (the loss is observability, not correctness).
- **L-P9-boundat-noop · CONFIRMED · correctness** (`P9-bound-at-min-noop`). §3.3's `bound_at`
  comprehension `min(index_in(...))` wraps a single scalar (a no-op) and reassigns per
  `join.joined_views` (a `WeakUseList`, no order guarantee, Query.h:839) — taking the LAST, not the
  earliest, nondeterministically. **Fix:** aggregate `min` over ALL containing predecessors explicitly.
  (Moot if B-P9's fix removes `SortedPredecessors` entirely — the ordering source is re-derived.)
- **L-P7-accessplan-arms · CONFIRMED · missed-reuse** (`P7-accessplan-arms-diverge-from-target`).
  `next-session-prompt.md` names a 5-arm AccessPlan (FullScanFilter / FullKeyHashLookup /
  ExistingTriePrefix / EnumeratePrefix / BuildLazyOrdering); §1.1 collapses to 3, folding
  Enumerate/BuildLazyOrdering into a single reserved `kTriePrefixWalk`, so `TrieRange` (EnumeratePrefix)
  and `InducedOrdering` (BuildLazyOrdering) carry no distinct refereed label. **Fix:** keep the 5-arm
  taxonomy so each physical structure carries its own honesty-refereed label, OR document the
  subsumption and how the §1.5 belt discriminates them via the `DataIndex.kind` tag (which the belt
  does not currently read).
- **L-cross-varfor-drift · PLAUSIBLE · soundness-gap** (`P7-varfor-signature-drift`). P7 §1.4 uses
  `ctx.VarFor(f)` (field-only); reconstruction-diffs P4 uses `ctx.VarFor(f,v)` over
  `binding_state.canonical_bindings` (field AND value). EmitScan's keyed arms need the bound VALUES
  as `InputVariables`. **Fix:** reconcile the VarFor contract in ONE place (cite the same M5
  interner / D1 pull-forward artifact); make P7 and P4 share one signature.

---

## Certifications (diffs / mechanisms that HELD under attack)

1. **§1.6.2 cursor-shape discriminator HOLDS** — the baseline `#query` cursor keys on `pos` inside a
   `<name>_cursor` struct (Database.cpp:1750, both via_index :1763-1767 and full-scan :1771); a P7
   region scan keys on `s<id>` (:3346). `pos` vs `s<id>` is a real structural distinguisher (also
   confirmed empirically in `key_neighborhood_witness`'s generated header).
2. **The three-arm EmitScan taxonomy is accurately anchored** — keyed_chain :3381-3386, keyed_probe
   :3387-3391, full-scan+self-filter :3392-3418, `assert(indexed_cols.size()==input_vars.size())` :3405.
3. **The `exact_full_key` OUTCOME is achieved** (all-columns bound → `Find`/keyed_probe) — an
   all-column index is excluded from `index_member` (:501) so keyed_chain's guard fails and control
   falls to keyed_probe. Correct emission despite the fictional HashArrangement framing (M-P7-exactkey).
4. **`GetOrCreateIndex` is order-free by `SortAndUnique`** (Data.cpp:350), so `[A,B]`/`[B,A]` are the
   SAME hash index today — correctly grounding why an ORDERED trie (P8) is the discriminating structure.
5. **§1.4's mandatory `bind_outputs` requirement is sound** — one out var per column (:3345-3356); an
   empty `out_vars` yields an empty body.
6. **§2.2 `TrieNext(cursor)=next[cursor]` IS a byte-identical reuse of `Index::Next`** (Table.h:821);
   the prepend/id-order-in `LinkRow` matches `Index::Add` (Table.h:764-773). This HALF of the reuse
   holds (the `TrieFirst` half does not — H-P8-schema-retrieval).
7. **§2.5's "cubic self-join premise" handling is CONSISTENT with CLAUDE.md's group_ids note** — both
   state the self-join lowers to one table + two hash indexes + a pivot loop (non-cubic); §2.5 claims
   only a 2-indexes→1-trie structural change, no complexity refutation. No false-start.
8. **The trie NAVIGATES the canonical DiffTable and never owns rows** — interior nodes + terminal
   row-id chains only; liveness via `kInI`/`InNew`/`Present` (Table.h:385-424); no double-buffer.
   Respects the single-fact-authority + sole-liveness-authority invariants. HELD.
9. **P9 SOURCE 1 treats a query adornment as ONE inferred source, NOT the key** — the declared `@key`
   stays the kDeclared baseline; §3.4 guarantees survival; the union only ADDS. Respects "@key is not
   a query adornment." HELD.
10. **§3.4 V-DECLARED-PATH-PRESERVED is a real discriminating anti-regression belt** — it aborts on a
    dropped/reordered/demoted declared path, exactly what the deleted V-DECLARED-KEY bijection
    (Demand.cpp:892-960) wrongly REJECTED; provenance-survival, not answer-equality; has its baseline
    at Regional freeze (`schema.access_paths.filter(kDeclared)` is stable under the additive union).
11. **§3.2's descent RULE `in_col = view.input_columns[in_col.Index()]` mirrors the real forwarding
    rule** (Demand.cpp:768-773) — the rule is correct; only the graph it is asserted to run over is
    wrong (H-P9-frozen-graph).
12. **§3.5's "which .contract goldens move" is accurate** — exactly `key_multi_adorn_witness.contract`
    (2 declared-key lines) and `key_tc_witness.contract` (1 line); both real files, not symlinks; no
    other contract golden affected.
13. **§3.5's "the single line encoded the deleted SIP bijection" is verifiable** — Format.cpp:1757-1795
    pairs each declared `InstanceKeySet` with the `RecognizedSubgraph` key_cols whose SET equals it
    (`set_eq` :1757); `inferred=` is exactly the `p_bound` the declared set had to EQUAL.
14. **§1.2 CodegenPlanCapabilities matches the real EmitScan arms** — kFullScanFilter + kFullKeyHashLookup
    exist; kTriePrefixWalk correctly held OUT (Table.h has only whole-key `Index<Key>`).
15. **§3.6.4 SOURCE ISOLATION holds at the render** — the emitter prints logical field NAMES only
    (Format.cpp:1782-1795), never an AccessPlan/PhysicalAccessStructure token.
16. **The F33 join-pivot kSectionWalk corollary is treated consistently** across §1.5/§1.6(4)/§2.6 —
    coupled-but-out-of-scope, no contradiction.
17. **§3.1 SOURCE 1 grounding is accurate** — `UniqueRedeclarations()` + per-BindingPattern dedup
    matches Demand.cpp:531-542; bound-index collection Demand.cpp:537-542.
18. **§3.3 `bound_at` is well-defined for genuine pivots** — `out_to_in` (Query.h:832) maps each pivot
    output col to input cols from ≥2 predecessors, so index_in is total (the min-vs-last defect is
    separate, L-P9-boundat-noop).

---

## Net verdict

Direction sound; every survivor is an addressable amendment folded into
`keyed-rewrite-p7p9-diffs.md` §7. The three blocking findings each force one real correction
(thread `plan_kind` at every mint / intern trie nodes on `BindingStateId` / re-source the
inferred-path order from a DataFlow signal). No finding invalidates the four-authority target,
the AccessPlan-as-its-own-domain move, or the reuse of `ProgramTableScanRegion`/`EmitScan`. The
two strongest structural belts survive intact: the §1.6.2 cursor-shape discriminator (P7) and the
§3.4 V-DECLARED-PATH-PRESERVED anti-regression belt (P9). The recurring lesson — P8's exit-gate
pins and §4's InducedOrdering belt both silently reduced to a compile-unstable count or
answer-equality — reaffirms the session's "structural-pins-only" discipline for the physical layer.

---

# Session-15 re-critique — the absorbed corrections attacked for NEW seams (ranked)

Session 15 (2026-08-07, tip `46a404d4`). After the three s14 blocking corrections + two residues were
FOLDED into `keyed-rewrite-p7p9-diffs.md §1–§3` (s15), an independent opus refuter panel (4 refuters:
C1 P7 / C2 P8 / C3 P9 / C4 completeness+invariants) attacked the AMENDED diffs specifically for new
seams the fixes introduced. Every finding was VERIFIED by opening the cited `file:line`. **All survivors
are now folded back into the diff** (marked `⊗ CRITIQUE …` in place). The direction held; the load-bearing
outcome is a real SIMPLIFICATION (the redundant "P8a" dissolved into P7).

## The headline: "P8a" was redundant — the intra-relation prefix seek is P7 `kFullKeyHashLookup`

Three C-panel findings (`p8a-p7-redundant-duplicate-index`, `p8a-kind-tag-self-contradiction`,
`p8a-prefixindex-breaks-cse-safety-proof`) converged on one truth: the s15-first-draft "P8a prefix-sibling
`Index`" duplicated P7. `EmitScan` `keyed_chain` (Database.cpp:3381) already fires for ANY strict-subset
bound over a `GetOrCreateIndex(subset)` hash index, and prefix sharing + `[A,B]`/`[B,A]` full-key
convergence fall out of `GetOrCreateIndex`'s order-free dedup — so no separate structure, field, or arm is
needed. Folding "P8a" into P7 (§2.0) resolved the blocking finding + 3 others at once; P8 is now PURELY the
genuinely-new cross-relation ORDERED trie (Free Join / COLT).

## Survivors (all folded)

- **BLOCKING · `p8a-prefixindex-breaks-cse-safety-proof` (C4, CONFIRMED).** The added `region.PrefixIndex()`
  was a THIRD emission-selecting input never folded into `ProgramTableScanRegionImpl::Hash/Equals`, and it
  FALSIFIED the §1.4 "emission reads only (index,in_vars)" CSE-safety proof — CSE could merge two scans
  differing only in `PrefixIndex` and emit the wrong loop. **FIX (folded §2.0/§2.2):** delete the separate
  `PrefixIndex` field/arm — the intra-relation seek is P7 `kFullKeyHashLookup` over `region.Index()` (already
  in Hash/Equals). No new emission input; the plan_kind CSE proof stays valid (plan_kind alone is excluded).
- **HIGH · `p7-exactkey-index-nullopt-unreachable` (C1, CONFIRMED).** ALREADY FIXED proactively this session
  before the critique returned: `keyed_probe` requires `region.Index()` truthy (Database.cpp:3379), so
  `kFullKeyExactProbe` with `index=nullopt` would fall to the full-scan arm. **FIX (folded §1.4/§1.5):**
  `kFullKeyExactProbe` carries the (codegen-dead) all-column index (Some); belt asserts `arm is keyed_probe`,
  not `index==nullopt`. C1's recommendation matches the applied fix exactly.
- **HIGH · `satellite-lifetime-contradiction` (C3, CONFIRMED).** §3.1 cited `proxy_view_to_decl` as the
  satellite precedent, but that map is Build-scoped, "destroyed at return" (Build.cpp:2578-2582), VIEW*-keyed
  — it CANNOT reach `FrozenRegionalProgram::Build`. **FIX (folded §3.1):** store `impl->inferred_access_paths`
  as a `QueryImpl` MEMBER keyed by Optimize-stable PARSE identity (`row_contracts`/`RecognizedSubgraphs`
  precedent), which Regional already friend-reads.
- **HIGH · `p8a-plan-kind-caps-seam-unreachable-arm` (C4, CONFIRMED).** The P8a/P8b split was never wired
  into §1.1/§1.2/§1.3 — the only prefix kind (`kTriePrefixWalk`) is gated OUT of caps, so the `keyed_prefix`
  arm was unreachable. **RESOLVED by the headline** — no `keyed_prefix` arm exists; the intra-rel seek is P7
  `kFullKeyHashLookup` (in caps), the ordered trie is `kTriePrefixWalk` (out until P8 lands).
- **MEDIUM · `p8b-navigate-value-keyed-leak` (C2, CONFIRMED).** The B-P8 `BindingStateId` interning was in the
  §2.1 struct comment but NOT in the `Navigate()` body, which still did value-keyed LOCAL edge lookup — so
  `[A,B]`/`[B,A]` did not converge. **FIX (folded §2.1):** rewrote `Navigate` + a new `intern(trie, bsid)`
  against a region-global `pool : map<BindingStateId → TrieNode>`; `edges` only CACHE pool lookups.
- **MEDIUM · `source23-adornment-key-undefined` (C3, CONFIRMED).** The union keyed every path by
  `(rel.decl, p.adornment)`, but SOURCE 2/3 have no adornment and SOURCE 1 never threaded it — `p.adornment`
  undefined, dropping non-adornment paths. **FIX (folded §3.1):** each `OrderedPath` carries a `PathSourceKey`
  (`kQueryAdorn`/`kJoinPivot`/`kContextual`); the union keys on `(rel.decl.Id(), p.source)`.
- **MEDIUM · `p8a-3-not-4-pin-nondiscriminating` (C2, CONFIRMED).** The "3 distinct DataIndexes" P8a pin was
  a LOST CHECK — the all-column `{A,B}` index always exists (Data.cpp:205), `{A}`/`{B}` pre-exist if
  join-pivoted, and 4 order-free hash siblings are impossible (SortAndUnique). **FIX (folded §2.7 pin 1):**
  pin the COMPILE-TIME `-region-out` binding-schema spine (3 `origin=declared` schema nodes), NOT a DataIndex
  count.
- **MEDIUM · `p8a-kind-tag-self-contradiction` (C2, CONFIRMED).** §2.3 said the P8a prefix minter passed
  `kTriePrefix` while also calling it an order-free hash — a `kTriePrefix {A}` would then NOT share with a
  join hash `{A}`, duplicating a fact index (test 14). **RESOLVED by the headline** — the intra-rel seek is
  P7 `kHash` (shares with join pivots); `kTriePrefix` is reserved for the P8 ordered trie only.
- **LOW · `p7-query-path-cursor-shape-scope` (C4, PLAUSIBLE).** Pin (1) `pos` vs `s<id>` discriminates only
  if P7 actually relocates the bound-query READ to a `ProgramTableScanRegion`; if the hand-emitted
  `<name>_cursor` factory stays, `s<id>` never appears on the query path. **FIX (folded §1.6 pin 1):** stated
  the precondition (P7 must lower the bound-query answer to a region scan, or pin (1) is interior-only and the
  query path relies on the P3 RequestEdge/RoutedResult pins).

## Certifications (amendments that HELD under attack)

- **plan_kind excluded from Hash/Equals is CSE-safe** (Operation.cpp Hash@1611-1626 mixes only op/table/index;
  Equals@1642-1643 ptr-equality; MergeEqual never mutates index/in_vars; emission label-blind, grep=0). Sound
  FOR plan_kind alone — the very reason the separate `PrefixIndex` field (an emission input) was rejected.
- **The kUnplanned sentinel + belt-skip keeps P7 a pure addition** — both legacy mints (Join.cpp:254-266
  index=Some; Build.h:469-475 index=Some-or-None) leave plan_kind default; the belt skips them.
- **H-P7-inkey-order sort is correct** (Data.cpp:350 SortAndUnique + Program.cpp:472 KeyColumns ascending;
  Join.cpp:268-278 builds in the same order).
- **Arm-A (pre-Optimize inference) holds** — a read-only pass slots at Build.cpp:2601 before Optimize;
  out_to_in order is canonicalization-disclaimed; the RAZOR (read-not-mutate) is airtight on its own axis.
- **`c.RelDeclOrdinal(rel)` names a real signal** (the pivot column's arrival position = `in_col->Index()`).
- **SOURCE-1 adornment-ordinal and SOURCE-2 decl-ordinal are non-contradictory** (they order DIFFERENT
  additively-unioned paths; neither overwrites the other).
- **V-DECLARED-PATH-PRESERVED's baseline survives the DataFlow→Regional handoff** (declared @key sets are
  re-derivable at freeze from parse data, independent of the inference member).
- **Avoid-false-starts all HELD**: no power-set materialization (prefix indexes only along declared/inferred
  paths); no per-ordering database (ordered indexes navigate the shared DiffTable, own no rows); no
  second fact owner (trie stores interior nodes + row-id chains, liveness via membership preds); no
  RequestEdge/RuleActivationEdge minted by the physical layer (goal 3 edge-agnostic); DataFlow-mutation
  authority NOT reintroduced (P9 reads-not-mutates); no full-scan mislabeled (kUnplanned SKIPPED, not
  mislabeled; kFullScanFilter asserted honest).
- **A prefix-projected index IS an ordinary DataIndex** and is not wrongly excluded from index_member; the
  8-call-site `kind`-threading list is COMPLETE (every entry a real GetOrCreateIndex call).
- **`@key(A,B)`/`@key(B,A)` genuinely converge on ONE full-key index; their length-1 prefixes `{A}`/`{B}`
  are two distinct DataIndexes** (the convergence-at-full-key mechanism is real).

## Net verdict (s15)

Direction sound; every survivor folded. The panel's highest-value output was DISSOLVING the redundant
"P8a" — the intra-relation prefix seek is P7, not a new structure — which killed the one blocking finding
plus three mediums and left P8 as a clean, single genuinely-new deliverable (the cross-relation ordered
trie). Two verified corrections-to-the-corrections carried from the grounding round (B-P7's "sole mint
site" was wrong; B-P9 Arm-A confirmed by the observed `demand_tc_witness` TUPLE/MERGE collapse). No finding
invalidated the four-authority target, the AccessPlan domain, or the ProgramTableScanRegion/EmitScan reuse.
