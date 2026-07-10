# Stage 3 checkpoint (d) working notes (committed working ledger)

Branch: derivation-counters. Slices 0–4 of checkpoint (c) are committed
(slice 4 = 04b17ef); this file seeds checkpoint (d), the NEGATION CROSSOVER
— the last work standing between the current tree and a green suite (the
red set is exactly the 11 needs-(d) names). A fresh session starts at
"Session bootstrap" at the bottom. Everything in §3 ("(d) as diffs") is a
SEED HYPOTHESIS written at slice-4 close from code + IR inspection: the
(d) session must re-derive it, critique it, and hand-trace it before any
code, exactly as the slice-4 method did (that method's payoff was F17,
found before a line of nonlinear code was written — see
checkpoint-c-notes.md "Slice 4 landing record" and FINDINGS.md F17).

Authoritative context stack (read in this order): this file;
checkpoint-c-notes.md (slice records + whole-program view — the slice-4
landing record and « slice 4 » pseudocode markers supersede its older plan
sections); StackSafeNegation.plan.md §(d) (Stage 3 bullet "(d) Negation
crossover"); StackSafeNegation.md §3.4, §5.4, §5.5, §8.

## 1. Whole-program view at slice-4 close (re-derived from code, not plan)

Code anchors: lib/ControlFlow/Build/{Build,Stratum,Negate,Procedure}.cpp,
include/drlojekyll/Runtime/Table.h, all at commit 04b17ef.

Per differential-table row: counters C_nr/C_r (DerivClass split); flags
kInI (frozen batch-start), kDel/kAdd (claimed this batch), kDelNow/kAddNow
(claimed this round, cleared by RETIRE), kExplicit, kTouched. Crossings:
`+` crossed ⇔ Total ≤0→>0; `−` crossed ⇔ kInI && post-C_nr ≤ 0.
« slice 4 » CLAIM GATES: TryClaimDel re-tests C_nr ≤ 0, TryClaimAdd
re-tests Total > 0 at claim time — stale queue entries whose crossing was
canceled intra-batch drop; any later genuine crossing re-enqueues. NetAdded
= kAdd && !kDel && !kInI (frontier = genuine presence GAIN); NetDeleted =
kDel && !kAdd.

```
proc entry(batch):
  ── INGEST ──
  monotone msg:      fold +NR; eager walk (BuildEagerInsertionRegionsImpl),
                     CUT at any successor with CanReceiveDeletions(); the
                     cut boundary accumulates a net-additions frontier.
                     At a NEGATE on the eager path (BuildEagerNegateRegion):
                     gate CHECKMEMBER Present(negated table), continue on
                     ABSENT, fold into the negate's own differential table,
                     crossing → its addQ.                    « forward pass »
  differential msg:  net ±; crossings park in the view's del/add queues.

  ── STRATUM PHASES (BuildStratumPhases), strata ascending ──
  (1) SEEDS (ALL hoisted before the loops — spec §5.0 order restored in
      effect by the claim gates, per F17):
      branch chains: loop source net frontier → chain steps → head fold.
        chain step AT a NEGATE (EmitChainStep): gate on the negated table,
        `+` walk continues iff key ABSENT in InNew, `−` walk iff ABSENT in
        InI (sign PRESERVED, gate dualized).                 « forward pass »
      dual-section joins: pivots from lower frontiers; added section =
        AND(InNew(side)) && OR(NetAdded), removed = AND(InI) &&
        OR(NetDeleted); fold ±R/±NR into the join's instance table.
        « slice 4: an all-same-SCC join has NO seed (suppressed) »
  (2) SINGLE-PASS tables (genuinely acyclic): ClaimDrain del/add,
      FrontierFilter del/add. One drain settles the table.
  (3) recursive-SCC tables (linear AND nonlinear): OVERDELETE claim-round
      INDUCTION (per round: clear Δs → ClaimDrain each SCC table → k-position
      JoinFire over Δ with the §5.1 matrix (j<p permissive / j>p strict;
      lower sides InNew) folding into the join instance table → projection
      SeedLoops over Δ → Retire) → output: REDERIVE (C_r>0 → addQ) →
      INSERT loop (mirror) → output: BOTH frontier filters.

  ── COMMIT ── per touched row: assert C ≥ 0 per class, publish was≠now,
      set kInI := (Total>0), clear scratch flags.
```

Scheduling: ComputeRecursiveSCCs + the stratum-lift fixpoint place every
emission above every table it reads, EXCEPT same-SCC reads (exempt);
`negated_tables_ready` lifts a chain above every negated table it reads
(Stratum.cpp ~1196), so a negated table is ALWAYS phase-final before any
consumer chain runs. EmitSectionWalk asserts no NEGATE inside a join
section walk (~521).

## 2. Negation today: what EXISTS vs what is MISSING (evidence-grounded)

EXISTS (do not rebuild):
- Forward pass, eager path: BuildEagerNegateRegion (Negate.cpp) — Present
  gate, continue-on-absent, differential negate table seeding. Also carries
  an inductive-negate branch (GetOrInitInduction) — see D5 below.
- Forward pass, stratum path: EmitChainStep's dualized InNew/InI gate.
- Scheduling: negated-table readiness lift.
- The (d) DELETION inventory is already EMPTY: MaybeRemoveFromNegatedView /
  MaybeReAddToNegatedView / PivotAroundNegation grep to nothing in lib/ —
  the plan's "(d) Build.cpp:269–405 deleted" was satisfied at (b), like
  FLAG-I at slice 3. Verify with the merge-criteria grep, then move on.
- @never: negated side is a monotone Table, gate is Present, no deltas —
  the eager gate already serves it; (d) must not disturb it.

MISSING (the actual (d) work) — the §5.4 CROSSOVER. Evidence, negate_1
(`out(A,B) : copy(A,B), !seen(A).`, message `unsee(A)` feeds `seen`), IR at
04b17ef, entry proc:

    vector-loop {@A} over $param            « the unsee receive »
      update-count +nonrecursive {@A} in %table:8   « seen — NO if-crossed
                                                      body. Full stop. »

A gained `seen(A)` row updates NOTHING downstream: every `out(A,B)` derived
while A was unseen survives forever (GOLDEN-DIVERGE, all 4 modes). The
dual (differential negated view losing a row ⇒ negate result must GAIN) is
equally absent. This is the entire mechanism of the 11 red cases modulo
per-case decoration (conditions = unit-relation negation; flip-flop =
same-batch cross; compare_4/insert_4/merge_5 = the same seam reached
through other view shapes — the (d) session must classify each).

## 3. (d) as diffs against the §1 pseudocode — SEED HYPOTHESIS, re-derive
##    and critique before coding

**D1 — crossover seeds for a DIFFERENTIAL negated view (spec §5.4).** At
the NEGATE's stratum (its consumers' seed block), two new seed emissions
per NEGATE, delta over the NEGATED view's consolidated frontiers,
SIGN-DUALIZED:

    + vector-loop {key} over negated.net_additions:      « gained ⇒ − »
    +   scan negate.predecessor's table by key            « §3.4 index on
    +     (other positions per the SEED schema)             the negated cols »
    +     update-count −class {pred row} in negate's table
    +       if-crossed → negate's delete_queue
    + vector-loop {key} over negated.net_removals:        « lost ⇒ + »
    +   scan predecessor by key, other positions per seed schema
    +     update-count +class {pred row} in negate's table
    +       if-crossed → negate's add_queue

  Downstream propagation then rides the EXISTING machinery (the negate's
  table is differential; its claim drain + frontier filters already exist).
  Class = the rule's fixed class (kNonRecursive unless the negate is inside
  a recursive SCC — see D5). CRITIQUE POINTS the session must settle by
  hand-trace: (a) what predicate the predecessor scan reads (InI vs InNew —
  MD says "other positions per the seed schema"; the − crossover's
  read and the + crossover's read differ; a wrong read double-fires against
  the forward pass on a same-batch pred-add × neg-add instance — this is
  the §5.1.1-class trap for negation, and the F17 claim gates change what
  is and isn't benign); (b) exactly-once vs the FORWARD pass when the same
  batch adds a pred row AND flips the negated key (phantom-pair analogue);
  (c) whether netting at ingest already prevents same-batch add-then-remove
  of a negated row from reaching the crossover (MD §5.4 says it must).

**D2 — crossover for a MONOTONE negated view (negate_1's shape).** A
monotone negated table has no frontier machinery; its gain is visible only
at the ingest fold. Options the session must weigh: (i) give the eager fold
an if-crossed body that appends to a per-negated-table "gained keys" vector
consumed by the NEGATE's stratum as a net-additions source (mirrors the
monotone-boundary frontier accumulation that already exists for the eager
cut); (ii) make any negated table differential by fiat. (i) is lighter and
matches the existing boundary-frontier idiom; (ii) buys uniformity at
storage cost. Monotone negated views can only GAIN ⇒ only the `−` crossover
arm exists for them.

**D3 — unit relations / conditions (cond_* cases).** No special case per
MD §5.4: the token row `(true)` has counters; a condition flip is a
crossing feeding the same crossover joins. The §3.4 "index" over zero
negated data columns degenerates to a full predecessor scan. Verify
cond_diff_flipflop's same-batch swap nets at ingest (its current
RUN-FAIL(134) is the Commit 0≤nr tripwire — F17's family on the crossover
path; expect the netting + crossover to fix it, but TRACE the swap batch
first: −support(s1)/+support(s2) on a 2-row backing with the token row's
C_nr crossing both ways).

**D4 — negation_flap re-bless.** Its golden changes by the ingest-netting
DECISION (deterministic same-batch outcome replacing dequeue-order), not by
a bug — FINDINGS F16's note and MD §5.5. One reviewed re-bless with the
oracle as referee, exactly like transitive_closure_diff at slice 4.

**D5 — NEGATE inside a recursive SCC (positive recursion, lower negated
table).** Legal post-Stratify (only the NEGATED view must be strictly
lower). Negate.cpp's inductive branch (GetOrInitInduction) belongs to the
legacy eager induction; slice 3's widened cut routes deletion-capable
recursive SCCs to D/R/I, so decide: delete the branch outright (Stratify's
diagnostic dominates the UNSTRATIFIED case; the stratified-inductive case
should already route to stratum phases via the cut) — but VERIFY with a
fixture (recursive rule carrying a !lower atom) before deleting; none of
the 11 red cases covers this shape, so author one.

**D6 — oracle parity.** Slice-4 carried concern: the oracle
(bin/Oracle/Main.cpp:1285) HARD-FAILS on a del claim with C_nr>0 where the
runtime now gate-drops; agreement holds only because the oracle uses the
spec's interleaved order. The (d) session should decide (and record)
whether the oracle mirrors the gates — BEFORE authoring any crossover
goldens from oracle truth, since crossover cases are where a divergence
would first bite.

## 4. Desired end state

- Suite: 128+ cases, SUITE: PASS — the 11 needs-(d) names green (negate_1,
  negate_3–6, negation_flap, cond_diff_flipflop, cond_both_polarities,
  compare_4, insert_4, merge_5), zero regressions elsewhere (the slice-4
  regression net: tc_*, nonlin_*, linear_rec_downstream, deep_chain,
  firewall_cycle, diamond_reenqueue, two_hop_phantom, two_base_flip_*).
- negation_flap: one reviewed re-bless (D4), oracle-refereed.
- New fixtures: D5's recursive-negation shape; a monotone-negated (D2) and
  differential-negated (D1) crossover discriminator each if the existing 11
  don't already pin them (classify first).
- Checkpoint (e) (commit sweep / publish / query filter / final deletion
  sweep) remains after (d); do not scope-creep into it.

## 5. Session bootstrap (fresh-session checklist)

- Read: this file top-to-bottom; checkpoint-c-notes.md "Whole-program
  view" + "Slice 4 landing record"; plan.md Stage 3 (d)+(e); MD §3.4,
  §5.4, §5.5, §8; FINDINGS.md F15 (the condition-flip history — its fix
  predates the counter swap and its machinery may be partially superseded)
  and F17.
- Method (the slice-4 method, which worked): BEFORE coding, dump the
  ACTUAL IR of at least negate_1, negate_3, cond_diff_flipflop,
  negation_flap; re-derive §1's pseudocode from the code; write the (d)
  diffs against it; adversarially critique them (the D1 critique points
  minimum); write the DESIRED emitted-IR shape for negate_1 and
  cond_diff_flipflop concretely; hand-trace the §5.4 crossover batches
  (incl. the same-batch phantom analogues) against that planned IR with
  the claim gates in force. Only then implement.
- Verification gates: full suite vs the 11-name red baseline (comm both
  directions); ctest (MiniDisassembler pre-existing red — flag any
  direction change); data/ corpus 4-mode sweep; goldens change ONLY via
  the D4 re-bless + new-case authoring (4-mode byte-agreement + oracle
  agreement, never to silence a red).
- Environment: export PATH="/Users/pag/Code/.brew/bin:$PATH" before any
  test run. Suite: DR=build/debug/bin/drlojekyll tests/OptDiff/runall.sh
  <workroot> [jobs] [filter]. macOS bash 3.2 — no declare -A.
