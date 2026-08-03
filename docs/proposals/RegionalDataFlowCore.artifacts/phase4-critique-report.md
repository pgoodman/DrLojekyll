# Phase-4 critique synthesis — RegionalDataFlowCore desired-state docs

Synthesis of the refute-verified critique passes over the four desired-state
artifacts. For each surface doc: the findings that SURVIVED refutation (with
their verdicts), the amendments an implementer should apply before treating the
doc as a spec, and a one-line disposition verdict. Owner-facing ADJUDICATION
INPUTS are collected at the end — each labeled with the panel's recommendation
where the panel has one, explicitly marked as the panel's view, not a decision.

Verdict vocabulary:
- **SOUND** — no surviving amendment gates use of the doc as a spec.
- **SOUND-WITH-AMENDMENTS** — usable as a spec once the listed edits land; none
  of the edits reopen a design question.
- **BLOCKED-ON** — a surviving finding cannot be closed by an edit; it needs an
  owner adjudication before the affected surface can be pinned.

---

## 1. df-stage-a-desired-states.md

### Surviving findings

- **RDA-C1 — CONFIRMED (blocking for that one golden; mechanism corrected).**
  The §4.2 `demand_tc_witness` census line reads `views=19 contracts=19 … na=8`,
  but the dump has 20 live views (^0–^19: 2 select + 11 tuple + 4 join + 2 merge
  + 1 insert) and §4.2 itself renders all 20 blocks. Correct values:
  `views=20 contracts=20 na=9` (2 sel + 4 join + 2 merge + 1 insert); `distinct=5`
  and `member=6` are already right. The other two witnesses' census lines match
  their dumps (agg_distinct=17, join_1=20). Mechanism correction: the real
  emitter self-counts 20/20 consistently — there is no dump-time abort; the
  failure is a bless-time byte-compare mismatch when this hand-written line
  becomes a golden.

- **RDA-T1 — CONFIRMED (one sub-claim weakened).** `role=` is
  normalization-provenance (clause-head projection ⇒ `kDistinct`; optimizer-minted
  forwarding facade ⇒ `kMember`), NOT recomputable from final-graph topology:
  demand_tc `tuple.2` (distinct) and `tuple.6`/`tuple.7` (member) all feed joins,
  so successor kind does not determine role. This is in genuine tension with
  §2.2's "pure, recomputable function of the final graph" claim — the fresh
  `*.contract.opt.golden`'s role column has no external referee beyond byte
  self-compare, so the first bless is blind. Weakened sub-claim: the derivation
  heuristic is NOT unstated — §3.3/§4.2 state it in prose.

- **RDA-T2 — CONFIRMED (partially mitigated).** §6 "verified structurally: no
  witness has a kMember/kDistinct pair otherwise structurally identical"
  extrapolates a 3-witness structural argument to a 180-case corpus byte-identity
  claim. Because `ProjectionRole` rides in Hash/Equals, any un-examined case with
  two otherwise-identical projections of differing role refuses a CSE merge and
  grows a `.df` line (the doc's own E-A1 failure mode). Mitigation: the doc does
  NOT assert this settled — the §6 exit-gate table lists `.df` byte-identity with
  Referee=byte-compare (a gate to RUN), and E-A1 flags the CSE-flip as the
  blocking risk. So this is a phrasing correction, not a missing gate.

- **RDA-D1 — WEAKENED (strong claim refuted; residue retargeted).** The "no
  canonical order for antichain elements when n≥3" charge is refuted by §2.2
  (L82–85: "every candidate-key antichain is rendered in canonical field-id
  order … permcheck N/A, no order-free field"). Surviving narrow residue: the
  §4.2/§5.2 join examples render `{ (M,F,T) | (M,T) }` — a full-then-minimized
  SEMANTIC pairing that reads inconsistently with the stated field-id order and
  does not generalize to n≥3 (no single "full").

- **RDA-C2 — WEAKENED (core refuted).** The alleged §15.11 violation does not
  hold: an identity-folded `ProjectionRole` enum + the §11 no-convert validator +
  role-differentiated §4.4 transfer rules IS the faithful realization of "member
  vs distinct are different logical operators." §15.11 nowhere requires distinct
  C++ subclasses or a distinct rendered token. Residue: cosmetic — the doc never
  cites §15.11 by number.

- **RDA-C3 — WEAKENED (real inconsistency, misattributed).** `regional-arch-
  pseudocode.md §6` (L603–605) says normalization emits member/distinct as
  DISTINCT NODES + a new validator — which would mint ids/kinds and break
  byte-identity. But the desired-state doc does NOT cite §6 as authority; §1
  loudly names H-A2 as "the OPPOSITE realization" and cites `stage-a-diff.md`,
  whose O-A1 already ratifies the enum (Variant 2, ".df untouched"). The stale
  doc is pseudocode §6, not the doc under review. Narrow residue: the §7/§8
  adjudication list omits Variant 1 (subclass/node-minting) entirely, so a reader
  who never opens stage-a-diff won't see it was considered and rejected.

- **RDA-N1 — WEAKENED (taste, minor).** `support=`/`candidates=` are derivable
  from kind+key on select/tuple/compare/insert (the antichain does discriminating
  work only at join/merge/agg). "Zero discriminating power" is overstated —
  support= still separates base-input from passthrough, and a uniform per-block
  grammar keeps the V-CONTRACT-CENSUS bijection visually parseable. Golden-
  verbosity taste, not a defect.

- **RDA-N2 — CONFIRMED-minor (taste).** `role=` is n/a on every non-tuple block;
  "pure noise" mildly overstates (role=n/a documents the question was considered).
  Legitimate verbosity trim (emit role= only on TUPLE blocks); cosmetic.

- **RDA-D2 — WEAKENED (minor).** The census has no non-singleton-candidate
  (join/merge-minimized) counter, but a mis-minimization changes that block's
  `key=`/`candidates=` lines, which the per-block byte-compare catches. Aggregate
  summary, not sole referee; low-value enhancement.

### Amendments to apply

1. **(RDA-C1, do first)** Recompute the §4.2 census line to
   `views=20 contracts=20 … na=9`; add a mechanical rule to the doc: every
   census line is arithmetic-checked against a grep of its `.df` before bless.
2. **(RDA-T2)** Replace "verified structurally: no witness has …" with "must be
   verified by the full suite run; the E-A1 CSE-flip is exactly what the suite
   byte-compare checks."
3. **(RDA-D1)** Make the field-id lexicographic sort normative in §4.2/§5.2 and
   drop the `full | minimized` framing so multi-candidate antichains have a
   pure-function rendering that generalizes to n≥3.
4. **(RDA-T1)** State explicitly that `role=` is a carried normalization stamp
   with no final-graph referee, so its first bless is self-compare-only; note the
   §7 in-`.df` variant is the "render role cross-referenceably" mitigation.
5. **(RDA-C2)** Add a one-line cross-reference: enum-in-identity + the no-convert
   validator + role-keyed transfer rules IS the §15.11 "different logical
   operators" realization.
6. **(RDA-C3)** Amend `regional-arch-pseudocode.md §6` to match O-A1 (enum, not
   distinct nodes); add a one-line O-A1 pointer in the desired-state doc and note
   Variant 1 (subclass/node-minting) as considered-and-rejected in §7/§8.
7. **(RDA-N1/N2/D2, optional)** Log as golden-shape adjudication inputs.

### Verdict

**SOUND-WITH-AMENDMENTS.** C1 must be corrected before the §4.2 line is blessed;
T2/D1 are phrasing/rendering corrections; C2/C3 are documentation cross-reference
hygiene (the substantive design — enum-in-identity — is correct and already
ratified by O-A1).

---

## 2. regional-dump-stage-b-desired-states.md

The critique inflated nine findings on one shared false premise — that a dumped
"row-contract" is one-per-persisted-table. It is not: the dump is a per-declared-
relation SKELETON (stage-b-diff.md's own G1 example renders a single contract E0,
not |live views|). Ground truth: `join_1` materializes SIX tables and p(A,B)/r(A)
DO persist with exactly the rendered schemas and declared names. What survives is
one coherent problem stated three ways.

### Surviving findings

- **T2 — CONFIRMED (strong; load-bearing).** `census row-contracts=N` has no
  adjudicating oracle. V-REGION-CENSUS-IDENTITY (H9) asserts only
  (region count==1) ∧ (every Rel op ∈ RegionId 0) — no contract count; the
  29-kind Rel census has no row-contract kind; Stage A's only contract count is
  |live views|. Worse, the one nearby number that reads "2" — `kEagerInsert=2` in
  all three witnesses' `.rel` censuses — counts terminal WRITE-SINKS (q/never for
  join_1), the COMPLEMENT of the read-source relations (p/r) the artifact names.
  So `row-contracts=2` is a byte no validator recomputes, grounded on the wrong
  set.

- **C3 — CONFIRMED (with one correction).** The undercount and the query-contract
  inconsistency are both real: demand_multi_adorn creates 5 tables, demand_tc 6,
  vs rendered `row-contracts=2`; `%table:15` (an R-DUP merge intermediate,
  persisted and READ, no declared-relation home) is uncounted; and join_1's
  #query q/never get NO row-contract while demand_tc's #query reachable_from gets
  E1 + a request-port, with no stated rule distinguishing them. Correction: the
  `demand__` relations are received MESSAGES, so rendering them as input-abi is
  defensible — the genuinely uncounted persisted member is the anonymous
  `%table:15`. The selection rule that fits ("declared relation read as a
  scan-source, excluding write-only sinks and anonymous intermediates") is never
  written.

- **C1 — CONFIRMED (narrowed; not blocking).** The "fundamentally different
  cardinality neither pass can produce" charge is refuted — a reduced count is BY
  DESIGN (the dump is a skeleton, stage-b-diff.md's own G1 renders one contract).
  Surviving: the artifact never specifies the projection from the ~20 per-view
  RowContracts down to the 2 rendered, nor an oracle for it.

- **D3 — CONFIRMED (minor).** §1.3 grounds "ONE shared pub" on the NESTED `.rel`
  (`i#0`/`i#1` share pub=%table:4, a `-demand-instance` artifact), but the golden
  it pins is the FLAT (bare `-demand`) dump. The flat `.df` independently grounds
  it: `demand_multi_adorn_witness.df:91` has a single `insert … into %table:4`
  and one insert node. Re-ground on the flat citation (and note %table:4 is a
  DataFlow-attributed id, consistent with §0.1's no-Program-TableId constraint).

- **C2 — WEAKENED.** Headline ("p/r are optimizer-inlined, do NOT survive") is
  FALSE — join_1.ir:23/:30 `create %table:12[i32,i32]`(p) / `%table:16[i32]`(r),
  both populated and join-read. Surviving kernel: the grounding sentence is
  self-contradictory — it calls q/never "table-less, no stored contract" while
  grounding the count on "kEagerInsert=2 — exactly two stored sinks," but those
  two ops ARE the q/never sinks (the complement of p/r).

- **T1 — WEAKENED (mostly refuted).** The label `p` IS a pure function of state
  at the dump slot: the ParsedModule `#local p` declaration (fields A,B) exists
  pre-Program, and ADJ-1 keys contracts on declared relation identity + field
  names. The real residual reduces to C3 (which relations are selected).

- **N1 — WEAKENED (= ADJ-2).** The artifact already raises the `demand__`
  input-abi-vs-region-internal choice as ADJ-2. Forward-looking merit
  (input-abi lines Stage C deletes ⇒ guaranteed re-bless) is real but is a design
  preference the artifact surfaces, not a hidden defect.

- **N2 — WEAKENED (= ADJ-3).** The all-free `fields=()` request-port observation
  is already ADJ-3, with the exact alternative (render all-free queries as
  output/permanent roots) named.

- **D1 — WEAKENED.** "relation-decl order undefined on members lacking a declared
  relation" bites only under the (refuted) assumption that anonymous intermediates
  are contract members. Over declared relations the #relation-decl order is a
  well-defined source-order total. Collapses into C1/C3.

- **D2 — REFUTED.** Premise factually wrong: join_1 creates SIX tables
  (%table:6,9,12,16,19,23), not 2; demand_multi_adorn creates 5. The "2 vs 5"
  under-materialization contrast is false, and the region census already
  discriminates the witnesses via input-ports (join_1=2 vs demand_multi_adorn=3).

### Amendments to apply

One paragraph closes the load-bearing problem (T2 + C3-tail + C1):

1. Define the dumped row-contract SET as a NAMED projection of
   `InferConservativeRowContracts` — the declared relations that are READ as a
   scan-source, excluding write-only sinks and anonymous intermediates — and give
   it its own recompute check.
2. Correct or remove the `kEagerInsert=2` grounding; it counts the write-sink
   complement (q/never), not the read-source relations (p/r).
3. State the join_1-vs-demand_tc selection rule explicitly (why join_1 drops its
   #queries while demand_tc keeps reachable_from), reconciled with the message-vs-
   relation distinction for `demand__`.
4. Give `row-contracts=` a recompute oracle (defer to Stage C if it cannot be
   refereed at Stage B, and say so).
5. **(D3)** Re-ground the "ONE shared pub" claim on the flat `.df`'s single
   insert into %table:4, not the nested `.rel`.

### Verdict

**SOUND-WITH-AMENDMENTS.** The p/r schemas are genuinely materialized and
declaration-recoverable, so the doc's substance holds; the one real defect — an
unspecified, un-refereed row-contract set/count grounded on the wrong number — is
a one-paragraph fix. Note the grammar pick itself (G1/G2/G3) is deliberately left
open by the artifact and is an owner adjudication, not an amendment (see inputs).

---

## 3. rel-stage-c-desired-states.md

The critique is sharper on effect-set decomposition than on testability. Two
findings survive as load-bearing design defects.

### Surviving findings

- **N1 — CONFIRMED (major).** Double-write in the desired effect-set split. The
  current `op.0 kSubgraphInstantiate` appends `kVecAppend(%table:15, kAddQueue)`
  exactly once. The split assigns that append to `op.<e> kChildResultAdd` (a clean
  1:1 take of op.0's `+` half) AND then gives `op.<g> kRoutedResultAdd` a SECOND
  `kVecAppend(%table:15, kAddQueue)` into the same table's same queue (symmetrically
  the `kDeleteQueue` is doubled). This contradicts §3.1/§3.3 ("routed … a derived
  join, NEVER materialized … rides the pub's existing queues"): child-result
  `%table:15` and `routed rr#0 child_result=%table:15` resolve to the SAME physical
  `kAddQueue`, so the second append increments the split counter twice for one
  logical row — the exact multiplicity the commit sweep asserts against. One
  authority must own the queue write; the other must be a pure read/derive.

- **C2 — CONFIRMED (major; on an admittedly un-pinned Variant).** Variant A's
  census (`kRequestEdgeAdd=1`/`Remove=1` with all `kChildResult*`/`kRoutedResult*`
  = 0) is exactly the "request edge without caller-qualified result maintenance"
  shape proposal §11 lists as a hard validator FAILURE (and §5.3/L946 forbids). But
  §6 (L554–555) forces tc's BOUND query to mint that edge (`kRequestEdgeAdd=1`).
  The disposition's option (a) ("mint NO edge like join_1") itself contradicts §6 —
  join_1 mints no edge only because its query is FREE. So Variant A must either
  contradict §6 or carry a degenerate self-routed result, and as written does
  neither. The artifact explicitly presents Variant A as un-pinned (Adjudication
  Input B), so this is a flaw in an admitted sketch — but the sketch cannot become
  a pin until resolved.

- **N2 — CONFIRMED (narrowed; "vestigial" framing weakened).** The `differential=`
  flag's stated semantics are self-contradictory: §3.1 justifies `differential=true`
  by "the edge is retractable — a kRequestEdgeRemove exists," yet §4.2 says
  `differential=false` edges STILL emit `kRequestEdgeRemove` (census
  `kRequestEdgeRemove=2 kRetireInactive=2`), and §3.3 gives every edge relation the
  full six-vec differential family unconditionally. So "a kRequestEdgeRemove exists"
  cannot be the flag's meaning. Weakened: a defensible unstated reading survives
  (steady-state user retraction via `-demand-retract` vs teardown-only
  lease-destructor retraction) that the op census cannot show and a flag could carry.

- **C1 — WEAKENED (comment fix).** The op-level desired state is correct — in
  `demand_neighborhood_witness-nested.rel`, `%table:7`(add_edge) is monotone
  (single `op.26 kIngestFold sign=+`, no removal fold), the retraction lives on the
  demand table `%table:4`, and op.`<c>`'s effect set is only the monotone a2 add
  drain. Defect confined to the trailing comment "the a2/a2′ split COLLAPSES to one
  signed op," which names an a2′ band that exists only for `demand_diff_input_1`, not
  a monotone input.

- **T1 — WEAKENED (core true, overstated).** Every demand-witness lifecycle body and
  census is `[shape]`; join_1's census is the sole `[pinned]` pre-implementation
  surface; V-LIFECYCLE-CENSUS is a self-consistency recount (a wrong-but-consistent
  lowering passes). But the witnesses' behavioral answers ARE adjudicated by the
  eqgate stdout referee (flat==nested==golden + sorted published-delta identity),
  and the artifact already self-discloses the residual as Adjudication Input C /
  stage-c-diff ESCALATION E1 ("the mono witnesses' new removal-arm goldens have no
  oracle — one must be authored").

- **T2 — WEAKENED (literal charge refuted).** The artifact does not attribute the
  census-diff capability to permcheck.py — L108–110 read "a permcheck-STYLE referee
  COULD auto-verify," an explicitly hypothetical new referee, and §1.1/§6 state the
  census is NOT in permcheck's order-free set. Residue: soft — the word "mechanical"
  leans on a not-yet-built referee.

- **C3 — WEAKENED.** `join_1.rel` does open directly at `op.8` with no vec/branch/join
  sections, so §2's "vec block, branch/join sections … byte-identical" describes
  absent sections — vacuously true, imprecise phrasing not an error; the load-bearing
  §2 claim ("byte-identical EXCEPT the census line") is correct.

- **D1 — WEAKENED (leans refuted).** `kChildResultAdd=2` alone cannot distinguish
  two-shared-pub from two-distinct-pub, but the premise "census is the one
  hard-adjudicated surface" is false — the ENTIRE `.rel` dump (incl. the
  `child-results:` section with `%table:N` ids) becomes the byte-compared golden
  once blessed, and the multi_adorn eqgate asserts each probe's exact answer, so a
  shared-pub split fails byte-compare and/or diverges. A dedicated `kSharedPub` token
  would be nicer but the realization is not unguarded.

- **D2 — WEAKENED (minor).** §6 already marks region order `[shape; determinism
  pinned]` (R# labels drift, relative order pinned — exactly the ask), and the
  grounding is real (`ApplyDemandTransform`'s two-phase loop over
  `UniqueRedeclarations()` orders `i#0` before `i#1`). Residual: cite the explicit
  Stage-A `LogicalNodeId` comparator key rather than "the same source that orders
  i#0 before i#1."

### Amendments to apply

1. **(N1)** Give the pub `kAddQueue`/`kDeleteQueue` write a SINGLE owning op
   (child-result), and make `kRoutedResultAdd`/`Remove` pure read/derive over the
   pub's existing queues — no second `kVecAppend`. Restate §3.3 so "RoutedResult adds
   NO new six-vec family" is enforced by the effect sets, not just prose.
2. **(N2)** Either drop `differential=` or re-ground it on the property that actually
   varies (input/child-result retractability under `-demand-retract` vs teardown-only),
   and reconcile §3.1 with §4.2.
3. **(C1)** Fix the trailing comment on op.`<c>`: for a monotone input there is no
   a2′ band; `kInputDelta` carries only the monotone a2 add drain.
4. **(T2)** Reword to "manual, or specify the new permcheck-style referee."
5. **(D2)** Cite the Stage-A `LogicalNodeId` comparator key in §6.
6. **(C2)** Owner adjudication — see Adjudication Input B below. This blocks pinning
   tc's `.rel`.

### Verdict

**SOUND-WITH-AMENDMENTS on the body; BLOCKED-ON the tc `.rel` pin (C2/Input-B).**
N1 must be fixed and N2 re-grounded before the effect-set decomposition is a spec;
the determinism/census-transformation/adjudication scaffolding is otherwise sound.
tc's `.rel` cannot be pinned until the owner resolves the H-J Variant A/B choice —
Variant A as sketched is the §11-forbidden shape.

---

## 4. header-stage-c-desired-states.md

Three factual corrections plus one real under-determination.

### Surviving findings

- **T2 — CONFIRMED (major).** §0's "every `.h.<mode>.golden` is byte-compared across
  all four optimization modes" is doubly wrong: (a) only `*.h.opt.golden` exist
  (`cf16_2`, `demand_tc_witness`) — there is no per-mode `.h` golden family; and (b)
  headers are NOT cross-mode byte-identical (df/cf-opt-off skips dead-flow
  elimination ⇒ different surviving node/table counts and numbering — exactly why
  the pin carries the `.opt` suffix and why the doc must use `<id>`). Cross-mode
  agreement is a `.stdout` property (one golden, four execution variants), never a
  `.h` property.

- **C1 — CONFIRMED (major; mis-location corrected).** §1.4(b) retypes the threaded
  relation to a 2-column owner-bearing `DiffTable<RowReq_<id>>` and threads it into
  `proc_19`/`flow_136`, while §1.8 says the region body is "the ordinary Rel
  differential fixpoint the flat arm already emits" — but that flat `flow_136` body
  keys `table_4.Find({v56})` single-column, incompatible with a 2-col
  `RowReq{owner,c0}`. §1.2 recovers presence via `ActiveInstanceRelation =
  DistinctProjection(child)` (owner projected away), but that presence/projection
  relation is never declared as a header member or threaded param. So the pinned
  proc signatures are under-determined. Correction: the locus is NOT `next()` (§1.7.1
  scans `neighborhood_15` via `idx_135`, never touches `table_4` — byte-identity
  fine); it is the `flow_136` single-col presence join vs the 2-col retype, plus the
  missing `DistinctProjection` member.

- **C2 — CONFIRMED (minor).** §1.1's "fixed alphabetical-within-group codegen order"
  is a load-bearing misstatement: Database.cpp:3639-3653 is a FIXED sequence
  (Allocator, Hash, Table, conditional StateCell/InstanceStore, Vec) with conditional
  headers slotted between Table.h and Vec.h — not alphabetical (StateCell/InstanceStore
  sort before Table yet emit after). An implementer following the stated rule would
  place `RootRequestLease.h` between Hash.h and Table.h (wrong); the doc's own drawn
  block correctly shows it after Table.h. Bytes right, rationale wrong.

- **C3 — CONFIRMED (minor).** §0 item 3 lists "Deleting the demand message entry
  (H-F)" inside the hidden-friend-emission paragraph, but the fabricated demand
  message is SUPPRESSED from the kMessageHandler friend loop (Database.cpp:1511-1512)
  — there is no demand hidden-friend to delete. The friend-section deletion is only
  `*_retract` (H-H); the demand deletions are internal procs (item 4's concern).

- **N1 — WEAKENED (major → open, on different grounds).** The stated justification is
  factually wrong: "under the drain contract at most one cursor/lease per query is
  live at a time" — draining a cursor does NOT destroy it or release its lease. A
  driver can hold drained cursor A for `Start=5` (lease alive), then call
  `neighborhood_bf(db,5)` again for cursor B — two concurrent live leases on the SAME
  key, two distinct `RequestEdgeId`s (the F4 scenario the owner column targets),
  reachable in Stage C. So owner-disambiguation is NOT purely Stage-D. What survives
  as genuinely open (= A1): whether Stage C needs owner IDENTITY vs a key-keyed
  `DemandSupportCount` refcount — and resolving A1 toward presence-only WOULD dissolve
  C1 (that linkage is valid).

- **T1 — WEAKENED (blocking → minor scoping).** §0's "PURE BYTE GOLDEN" is about how
  the eventual `.h` golden is refereed, and the doc's own grounding rule (L21–24)
  already declares new constructs "shape-exact with metavariables (`<id>`, `<edge>`,
  `RowReq`)" and flags A1–A5. Residue: scope the "PURE BYTE GOLDEN" sentence to
  SURVIVING constructs; A1–A4 genuinely differ between conforming implementations,
  which the metavariables already acknowledge — a scoping edit, not a bless-blocker
  for a design doc.

- **D1 — WEAKENED (major → minor).** "cannot claim byte-stability for lines carrying
  those labels" is wrong — the new ids are a deterministic function of the frozen
  program (Stage-A `LogicalNodeId`, HP-9). Residue: the doc should name the
  id-DERIVATION rule for the new procs/relations (e.g. "acquire inherits the deleted
  inject's node id" vs "lowest free proc id at the forcing site") so the literal is
  pinnable at bless.

- **N2 — WEAKENED (near-refuted).** `log`/`Functors` on `acquire_<id>` are
  load-bearing, not vestigial: acquisition GENERALLY materializes the demanded
  subgraph (publishing to log, possibly calling MAP functors), and the forcing-query
  hidden-friend signature is a fixed ABI family (EmitQueryFriends emits Log/Functors
  whenever `spec.forcing_function`). Residual: editorial — add "ABI uniformity"
  alongside "always publishes" as the justification.

- **D2 — REFUTED.** The `// Query neighborhood/2 (bf).` comment is emitted
  unconditionally at the TOP of EmitQueryFriends (Database.cpp:1635), independent of
  the retract; the retract block (1679-1704) carries its own separate comment and
  sits between the query comment and the cursor. Deleting the retract (H-H) leaves
  the query comment directly above the cursor — exactly §1.6's claim. Behavior IS
  derivable from codegen.

### Amendments to apply

1. **(T2)** Correct §0: the `.h` golden family is OPT-MODE ONLY (`*.h.opt.golden`);
   headers are not cross-mode byte-identical; cross-mode agreement is a `.stdout`
   property. Drop the "four-mode `.h` byte-comparison" claim.
2. **(C1)** Resolve the 2-col retype vs single-col carried-forward join: declare BOTH
   the owner-bearing request-edge relation AND the single-col
   `DistinctProjection`/presence relation as header members (or state which one
   `flow_136`'s join consumes and thread it). Note this couples to A1/N1 — a
   presence-only (refcount) resolution dissolves C1.
3. **(C2)** Replace "fixed alphabetical-within-group" with "fixed conditional-header
   slot between Table.h and Vec.h, mirroring the StateCell/InstanceStore guards."
4. **(C3)** Reword §0 item 3: the friend-section deletion is `*_retract` (H-H) only;
   the demand message is suppressed (never emitted as a friend); demand internal-proc
   deletions belong to item 4.
5. **(T1)** Scope §0's "PURE BYTE GOLDEN" sentence to surviving constructs; keep the
   metavariable carve-out for new constructs.
6. **(D1)** State the id-derivation rule for the new acquire/request-edge procs and
   relations so the literal ids are pinnable at bless.
7. **(N2)** Add "ABI uniformity" to the `acquire_<id>` log/functors justification.
8. **(N1/A1)** Owner adjudication — owner identity vs key-keyed refcount; see input.

### Verdict

**SOUND-WITH-AMENDMENTS.** T2 and the C1 under-determination must be resolved before
the header proc signatures are pinned; C2/C3 are mechanism-of-codegen corrections;
D2 is refuted and dropped; T1/D1 are scoping/derivation-rule edits, not blockers. The
public arity is UNCHANGED (the lease does not alter any `(db, log, functors, bound…)`
signature) except the open A5 all-bound-existence-check case.

---

## Collected ADJUDICATION INPUTS (owner-facing)

Each input states the DECISION, the ALTERNATIVES, the EVIDENCE the critiques
surfaced, and — where the panel has one — a RECOMMENDATION explicitly labeled as
the panel's, not a decision.

### AI-1 (Stage A) — in-`.df` role rendering vs separate `contract`-out sink

- **Decision.** Where does the `ProjectionRole` (member/distinct) surface for
  review: inline in the existing `.df` dump, or only in a separate
  `*.contract.opt.golden` sink?
- **Alternatives.** (a) In-`.df`: render `role=`/`support=`/`candidates=` on the
  `.df` block itself (one-surface reviewer ergonomics). (b) Separate sink (the
  df-stage-a stage-doc choice): keep `.df` byte-untouched, put contracts in their
  own golden.
- **Evidence.** Both realize the SAME Stage-A design — the enum rides in
  Hash/Equals identity with the §11 no-convert validator (RDA-C2 confirms this is
  the faithful §15.11 "different logical operators," independent of the sink
  choice). The distinguishing cost: in-`.df` mutates the `.df` ATTRIBUTES surface
  (ripples into any `.df`-parsing tooling) and couples contract determinism to
  `.df` determinism; the separate sink keeps `.df` frozen but gives `role=` no
  final-graph cross-reference (RDA-T1: `role=` is normalization-provenance with no
  external referee — the first bless is self-compare-blind under EITHER sink). A
  third, node-minting realization (Variant 1: distinct subclasses, per stale
  pseudocode §6) is REJECTED — it mints ids/kinds and breaks the byte-identity
  claim; it should be recorded as considered-and-rejected, not offered live.
- **Panel recommendation.** None on the sink axis (genuine ergonomics-vs-coupling
  taste). The panel DOES recommend: kill the node-minting alternative explicitly
  (amend pseudocode §6 to O-A1's enum), and — because `role=` is unrefereeable
  under either sink — the in-`.df` variant's one advantage is that it puts `role=`
  next to the topology a reviewer can eyeball, so if reviewer ergonomics is the
  deciding weight, that tilts in-`.df`.

### AI-2 (Stage B) — the `-region-out` grammar: G1 vs G2 vs G3

- **Decision.** Which grammar the new `-region-out` dump uses. The artifact
  renders all three fully on `join_1` and declines to pick.
- **Alternatives.**
  - **G1 — indented region/port/contract block (`.ir`-like).** Determinism: fully
    positional, block nesting fixed. Stage-C extension: request/lifecycle edges
    append as new indented lines; re-blesses only the census line. Permcheck: a
    dedicated `request-edges{…}` multiset earns order-free diffability. Precedent:
    closest to the existing `.ir` region indentation.
  - **G2 — flat BB-with-args / tail-call (`.df`-like).** Determinism: the block-arg
    signature line encodes ports. Stage-C extension: edges become EXTRA ARGS on the
    signature — structurally natural for edges, but re-blesses the signature line
    (higher re-bless cost than G1). Precedent: matches `.df`/`.rel`; unifies the
    dump family under one grammar (uniformity argument), but less obvious to a
    first-time reader.
  - **G3 — typed-record / S-expression keyed by RegionId.** Determinism: sexp child
    order fixed as G1's; census is itself a parseable `(census …)` node. Stage-C/D
    extension: new `(request-edge …)` nodes, existing text untouched; a structured
    referee can re-bless NOTHING. Permcheck: best-in-class (a future `regioncheck.py`
    parses it). Precedent: NONE — a new paren grammar reviewers must learn; least
    human-friendly, most machine-friendly.
- **Evidence.** The choice CO-DECIDES the Stage-C referee strategy (ADJ-4): G2's
  ports-as-signature-args make Stage-C edges natural but re-bless the signature
  line; G1 re-blesses only the census line; G3 re-blesses nothing under a structured
  referee. Census-as-line (G1/G2, reusing the `.rel` idiom) vs census-as-node (G3,
  directly consumable by a parsing V-REGION-CENSUS) is the sub-axis ADJ-5. All three
  share the ADJ-1 no-`TableId` identity constraint (contracts key on declared
  relation, not `%table:N`), so no grammar escapes the row-contract-set
  specification the Stage-B amendments demand. NB: whichever grammar is chosen, the
  row-contract SET/count must still be specified and refereed (T2/C3) — that is
  orthogonal to the grammar.
- **Panel recommendation.** None (the artifact's stop-condition charge is to present
  samples, and the axis is a genuine human-referee-vs-machine-referee tradeoff). The
  panel notes the decision is not independent of the Stage-C referee strategy: pick
  G3 only if a structured `regioncheck.py` is committed to; pick G1 if human bless of
  byte-goldens remains the referee; G2 buys grammar-family uniformity at a
  signature-line re-bless cost each stage.

### AI-3 (Stage C) — the census-line field-set growth for all 180 cases

- **Decision.** How to absorb the `.rel` census growing from 29 to 36 kinds
  (`29 − 3 + 10`: three zero instance-fields removed, ten lifecycle fields added).
- **Alternatives.** (a) Accept a whole-corpus `.rel` census re-bless — every
  program's body stays byte-identical, only the census line changes by the fixed
  3-out/10-in token edit, so the review is mechanical. (b) Emit only NONZERO census
  fields (variable-length census) — REJECTED by the artifact: breaks the fixed-width
  census contract, changes far more goldens, and defeats "cross-mode agreement is a
  byte-compare of the same census."
- **Evidence.** The census is a HARD byte-compare surface (permcheck relaxes only
  published-delta order, never the census), so NO program's `.rel` census line stays
  byte-identical — the re-bless is unavoidable under option (a). The panel's Stage-B
  T2 finding is adjacent: the census must actually be recomputable/refereed (Stage C
  should ensure the 10 new lifecycle kinds each have a recount in
  V-REGION/LIFECYCLE-CENSUS, and — separately — resolve `row-contracts=`'s missing
  oracle).
- **Panel recommendation (labeled panel view).** Accept option (a), the fixed-width
  whole-corpus re-bless — it preserves the fixed-width cross-mode byte-compare
  invariant the suite relies on, and the mechanical 3-out/10-in edit is the honest
  cost of the lifecycle vocabulary. Pair it with the Stage-C requirement that each
  new kind is recount-refereed (closes Stage-B T2).

### AI-4 (Stage C, rel) — tc's `.rel` under the request-edge model (H-J Variant A/B)

- **Decision.** How the bound `#query reachable_from(bound A, free)` (tc) lowers
  under the Stage-C request-edge model — this gates whether tc's `.rel` can be
  pinned at all.
- **Alternatives.** (a) **Variant A** — tc compiles as full materialization; `.stdout`
  stays byte-identical (answer-neutral), only shape/perf changes; its census is
  `kRequestEdgeAdd=1`/`Remove=1` with all `kChildResult*`/`kRoutedResult*` = 0.
  (b) **Variant B** — the admissibility gate rejects recursive demanded content
  (mapping tc onto the `demand_recursive_content_1` exclusion), flipping tc to
  expected-diagnostic with no `.rel`.
- **Evidence.** Variant A as sketched is INTERNALLY INCONSISTENT (panel C2, major):
  its census IS the "request edge without caller-qualified result maintenance" shape
  proposal §11 lists as a hard validator failure and §5.3/L946 forbids, while §6
  forces the bound query to mint the edge (`kRequestEdgeAdd=1`) — so Variant A must
  either contradict §6 or carry a degenerate self-routed result, and does neither.
  The disposition's "mint no edge like join_1" does not rescue it (join_1 mints no
  edge only because its query is FREE). Separately, full materialization defeats
  demand's pruning (the demand-cost-model perf regression, the artifact's own
  Adjudication Input B), tying the choice to the Stage-C-vs-Stage-D recursion
  boundary.
- **Panel recommendation (labeled panel view).** Do NOT pin Variant A as written —
  either give the bound self-request a well-defined (possibly degenerate self-routed)
  result so its census is not the §11-forbidden shape, or take Variant B and flip tc
  to expected-diagnostic. The panel leans toward resolving the §6-vs-§11 conflict
  first (define what a bound query's request edge routes to) since that rule is needed
  for every bound demanded query, not just tc.

### AI-5 (Stage C, rel) — the `differential=` edge-relation flag

- **Decision.** Whether the request/edge relation carries a `differential=` flag and,
  if so, what property it names.
- **Alternatives.** (a) Keep `differential=`, re-grounded on the property that varies:
  input/child-result retractability under `-demand-retract` vs teardown-only
  lease-destructor retraction. (b) Drop the flag.
- **Evidence.** The flag's current justification is falsified by the artifact itself
  (panel N2): §3.1 grounds `differential=true` on "a kRequestEdgeRemove exists," but
  §4.2 emits `kRequestEdgeRemove` for `differential=false` edges too, and §3.3 gives
  every edge relation the full six-vec differential family unconditionally. The op
  census cannot show the steady-state-vs-teardown distinction, so a flag COULD
  legitimately carry it — but only if re-grounded.
- **Panel recommendation (labeled panel view).** Drop the flag unless a concrete
  consumer needs the steady-state-vs-teardown distinction; if kept, re-ground it on
  input/child-result retractability and reconcile §3.1 with §4.2. Either way the
  current §3.1 justification must go.

### AI-6 (Stage C, header) — request-edge row schema & owner identity vs refcount (A1/N1)

- **Decision.** Does the header carry an owner-IDENTITY-bearing request-edge relation,
  or a key-keyed `DemandSupportCount` refcount? And what is the request-edge row
  schema.
- **Alternatives.** (a) Owner-bearing `RowReq{owner, key}` (2-col), threaded into
  `flow_136` (§1.4b) — disambiguates concurrent same-key leases. (b) Presence-only /
  key-keyed refcount — the `flow_136` join stays single-column (byte-identical to the
  flat arm), presence recovered by `DistinctProjection`.
- **Evidence.** The N1 "drain contract bounds concurrent leases to one" argument is
  FALSE (panel): draining a cursor does not release its lease, so two concurrent live
  same-key leases (the F4 case) ARE reachable in Stage C — owner disambiguation is a
  real Stage-C concern, not Stage-D-only. But the 2-col owner-bearing retype (a)
  collides with §1.8's carried-forward single-column flat differential fixpoint and
  needs the never-declared `DistinctProjection` presence member surfaced (panel C1).
  Resolving toward (b) presence-only DISSOLVES C1 (the join stays single-column) at
  the cost of not distinguishing concurrent same-key owners. Sub-questions: whether
  `owner` is a Stage-A newtype vs bare `uint64_t`, and whether a `call_site` column is
  needed (A1).
- **Panel recommendation (labeled panel view).** This is the pivotal header decision —
  resolve it before pinning the `flow_136`/`proc_19` signatures. If concurrent
  same-key leases must be individually retractable, (a) is required and the doc must
  declare BOTH the owner-bearing request-edge relation and the presence relation; if a
  refcount suffices for correctness, (b) is simpler and dissolves C1. The panel cannot
  pick without the concurrency requirement, which is an owner call.

### AI-7 (Stage C, header) — related open header adjudications (A2–A5)

- **A2 — pending-lease-removal drain storage.** Reuse the request-edge `kDeleteQueue`
  vs a dedicated private pending-lease member. Evidence: coupled to A1's schema choice.
  No panel pick.
- **A3 — explicit vs implicit cursor special members.** Emit `= delete` copy /
  `= default` move + explicit `~cursor()` for documentation, vs rely on the move-only
  lease member's implicit rules. Byte-stable either way; the golden must pick one. No
  panel pick (documentation taste).
- **A4 — one shared request-edge relation with a call_site discriminator vs N
  per-adornment relations** for a multi-adornment query name. Changes private-member
  count and row schema; couples to A1. No panel pick.
- **A5 — all-bound existence-check lease (`!has_free`).** The query returns `bool`,
  has no cursor to own the lease, so it needs a function-scoped lease with a
  destruct-at-return lifetime. NO witness pins it. Panel recommendation: the exit gate
  should author/identify a witness for this case before Stage C — it is the one
  public-surface-adjacent lifecycle the corpus does not currently exercise (and the
  tagged binary cannot adjudicate it). Note the PUBLIC ARITY is unchanged in every
  case: the lease lives in the cursor (or a function scope for `!has_free`), not the
  signature.

### AI-8 (Stage B) — carried-forward open questions (artifact's own ADJ-2/ADJ-3)

- **ADJ-2 — `demand__` fabricated-message placement.** Program-root `input-abi` line
  (honest to the graph; Stage C deletes it) vs region-internal injected input (hides
  the scaffolding the cutover removes). Evidence: pinning the input-abi line
  guarantees a re-bless one stage on (panel N1). Panel recommendation: lean
  region-internal so the Stage-B skeleton does not pin scaffolding the immediate next
  stage removes — but this is a preference, and the input-abi choice is defensible as
  the "what evolves at Stage C" signal.
- **ADJ-3 — all-free request port `fields=()`.** Render all-free queries (join_1's
  q/never) with an empty-field request port vs as output/permanent roots with no
  request port. Evidence: a `fields=()` port carries no request semantics and would
  inflate the Stage-C lifecycle-census request-ports count (panel N2). Panel
  recommendation: lean toward output/permanent roots (no request port) so the
  request-port count means "has demand" — but this is a preference on an open axis.
