# Owner-adjudication brief — RegionalDataFlowCore epoch

Synthesis close of the design-grounding session, 2026-08-02, tip f0c913e0
(branch `keyed-instances`). This is a DECISION QUEUE: every open question the
six-phase pass surfaced, deduplicated across the pseudocode, the five stage
diffs, the two Phase-3 reports, the four Phase-4 desired-state docs + their
critique, the necessity audit, and the empirically-verified test matrix. Each
item: ID · the decision · the alternatives · the evidence (artifact + section) ·
the panel's recommendation where one exists (labelled as the panel's, never a
decision) · what it blocks · effort/urgency.

Many decisions arrive under several labels; the consolidation map is stated at
each item (e.g. the review's Concern 2 = stage-c H-J = stage-d §0.1-vs-E3
T-oracle-4 = phase4 AI-4 = test-matrix H-J re-adjudication set). Nothing here is
decided; nothing was implemented; no golden was touched.

Legend for URGENCY: **T1** blocks the whole stage sequence; **T2** blocks one
stage's exit gate; **T3** is standalone pre-Stage-A work. Effort: **S** ≤ ~½ day;
**M** = multi-site rewrite; **L** = a data-model change.

---

## TIER 1 — blocks the stage sequence itself

### D1.1 — Ratify the sequence + interpreter-before-cutover + tagged-binary oracle
- **Decision.** Ratify A → I0 → B → C → D as the landing order, with the
  reference relational interpreter (I0) built and corpus-validated against the
  CURRENT compiler BEFORE Stage C's cutover, and the pre-deletion compiler kept
  as a TAGGED BINARY external oracle (final membership + sorted published deltas)
  through the cutover.
- **Alternatives.** (a) Ratify as proposed. (b) Fold I0 into Stage C (build the
  referee during the cutover it refereess) — the shape the review rejects. (c)
  Re-rank (e.g. Stage A last).
- **Evidence.** `fable-review-2026-08-01.md` Concern 1 + Ranking (Stage A is
  independently valuable and lowest-risk; the eqgate tests are the oracle class
  that caught HP-5 over-materialization; "no old/new selector" bans a runtime
  fork, not a test harness). `stage-i0-interpreter.md` OG1 (parsed-clause vs
  Oracle-core engine), OG2 (tagged-binary emitter tool). `regional-arch-
  pseudocode.md` §6 ranking + §7 open decision 1.
- **Panel recommendation (panel view).** Ratify (a). The interpreter as a Stage
  A/B deliverable and the tagged binary as external oracle are both zero-
  architecture-cost and close the single biggest sequencing risk. OG1/OG2 (which
  interpreter engine, which emitter) are I0-internal and can be settled at I0
  authoring time, not now.
- **Blocks.** Everything downstream — this is the frame every other item sits in.
- **Urgency/effort. T1 / decision-only.**

### D1.2 — Stage A cyclic-graph contract soundness (T-conf-1 + T-conf-2)
- **Decision.** How `InferConservativeRowContracts` handles recursive programs,
  whose contract-dependency graph is genuinely cyclic (recursion realized through
  kMember proxy TUPLEs after `ConnectInsertsToSelects`/`ProxySelects`, so there is
  no contract-leaf inside the cycle and `Depth()` is a cycle-CUT, not a
  topological order).
- **Alternatives.** (i) Make the pass a real fixpoint over the SCC condensation
  with a monotone lattice — contracts only grow toward `AllFields(columns)`,
  bounded by `|columns|`, with an explicit termination measure. (ii) Give every
  view on a cycle (from `IdentifyInductions`' SCC membership, NOT visitation
  order) a conservative `member_key = AllFields(columns)`, a pure function of SCC
  structure.
- **Evidence.** `phase3-critique-report.md` Stage A, T-conf-1 (CONFIRMED
  blocking) + T-conf-2 (CONFIRMED blocking): without the fix, `V-MEMBERKEY-
  REALIZED` fires on a recursive kMember TUPLE that inherited an empty back-edge
  key, HARD-REJECTING `demand_tc_witness`, `d5_recursive_negate`,
  `fixpoint_stress_1`, `reconverge_1` and breaking the "zero of 180" exit gate.
  The two share one fix. A-corr-3 (major, CONFIRMED) is adjacent: `DeterminedBy`/
  JOIN-`Minimize` are wired to a non-existent `impl->equivalence_sets`; they must
  read column-ID identity + persisted JOIN/CMP structure instead (escalate
  whether a persisted column-equality relation exists at the H-A4 slot).
- **Panel recommendation (panel view).** Take (i) or (ii) — the panel does not
  pick, but notes (ii) is strictly cheaper and makes the recursive-view contract a
  PURE GRAPH FUNCTION (which also kills the H-A8 `.contract.opt.golden` run-order
  artifact T-conf-2 flags). (i) is more precise but needs the termination measure
  written into the hunk. Either way: add escalation E-A4 (cyclic-graph contract
  soundness) and gate the 180-byte-identical claim on ACTUALLY running the suite
  with the two validators live over the recursive subset before blessing any
  `.contract` golden.
- **Blocks.** Stage A exit gate (BLOCKED-ON until resolved). Also fix A-corr-3's
  DeterminedBy source before DeterminedBy is implementable.
- **Urgency/effort. T1 / M** (a pass-shape change + escalate A-corr-3's backing).

### D1.3 — I0 bound-query probe-enumeration contract (A-corr-1 I0)
- **Decision.** Does I0 emit FULL-RELATION answers for a bound `#query`, or
  PROBE-RESTRICTED answers matching exactly the keys the demand binary can serve?
- **Alternatives.** (a) Full-relation — needs a non-demand enumeration path the
  demand ABI lacks. (b) Probe-restricted via a per-case `.probes` sidecar. (c)
  Probe-restricted by threading the bespoke driver's hand-picked probe list (e.g.
  `demand_tc_witness.main.cpp`'s `{1,3,7,5}`) into BOTH the emitted harness and a
  matching restriction of I0's QUERY block.
- **Evidence.** `phase3-critique-report.md` I0, A-corr-1 (CONFIRMED blocking):
  for the 5 pure demand witnesses the QUERY block is the ONLY non-empty CBF
  content (`demand_tc_witness` publishes nothing — empty FINAL/epoch blocks), and
  the demand ABI exposes only per-key `_bf` cursors with no probe-enumeration
  contract, so I0's full definitional answer and the binary's probe answers CANNOT
  be equal by construction. Hits all 19 bound-query `.batches` cases.
- **Panel recommendation (panel view).** Resolve toward (b)/(c) probe-restricted:
  a `.probes` sidecar (or threaded driver list) is the least-invasive fix and
  matches how the corpus drivers already work. Escalate the open decision this
  forces (full-relation needs an ABI the demand path lacks). Until resolved, the
  flagship demand gate compares empty content.
- **Blocks.** I0 exit gate (BLOCKED-ON). A-test-3 rides on it: the four reject→
  full-materialize flip cases (see D1.6) have no `.batches` and no pre-cutover
  binary, so post-flip I0 is their SOLE oracle — author demand-blind fixpoint
  goldens once D1.6 is decided.
- **Urgency/effort. T1 / M.**

### D1.4 — force.dr / @first query-time forcing fate (Stage C corr-1)
- **Decision.** `force.dr` (shipped as both `data/examples/` and
  `tests/OptDiff/cases/`, the SOLE `@first`-body corpus file) proves query-TIME
  message injection. §10/§14 delete query-body `@first` forcing with no regional
  replacement, so its round-1 output materially changes. Does the regional model
  offer ANY query-time-forcing replacement, or is `force.dr`'s generative
  behavior intentionally dropped per §10?
- **Alternatives.** (a) Drop the behavior: re-bless `force.stdout` to the reduced
  output (round 1 collapses to `{5,6}`) or delete the case, and remove/rewrite the
  source (it will no longer parse once `@first` body-forcing is removed at Stage-C
  step 7). (b) Define a regional query-time-forcing replacement (a request edge
  cannot inject a message — §7.2 pins the effect in ProgramRoot — so this would be
  a new mechanism).
- **Evidence.** `phase3-critique-report.md` Stage C, corr-1 (CONFIRMED
  blocking): EG.1 asserts every case's `.stdout` stays byte-identical and names
  only the six demand witnesses as rewritten — a concrete byte-identity OVER-CLAIM
  on a shipped example whose source uses deleted syntax. `stage-c-diff.md` §13
  step 7 (removes `@first` body-forcing), §7.2 (effect placement in ProgramRoot).
- **Panel recommendation (panel view).** No pick on (a) vs (b) — it is a genuine
  capability question for the owner. Regardless of the answer, EG.1's "every
  case's `.stdout` stays byte-identical" is FALSE as written and must be corrected;
  enumerate `force.dr` alongside the six demand witnesses as a changed case, in a
  deleted-behavior bucket.
- **Blocks.** Stage C exit gate (BLOCKED-ON).
- **Urgency/effort. T1 / S** (correct the claim) **+ decision** (the capability).

### D1.5 / D1.6 — Inadmissible-extraction semantics (Concern 2) — THE MASTER DECISION
This single decision surfaces under five labels — resolving it once discharges
all of them:
- review Concern 2 (fable-review §Concern 2)
- Stage C H-J Variant A / Variant B (`stage-c-diff.md` :485–:518)
- Stage D §0.1-vs-E3 reject-vs-silent contradiction (**T-oracle-4 (D)**,
  `phase3-critique-report.md` Stage D — CONFIRMED **blocking**)
- phase4 AI-4 (tc's `.rel` under the request-edge model)
- test-matrix H-J re-adjudication set (C1/C5/C7/C10/C11/C12) + PF-4 barrier fence

- **Decision.** When extraction is inadmissible, what happens — and what do
  today's clean diagnostics become?
- **Alternatives.**
  - **Variant A — uniform full materialization.** Inadmissible extraction ⇒ the
    queried relation is fully materialized in the observation root, the lease just
    scopes a cursor; NEVER a reject. Answer-correct; `.stdout` byte-identical
    (answer-neutral). Owes a NEW §12.3 rejection witness (Variant A removes the
    reject that §12.3 row 13 requires).
  - **Variant B — retain the recursion feature-gap rejects (hybrid).** Recursive
    demanded content stays an expected-diagnostic reject; non-recursive
    inadmissible slices full-materialize. Smaller behavior change; satisfies §12.3
    via `demand_cyclic_1`'s recursive dataset.
- **Evidence.** `phase3-coverage-audit.md` Table 5 (escalated-to-owner, blocking).
  `phase4-critique-report.md` AI-4: Variant A as sketched is INTERNALLY
  INCONSISTENT — its census (`kRequestEdgeAdd=1`/`Remove=1`, all `kChildResult*`/
  `kRoutedResult*`=0) IS the "request edge without caller-qualified result
  maintenance" shape §11 lists as a hard validator FAILURE and §5.3/L946 forbids,
  while §6 forces tc's bound query to MINT that edge; so Variant A must either
  contradict §6 or carry a degenerate self-routed result and does neither. The
  §6-vs-§11 conflict (what a bound query's request edge routes to) must be defined
  for EVERY bound demanded query, not just tc. `phase3-critique-report.md` Stage D
  T-oracle-4: §0.1 promotes `cyclic_demand` to a permanent planning-time abort
  bound to `V-NEST-DEPTH-FINITE`, while E3 says a recursive region call stays
  inadmissible → silent full-materialization FOREVER; a recursive region call
  cannot be both, and the §12.3 rejection witness has no defined referee until
  this is settled. `test-matrix-proposal.md` §2.2/§2.3: the H-J re-adjudication
  set (C1 agg-body, C5 KV-body, C7 @product, C11 config-agg, C12 mutual-content)
  each flips reject→compile under Variant A or stays reject under Variant B; PF-4
  records `@barrier`/`:-` (C10) as a demand fence TODAY — decide KEEP vs lift-
  candidate under the regional model (see D3.5). Today's four diagnostics whose
  fate is owed: `demand_cyclic_1`, `demand_recursive_content_1`,
  `demand_multi_adorn_allfree_1` (review Concern 2), plus the C1/C5/C7/C11/C12
  H-J-adjacent cases the test matrix surfaced.
- **Panel recommendation (panel view).** Resolve the §6-vs-§11 conflict FIRST
  (define what a bound query's request edge routes to — a possibly-degenerate
  self-routed result), because that rule is needed for every bound demanded query.
  On the variant axis the panel leans Variant B (E2's recommendation): it is the
  smaller behavior change, satisfies §12.3 without a new witness, and gives the
  rejection witness a source-fireable home (`demand_cyclic_1`'s recursive dataset).
  If Variant A is chosen, the new reject must be shown to actually trigger one-
  level (not merely named), and each flipped case owes a full-materialization
  golden. Silent full-materialization is a BEHAVIOR CHANGE from today's rejects
  (arguably an improvement) — it must be a STATED decision either way. Then make
  §0.1 and E3 consistent (Stage D): if reject, fix E3 and prove `V-SCC-INSTANCE-
  CLOSED` / the `cyclic_demand` abort is source-fireable; if silent full-mat, fix
  §0.1's abort-validator language (the belt fires only on a planner BUG).
- **Blocks.** Stage C H-J pin, Stage D exit gate (BLOCKED-ON T-oracle-4), AI-4
  tc `.rel` pin (BLOCKED per rel-stage-c desired-states), the H-J re-adjudication
  cases' authored goldens, and the fate of the four diagnostics.
- **Urgency/effort. T1 / M** (spans two stage docs + the §6/§11 rule).

---

## TIER 2 — blocks individual stage exit gates

### D2.1 — Stage-C H-I validator under-delivery cluster (one amendment decision)
- **Decision.** Close the five §11 ORPHAN validators + two UNHOMED §12.3 witnesses
  that Stage B deferred to Stage C but Stage C H-I under-delivers (X1/X2).
- **The cluster.** §11 line 5 (parent/child frozen-port disagreement — add to
  H-I; a `FrozenChildCall` mapping must match the child's frozen port schema);
  line 9 (`V-PURE-REGION`, effectful operator inside a region — add the named
  validator, only the H-E admissibility purity clause survives today; undermines
  §15.13); line 3 (symbolic-parameter-escape) and line 6 (sealed-ABI mutation) —
  either add validators or STATE that the §11 "compilation fails" semantics is
  intentionally softened to "declines extraction into full materialization";
  line 10 (sequestered-key-present-in-InstancePath — Stage D adds alias/not-
  derived checks but not the presence check; undermines §15.12). UNHOMED §12.3
  row 11 (Permanent root) — closed by **corr-3** below; row 12 (Effects) — author
  a directed effects witness (tied to the orphaned `V-PURE-REGION`).
- **Included required amendment — corr-3 (major, CONFIRMED).** `PermanentRoot` is
  a first-class `RequestOwnerId` (§5.2) Stage C owns, needing an add-only request
  edge born at init, but H-G.2's edge-birth mapping sources only `RootLease`/
  `RegionalMember` and an add-only edge collides with `V-EDGE-BALANCE`. Add
  `PermanentRoot` as a `kRequestEdgeAdd` source, carve add-only permanent edges out
  of `V-EDGE-BALANCE` (or define a teardown removal), and author the §12.3
  permanent-root witness. Without it an extracted permanent-output child (legal
  under always-true `ExtractionPolicy`) publishes nothing or the census aborts.
- **Evidence.** `phase3-coverage-audit.md` Table 1 (ORPHANs 3/5/6/9/10), Table 3
  (UNHOMED 11/12), X1/X2; `phase3-critique-report.md` Stage C corr-3 + coverage-
  audit results bearing on Stage C.
- **Panel recommendation (panel view).** Land line 5 and line 9 as named
  validators at Stage C H-I (they were promised); adopt corr-3's permanent-root
  edge + witness; author the effects witness (§12.3 row 12); for lines 3/6 pick
  ONE contract — either named validators or the explicit "declines extraction, not
  compile failure" softening — and state it, do not leave §11's "compilation
  fails" prose contradicting the admissibility-gate behavior. Line 10's presence
  check re-homes to Stage D.
- **Blocks.** Stage C exit gate (validator-completeness, §15.16 caveat).
- **Urgency/effort. T2 / M.**

### D2.2 — AI-2: the `-region-out` grammar (G1 vs G2 vs G3)
- **Decision.** Which grammar the new Stage-B `-region-out` dump uses. Samples for
  all three are rendered fully on `join_1` in `regional-dump-stage-b-desired-
  states.md`.
- **Alternatives.** **G1** indented region/port/contract block (`.ir`-like):
  fully positional, Stage-C edges append as indented lines, re-blesses only the
  census line, a `request-edges{…}` multiset earns permcheck order-free
  diffability. **G2** flat BB-with-args / tail-call (`.df`-like): ports in the
  block-arg signature, edges become extra signature args (natural for edges) but
  re-bless the signature line each stage; unifies the dump family. **G3** typed-
  record / S-expression keyed by RegionId: census is itself a `(census …)` node, a
  structured referee re-blesses NOTHING, best machine-referee — but a new paren
  grammar reviewers must learn.
- **Evidence.** `phase4-critique-report.md` AI-2. The choice CO-DECIDES the
  Stage-C referee strategy (ADJ-4) and the census-as-line-vs-node sub-axis
  (ADJ-5). All three share the ADJ-1 no-`TableId` identity constraint. Orthogonal:
  whichever grammar, the row-contract SET/count must still be specified and
  refereed (T2/C3, see D2's Stage-B amendment note).
- **Panel recommendation (panel view).** None (a genuine human-vs-machine-referee
  taste). The panel notes: pick G3 only if a structured `regioncheck.py` is
  committed to; G1 if human bless of byte-goldens stays the referee; G2 buys
  grammar-family uniformity at a signature-line re-bless cost each stage.
- **Blocks.** Stage B exit gate (the regional dump pin).
- **Urgency/effort. T2 / decision-only** (samples exist).

### D2.3 — AI-1: in-`.df` role rendering vs a separate `contract`-out sink
- **Decision.** Where `ProjectionRole` (member/distinct) surfaces for review:
  inline in the existing `.df` dump, or only in a separate `*.contract.opt.golden`.
- **Alternatives.** (a) In-`.df`: `role=`/`support=`/`candidates=` on the `.df`
  block (one-surface reviewer ergonomics; mutates the `.df` attributes surface and
  couples contract determinism to `.df` determinism). (b) Separate sink (the
  stage-doc choice): `.df` byte-untouched, contracts in their own golden; but
  `role=` gets no final-graph cross-reference.
- **Evidence.** `phase4-critique-report.md` AI-1 + RDA-T1 (`role=` is
  normalization-provenance, unrefereeable under EITHER sink — the first bless is
  self-compare-blind). A third node-minting realization (Variant 1) is REJECTED
  (mints ids/kinds, breaks byte-identity).
- **Panel recommendation (panel view).** None on the sink axis. The panel DOES
  recommend: kill the node-minting alternative explicitly (amend pseudocode §6 to
  O-A1's enum — see erratum §Errata-3), and note that because `role=` is
  unrefereeable either way, in-`.df` at least puts it next to the topology a
  reviewer can eyeball — if reviewer ergonomics is the deciding weight, that tilts
  in-`.df`.
- **Blocks.** Stage A dump surface pin.
- **Urgency/effort. T2 / S.**

### D2.4 — AI-3: `.rel` census-line growth (29 → 36 kinds) whole-corpus re-bless
- **Decision.** How to absorb the census growing by `29 − 3 + 10` (three zero
  instance-fields removed, ten lifecycle fields added).
- **Alternatives.** (a) Accept a whole-corpus `.rel` census re-bless — every
  body stays byte-identical, only the census line changes by the fixed 3-out/10-in
  token edit (mechanical review). (b) Emit only NONZERO census fields — REJECTED
  by the artifact (breaks the fixed-width census contract).
- **Evidence.** `phase4-critique-report.md` AI-3. The census is a HARD byte-
  compare surface (permcheck never relaxes it), so NO program's census stays
  byte-identical under (a) — the re-bless is unavoidable.
- **Panel recommendation (panel view).** Accept (a), the fixed-width whole-corpus
  re-bless — it preserves the fixed-width cross-mode byte-compare invariant. Pair
  it with the Stage-C requirement that each of the 10 new lifecycle kinds has a
  recount in `V-REGION`/`V-LIFECYCLE-CENSUS` (closes Stage-B T2), and separately
  resolve `row-contracts=`'s missing oracle (Stage-B amendment).
- **Blocks.** Stage C exit gate (census bless).
- **Urgency/effort. T2 / S** (mechanical, but 180 files).

### D2.5 — AI-5: the `differential=` edge-relation flag
- **Decision.** Whether the request/edge relation carries a `differential=` flag
  and what property it names.
- **Alternatives.** (a) Keep it, re-grounded on the property that actually varies
  (input/child-result retractability under `-demand-retract` vs teardown-only
  lease-destructor retraction). (b) Drop the flag.
- **Evidence.** `phase4-critique-report.md` AI-5 / rel-stage-c N2: the current
  justification is falsified by the artifact itself — §3.1 grounds
  `differential=true` on "a kRequestEdgeRemove exists," but §4.2 emits
  `kRequestEdgeRemove` for `differential=false` edges too and §3.3 gives every
  edge relation the full six-vec family unconditionally.
- **Panel recommendation (panel view).** Drop the flag unless a concrete consumer
  needs the steady-state-vs-teardown distinction; if kept, re-ground on
  retractability and reconcile §3.1 with §4.2. Either way the current §3.1
  justification must go.
- **Blocks.** The Stage-C `.rel` effect-set pin.
- **Urgency/effort. T2 / S.**

### D2.6 — AI-6: request-edge row schema & owner-identity vs refcount (A1/N1) — PIVOTAL header decision
- **Decision.** Does the header carry an owner-IDENTITY-bearing request-edge
  relation, or a key-keyed `DemandSupportCount` refcount? And what is the row
  schema?
- **Alternatives.** (a) Owner-bearing `RowReq{owner, key}` (2-col) threaded into
  `flow_136` — disambiguates concurrent same-key leases; needs the never-declared
  `DistinctProjection` presence member surfaced. (b) Presence-only / key-keyed
  refcount — the `flow_136` join stays single-column (byte-identical to the flat
  arm), presence via `DistinctProjection`; does NOT distinguish concurrent same-
  key owners.
- **Evidence.** `phase4-critique-report.md` AI-6 / header-stage-c C1+N1: the N1
  "drain contract bounds concurrent leases to one" argument is FALSE (draining a
  cursor does not release its lease), so two concurrent live same-key leases (the
  F4 case) ARE reachable in Stage C — owner disambiguation is a real Stage-C
  concern, not Stage-D-only. Resolving toward (b) DISSOLVES C1 (the 2-col retype
  vs single-col carried-forward fixpoint collision). Sub-questions: is `owner` a
  Stage-A newtype vs bare `uint64_t`, is a `call_site` column needed.
- **Panel recommendation (panel view).** Resolve this before pinning the
  `flow_136`/`proc_19` signatures. The panel cannot pick without the concurrency
  requirement (an owner call): if concurrent same-key leases must be individually
  retractable, (a) is required and BOTH relations must be declared; if a refcount
  suffices for correctness, (b) is simpler and dissolves C1.
- **Blocks.** Stage C header proc-signature pins; couples to D2.7 A2/A4.
- **Urgency/effort. T2 / M-L** (data-model touch).

### D2.7 — AI-7: related open header adjudications A2–A5
- **A2** pending-lease-removal drain storage: reuse the request-edge `kDeleteQueue`
  vs a dedicated private pending-lease member (coupled to A1/D2.6). **A3** explicit
  vs implicit cursor special members (`= delete`/`= default`/explicit `~cursor()`
  vs implicit) — byte-stable either way, the golden must pick one. **A4** one
  shared request-edge relation with a `call_site` discriminator vs N per-adornment
  relations for a multi-adornment name (couples to A1). **A5** all-bound
  existence-check lease (`!has_free`): the query returns `bool`, has no cursor, so
  it needs a function-scoped lease with destruct-at-return lifetime — NO witness
  pins it.
- **Panel recommendation (panel view).** No pick on A2/A3/A4 (documentation/schema
  taste, most coupled to D2.6). On **A5**: the exit gate should author/identify a
  witness before Stage C — it is the one public-surface-adjacent lifecycle the
  corpus does not exercise and the tagged binary cannot adjudicate. Public arity is
  UNCHANGED in every case (the lease lives in the cursor or a function scope).
- **Blocks.** Stage C header pins.
- **Urgency/effort. T2 / S each, decision-coupled to D2.6.**

### D2.8 — AI-8: Stage-B carried-forward open questions (ADJ-2 / ADJ-3)
- **ADJ-2** `demand__` fabricated-message placement: program-root `input-abi` line
  (honest to the graph; Stage C deletes it) vs region-internal injected input
  (hides scaffolding the cutover removes). **ADJ-3** all-free request port
  `fields=()`: render all-free queries (join_1's q/never) with an empty-field
  request port vs as output/permanent roots with no request port.
- **Panel recommendation (panel view).** Lean region-internal for ADJ-2 (do not
  pin scaffolding the immediate next stage removes) and output/permanent-roots for
  ADJ-3 (so a request-port count means "has demand") — both preferences on open
  axes, not picks.
- **Blocks.** Stage B dump pin (couples to D2.2 grammar).
- **Urgency/effort. T2 / decision-only.**

### D2.9 — O-A1 / O-A2 / O-A3 (Stage A realization decisions)
- **O-A1** Member/Distinct realization: enum-in-Hash/Equals identity + §11 no-
  convert validator (recommended, `.df` untouched — Variant 2) vs distinct
  subclasses/node-minting (Variant 1, REJECTED — breaks byte-identity). **O-A2**
  contract-map key: `QueryViewImpl *` vs `LogicalNodeId`. **O-A3**
  `DemandSupportCount` now-vs-Stage-C (defer the populated support to Stage C per
  A-nec-1 / candidate 1 — see D3.4).
- **Evidence.** `stage-a-diff.md` O-A1/O-A2/O-A3; `phase4-critique-report.md`
  RDA-C2/C3/AI-1 confirm the enum IS the faithful §15.11 realization.
- **Panel recommendation (panel view).** Ratify O-A1 = enum-in-identity (Variant
  2); record Variant 1 as considered-and-rejected and amend pseudocode §6 to match
  (Errata-3). O-A2/O-A3 are cheap and fold with D3.4.
- **Blocks.** Stage A pins.
- **Urgency/effort. T2 / S.**

### D2.10 — E-A2: negative-witness constructibility for member-collapse
- **Decision.** Is a `kMember`-collapse reject shape constructible from surface
  Datalog? If not, `member_collapse_1` is mode-split at best (T-oracle-3:
  kMember facade TUPLEs are minted only inside `::Canonicalize`, skipped under
  `-disable-dataflow-opt`, so V-NO-COLLAPSE has nothing to fire on in nodf/none —
  NOT "all-4-modes-diagnostic").
- **Evidence.** `stage-a-diff.md` E-A2; `phase3-critique-report.md` Stage A
  T-oracle-3 + T-oracle-1 (add a directed positive witness exercising the role-in-
  `Equals` refusal branch, else the CSE-cannot-fold-kMember property is asserted-
  unreachable, not tested).
- **Panel recommendation (panel view).** Either exhibit one surface-constructible
  reject (possibly a build-time non-canon aggregate-input shape for `V-AGG-INPUT-
  KEY` that could be 4-mode) or state plainly the lint's user-facing half is
  deleted with only belt-level replacement, and reconcile with §13.5. Correct the
  exit-gate row: `member_collapse_1` is MODE-SPLIT, like `kvindex_1`.
- **Blocks.** Stage A witness pins.
- **Urgency/effort. T2 / S.**

### D2.11 — Ratify `demand_diff_pub_1` (R-DIFF oracle) as a pre-Stage-A landing
- **Decision.** Author the R-DIFF differential-published-answer demand witness now,
  as an ordinary corpus case, before Stage A.
- **Evidence.** The R-DIFF pub arm (`DRInstance::differential`) is FALSE program-
  wide in today's corpus (`regional-arch-pseudocode.md` §3 — the `if (diff)` arm
  is code-complete but UNEXERCISED), so the tagged binary will have NO pre-cutover
  differential-pub demand-instance witness to compare against unless one is
  authored. `stage-i0-interpreter.md` H8/OG3 (the R-DIFF witness); `stage-c-diff.md`
  ESCALATION E1 (the Stage-C exit gate DEPENDS on this case); `test-matrix-
  proposal.md` Tier-1 #1 (C8, the top-ranked case). NOTE the name drift (Errata-1):
  `demand_diff_pub_1` in the test matrix vs `demand_diff_pub_witness` in I0/
  coverage-audit — pick one.
- **Panel recommendation (panel view).** Ratify: author `.batches` + `.oracle.
  stdout` + `.eqgate` now (epoch 1 add+probe, epoch 2 remove an in-neighborhood
  edge while demand stands → the answer retracts through the `@differential
  nbhd_out` tap). If the pub is always provisioned monotone (arm dead), the
  discovery that the `if (diff)` arm is unreachable is itself the finding.
- **Blocks.** The tagged-binary oracle's COMPLETENESS for Stage C (review open
  decision 1, sharpened).
- **Urgency/effort. T2 / S** (one corpus case, compiles today).

### D2.12 — Stage-D stratification realization: V-PI vs V-CW
- **Decision.** How per-instance local recursion is realized: per-instance vector
  sets (V-PI) or instance-column-widened vectors (V-CW).
- **Alternatives.** **V-PI** allocates a fresh `view_to_swap_vec` per instance;
  `LowerDRRounds` parameterized by InstanceId (each instance's fixpoint a separate
  invocation). **V-CW** keeps ONE `view_to_swap_vec`, widens the SCC's tables/vecs/
  indices/claim-gates by the instance key (the widened fixpoint = the DISJOINT
  UNION of per-instance fixpoints).
- **Evidence.** `stage-d-diff.md` :228–:258, Part 4; `phase3-critique-report.md`
  Stage D A-corr-4 (V-CW satisfies §15 invariant 4 only via the disjoint-union
  lemma, discharged by clause (b)/`V-INSTANCEKEY-NOT-DERIVED` + the nonlinear-
  recursion-under-two-keys witness — not only termination), A-term-2.
- **Panel recommendation (panel view).** V-CW is the stage-doc's recommended FIRST
  realization (smaller-architecture, aligned with the necessity-audit goal); keep
  the choice owner-gated (do not silently promote V-PI to default). Restate
  `V-INSTANCEKEY-NOT-DERIVED` as a PROVENANCE check ("verbatim carry of the bound
  key, never a derived value") that runs on the FrozenRegionalProgram before V-CW's
  Rel key-widening.
- **Blocks.** Stage D exit gate (couples to D1.6's T-oracle-4).
- **Urgency/effort. T2 / decision + M realization.**

---

## TIER 3 — standalone pre-Stage-A work to ratify

### D3.1 — The `df.dfe` hygiene/optimization split + validator promotion (NEW finding)
- **Decision.** Split `EliminateDeadFlows` (df.dfe) into a REQUIRED reachability-
  hygiene half and a gated taint-based dead-flow OPTIMIZATION half, then promote the
  Stratify debug assert to an always-on validator.
- **The finding (established with the owner; not previously in any artifact; folded
  in verbatim-faithfully).** Phase-5's covering-array probes found `-opt-disable=
  df.dfe` (canon/cse ON) SIGABRTs at `Stratify.cpp:420` (a debug-only assert inside
  `#ifndef NDEBUG`) on 4 cases: `deadflowelimination_1/2/4` and `recursion`. Root
  cause (verified against code): the dead cycles are USER-AUTHORED legal programs
  (e.g. `recursion.dr`'s `direct_only(A) : direct_only(A).`) denoting empty
  relations — the IR is well-formed and semantically meaningful, NOT corrupt. The
  violated thing is an UNSTATED, UNOWNED pipeline postcondition: Stratify's debug
  invariant ("every multi-view SCC has an inductive MERGE or io seam") is jointly
  maintained by canonicalization (which DESTROYS the merge/seam structures — folding
  1-arm MERGEs, collapsing io seams, leaving a pure TUPLE self-cycle) and DFE (which
  DELETES the resulting dead cycles — its doc comment names underivable recursive
  cycles as its reason for existing, and `IsTrivialCycle` handles exactly this
  residue). canon-ON + dfe-OFF = demolition without the janitor. `nodf`/`none` pass
  because both are off. The codebase ALREADY draws the required-hygiene-vs-optional-
  optimization line (`Optimize.cpp:884`: "RemoveUnusedViews is REQUIRED graph
  hygiene and never consults the policy (P1 pinned contract §2b)") — DFE's
  underivable-cycle collection is MIS-BINNED on the optional side of that existing
  line. This is the F1 disease class (an invariant maintained by coordination
  between passes, owned by none, enforced by a debug assert in a third) recurring at
  the OPTIMIZATION layer — the same smell RegionalDataFlowCore diagnoses at the
  demand layer.
- **Sharpened fix (owner to ratify; effort S; pre-covering-array, NOT blocking
  Stages A/I0).** Split DFE's reachability hygiene (underivable-cycle + trivial-
  cycle collection) onto the REQUIRED side next to `RemoveUnusedViews`; keep taint-
  based dead-flow OPTIMIZATION gated. Then PROMOTE the `Stratify.cpp:420` debug
  assert to a real always-on validator (fprintf+abort surviving NDEBUG, Rel V-*
  style), since its precondition is then unconditionally established.
- **Exit gate.** dfe-off byte-matches the mode goldens on all 181 cases; carve-out
  B disappears from the covering array; the 4 cases become directed witnesses.
- **Also to record.** A `FINDINGS.md` F-record proposal to land WITH the fix; and
  an AUDIT PROMPT — check every remaining gate against the same "whose absence
  breaks whom" question (the known sibling: `df.canon` is load-bearing for
  `kvindex_1` ACCEPTANCE, already a pinned mode-split diagnostic — covering-array
  carve-out A).
- **Panel recommendation (panel view).** Ratify the split + validator promotion as
  a standalone pre-Stage-A cleanup — it is the exact bug-class the epoch exists to
  eliminate, caught in landed code, cheap, and it turns a red covering-array cell
  into four directed witnesses.
- **Blocks.** The covering array (carve-out B) — not the stages.
- **Urgency/effort. T3 / S.**

### D3.2 — PassPolicy.h stale-prose fix (standalone doc fix)
- **Decision.** `PassPolicy.h`'s byte-identity contract prose still lists 4
  dataflow-body gates, but the array gained `df.ident_join` (commit 60608468) — the
  test matrix correctly enumerates 5 `kDataFlowBody` gates (C/N/D/K/J). Stale prose,
  behavior consistent.
- **Panel recommendation (panel view).** Land the one-line prose fix with the D3.1
  cleanup (same file neighborhood, same session).
- **Urgency/effort. T3 / S.**

### D3.3 — The pre-Stage-A landing set (9 test-matrix cases)
- **Decision.** Land the 9 ★ cases as ordinary corpus cases before Stage A, to pin
  answers the Stage-A→D flips will need a referee for BEFORE the reject lifts remove
  the current diagnostic.
- **The set** (`test-matrix-proposal.md` §3): C8 `demand_diff_pub_1` (= D2.11),
  C12 `demand_mutual_content_1` [H-J], C1 `demand_agg_body_1`, C5 `demand_kv_body_1`,
  C11 `demand_config_agg_body_1` (all five with `.batches`+`.oracle`); C14
  `demand_two_queries_1`, C4 `demand_beside_recursion_1`, C13 `demand_beside_mutual_1`
  (C4/C13 with `.batches`); plus covering-array carve-out B as a NEW expectation
  (not a new case). Tier-1 of these (C8, C12, C1, C5, C11, C9) fill declared
  blocking oracle gaps (stage-c E1, stage-d E2, the §8.1 stateful/pure-slice
  admissibility question no stage doc resolves).
- **Panel recommendation (panel view).** Land the five `.batches`/`.oracle` cases
  first (they fill blocking oracle holes); the three determinism/negative
  companions and C14 are cheap follow-ons. All compile-or-reject outcomes are
  VERIFIED-EMPIRICALLY at tip; only the future-architecture answers are
  UNVERIFIABLE-TODAY by construction.
- **Urgency/effort. T3 / M** (author drivers + `.batches` + oracle goldens).

### D3.4 — Necessity-audit deferral candidates
- **Candidate 1 (top; land at Stage A) — strip the POPULATED support field.** Remove
  `RowContract.derivation_support`, the merge-arm `DeltaSign` add/remove fold, and
  the join-arm support product; KEEP the domain types (`DeltaSign`/
  `DerivationSupportCount`/`DemandSupportCount`/`SupportAlgebra` — they earn their
  place via the H-A1 static_assert cross-domain-disjointness battery, the F4
  deliverable). No Stage-A validator reads the value; the first genuine consumer is
  Stage C's `RequestEdgeRelation`. Confidence HIGH (E-A3 endorses it), effort S.
- **Candidate 2 (land at Stage B) — defer `V-OWNERSHIP-ACYCLIC` to Stage C.** It is
  provably unfireable over a one-node, zero-edge ownership forest; born with its
  failure mode at Stage C's first `FrozenChildCall`. Keep `V-FROZEN-NO-OPEN-PORT`
  and `V-REGION-CENSUS-IDENTITY` (they belt Stage-B-new code). Confidence HIGH,
  effort S.
- **Candidate 3 (owner-gated) — reduce Stage-A RowContract to `{visible_fields,
  member_key}`; defer antichain + `Minimize` + FieldExpression classes to Stage B.**
  Contingent on the owner ratifying the flat-key data model over §4.2's antichain-
  as-identity — an OWNER CALL, not a clear defect. Confidence MEDIUM, effort M.
- **Candidates 4/5 — recorded-and-KEPT.** Defer `logical_origins`/`LocalNodeId`↔
  `LogicalNodeId` split (4) and defer the whole Planning tier (5) are both REJECTED
  by their refuters — keeping the typed-id wall and the Planning tier under the
  byte-identity gate is the defensible call.
- **Headline (necessity-audit).** NO `legacy-two-authority` and NO `unnecessary`
  (zero-consumer) mechanism survived — every candidate is a deferral/reduction that
  FOLDS INTO a stage diff; there are ZERO pre-existing standalone cleanups. The
  smaller-not-larger verdict holds: the new design is smaller than what it replaces.
- **Panel recommendation (panel view).** Land candidates 1 and 2; take candidate 3
  only after the §4.2 data-model call; do not land 4/5.
- **Urgency/effort. T3 / S–M, folded into stage diffs.**

### D3.5 — PF-3 / PF-4 recording
- **PF-3 (reject-vocabulary split).** The R-BODYWALK reject-site row is over-broad:
  demanded-body rejects are THREE distinct diagnostics, not one — the demand-SINK
  message (`!`/`@never`/`over(){}`/config-`@recompute`), the R-MAT "single derived
  relation" message (KV body), and the literal R-BODYWALK "Unsupported rule-body
  shape" (`@product`/`@barrier`/mutual-content). The reject OUTCOME is correct
  everywhere; only the named site was wrong. Sub-note: `!`↔`@never` are
  INDISTINGUISHABLE by message today (both = demand-sink). Corrected vocabulary in
  `test-matrix-proposal.md` §2.1. Decision: adopt the corrected vocabulary in the
  Stage-C reject-mapping.
- **PF-4 (`@barrier`/`:-` is a demand fence TODAY).** C10 `demand_barrier_body_1`
  REJECTs all 4 modes (R-BODYWALK) — the SIP walk does not traverse a barrier-staged
  join chain. Decision owed: is that fence a KEEP or a LIFT-candidate under the
  regional model? (`@barrier` staging is a local-graph property Stage C carries
  forward; if the fence lifts, the case becomes a regional dump showing the barrier'd
  binary-join staging preserved inside the extracted child.)
- **Panel recommendation (panel view).** Adopt PF-3's corrected vocabulary. On
  PF-4, lean lift-candidate (barrier staging is answer-neutral and local to a region,
  so there is no semantic reason a demanded body cannot contain it), but it is a
  genuine owner call coupled to the D1.6 admissibility variant.
- **Urgency/effort. T3 / decision + doc.**

---

## Fast-path — if the owner ratifies every panel recommendation

If the owner ratifies the panel view on every item above, the next session does
exactly this (no further owner input needed until Stage A lands):

1. Land the **D3.1 df.dfe split + Stratify-validator promotion** and the **D3.2
   PassPolicy.h prose fix** as a standalone pre-Stage-A cleanup (with a FINDINGS.md
   F-record); run the covering array to green (carve-out B → 4 directed witnesses).
2. Land the **D3.3 nine pre-Stage-A test-matrix cases** (five with `.batches`/
   `.oracle`, incl. **D2.11 `demand_diff_pub_1`** under one ratified name), pinning
   the reject-lift and R-DIFF answers by `bin/Oracle` before any reject moves.
3. Apply the **Phase-3 amendments to the blocked stage docs** (mandatory before
   any implementation): Stage A T-conf-1/2 SCC-or-conservative rule (i)/(ii) +
   A-corr-3 DeterminedBy re-source + candidate-1 support strip + O-A1 enum; I0
   probe-restricted answer contract (`.probes`); Stage C corr-1 force.dr re-bless +
   Variant B + corr-3 permanent-root + the H-I validator cluster + N1 double-write
   fix; Stage D §0.1/E3 reconciliation to Variant B + A-corr-3 recursion×detach
   witnesses + V-CW.
4. Fix the six session errata below (mechanical).
5. Begin the **Stage A implementation slice** against pinned expectations:
   typed identity ids + Member/Distinct enum-in-identity + `InferConservative-
   RowContracts` as an SCC-aware pass + explicit aggregate input key + the lint→
   contract-validation swap; exit gate = 180+9 byte-identical except the directed
   projection/aggregate witnesses.

If the owner DIVERGES on D1.6 (picks Variant A) or D2.6 (picks owner-bearing
schema), steps 3–5 change shape and the next session pauses for a re-brief before
Stage C authoring — Stage A/I0/B are unaffected by both.

---

## Session errata (inconsistencies found while consolidating)

1. **R-DIFF case name drift.** The R-DIFF oracle case is `demand_diff_pub_1` in
   `test-matrix-proposal.md` (§2.3 C8, §3) but `demand_diff_pub_witness` in
   `stage-i0-interpreter.md` (H8/OG3) and `phase3-coverage-audit.md` ("Consistent"
   line). Same case, two names — pick one before landing (D2.11).
2. **Residual bound-query count drift beyond X4.** The coverage audit's X4 lists
   ~38 flag-off (stage-c), ~53/~127 non-demand (stage-d), 38/165 (CLAUDE.md); the
   Phase-3 critique (Stage D A-corr-2 amendment) cites a FOURTH measured value "~41
   bound of 180," which X4's arithmetic (38+15≈53) does not reconcile. State one
   authoritative count.
3. **Pseudocode §6 Stage-A hunk is stale vs the ratified realization.**
   `regional-arch-pseudocode.md` §6 (Stage A) says normalization "emits
   MemberProjection | DistinctProjection as DISTINCT NODES + a new validator" —
   which mints ids/kinds and breaks byte-identity. The ratified design (stage-a-diff
   O-A1, df-stage-a-desired-states, RDA-C3/AI-1) is the OPPOSITE: an enum folded into
   Hash/Equals identity, `.df` untouched. Amend pseudocode §6 to the enum.
4. **df-stage-a-desired-states §4.2 census factual error.** The `demand_tc_witness`
   census line reads `views=19 … na=8`; the dump has 20 live views — correct to
   `views=20 contracts=20 na=9` (RDA-C1). A hand-written line that becomes a golden.
5. **Stage-B→Stage-C validator handoff mismatch (X1/X2/X3).** Stage B H9 asserts
   §11 line 5 (parent/child frozen-port) and line 9 (`V-PURE-REGION`) are "covered
   at Stage C," but Stage C H-I lands NEITHER (folded into D2.1). Separately, Stage C
   H-F cites `V-DEMAND-SUPPORT-DERIVED` as a Stage-A NAMED validator, but Stage A
   defines it only as a static_assert battery (X3) — reconcile the name or add the
   validator.
6. **PassPolicy.h prose vs the real gate count.** The header prose lists 4
   dataflow-body gates; the actual array has 5 (`df.ident_join` added at commit
   60608468, correctly enumerated by the test matrix). Landed-code doc bug, folded
   into D3.2.
</content>
</invoke>
