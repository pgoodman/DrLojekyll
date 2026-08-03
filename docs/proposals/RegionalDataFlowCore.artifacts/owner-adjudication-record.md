# Owner-adjudication record — RegionalDataFlowCore epoch

Adjudicated 2026-08-02, tip f0c913e0, branch `keyed-instances`. The owner walked
`owner-adjudication-brief.md` tier by tier; every decision below is RATIFIED
owner state (divergences from the panel recommendation are flagged). This record
supersedes the brief's open-question status for these items.

## Tier 1

- **D1.1 — RATIFIED (panel line).** Landing order A → I0 → B → C → D; the
  reference interpreter (I0) built and corpus-validated against the CURRENT
  compiler before Stage C's cutover; the pre-deletion compiler kept as a TAGGED
  BINARY external oracle (final membership + sorted published deltas) through
  the cutover. OG1/OG2 settle at I0 authoring time.
- **D1.2 — RATIFIED: rule (ii), conservative AllFields.** Every view on a cycle
  (from `IdentifyInductions`' SCC membership, NOT visitation order) gets
  `member_key = AllFields(columns)` — a pure graph function. Add escalation
  E-A4; run the two validators live over the recursive subset before blessing
  any `.contract` golden. Rule (i) remains a recorded future precision upgrade.
- **D1.3 — RATIFIED: (b), probe-restricted via a per-case `.probes` sidecar.**
  I0 emits probe-restricted answers for bound `#query`s. The full-relation ABI
  gap is recorded, not built.
- **D1.4 — RATIFIED: (a), drop the behavior.** Query-time `@first` forcing is
  intentionally dropped per §10. `force.dr` enters a deleted-behavior bucket:
  re-bless to the reduced output or delete the case; source removed/rewritten
  when `@first` body-forcing stops parsing (Stage-C step 7). EG.1's byte-
  identity claim corrected to enumerate force.dr alongside the six demand
  witnesses.
- **D1.5/D1.6 — RATIFIED: Variant B (hybrid).** Recursive demanded content
  stays an expected-diagnostic reject; non-recursive inadmissible slices
  full-materialize. Stage D §0.1/E3 reconcile toward reject
  (`V-SCC-INSTANCE-CLOSED` / cyclic_demand abort must be shown source-
  fireable); §12.3's rejection witness homes on `demand_cyclic_1`'s recursive
  dataset. The §6-vs-§11 rule (what a bound query's request edge routes to)
  still must be defined for every bound demanded query at Stage-C authoring.

## Tier 2

- **D2.1 — RATIFIED (panel package).** §11 line 5 (parent/child frozen-port
  agreement) + line 9 (`V-PURE-REGION`) land as named Stage-C H-I validators;
  corr-3 adopted (PermanentRoot as a `kRequestEdgeAdd` source, add-only
  permanent edges carved out of `V-EDGE-BALANCE`, §12.3 permanent-root witness
  authored); the §12.3 row-12 effects witness authored; line 10's presence
  check re-homes to Stage D. **Sub-pick: lines 3/6 = SOFTENING, stated** —
  §11's semantics for symbolic-parameter-escape and sealed-ABI mutation is
  explicitly "declines extraction into full materialization"; no named
  hard-fail validators.
- **D2.2 — RATIFIED: G1**, the indented region/port/contract block (`.ir`-like)
  for `-region-out`. Human bless of byte-goldens stays the referee; a
  `request-edges{…}` multiset may earn permcheck order-free diffability at
  Stage C.
- **D2.3 — RATIFIED: (b), separate `.contract` sink.** `.df` stays
  byte-untouched; contracts in their own `*.contract.opt.golden`, keyed by the
  same view ids as the `.df` dump for side-by-side review.
- **D2.4 — RATIFIED: (a).** The whole-corpus fixed-width `.rel` census
  re-bless (29 → 36 kinds) at Stage C, paired with `V-REGION`/
  `V-LIFECYCLE-CENSUS` recounts for each new lifecycle kind.
- **D2.5 — RATIFIED: drop the `differential=` flag.** Reintroduce re-grounded
  on retractability only if a concrete consumer appears.
- **D2.6 — DEFERRED (owner divergence from "pick now").** The deciding fact is
  recorded: the schema choice hinges on whether concurrent same-key leases
  must be INDIVIDUALLY retractable (then (a) owner-bearing `RowReq{owner,key}`
  is required) or a refcount suffices (then (b), which dissolves C1). The
  `flow_136`/`proc_19` signature pins PAUSE until the owner answers the
  concurrency requirement. Per the charter, Stage C header authoring pauses
  for a re-brief; Stage A / I0 / B are unaffected.
- **D2.7 — RATIFIED: A3 explicit + A5 witness; A2/A4 defer with D2.6.**
  Cursor special members spelled explicitly (`= delete` / `= default`) in the
  header golden; the all-bound existence-check lease witness is authored/
  identified BEFORE Stage C; A2 (drain storage) and A4 (shared-vs-per-adornment
  edge relation) ride the D2.6 deferral.
- **D2.8 — RATIFIED (both panel leans).** ADJ-2: fabricated `demand__` input
  renders region-internal. ADJ-3: all-free queries render as output/permanent
  roots with NO request port (request-port count = "has demand").
- **D2.9 — RATIFIED.** O-A1 = enum-in-Hash/Equals-identity (Variant 2;
  Variant 1 recorded rejected). O-A2 = contract map keyed by
  `QueryViewImpl *`. O-A3 folds into D3.4 candidate 1.
- **D2.10 — RATIFIED (panel line).** `member_collapse_1` corrected to
  MODE-SPLIT; during Stage A authoring attempt ONE surface-constructible
  4-mode reject for `V-AGG-INPUT-KEY`; if none exists, state the belt-only
  softening and reconcile §13.5. Add the T-oracle-1 positive role-in-Equals
  refusal witness. Assert agg_distinct_1 emits ZERO warnings post-swap.
- **D2.11 — RATIFIED, name = `demand_diff_pub_1`** (Errata-1 resolved; the
  I0/coverage-audit docs take the name fix). Author now with `.batches` +
  `.oracle.stdout` + `.eqgate`; if the `DRInstance::differential` arm proves
  unreachable, that discovery is the finding.
- **D2.12 — RATIFIED: V-CW first, owner-gated.** `V-INSTANCEKEY-NOT-DERIVED`
  restated as a provenance check on the FrozenRegionalProgram before the Rel
  key-widening; V-PI recorded as the alternative; no silent default flip.

## Tier 3

- **D3.1 + D3.2 — RATIFIED in full** (fix + validator promotion + FINDINGS.md
  F-record + the whose-absence-breaks-whom audit over every remaining gate +
  the PassPolicy.h 4→5 prose fix). This session's Branch A.
- **D3.3 — RATIFIED: all 9 cases.** Five `.batches`+`.oracle` first
  (C8/C12/C1/C5/C11), then C14/C4/C13. Answers from `bin/Oracle` or by hand
  only.
- **D3.4 — RATIFIED: candidates 1+2 land; candidate 3 RATIFIED FLAT-KEY**
  (owner call made): Stage-A RowContract reduces to `{visible_fields,
  member_key}`; antichain + `Minimize` + FieldExpression classes defer to
  Stage B. Candidates 4/5 stay rejected.
- **D3.5 — RATIFIED: PF-3 vocabulary adopted; PF-4 = LIFT-candidate.** The
  `@barrier`/`:-` demand fence is recorded as a lift-candidate under the
  regional model (actual lift lands with Stage C, consistent with Variant B);
  C10 then becomes a regional-dump case showing barrier'd staging preserved
  inside the extracted child.

## Errata disposition

1. RESOLVED — name is `demand_diff_pub_1` (D2.11).
2. RESOLVED 2026-08-02 — authoritative count measured at tip f0c913e0: 49 of
   181 HEAD-tracked cases carry ≥1 bound #query (phase3-coverage-audit.md
   §X4 resolution; re-measure, never propagate the constant).
3. RESOLVED 2026-08-02 (pseudocode §6 amended pre-adjudication).
4. RESOLVED 2026-08-02 (census line corrected pre-adjudication).
5. RESOLVED 2026-08-02 — stage-c-diff.md H-I now lands V-PORT-AGREE +
   V-PURE-REGION by name (dated amendment), the lines-3/6 softening and
   corr-3 permanent-root carve-out are stated, stage-b-diff.md H9 points at
   them, and the X3 `V-DEMAND-SUPPORT-DERIVED` miscite is reconciled to the
   Stage-A SupportAlgebra static_assert battery.
6. RESOLVED BY RATIFICATION — lands as D3.2 in Branch A.

## Consequences for this session

Fast path holds with two shape notes: (1) D2.6's deferral pauses only Stage C
header authoring; (2) candidate-3 flat-key JOINS the Stage-A amendment set —
stage-a-diff.md must be amended to the reduced RowContract (alongside
T-conf-1/2 rule (ii), A-corr-3, candidate-1, O-A1, T-oracle-1/3/4) before
implementation. Plan: Branch A (df.dfe split + validator promotion + prose fix
+ 9 cases + errata 2/5) then Branch B (Stage A under the standing method).

## Owner directive — 2026-08-03 (mid-session): DOT digraph twins for the new surfaces

The new observability surfaces gain GraphViz DOT twins, as extensions of the
existing `-dot-out` family: (1) Stage A NOW — the dataflow DOT rendering
carries the new identity annotations (`role=`, the flat `key=(…)`) on nodes,
and organizes the graph with `subgraph cluster_stratum_<id>` per multi-view
stratum (the SCC condensation is the pre-regional organizing structure);
(2) Stage B — the `-region-out` (G1) dump gains a DOT twin where every
region renders as `subgraph cluster_region_<id>` (ports on the cluster
boundary; request edges as inter-cluster edges when Stage C lands). GOLDEN
POLICY: DOT surfaces stay ADVISORY visualization (like today's `-dot-out`,
never byte-goldened); the G1 text dump remains the referee — this directive
does not reopen D2.2.

## Stage-A implementation adjudications — 2026-08-03 (orchestrator, per charter stop-conditions)

Stage A LANDED (suite green at 190, zero existing-golden churn, validators
live corpus-wide). Four implementation adjudications, all evidence-backed:
1. **Hash exclusion (H-A2 refinement).** projection_role folds into
   `Equals` ONLY, deliberately NOT `Hash` — empirically isolated: the Hash
   fold perturbs hash-derived Rel/ControlFlow tie-breaks (demand_tc join-emit
   seq/order swap) for ZERO CSE benefit (CSE buckets by cse_color, Equals
   decides). The T-oracle-1 "Hash differs" unit half is unrealizable;
   Equals==false with Hash-same is the correct unit.
2. **V-NO-COLLAPSE is belt-only** (E-A2's fallback, forced): the amended
   user-facing hard-abort false-fired on valid programs (MiniDisassembler,
   PointsTo) because Stage-A AllFields producer keys make benign drops
   indistinguishable from real collapses; real minimal keys are Stage-B
   Minimize territory. Belt fires only on empty-key-with-visible-fields (a
   pipeline regression).
3. **All three H-A9 witnesses proven UNCONSTRUCTIBLE** (the stage doc's own
   provisioned outcomes): role-refusal split (role-OFF full-corpus sweep =
   zero .df diffs; refusal load-bearing only on dead-flow-doomed self-loops),
   V-AGG-INPUT-KEY 4-mode reject (over-body heads always project a non-empty
   key; AllFields floor), member_collapse_1 (both mint-site branches fail for
   the same AllFields reason). Corpus stays 190; no fabricated witnesses.
4. **role= divergence resolved toward the produced dumps** (wrong PREDICTION,
   not a compiler bug): the ratified structural stamp does not survive
   proxying; blessed goldens carry the member-heavy reality (see the
   desired-states RECONCILED note). NEW OWNER ITEM for the Stage-B re-brief:
   should proxy mints INHERIT the source's role so set-boundary provenance
   survives onto the final graph (zero behavioral risk — role is inert on
   survivors — but it changes the dump semantics and the D2.9 default rule)?

## Owner design conversation — 2026-08-03: shared arrangements / table provenance (input to the Stage-B + D2.6 re-brief)

Mapping established: EquivalenceSet model sharing = identity-ERASING storage
sharing; per-join indexes = unregistered arrangements; the D3.a.3 shared pub
= the true multi-reader analog that RoutedResult generalizes. Sharpenings
for the paused decisions: (1) D2.6 is the arrangement READER-HANDLE question
(owner-bearing = individually retractable capabilities; refcount = anonymous
handle count) — decide it with that framing; (2) once Stage-C leases or the
agent substrate let readers outlive an epoch, dead-row compaction needs
reader FRONTIERS (the cursor-invalidation contract is the degenerate form) —
bears on A5's lease-lifetime witness; (3) DIRECTION (not yet a ratified
stage item): carry LOGICAL-ORIGIN SETS on physical data models as
compile-time provenance — planner split-vs-share input at region boundaries,
cost attribution for shared maintenance, D5 trie column-order choice, and
the MODEL-membership dump directive's debugging half. Candidate home: the
Stage-B planning tier's table nodes.

## Owner design conversation — 2026-08-03: demand areas vs SCCs (containment + replication)

Owner's model, ratified as the framing for Stage-B/D dumps and D2.12:
(1) a demand area (keyed instance / future region) includes the ACYCLIC
CONNECTIVE TISSUE — views leading into and between SCCs; region boundaries
are admissibility-drawn (SIP-reachability-shaped), never condensation-drawn;
(2) one demand area may contain MULTIPLE SCCs (Stage D = instance-qualified
interior fixpoints); (3) a demand area conceptually REPLICATES its interior
SCC per demanded key, with the key INVARIANT inside each copy — this is the
semantic content of the nested lowering: the sequestered-key contract /
V-INSTANCEKEY-NOT-DERIVED is the invariance obligation, V-CW is LOGICAL
replication (key-widened disjoint union), V-PI is PHYSICAL replication,
flat -demand is the zero-replication degenerate point (key stays a row
column, guard merely filters), and D3.a.3 multi-adornment is the
compile-time sibling (replication per binding pattern). DOT consequence:
a demand/region cluster renders ONE box annotated `key-invariant: <K>`
(a static graph shows the schema of the replication, not the copies);
stratum clusters are the box's INNER structure, not the region boundary.

## Owner design conversation — 2026-08-03 (cont.): the disassembler example — nested keys, two recursions, hoist-vs-nest

Owner's concrete instantiation of the demand-area model (the MiniDisassembler
shape, THE motivating domain): outer area keyed by FUNCTION address (interior
recursion #1: block discovery to fixpoint per function); inner area keyed by
BLOCK address (interior recursion #2: instruction sweep per block). Keys form
the Stage-D InstancePath — (F) outer, (F,B) inner — one sequestered invariant
key per nesting level; two instance-qualified local fixpoints at different
forest depths, each V-CW-widened by its path prefix. DESIGN FORK the example
forces: if inner content is a function of B alone (decoding is
function-independent), the inner area HOISTS to a single global block-area
shared by all reaching functions (the shared-arrangement question at the
instance level; overlapping functions make the sharing real) — ownership
DAG-vs-forest tension with §11 V-OWNERSHIP-ACYCLIC; if inner content consumes
the outer key (ARM/Thumb mode, function-local literal pools), (F,B) is
irreducible and the areas truly nest. The hoist-vs-nest decision is a
DeterminedBy question over the inner slice's ROW CONTRACT ("is the outer key
carried but never consumed?") — the Stage-A contracts layer is the seed of
demand-area lambda-lifting, a necessity argument for contracts no stage doc
had stated. Stage-D witness direction: a two-level disassembler-shaped corpus
case; MiniDisassembler is the eventual end-to-end carrier.

## Owner design conversation — 2026-08-03 (cont.): key peeling reveals nested SCCs = LOCAL STRATIFICATION, operational

Owner's refinement: inside a demand instance, PEELING the invariant key
(sequestration removes it from row types; key-equality joins trivialize;
key-transport edges vanish) yields a residual interior graph whose SCC
condensation is FINER than the global one — apparently-monolithic global
SCCs factor into nested interior SCCs glued only by key threading (the
disassembler: block-discovery and instruction-sweep separate once F then B
peel). Consequences, recorded as Stage-D framing amendments:
(1) INTERIOR RE-STRATIFICATION IS MANDATORY — Stage D runs its own Tarjan
on the peeled interior per region; global stratum ids must never be
inherited inward (over-glued); V-CW widening applies per INTERIOR SCC
(finer = less widening); region-internal scheduling gains staging
boundaries the global view cannot see.
(2) ADMISSIBILITY CAN IMPROVE PER INSTANCE: a globally-unstratified
negation/aggregation whose in-SCC path runs only through the peeled key
becomes stratified per instance — the LOCALLY STRATIFIED program class,
given an operational home by keyed instances for the first time. The
global Stratify reject stays correct for undemanded programs; a
key-invariant demanded instance of the same shape is legitimately
evaluable. Recorded as a FUTURE capability-lift candidate (post-Stage-D,
owner-gated), not a near-term work item.
(3) DOT/dumps: interior SCC boxes are computed on the peeled graph and
nest INSIDE region clusters; they genuinely differ from global stratum
boxes — region clusters and stratum clusters are distinct layers.

## Owner direction — 2026-08-03: THE REGION IS THE LOOP (cycles become external port connections)

Owner's architectural refinement, ratified as DIRECTION for the Stage-B/D
re-brief (not a near-term work item): a recursive slice lowers to a
sub-region whose BODY IS ACYCLIC — the back-edge leaves the body entirely
and becomes an EXTERNAL parent-level connection from the region's output
port to its input port; iteration semantics live in the region NODE
("run to fixpoint"), never in graph topology. "Internally an SCC is not
an SCC anymore." Priors: Naiad iterate scopes (ingress/egress/feedback at
the boundary), MLIR scf structured loops, classical loop-nest trees. The
operational layer ALREADY matches (LowerDRRounds bodies are acyclic;
inductive MERGE = the loop-header port join; swap vectors = loop-carried
variables) — this makes the structural IR agree with the emission.
Consequences recorded: (1) the cyclic-contract problem (T-conf-1/2,
Stage-A AllFields rule, E-A4) LOCALIZES to loop-carried PORT contracts =
the invariant/variant field split (the demanded key is the degenerate
invariant port field); (2) the condensation becomes a LOOP TREE nesting
into the ownership forest (incl. the key-peeling-revealed nested SCCs);
(3) keyed instance and loop region are ONE construct sequestering two
axes — key (space) and trip (time), the Mobius product order's
coordinates; egress-only-after-fixpoint is why the epoch-boundary
publishing quotient stays sound; (4) "every cycle is a region" becomes a
canonicalization invariant with structural validators (no cyclic edge
inside a frozen body; parent feedback edges are a distinguished class,
like io seams). Caveats: mutual recursion = multiple loop-carried ports
(block-args; MERGE already is that); parent acyclicity checks must class
feedback edges specially. Cutover risk concentrates in dumps/validators,
not semantics.

ADDENDUM (owner, same conversation): the loop-region is PARAMETERIZED ON
KEY COLUMNS — region ≈ λ(key columns). fixpoint(acyclic body). Keys are
the region's FORMAL PARAMETERS (invariant by construction, sequesterable,
peelable); a demand is an APPLICATION; a keyed instance is an ACTIVATION;
the InstanceStore is the MEMO TABLE of activations (= the shared
arrangement; D2.6 is its handle discipline). Port fields split into
parameter fields (keys) vs loop-carried fields (accumulators, the
contract-bearing variants). Flat -demand / nested lowering / Stage-D
local recursion = three evaluation strategies for the one construct:
inline-with-filter / memoized-activation / per-activation-fixpoint.

CORRECTION (owner, same conversation): the region body is NOT necessarily
acyclic — "every cycle is a region" is WITHDRAWN as a canonical invariant.
Externalization of a back-edge is an AVAILABLE TRANSFORMATION whose
enabling condition is that the cycle operates WITH the parameterized key;
it is a planner/cost-ranked choice (the same tier as cost-ranked
extraction), never forced. Two flavors: (a) KEY-INVARIANT back-edge ->
external feedback at the SAME activation (the trip-count loop; the clean
case); (b) KEY-CHANGING back-edge -> a RECURSIVE REGION CALL
(self-application at a different actual parameter = demand propagating to
a new activation — the recursive-demand shape demand_cyclic_1 fences
today; needs the finite-activation termination story + cost story before
lifting). Non-key-valued interior recursions may simply remain interior
fixpoints. The earlier consequences (port-localized contracts, loop tree,
local stratification) apply WHERE externalization is taken, not
universally.


## Owner refinement — 2026-08-03: the X=F/X≠F pivot split sharpens D2.12 (V-CW)

The right arm of a non-linear recursive join tc(X,T) is the query at argument
X ("ask region(X)"). Splitting the pivot: X=F = same-key intra-region
FEEDBACK (the fixpoint self-loop, non-externalizable); X≠F = a CROSS-REGION
CALL to a sibling activation (the externalized edge). This is the precise
(runtime) externalize-vs-keep discriminator, and it is the MECHANISM behind
A-corr-4: V-CW's disjoint-union lemma holds only for instance-INDEPENDENT
recursions; X≠F is a cross-instance read, so non-linear TC is the COUPLED
case the nonlinear-recursion-under-two-keys witness must exercise (the
widened fixpoint is a coupled fixpoint in one keyed table, NOT a disjoint
union). D2.12 re-brief action: restate the V-CW lemma as
independent→disjoint-union / coupled→widened-coupled-fixpoint, with the
cross-instance-read test (X≠F reachable?) as the discriminator, and make the
nonlinear witness mandatory. Full: running-example-disassembler.md §pivot
split. Also sharpens the externalization direction (region-is-a-loop): only
X=F feedback is non-externalizable; X≠F is inherently the cross-region port
edge.

## Owner proposal — 2026-08-03: request-edge notation rel[Bound...](Free...)

Concrete syntax for the request-edge/region-call node (the primitive the
dataflow IR lacks): `rel[Bound...](Free...)` — bracket = instance key (matrix
row index), parens = answer schema. It is the adornment made syntactic and
unifies #query decl / request edge / region-call node; degenerates correctly
(rel[K]()=bool probe A5, rel[](C)=ordinary relation ADJ-3); multi-adornment =
distinct bracket partitions of one rel over one pub (D3.a.3); the bracket set
IS the trie/arrangement prefix (prefix-nesting = shareable arrangement); the
X=F/X≠F recursion test = bracket-arg identity (syntactic). Pins: column order
in [...] is the trie key order; InstanceStore key = the bracket tuple.
Candidate for the Stage-C request-edge op surface and the Stage-B -region-out
+ DOT node syntax. Full: running-example-disassembler.md §notation.

## Owner direction candidate — 2026-08-03: USER-DECLARED REGIONS (explicit brackets on internal relations)

Proposal: make `rel[Bound](Free)` a SURFACE feature — extend mode-declaration
(today only on #query) INWARD so any local relation can be written as a region
directly; Build lowers it structurally, bypassing the demand-transform
inference. ASSESSMENT (orchestrator): STRONG idea in the DECLARED form, weak in
the PEELED form.
- ADOPT (declared): turns region INFERENCE into region CHECKING (far more
  robust); makes Build SIMPLER not larger (bypasses the fenced magic-set
  machinery = the smaller-not-larger thesis); BOOTSTRAPS B/C/D against explicit
  input before demand is retrofitted (the D1.1 "referee before cutover"
  instinct, one level up); completes the rel[Bound](Free) unification at the
  source level (one spelling: source decl / request-edge IR node / InstanceStore
  activation); the Stage-A contract's invariant/variant analysis IS the
  bracket-is-a-key CHECKER (machinery already in flight).
- REJECT (peeled): "spell out all peeled cases" over-asks — F=X/F≠X is
  peel-depth ZERO (runtime memo dispatch), so enumerating self/cross cases is a
  surface/lowering mismatch + an exhaustiveness-checking miscompile risk. The
  user writes only base+recursive clauses (plain Datalog already requires them)
  + the bracket. Burden = "declare your modes," light.
- TENSIONS (owner calls): (1) more OPERATIONAL surface — keep it OPTIONAL
  (declared regions for control; flat+demand for "just write rules"), so it
  COMPLEMENTS, not replaces, the demand transform; (2) optimizer authority —
  declared regions as planner HINTS (overridable, preserves cost-ranked
  re-factoring/arrangement-sharing) vs MANDATES; (3) raises I0's value — explicit
  == flat == demand must be answer-identical (the eqgate/interpreter referees
  it).
- RECOMMENDATION: pursue the DECLARED (optional, hint-not-mandate) form; slot
  AFTER I0 + Stage B (gives C/D user-written test input independent of the demand
  transform; natural home for the request-edge node); keep peeled cases OUT of
  the surface. OPEN: hint-vs-mandate; where exactly in the A→I0→B→C→D sequence;
  does it need its own stage doc. Full design context:
  running-example-disassembler.md (the whole 2026-08-03 thread).

## Grounding synthesis — 2026-08-03: is "bound implies [...]" just -demand? (mostly yes; what's new)

SAME MECHANISM: -demand (FLAT: key-as-column, region call = guard-join d_p⋈p)
and -demand-instance (NESTED: key = InstanceStore key, sequestered) ALREADY
are the two lowerings of "bound query → keyed region"; the transform reads the
query's bound columns as the demand key = the bracket. The core
bound→region identification is NOT new — this conversation re-derived the
RegionalDataFlowCore thesis from first principles (TC + disassembler), it did
not invent a new transform.
NEW (increasing depth): (1) the bracket on the SURFACE for INTERNAL relations
(user-declared regions) turns demand's INFERENCE of inner keying into CHECKING;
(2) the first-class REGION-CALL / REQUEST-EDGE node — demand expresses the call
IMPLICITLY (flat guard-join / nested instance lowering), never as an explicit
node; making it first-class IS the epoch's core thesis; (3) the THEORY (matrix
rows, down/across activation-graph stratification, lazy-vs-force-complete edge
typing by consumer monotonicity = the negation AND aggregate fences unified as
ONE CALM boundary, invariance-typed peeling) — the existing modes OBEY these
boundaries; the theory explains WHY and how to LIFT them.
SUBTLETY (a real decision): `bound` does NOT currently imply demand — demand is
opt-in (mode-gated off); a bound query compiles as full-materialize+filter by
default. "bound implies [...]" = making region lowering the DEFAULT for bound
queries = a genuine call gated by the cost model (demand doesn't always pay).

## Owner refinement — 2026-08-03: region calls as BODY ATOMS; query wraps region

Proposed surface: `reaches_to(bound F, free T) : tc[F](F, T).` — the QUERY is a
thin wrapper whose body INVOKES a region; `tc[F](...)` is a region call
appearing as a BODY ATOM (the surface realization of the request-edge node).
Better than "bound implies [...]" because it DE-CONFLATES query adornment
(interface) from region key (implementation): you WRITE the connection, infer
nothing. Separating reaches_to (query, one F-keyed entry) from tc (region,
invoked at MANY keys) is the RIGHT factoring — one region template, many
activations (the InstanceStore memo structure).
KILLER PROPERTY — the recursive region def makes DOWN/ACROSS syntactic:
    tc[F](F,T) : edge(F,T).
    tc[F](F,T) : tc[F](F,X), tc[X](X,T).
body atom tc[F] (SAME bracket as head) = self-recursion/FEEDBACK (X=F, down);
tc[X] (DIFFERENT bracket) = cross-region CALL (X≠F, across). The compiler reads
the self-vs-call structure straight off bracket-variable identity — no SIP walk,
no inference. This is the concrete surface for the declared-regions direction.
CONVENTION QUESTION (changes how composition reads, interacts with D2.3
visible-columns): key-in-output `tc[F](F,T)` = schema-compatible with flat
tc(F,T); SEQUESTERED `tc[F](T) : tc[F](X), tc[X](T)` = X flows from region F's
OUTPUT into region X's BRACKET (key) = the sink-then-source seam / matrix
composition made literal, key never in output. Pick one convention.
OPEN: does the query wrapper collapse into the region (tc carrying the
adornment directly) when the region is singly-invoked, or always stay
separate; how the recursive head binds its bracket var (head tc[F] binds F;
body tc[X] binds X from a join). Full thread: running-example-disassembler.md.

## Owner refinement — 2026-08-03: brackets bind arbitrary (non-prefix, ordered, multi-column) keys → the logical/physical key split

The bracket DECOUPLES the region key from the relation's argument order: it can
bind an arbitrary SUBSET, in a CHOSEN ORDER, NOT necessarily a prefix — strictly
more expressive than the positional bound/free adornment (an unordered
position-set). (-demand already allows non-prefix binding e.g. tc(free F,bound
T); the bracket ADDS explicit multi-column ORDERED keys + decoupling from arg
order.) The bracket IS the trie/arrangement key spec, which forces the
distinction the flat adornment hides:
- LOGICAL key = WHICH columns (a set) = the region's PARTITION = SEMANTIC.
  rel[A,C] and rel[C,A] are the SAME region logically.
- PHYSICAL order = the sort order WITHIN the bracket = the ARRANGEMENT choice =
  OPTIMIZATION. rel[A,C] and rel[C,A] are DIFFERENT arrangements; two regions
  share a trie only if orders are PREFIX-COMPATIBLE (McSherry cost: distinct
  orders fragment sharing).
So the bracket order is a HINT: canonicalize the LOGICAL key for
equivalence/CSE, choose the PHYSICAL order separately for arrangement-sharing.
Maps onto landed work: the Stage-A contract's member_key IS the logical key
(ordered canonically by field-id for deterministic RENDERING, not a sort order);
the physical arrangement order lives at the D5/planning tier. The bracket is
where they meet: user/planner names a composite non-prefix key, the contract
validates it is a real key, the order (share-vs-fragment) is the cost knob.
CONSEQUENCE: "bind more than one thing" lets a REGION be keyed on a join pivot
or composite index with NO name in the flat schema — the disassembler's
(FuncEA,BlockEA) nested key and the non-linear-TC pivot. Full:
running-example-disassembler.md.

