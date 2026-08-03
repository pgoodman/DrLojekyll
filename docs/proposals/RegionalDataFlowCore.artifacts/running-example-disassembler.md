# Running example — recursive disassembler (the epoch anchor)

Owner-chosen running example, 2026-08-03. Grounds the region model on a
domain that exercises every hard axis. `tests/MiniDisassembler/database.dr`
is the STRATIFIED SKELETON; the RICHER version below is the target.

## The three levels (loop nest)

- **Function identification (outer, key: none / bound FuncEA).** A function
  head is a CALL target, or an instruction with no predecessor. Recursion:
  CALLs found while sweeping introduce NEW heads.
- **Block identification (middle, key: FuncEA).** A block starts at a head,
  extends by fall-through to a terminator; branch targets start new blocks.
- **Instruction sweep (inner, key: BlockEA).** Instruction by instruction
  via fall-through until a terminator.

## Compiled skeleton (verified, tip + Stage A)

`database.dr` -df/-contract/-dot: 26 strata, EXACTLY ONE multi-view SCC =
stratum 21, the `function_instructions` fall-through sweep
(`key=(FuncEA,InstEA)`); 4 negations; `function` sits in a LOWER stratum
(computed from raw_transfer/instruction MESSAGES only, so `!function` reads
a final relation → globally STRATIFIED). One interior loop, precomputed
outer relation. The single stratum-21 cluster is the only box in
`minidis.dot`.

## What the RICHER version adds (the real target)

1. **Instruction existence is DERIVED** (decode from heads, follow edges,
   decode targets) — `instruction`/`raw_transfer` become recursive, not
   messages. The loop nest becomes genuine (sweep ⊂ blocks ⊂ functions).
2. **Cross-level cancellation** ("new heads cancel old things"):
   `function(H)` fires on a CALL found DURING the sweep, so `function`
   depends on the sweep and the sweep depends on `!function(Inst)` — a
   cycle THROUGH A NEGATION. Discovering H-as-head RETRACTS H's prior
   attribution to the enclosing block (the `!function` gate flips) and the
   OVERDELETE cascades forward through fall-through. GLOBALLY UNSTRATIFIED
   (the evm_func_parse reject class).

## What manifests (the payoff mapping)

- **InstancePath = the key nesting**: FuncEA outer, (FuncEA,BlockEA) inner.
- **Hoist-vs-nest is a DeterminedBy query on the Stage-A contract**: if
  decode is mode-independent, the block-area carries FuncEA but never
  CONSUMES it → hoists to a shared global block-area (overlapping functions
  sharing blocks = the shared arrangement made real); if ARM/Thumb mode /
  literal pools make decode function-dependent, (FuncEA,BlockEA) is
  irreducible and the areas nest. THIS is why the contracts layer exists.
- **Local stratification, operational**: the `function ↔ sweep ↔ !function`
  cycle is globally unstratified, but PEELING FuncEA makes `!function` read
  the GLOBAL (external-key-space) function set per activation — a
  stratified negation of an external relation. Key peeling can DISSOLVE the
  unstratification. Concrete carrier for the "globally unstratified,
  locally stratified" lift.
- **Loops externalize on different conditions**: inner sweep = KEY-INVARIANT
  back-edge → same-activation feedback (trip-count loop, clean); call→head
  edge = KEY-CHANGING → recursive region CALL to a new activation
  (demand_cyclic_1-fenced; needs the finite-activation termination story —
  the binary is finite, but that is a real obligation).
- **Fence to clear first**: `-demand` on database.dr rejects with "Multiple
  demanded (bound) queries" (function + function_instructions = C14); the
  multi-query lift (expected at Stage C) is a prerequisite to demand-driving
  it.

## Staged use

- Stage A (LANDED): the contract on the stratum-21 sweep + the block-area
  hoist-vs-nest DeterminedBy question already have a home.
- I0: the disassembler .batches (message decode order → final function/
  block/instruction membership + sorted deltas incl. the head-discovery
  RETRACTIONS) is a high-value differential referee case.
- Stage D: the two-level nest is THE local-recursion + local-stratification
  witness; MiniDisassembler is the eventual end-to-end carrier.

## Abstract companion — non-linear TC (the minimal skeleton)

`tc(F, T) : edge(F, T) : tc(F, X), tc(X, T).` (corpus: tc_nonlinear_diff) is
the disassembler's structure with the domain noise removed — it isolates the
"two loops" in one body. NOTE THE SEPARATOR: `:` is the UNORDERED (pure
Datalog) clause separator — the two bodies `edge(F,T)` and `tc(F,X),tc(X,T)`
carry no ordering, and the recursive body's join order is the compiler's to
pick. The `:-` separator is the DIFFERENT, forced-order construct (sugar for
`@barrier` between every two body conjuncts, the barrier_neck_1 witness);
using it here would wrongly STAGE the self-join and change exactly the join
structure this analysis is about. Bound query tc(bound F, free T); the
recursive JOIN has TWO recursive inputs, keyed DIFFERENTLY by the SIP walk:
- LEFT `tc(F,X)` demands tc(F,·): SAME key F = the invariant-key self-edge
  = trip-count FEEDBACK (the activation growing its own F-rooted set) =
  externalizable as same-activation feedback.
- RIGHT `tc(X,T)` demands tc(X,·): CHANGED key X = the key-changing
  self-edge = a recursive region CALL to sibling activation tc(X,·) = NOT
  externalizable as feedback (self-application at a new parameter). Mirrors
  the call→new-function-head edge.
Realizing the right arm as a call (vs today's one-flat-fixpoint self-join:
one table + two indexes + a pivot loop) forces REGION DUPLICATION =
instantiate the region at X. The duplicates are INSTANCES OF ONE TEMPLATE
λk.reach-from(k) at F and X → structurally identical by construction →
dedup to ONE region definition + N memoized activations (InstanceStore =
memo table; cf.procdedup = the control-flow shadow). CAVEAT: F-region and
X-region are the SAME region (one lambda, two applications), NOT duals. The
genuine DUAL is the OTHER adornment tc(free F, bound T) = backward
reachability = the TRANSPOSE region (pivot column roles swapped); D3.a.3
multi-adornment already co-locates the bf/fb pair over one pub, and the
transpose relation is itself structurally recognizable (same template,
mirrored column order). Non-linear TC is the one body containing both
growth directions, hence the natural carrier for the two-loops + duplication
+ dual-recognition analysis.

## The endpoint-key vs interior-pivot mismatch (owner, 2026-08-03)

The subtle thing about non-linear TC under a bound query: the QUERY expresses
interest as an ENDPOINT (tc(bound F,·) = source F; tc(·,bound T) = sink T) —
one key, one end. But the recursive join tc(F,X),tc(X,T) pivots on X, an
INTERIOR node that is the SINK of the left atom and the SOURCE of the right.
The join is composition reach(F,·)∘reach(·,T) glued at a SINK-THEN-SOURCE
seam. So the recursion's sub-problem key (interior pivot) lives in a different
space than the query's interest key (endpoint).
- Demand consequence: bound-F propagates cleanly to the LEFT arm (source key
  F, invariant), but the RIGHT arm tc(X,T) is demanded on X = a sink the left
  arm just produced. The sink-then-source seam IS the key-change mechanism:
  every discovered sink becomes a new demanded SOURCE. The magic relation
  seeds {F} then fills with EVERY node reachable from F — demanded source-key
  set == answer sink set. Demand computes {(X,Y):X reach F, Y reach X} (the
  whole sub-relation over the reachable subgraph), NOT {(F,Y)} — quadratically
  bigger; nearly self-defeating for pruning (it still prunes non-reachable
  nodes, but activates a full tc rooted at every reachable node).
- Linear TC (tc(F,X),edge(X,T)) has no such seam: the sole recursive atom is
  source-position F, so the source key threads UNCHANGED = tail-recursive in
  the key = one loop, one activation, real pruning. Non-linear TC is
  subproblem-GENERATING = one activation per reachable interior node.
- Payoff 1 (demand-pays discriminator): recursion shape (tail-in-key vs
  pivot-generating) IS the linear/non-linear distinction and predicts demand
  effectiveness directly.
- Payoff 2 (InstanceStore-as-memo justification): without memoizing the
  interior activations, non-linear demand recomputes tc-from-X once per path
  reaching X = EXPONENTIAL; the memo table (InstanceStore = shared
  arrangement) collapses it to the quadratic sub-relation. Here the shared
  arrangement is not an optimization but the thing keeping composition
  recursion out of exponential blowup.

## The pivot split X=F vs X≠F (owner, 2026-08-03) — same-key feedback vs cross-region call, and the V-CW coupling caveat

The right arm tc(X,T) [X bound, T free] IS the query tc(bound X, free T) at
argument X — "ask region(X) for its answer." Decompose by the pivot:
- X = F (SAME key): region(F) referencing itself → intra-region FEEDBACK =
  the semi-naive same-key fixpoint self-reference (answer not yet final →
  needs iteration). Cannot be externalized; it IS the loop.
- X ≠ F (DIFFERENT key): region(F) asking region(X), a different ACTIVATION
  of the same template = the "cross-region self-loop" (self-loop in the
  template's call graph, cross-activation edge in the instance graph). This
  IS the externalized edge (leaves region(F), enters region(X)).
Consequences:
1. This is the PRECISE externalize-vs-keep discriminator, but RUNTIME (X is
   data): the body always demands region(X); X=F resolves as a MEMO SELF-HIT
   on the active region (InstanceStore), taking the fixpoint path instead of
   infinite descent.
2. X=F fires IFF the key is on a cycle. Acyclic instance graph → X≠F always
   → pure recursive DESCENT (terminating call DAG over activations, no
   same-key fixpoint); cyclic → coupled same-key fixpoint. Disassembler: X≠F
   = function-region(F) calling function-region(G) for callee G (the call
   graph); recursion in the BINARY = cycles in that call graph = coupled
   cross-region fixpoint.
3. V-CW COUPLING CAVEAT (the mechanism behind A-corr-4 / the
   nonlinear-recursion-under-two-keys witness): V-CW claims the key-widened
   fixpoint = DISJOINT UNION of per-instance fixpoints — clean ONLY for
   instance-INDEPENDENT recursions (no cross-instance reads: linear TC,
   independent sweeps). X≠F is exactly a cross-instance read (region(F)
   depends on region(X)), so non-linear TC is the COUPLED case: "disjoint
   union" mischaracterizes it; the widened fixpoint is a COUPLED fixpoint
   V-CW lays out in one keyed table (cross-key reads land in the same widened
   iteration). This is why the disjoint-union lemma must be discharged by the
   nonlinear witness, not termination alone (brief A-corr-4). The pivot split
   is the load-bearing "why."

## The matrix / activation-call-graph frame (owner, 2026-08-03) — the unifying picture

Curry the relation: tc(X,T) = tc[X](T), where tc[F] is the region as a
function of its key. The region-call node the dataflow IR LACKS today IS the
proposal's REQUEST EDGE ("region(F) asks region(X) for its answer") — the
missing primitive converges from two directions. The X=F/X≠F split lowers as:
X=F → a SELF-request the induction/InstanceStore short-circuits into fixpoint
feedback (no edge, it's the loop); X≠F → a genuine REQUEST EDGE to activation
X.

ACTIVATION-LEVEL STRATIFICATION (the SCC/stratum connection, lifted to KEYS):
region calls form an activation call graph (which key requests which) with
its OWN SCC condensation, distinct from the relation-level strata.
- DOWN = request crossing to a LOWER activation-SCC = well-founded BY DESCENT.
  If the whole activation graph is acyclic (data graph is a DAG), every
  request goes down → evaluable by TERMINATING MEMOIZED DESCENT, no iteration,
  each row computed once from its successors' rows.
- ACROSS = request WITHIN an activation-SCC = NOT well-founded by descent =
  the "truly non-linear" case. It IS well-founded, but only in the
  MONOTONE-LATTICE sense (answers grow, bounded, converge), NOT the descent
  sense → needs the coupled fixpoint. Calls never go strictly UP across SCCs
  (contradicts the defining dependency); apparent "up-and-back" is one SCC =
  one coupled fixpoint.
- So: down-only ⟺ acyclic activation graph ⟺ descent, no iteration;
  any across ⟺ a cycle in the activation graph ⟺ coupled fixpoint. Compile
  catch: data-graph cyclicity is a RUNTIME fact, so emit coupled-fixpoint
  code (V-CW) unless acyclicity is provable; the DAG case degenerates to
  descent as a cost-model optimization.

THE MATRIX VIEW (the unification): tc = a SPARSE BOOLEAN MATRIX M over (F,T);
KEYED INSTANCES ARE THE ROWS (tc[F] = row F); the InstanceStore is M stored
ROW-WISE = an arrangement/index keyed by F = a TRIE by F. Then:
- non-linear rule tc(F,X),tc(X,T) = M ∨ M·M (matrix square over the boolean
  semiring); region calls = the inner sum ⋁_X over the shared index X.
- TC = Kleene star M+ = closure; the fixpoint is Gauss-Jordan/Floyd-Warshall
  over the semiring.
- the SPARSITY PATTERN of M IS the activation-call graph ("row F needs row X"
  for the X in F's support); cyclic sparsity = cyclic activation graph =
  coupled row-fixpoint; acyclic = descent.
This is ONE object with the three research threads on the board: differential
dataflow ARRANGEMENTS are row-indexed matrices (= the memo/InstanceStore); the
MÖBIUS/semiring framing is the closure; WCOJ TRIES are row-keyed matrices. The
region-call structure is SEMIRING MATRIX CLOSURE, the down/across split is
whether the sparsity graph is a DAG or has SCCs. Directly informs: the
request-edge design (= the region-call node), D2.12 (V-CW = coupled row
fixpoint), and D5 storage (row-wise sparse = arrangement = trie).

## Proposed request-edge / region-call notation (owner, 2026-08-03): rel[Bound...](Free...)

`region:rel[Bound...](Free...)` = the adornment made syntactic: `rel` is the
region, `[Bound...]` is the INSTANCE KEY (the matrix ROW index / the columns
supplied), `(Free...)` is the ANSWER SCHEMA (the row contents). Literally
tc[F](T) = M_tc[F] = {T}. Validations:
- UNIFIES three surfaces: #query rel(bound F, free T) decl, the request edge
  that invokes it, and the region-call node are ONE shape (bracket=bound,
  parens=free). A request edge IS a query invocation (bound in, free tuples
  back).
- DEGENERATES correctly: rel[F,T]() (all-bound, empty parens) = bool existence
  probe, no cursor (D2.7 A5); rel[](F,T) (empty brackets) = one anonymous
  global instance = the ordinary non-demanded relation, no request port
  (D2.8 ADJ-3). The []/() extremes span the whole spectrum.
- MULTI-ADORNMENT = different bracket/paren partitions of one rel: tc[F](T)
  vs tc[T](F) = bf/fb duals = transpose regions = syntactically distinct nodes
  over one pub (D3.a.3).
- BRACKET SET IS THE TRIE PREFIX: [Bound...] names the index key order
  (prefix), (Free...) the leaves → choosing the bracket IS the implied-trie/
  arrangement column-order choice; rel[A](.) and rel[A,B](.) share a trie iff
  prefixes nest (arrangement-prefix-sharing, exposed directly).
- RECURSION TEST BECOMES SYNTACTIC: in tc[F](T) : ..., tc[X](T), whether the
  call's bracket arg [X] is the SAME variable as the head bracket [F] IS the
  self-loop-vs-cross-region test (bracket-arg identity, not a runtime compare).
Pins it forces (features, not bugs): column ORDER within [Bound...] is
significant (= trie key order = an arrangement choice); the InstanceStore key
IS the bracket tuple (the store's dense-group-id space is keyed on
[Bound...]). Candidate concrete syntax for the Stage-C request-edge op and the
Stage-B -region-out / DOT node label.

## Lazy down-requests and the negation barrier (owner, 2026-08-03) — the push-method hazard, avoided the same way

Concern (owner): naive bottom-up computes "down" EAGERLY to fixpoint, so a
higher stratum reads a FINAL answer; a lazy down-REQUEST gives a PARTIAL
answer. Does this re-create the Stefan-Brass push+negation intractability
(continuation resumes across a stratum with STALE negated state)?
ANSWER: yes IF unguarded, and it is avoided by the SAME principle that forced
the vector/fixpoint architecture.
- MONOTONE consumer (join/union/projection): a partial answer is a sound
  under-approximation that only GROWS → laziness is safe (soundness by
  IRREVOCABILITY; CALM/free-termination; the e5 witness).
- NON-MONOTONE consumer (negation, aggregate/KV): absence/summary is NOT
  irrevocable ("X not in r yet" flips to "X in r"; a max can still rise) →
  reading a partial answer is the push-method STALE-ABSENCE drift. A request
  edge feeding a non-monotone consumer must FORCE the target region to
  QUIESCENCE (completeness) before the gate fires = a coordination point
  (quiescence detection) = the demand analog of the stratum boundary.
- So laziness is TYPED by the monotonicity of what it feeds; the differential
  flags (can_receive/produce_deletions) already carry that bit; in
  rel[Bound](Free) it is a distinguished edge KIND (lazy request vs
  force-complete request). This IS the CALM boundary (coordination-free iff
  monotone).
- CONFIRMATION: the current -demand slice fences BOTH negation AND aggregate/KV
  in a demanded body — NOT two limitations but the single "non-monotone
  consumer needs completeness, lazy demand lacks the barrier" boundary, drawn
  conservatively as a reject until force-to-quiescence-per-key is built.

CORRECTION to the earlier "local stratification" claim: in the NATURAL
disassembler, `function` is computable from instruction/raw_transfer
(messages/monotone) ALONE — it does NOT depend on the sweep — so it sits in a
strictly lower stratum and `!function` reads a FINAL relation. The
disassembler is ALREADY STRATIFIED WITHIN A BATCH; the "new heads cancel old
things" retraction is CROSS-BATCH DIFFERENTIAL MAINTENANCE (new transfer →
function grows → function_instructions OVERDELETEs), sound and standard, with
NO lazy-negation hazard. So the disassembler is a DIFFERENTIAL witness, not an
unstratified-negation one, and does NOT stress the lazy-negation barrier. The
genuinely-unstratified "globally unstratified → locally stratified" lift
applies only to a DIFFERENT formulation where function depends on the sweep
(recursion through negation) — flagged earlier too strongly; corrected here.

## How far to peel the F=X/F≠X split (owner, 2026-08-03) — invariant peels, variant dispatches

Sharpening (owner): F=X/F≠X is NOT a branch at a node — the body downstream is
a general subgraph (the non-linear JOIN + the fixpoint back-edge), so
"introducing the branch" is a whole-subgraph specialization (trace/path
duplication + loop versioning), and "how far to peel" is the
partial-evaluation/supercompilation termination question
(heuristic-to-undecidable in general). The principled part:
- CRITERION: peel a split only if its condition is LOOP-INVARIANT (peels once
  to the region boundary — bounded, cheap = region versioning); a
  LOOP-VARIANT split is NOT peeled — runtime dispatch (peeling past a join =
  code blowup, past a back-edge = unbounded unrolling / data-dependent trip
  count).
- APPLY to F=X: X is the loop-carried FRONTIER (variant), F is the region
  PARAMETER (invariant) → the split is LOOP-VARIANT → peel depth ZERO. It is a
  DATA-STRUCTURE DISPATCH (InstanceStore lookup: X=F → self-hit/feedback,
  X≠F → other-row/request), not specialized code. The self-partition is a
  singleton (just F, present iff F on a cycle) — nothing to specialize.
  Confirms the "memo self-hit" framing is FORCED by invariance, not chosen.
- "IT ISN'T LINEAR" is the tell: linear TC's body is a linear chain
  (peel-friendly); non-linear TC's body is a JOIN of two recursive streams —
  the join you would duplicate to peel IS the non-linearity → leave it as
  memo dispatch.
- WHERE PEELING APPLIES (bounded): loop-INVARIANT splits — the region key,
  mode flags (ARM/Thumb), base-vs-recursive — peel once to the region
  boundary (region duplication / the seed-vs-fixpoint split already done).
- GENERAL "HOW FAR" BOUND when peeling a variant for a specific optimization:
  stop at the first RE-CONVERGENCE (nearest join or loop back-edge) = one
  layer; the fixpoint/memo absorbs the rest.
- ENABLING ANALYSIS: the row contract's INVARIANT-vs-VARIANT field split IS
  which columns are peelable (region parameters) vs runtime-dispatch
  (loop-carried) — "how far to peel" is READ OFF the contract. Third
  independent motivation for the Stage-A contracts layer (after hoist-vs-nest
  and local-stratification).
