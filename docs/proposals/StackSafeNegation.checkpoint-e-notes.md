# Stage 3 checkpoint (e) working notes (committed working ledger)

Branch: derivation-counters. Checkpoint (d) is COMMITTED (2481340: negation
crossover + F18 position-keyed gates; c649f29: OQ3 set-semantics netting).
Suite: SUITE: PASS (149 cases, 4 modes + oracle + monotone variants);
ctest 3/3 (MiniDisassembler.DifferentialUpdatesWork flipped red→green at
(d) — its gap WAS the crossover). This file seeds the NEXT session:
E0 (the member-list dedup follow-up, owner-approved in direction) and
checkpoint (e), the Stage-3 closeout. A fresh session starts at "Session
bootstrap" at the bottom. Everything in §3 is a SEED HYPOTHESIS written at
(d) close from code + grep inspection: the next session must re-derive it,
critique it, and verify the inventory claims before any code — the method
that found F17 (slice 4) and F18 ((d)) before a line of code each time.

Authoritative context stack (read in this order): this file;
checkpoint-d-notes.md (the (d) landing record + the OQ3 addendum — these
supersede its older §1/§3 where they disagree); checkpoint-c-notes.md
(slice records); StackSafeNegation.plan.md Stage 3 (e) + merge criteria
(:767-788); StackSafeNegation.md §5.5, §6, §9, §11.

## 1. Whole-program view at (d) close (re-derived from code, not plan)

Code anchors: lib/ControlFlow/Build/{Build,Stratum,Negate,Procedure}.cpp,
include/drlojekyll/Runtime/{Table,Vec}.h, bin/Oracle/Main.cpp, all at
c649f29.

Row state (DiffTable): counters C_nr/C_r; flags kInI (frozen batch start),
kDel/kAdd (claimed this batch), kDelNow/kAddNow (this round), kExplicit,
kTouched. Crossings: `+` ⇔ Total ≤0→>0; `−` ⇔ kInI && post-C_nr ≤ 0.
Claim gates (F17): TryClaimDel re-tests C_nr ≤ 0, TryClaimAdd re-tests
Total > 0. NetAdded = kAdd && !kDel && !kInI; NetDeleted = kDel && !kAdd.
Monotone Table: sealed row-id watermark gives InI; Seal() at commit sweep;
InNew/Present ≡ true. « (d): negated monotone tables are auto-enrolled in
Seal via their kNetAdditions vector. »

```
receive(msg adds, removes):                    « generated receive procs »
  NetBatch(adds, removes)     « OQ3: dedupe each side; adds∩removes
                                ANNIHILATES (dropped from both) — the pair
                                has no effect through the explicit channel;
                                the row keeps what the IDB can otherwise
                                prove. Vec.h NetBatch; oracle mirrors ×2. »
  call entry(adds, removes)

proc entry(batch):
  ── INGEST ──
  monotone msg:     fold +NR; eager walk, CUT at any CanReceiveDeletions()
                    successor (cut boundary accumulates net-additions).
                    Eager NEGATE gate (monotone negated only, post-(d)):
                    CHECKMEMBER InI(negated) — batch-frozen, arm-order
                    independent; @never keeps Present. « F18 »
                    A monotone NEGATED table's fold gains if-crossed →
                    append into its kNetAdditions boundary vector. « D2 »
  differential msg: explicit ±; crossings park in del/add queues.

  ── STRATUM PHASES (BuildStratumPhases), strata ascending ──
  (1) SEEDS (hoisted; claim gates make the §5.0 order-restoration sound):
      branch chains: loop source net frontier → chain steps → head fold.
        chain step AT a NEGATE: context-keyed gate « F18 »:
          seed context      → key ABSENT in InI   (BOTH signs)
          fixpoint refire   → key ABSENT in InNew (BOTH signs)
      dual-section joins: unchanged from slice 4.
      CROSSOVER (one per non-@never NEGATE) « (d) »:
        − arm: loop negated.kNetAdditions {negated cols}
                 scan pred table by key (§3.4 index; unit rel = full scan)
                   CHECKMEMBER InNew {pred row}
                     fold −class into the NEGATE'S OWN table
                       if-crossed → its delete_queue
        + arm (iff negated table differential): mirror over kNetRemovals
        class = RuleClass(sccs, negate_table, {pred, negated});
        emitted ×multiplicity = forward branch-chain count (see §3 E0 —
        an artifact-compensation to be demoted to an assert);
        registered with the scheduling fixpoint: stratum ≥ readiness of
        negated AND pred tables; negate table's drain lifted above it;
        seed-before-drain asserted (load-bearing).
  (2) SINGLE-PASS tables: ClaimDrain del/add, FrontierFilter del/add.
  (3) recursive-SCC tables: OVERDELETE claim rounds (Δ-clear → drain →
      k-position JoinFire (§5.1 matrix) → projection SeedLoops over Δ
      (negate gates read InNew here, R2) → Retire) → REDERIVE (C_r>0) →
      INSERT mirror → BOTH frontier filters.

  ── COMMIT ── per touched row: assert C ≥ 0 per class, publish was≠now
      (differential-backed @differential messages), kInI := (Total>0),
      clear scratch; monotone tables with delta vectors Seal().
      Generated DBs self-check DebugValidateCounts per batch (debug).
```

Scheduling: unchanged from slice 4 (readiness lift + same-SCC exemption +
negated-table readiness), extended by the crossover's two read constraints.

## 2. What EXISTS vs what is LEFT (evidence at c649f29 — VERIFY, then act)

ALREADY DONE (candidate no-ops in plan.md's (e) list — like (d)'s deletion
inventory, much of (e) was satisfied by (b)-(d); the session must verify):
- Publish: the flusher checker branch is GONE; differential-table-backed
  @differential messages publish through COMMITSWEEP (Procedure.cpp
  ~307-345, commit_published_view); monotone-backed keep kMessageOutputs
  (~200-230). This IS the §5.5 end state.
- Query cursors: Database.cpp already filters with `.Present(id)`
  (~1923, ~1951). No find_* checker calls remain.
- The plan's merge-criteria deletion grep (plan.md:784-788) returns ZERO
  runtime/CF hits: all 28 residual matches are the PARSER's unrelated
  `Language::kUnknown` enum (Parse/Foreign.cpp, DataFlow spelling lookups).
  TupleState / TryChange* / TryTransition / TopDownChecker / tuple_checker
  / kTupleFinder / MODESWITCH / CHANGETUPLE / kAbsentOrUnknown: no hits.
- DebugValidateCounts: emitted into generated DBs (Database.cpp ~1329) and
  exercised by every debug OptDiff run.
- deep_chain_retract at full depth under ulimit -s 1024: verified at (d).

LEFT (the real (e) work, pending re-derivation):
- E0 (owner-approved direction, own commit): TABLE::GetOrCreate member-list
  identity dedup + crossover-multiplicity demotion to an assert (§3).
- The plan.md merge-criteria residue: Stage 0's three mixed-batch
  sentinels (identify them; confirm in-suite), the 30-seed randomized
  mixed add/remove stress vs the oracle (re-run at HEAD; tc_nonlinear_diff
  covers 12 seeds in-suite).
- Carried-forward from (d): MiniDisassembler known-red bookkeeping (now
  green — purge stale "pre-existing red" references from docs/notes);
  oracle INVARIANT-line format skew (I9) decision; non-ASCII lexer
  truncation diagnostic (authoring gotcha found at (d): a `§` in a .dr
  comment silently truncates parsing — consider a diagnostic).
- Stage 4 docs (plan.md:790-800): rewrite differential sections of
  docs/RuntimeAndCodegen.md and docs/ControlFlowIR.md present-tense;
  CLAUDE.md core-invariants update (replace the inductive-back-edge
  invariant with the UPDATECOUNT/zero-crossing form; add the seed/fixpoint
  schemas, membership-predicate vocabulary, commit asserts, F18's
  context-keyed negate-gate rule, OQ3 annihilation contract); FINDINGS.md
  round entry if the session finds anything.
- OUT OF SCOPE: Stage 5 (differential @product), subgraphs/demand
  (see push-method/SLDMagic session memory), perf roadmap.

## 3. Path forward as diffs against §1 — SEED HYPOTHESIS, re-derive first

**E0 — member-list identity dedup; demote crossover multiplicity.**
Root cause (found at (d), recorded in the (d) landing-record addendum):
TABLE::GetOrCreate (lib/ControlFlow/Data.cpp:231-283) pushes the view on
every call and relies on sort + adjacent std::unique; the depth-keyed
comparator interleaves DISTINCT equal-depth views between the identity
duplicates, so unique removes nothing → duplicated member lists →
duplicated branch chains → forward fold ×2 (merge_5) → the crossover's
multiplicity compensation.

    - model->table->views.push_back(view);           « unconditional »
    + if (std::find(views.begin(), views.end(), view) == views.end())
    +   views.push_back(view);                       « IDENTITY dedup ONLY »

CRITICAL GUARD: identity, never structural equality — distinct-but-equal
views sharing a model are INTENTIONAL (the group_ids CSE guard,
lib/DataFlow/View.cpp:1455, node_pairs example: merging the two sides of a
self-join/cross-product collapses the join to a pass-through TUPLE via
Join.cpp:449's trivial-join fold). Cite that at the change site.
Consequence to verify: every forward chain count to a negate becomes 1
structurally (a chain to a negate can only root at its single materialized
pred view) ⇒ CrossoverEmission.multiplicity demotes to a debug assert
(== 1), deleting the replay loop. Expect NO golden churn (doubled chains
are sign-balanced; crossings fire on the same count edges) — the 149-case
suite × 4 modes + oracle is the net; run comm both directions vs the
zero-red baseline. Also expect IR shrinkage on any dup-member program
(doubled seeds for non-negate targets halve too). Hand-trace merge_5's
same(2) retraction against the post-dedup IR before blessing anything
(nothing should need blessing).

**E1 — inventory verification pass (expected mostly no-op).** Re-run the
plan.md:784-788 grep and classify every hit (expected: only
Language::kUnknown); confirm the §6 table rows are all satisfied; confirm
publish order determinism (does the commit sweep need the sort the plan
contemplated? evidence: goldens byte-stable across 3+ fresh suite runs —
decide and record).

**E2 — merge-criteria residue.** Identify Stage 0's three mixed-batch
sentinels by name (checkpoint-a/b notes or plan Stage 0) and confirm
green; write/run the 30-seed randomized mixed add/remove stress vs the
oracle at HEAD (pattern: tc_nonlinear_diff's driver); record results in
this file's landing record.

**E3 — Stage 4 docs sweep.** Per §2 LEFT list. CLAUDE.md invariants MUST
gain: context-keyed negate gates (seed=InI, fixpoint=InNew, both signs);
crossover folds target the negate's own table, seed-before-drain;
annihilation netting; monotone negated ⇒ boundary frontier + Seal. Purge
"MiniDisassembler pre-existing red" from CLAUDE.md/notes where stale.

**E4 — decisions to close: I9** (oracle INVARIANT line: regenerate all
oracle goldens with the line, or suppress it in the oracle — the strip
side is now entrenched by 37 committed goldens; recommend suppress-forever
+ one-line oracle change OR ratify strip as the contract, but CLOSE it);
**non-ASCII lexer diagnostic** (small Lex change + a negative test, or an
explicitly recorded won't-fix).

## 4. Desired end state

- Stage 3 CLOSED: plan.md merge criteria all check off; suite PASS with
  E0's assert in place; zero golden churn from E0; docs current
  (Stage 4); FINDINGS updated; this file gains a landing record with
  deviations-for-ratification (there should be none — E0 is
  owner-pre-approved; anything else discovered must be listed).
- The next majors after Stage 3 (recorded, not started): Stage 5
  differential @product; the perf roadmap (bench harness → data
  structures → delta-relational IR); subgraphs/demand (SLDMagic-as-
  transformation lens; see memory push-method-origin-and-negation).

## 5. Session bootstrap (fresh-session checklist)

- Read: this file top-to-bottom; checkpoint-d-notes.md landing record +
  OQ3 addendum; plan.md Stage 3 (e) + merge criteria + Stage 4; MD §5.5,
  §6, §9, §11 (OQ3 now DECIDED — read its recorded rationale).
- Method (worked at slice 4 and (d); use it again): BEFORE coding, dump
  the ACTUAL IR of merge_5, negate_4, and one dup-member non-negate case
  (find one: a program with two same-shape tuples over one source feeding
  different consumers); re-derive §1's pseudocode from the code; verify
  §2's ALREADY-DONE claims by grep/IR (do not trust this file); write the
  E-diffs against the re-derived pseudocode; adversarially critique them
  (minimum: E0's identity-vs-equality guard, the multiplicity==1
  structural argument — find the counterexample shape if one exists, e.g.
  a negate whose pred model has TWO distinct member views both feeding the
  same negate; and whether E0 changes any emission ORDER the suite
  byte-compares); write the desired post-E0 emitted IR for merge_5
  concretely and critique it; hand-trace merge_5 same(2) and tagd through
  it. Only then implement. Use workflows: opus for traces/critique/
  implementation/adversarial review, sonnet for mechanical audits and
  suite gates.
- Verification gates: full suite vs the ZERO-red baseline (comm both
  directions, expect byte-identical goldens throughout E0-E4); ctest 3/3
  (MiniDisassembler is GREEN now — any regression is a blocker); data/
  corpus 4-mode sweep (known feature-gaps + pre-existing evm_array_parse
  134 only); goldens change ONLY via new-case authoring (none expected).
- Environment: export PATH="/Users/pag/Code/.brew/bin:$PATH" before any
  test run. Suite: DR=build/debug/bin/drlojekyll tests/OptDiff/runall.sh
  <workroot> [jobs] [filter]. macOS bash 3.2 — no declare -A. Fixture
  files must be ASCII-only (lexer truncates silently at non-ASCII).
