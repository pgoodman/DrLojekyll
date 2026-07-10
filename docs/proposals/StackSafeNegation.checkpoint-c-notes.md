# Stage 3 checkpoint (c) working notes (committed working ledger)

Branch: derivation-counters. Slices 0–4 landed (slice 3 = 6faf6d9; slice 4 =
uncommitted working-tree diff, see the "Slice 4 landing record" at the very
bottom — it supersedes the "Slice 4 as diffs" plan sections where they
disagree). Remaining before (c) closes: the needs-(d) crossover set only.
Delete this file from the repo when checkpoint (c) lands (the durable
record belongs in StackSafeNegation.plan.md).

Seeded from the adversarial review of the checkpoint-(c) recursion plan
(the projected post-(c) `tc_mixed_batch` IR, reconciled across the
counters / termination / reads verifier lenses against MD §5–§5.3, plan
errata item 2, and `Stratum.cpp` at HEAD).

## Review verdict (one paragraph)

The **plan** (`.plan.md` §(c), the notes' increment-4/5 hooks) is
directionally sound — the D→R→I triple, claim rounds, REDERIVE-as-counter-
read, deletion of the add-only re-entry loop, the DerivClass rule, and the
one-D/R/I-pass-per-SCC licence are all correctly scoped and consistent with
MD. But the **specific lowering** in the simulation is **unsound as
written** on four independent grounds (two tracing back to plan text that
would actively mislead a coder), so the plan has holes to patch in
`.plan.md`/notes before (c) is coded, not merely implementation nits. The
four blockers: **B1** the base rule `t:edge` emits no `%table:4` seed fold
(missing region ⇒ wrong golden); **B2** the recursive-rule seed is
mislabeled `kNonRecursive` (must be `kRecursive` per errata item 2 —
tc's golden passes under both, a false negative; a double-derivation trace
drives `C_nr` to −1 and SIGABRTs at commit); **B3** the fixpoint fires over
the drained queue `$t_delta`, not the per-round claimed set `Δ_D` (a
diamond re-enqueue double-steps downstream `C_r` and the queue-emptiness
break can live-lock; tc's length-1 chain never re-enqueues, so tc is
blind); **B4** the recursive seed reads `%table:4` via the (b) JoinEmission
path and trips the `ready_after` build assert in a debug build. No DF-IR
changes (the DF graph already carries both rules into `t`; the hole is
entirely in the CF lowering). These land as amendments A1–A8 in `.plan.md`,
`StackSafeNegation.md` §5.5, and `checkpoint-b-notes.md`.

## Corrected CF-IR shape for tc (review §3, verbatim)

Folds in B1 (base-rule seed regions), B2 (recursive seed class =
`recursive`), B3 (per-round claimed frontier `$t_ΔD`/`$t_ΔA`; firing,
break, and retire all range over it). B4 is a build-assert scoping fix, not
an IR shape change, so it does not appear in the emitted IR — it is A4.
`+` = added vs the simulation; `!` = **corrected** vs the simulation's line.

```
+   vector-define $t_ΔD:208<u64,u64>       « per-round CLAIMED frontier (B3) »
+   vector-define $t_ΔA:209<u64,u64>       « per-round CLAIMED frontier, add side (B3) »

    ══ OVERDELETE(t) : seed ══
+   vector-loop {@bx,@bto} over $net_removals:41        « B1: BASE rule t:edge seed »
+     update-count -nonrecursive {@bx,@bto} in %table:4 « base rule ⇒ class NR (errata 2) »
+       if-crossed: vector-append into $t_delQ:200
    vector-loop {@ex,@eto} over $net_removals:41 → pivot X → join %table:12 removed-section
      check-member InI {@From,@X} in %table:4                        « seed same-stratum read = InI »
!       update-count -recursive {@From,@To} in %table:4  « B2: RECURSIVE rule seed ⇒ class R »
          if-crossed: vector-append into $t_delQ:200
    loop:                                               « OVERDELETE fixpoint »
      vector-unique $t_delQ:200 ; vector-swap $t_delQ:200 <-> $t_delta:204
      vector-clear $t_ΔD:208
      vector-loop {@d1,@d2} over $t_delta:204
        claim-del {@d1,@d2} in %table:4
!         if-claimed: vector-append into $t_ΔD:208 ; vector-append into $t_D:202  « B3: claimed→ΔD and D_s »
!     if empty $t_ΔD:208: break                         « B3: break on CLAIM progress, not queue »
!     vector-loop {@dFrom,@dX} over $t_ΔD:208            « B3: fire over CLAIMED set only »
        scan edge %index:11 col X=@dX → check-member InNew {@dX,@To} in %table:8
          if-member: update-count -recursive {@dFrom,@To} in %table:4
            if-crossed: vector-append into $t_delQ:200
!     retire-del $t_ΔD:208 in %table:4                   « B3: retire ranges over ΔD »

    ══ REDERIVE(t) ══  vector-loop over $t_D:202 → check-member recursively-supported in %table:4
                         if-member: vector-append into $t_addQ:201     « unchanged, correct »

    ══ INSERT(t) : seed (mirror; $t_addQ already holds REDERIVE output) ══
+   vector-loop {@bx,@bto} over $net_additions:45 → update-count +nonrecursive {@bx,@bto} in %table:4  « B1 base »
    vector-loop over $net_additions:45 → pivot X → join %table:12 added-section
      check-member InI {@From,@X} in %table:4
!       update-count +recursive {@From,@To} in %table:4  « B2: recursive rule seed ⇒ class R »
    loop: … claim-add → $t_ΔA:209 + $t_A:203 ; if empty $t_ΔA break ; fire over $t_ΔA ; retire-add $t_ΔA
    ══ BUILDFRONTIERS(t) ══  (unchanged: outDel from $t_D∧!kAdd, outAdd from $t_A∧!kDel)
```

Everything else (the (b) ingest/edge phase, commit sweeps, the deletion of
the HEAD one-shot `%table:17` seed) is as the simulation had it and correct.

## Implementation order (review §5, 5 slices)

Respect Risk #3 (no resurrecting deleted mechanisms — the add-only re-entry
loop, MODESWITCH, kInductionRechecks, BuildUnknownRecheck stay dead) and
Risk #2 (traces re-run by hand before code).

0. **Settle the plan text (no code):** apply A1, A2, A3, A4 to
   `.plan.md`/notes. Documentation holes that would mislead a coder; A2 in
   particular — a coder following the simulation ships B2. Done when errata
   item 2, the multi-rule-head rule, the `Δ_D` discipline, and the
   `ready_after` scoping decision are written down. (This slice = the
   present doc task.)
1. **Vocabulary + del-side allocation:** land A5 (`kRecursivelySupported`
   through the ten-site threading list) and A6 (del-side vectors incl.
   `$t_ΔD`/`$t_ΔA` for recursive tables). Pure additive plumbing, no
   behavior change. Verify: build green all 4 opt modes; grep new
   predicate/vector through printer + codegen; OptDiff still byte-identical.
2. **[DONE 2026-07-10 — see "Slice 2 landing record" below; uncommitted]**
   **Correctness core — OVERDELETE + REDERIVE for tc:** emit the base-rule
   seed (A1, class NR), the recursive-rule seed (A2, class **R**), the
   claim-round loop firing/breaking/retiring over `$t_ΔD` (A3), and REDERIVE
   over `$t_D` reading `kRecursivelySupported`. Route the seed join per A4
   (exempt-and-document, split the assert). Do not touch INSERT beyond the
   seed; do not delete the re-entry loop yet. Verify: the §5.1.1 batch
   (`−edge(1,2), +edge(2,5)` on `edge={(1,2),(1,4),(4,5)}`) vs golden
   `{(1,4),(1,5),(2,5),(4,5)}`; then the counters-lens **double-derivation**
   trace (the one-batch repro that distinguishes correct-R from wrong-NR —
   drives `C_nr` to −1 → commit SIGABRT under the bug; highest-value single
   test for A2/A3).
3. **[RESHAPED by slice 2 — the INSERT claim loop already landed there; the
   remaining slice-3 scope is listed in the landing record below]**
   **INSERT + BUILDFRONTIERS + delete the add-only loop:** mirror the
   OVERDELETE claim loop for the add side; wire BUILDFRONTIERS into the
   existing `outDel`/`outAdd`; only now delete `EmitInduction`'s
   `for(bool changed…)` re-entry loop — but first confirm against
   `deep_chain_retract` whether it still emits a `ProgramInductionRegion`.
   Verify: `deep_chain_retract` at N=100000 in constant stack (the
   acceptance gate); full OptDiff recursive-differential cases go green.
4. **Close the coverage gaps — matrix fixtures:** after tc + deep_chain are
   green, bring up `p(X,Z):p(X,Y),p(Y,Z)` and the `p(1)/p(2)` trace. First
   real exercise of FLAG-F/G/H and the same-round exactly-once machinery,
   and where A3's `Δ_D` discipline is stress-tested on genuine re-enqueue.
   Re-run the 2-same-stratum-atom trace by hand (Risk #2) before coding the
   third join-section flavor.

## Open flags / unknowns (review §6 — only running code settles these)

- **FLAG-F — RESOLVED 2026-07-09** (method: positive-only equivalence oracle
  + hand-trace). `InNewWithFrontier` is an intentional **alias** of `InNew`,
  not a typo: `InNewWithFrontier(id) := (kInI && !kDel) || kAdd`, byte-for-byte
  `InNew`. The name marks the INSERT fixpoint same-stratum `j < i` read site;
  it may be implemented as a call-through to `InNew`. Both disjuncts are
  load-bearing at that site: dropping `(kInI && !kDel)` (reading R-iv) zero-fires
  a head whose earlier same-stratum sibling is a prior-batch `kInI` row not
  re-added this round (fails on the `nonlin_tc_both_change` batch-4 mixed
  old-`kInI`/new-`kAdd` case); restricting `kAdd` to `kAddNow` (reading R-ii)
  under-counts `C_r` for a head double-derived in one round (fails the
  per-class counter assert on the 3-hop same-round separator `+7-8 +8-9 +9-10`,
  where C_r(7,10) truth = 2 and the retraction of edge(8,9) drains cleanly to
  absent under R-i but drives C_r negative under R-ii). The same-round
  exactly-once bit lives ONLY in the `j > i` cell (`InNewSansFrontier`'s
  `!kAddNow`), never in the `j < i` cell — so `InNewWithFrontier` correctly
  carries no `kAddNow` term. Evidence: `docs/proposals/StackSafeNegation.evidence/flag-f-resolution.md`
  (full trace); confirmed by oracle `sep1`/`sep2` (334/589 assertions clean,
  p(7,10) present then correctly absent) and the `nonlin_tc_both_change`
  fixture (1410 assertions, monotone agrees). Was inert on tc (one same-stratum
  atom); now exercised by `tests/OptDiff/cases/nonlin_tc_both_change`.
- **FLAG-H — RESOLVED 2026-07-09** (method: positive-only equivalence oracle
  + hand-trace). The third (fixpoint) join-section flavor is **k separate
  emissions per recursive-stratum join, one per same-stratum delta position**
  (k = number of same-stratum body atoms), each pinning the delta at its
  position over the per-round CLAIMED frontier (`Δ_D`/`Δ_A`) and reading every
  OTHER same-stratum atom with a predicate fixed at compile time by its static
  position relative to the delta (`j < p` → `SurvivesSoFar`/`InNewWithFrontier`;
  `j > p` → `AliveAtClaim`/`InNewSansFrontier`; lower `j` → `InNew`). It is NOT
  a single emission with dynamic per-side dispatch: an `added_body`/`removed_body`-
  style per-side-symmetric predicate double-fires a same-round double-claim
  instance (both sides in Δ enumerated under both drivers), driving `C_r`
  negative in OVERDELETE (commit SIGABRT) or inflating multiplicity in INSERT
  (phantom survival on a later single retraction). The same-round exactly-once
  guarantee is delivered structurally by the strict/permissive predicate
  asymmetry keyed on `kDelNow`/`kAddNow` (earlier-as-delta fires via the
  permissive later-position read; later-as-delta does not, the earlier position
  failing the strict read) — orthogonal to, and additionally requiring, the
  `Δ_D`/`Δ_A` claim discipline (A3) so a diamond re-enqueue of an
  already-claimed row cannot re-fire across rounds. This is FixReads' shape in
  the oracle (`for p : same_pos` with `FixReads(r,p,deleting)` a pure function
  of `p`), independent of FLAG-F. Evidence: `docs/proposals/StackSafeNegation.evidence/flag-h-resolution.md`
  (batch-2 both-deleted p(2,4) and batch-3 both-added p(4,6) traces); confirmed
  by `nonlin_tc_both_change` (k=2), `diamond_reenqueue` (Δ_D cross-round axis),
  and the `k3_join` attack case (k=3 same-round triple-claim, 460 assertions,
  monotone agrees). Was inert on tc; now exercised by the OptDiff fixtures below.
- **Adversarial re-check (2026-07-09): resolutions survive.** ~1040 seeded
  stress runs (k=2/k=3, diagonal self-loops, self-supporting cycles,
  diamonds, phantom-support pairs, REDERIVE/firewall) plus 10 hand-traced
  attack fixtures produced zero wrong final IDBs, zero counter-exactness
  violations, and zero aborts; every attack under the resolved semantics
  agreed with the oracle differential + `--project-monotone`. Evidence:
  `docs/proposals/StackSafeNegation.evidence/flag-attack.md`. Caveat: the oracle IS the resolved semantics
  for the differential path, so the independent leverage is the from-scratch
  full-join counter cross-assert (delta=-1) and the positive-only projection;
  a bug invisible to BOTH is outside the kill criteria and unaddressed here.
  Fidelity of the eventual `lib/` lowering to this resolved semantics is a
  separate slice-2/4 bring-up question.

### Queued proposal edit (owner review — do NOT self-apply to StackSafeNegation.md)

`StackSafeNegation.md` is the authoritative spec; the FLAG-F resolution wants
a wording annotation on the §5.1 fixpoint table so no future reader re-files
FLAG-F. Proposed (owner to apply): change the `same j < i` INSERT cell from
bare `InNewWithFrontier` to
`` `InNewWithFrontier` (≡ `InNew`; the earlier-position add-side read is the
plain final-so-far predicate — the same-round exactly-once bit lives only in
the `j > i` cell) ``, and add one line under the table:

> `InNewWithFrontier` is defined identically to `InNew` and may be implemented
> as an alias; it is named distinctly only to mark the INSERT fixpoint `j < i`
> read site. As in the OVERDELETE column — where the exactly-once frontier bit
> `kDelNow` appears in the *later*-position cell (`AliveAtClaim`) — the INSERT
> column's frontier bit `kAddNow` appears only in the *later*-position cell
> (`InNewSansFrontier`). The earlier-position cells (`SurvivesSoFar`,
> `InNewWithFrontier`) carry no frontier bit. Dropping the `(kInI && !kDel)`
> disjunct from `InNewWithFrontier`, or restricting its `kAdd` disjunct to
> `kAddNow`, are both unsound (lost derivations / under-counted `C_r` — see the
> nonlinear both-added and two-support-retraction traces).

### Promoted standing OptDiff fixtures (2026-07-09)

Four fixtures committed to `tests/OptDiff/cases/` with `.dr` + `.batches` +
`.main.cpp` (per-batch send + final state dump, tc_mixed_batch pattern) and
`goldens/<name>.oracle.stdout` / `.monotone.stdout` authored from the
positive-only oracle:

- `nonlin_tc_both_change` — nonlinear TC, both same-stratum positions change
  per batch. THE FLAG-F (R-iii/R-iv/R-ii kill) + FLAG-H (k=2 same-round
  double-claim) discriminator. Final `reachable={(4,5),(4,6),(4,7),(5,6),(5,7),(6,7)}`.
- `diamond_reenqueue` — Δ_D cross-round claim discipline; the re-enqueue axis
  FLAG-H needs that nonlin does not cover. Final `q_out={}`.
- `firewall_cycle` — REDERIVE drain vs `C_nr>0` firewall refusal (FLAG-E
  adjacent, mandated (c) companion). Final `p_out={3,4}`.
- `recursive_to_downstream` — BUILDFRONTIERS producer→consumer seam. Final
  `flagged_out={4}`.

**GOLDEN-MISSING (deliberate).** The compiled-driver goldens
(`goldens/<name>.stdout`) are ABSENT for all four: these cases are red until
(c) wires recursive-stratum OVERDELETE/BUILDFRONTIERS, and the drivers dump
intermediate per-batch state that the oracle final-state truth does not
provide, so a mechanically-correct `.stdout` cannot be derived now. They must
be authored at slice-2/3 bring-up once the compiled binary is correct. Until
then `runall.sh` will report `GOLDEN-MISSING` on the compiled step for these
four — an accepted red-until-(c) verdict; the oracle/monotone goldens are the
authoritative end-state gate meanwhile.

- **Whether `deep_chain_retract` still emits a `ProgramInductionRegion`**
  — **RESOLVED 2026-07-10 (slice 3):** it DID, and that was the bug, not the
  re-entry loop. See the slice-3 landing record: the legacy eager induction
  wrongly claimed ownership of the differential recursion (mis-routing via
  the monotone lower atom); post-fix the case emits zero legacy
  `check-member present` induction loops and is fully D/R/I-owned.
- **How much of the §(c) deletion inventory is already satisfied at (b)**
  (FLAG-I) — **RESOLVED 2026-07-10 (slice 3): ALL of it.** The
  `for (bool reenterN...)` re-entry wrapper was deleted at checkpoint (a)
  (`git log -S reenter` pins it to efacc67); `BuildUnknownRecheck`,
  `kInductionRechecks`, MODESWITCH all grep to nothing in lib/+include/.
  The sole surviving `for (bool changedN...)` (Database.cpp:1875) is the
  slice-2 A3 Δ-emptiness break and stays. Nothing to delete at (c).
- **The A4 `ready_after` decision's downstream effects** — **RESOLVED
  2026-07-10 (slice 3)** for the LINEAR shape: the standing fixture
  `linear_rec_downstream` crosses the producer→downstream-differential seam
  in both directions plus the overdelete-then-rederive no-flap survivor,
  green in all 4 modes and oracle-matched. The NONLINEAR seam
  (`recursive_to_downstream`) stays GOLDEN-MISSING until slice 4.
- **Multiset exactness under long batch sequences** — the counters lens's
  multi-batch corruption traces are hand-derived on ≤3 batches; net-exactness
  over arbitrary batch streams is a fuzz/differential-vs-oracle property.

## A4 owner decision (2026-07-09)

Recursive-stratum seed reads vs the (b) `ready_after` build assert
(Stratum.cpp:681/687/696): resolution **(ii)** — REUSE the (b) JoinEmission
path and **SPLIT the assert by emission-site kind**. Seed emissions must
still read strictly-lower strata **excluding** same-SCC reads (which are
`InI` batch-frozen, or lower-table `InNew`, never a drain-order dependency,
so they close no scheduling cycle); claim-loop emissions must have ≥1
same-SCC read. The `kNonRecursive` soundness comment (`Stratum.cpp:670–682`)
survives for base-rule seeds. The assert split is (c) implementation work.

## Slice 2 landing record (2026-07-10 — UNCOMMITTED, pending owner review)

The slice-2 diff sits uncommitted in `lib/ControlFlow/Build/Stratum.cpp`
(+884/−88, the only changed file; goldens untouched). Method: workflow with
an opus implementer, an independent adversarial opus spec-fidelity reviewer
(re-derived requirements from these docs, audited the diff, hand-traced
double-derivation and a diamond re-enqueue through the actual emitted IR),
and a sonnet full-suite regression gate. Converged on round 3:
**review = approve, 0 blockers; suite = zero regressions vs the HEAD
baseline; 2 cases newly green.**

### What landed (anchors are post-diff Stratum.cpp)

- `EmitClaimDrain` (~588): claim's `if-claimed` body appends the row to BOTH
  the accumulated set (`kOverdeleteSet`/`kAdditionSet` = `D_s`/`A_s`) and the
  per-round claimed frontier (`kClaimedDeleteFrontier`/`kClaimedAddFrontier`
  = `Δ`).
- `EmitRetireFrontier` (~668): first RETIRE call site; ranges over `Δ`.
- `EmitRederive` (~685): loop over `D_s` → CHECKMEMBER
  `kRecursivelySupported` → append to the add queue. First builder use of the
  predicate (IR/printer/codegen landed at (a)).
- `EmitJoinFire` (~789): the fixpoint fire. For a linear recursive JOIN,
  deltas over the same-SCC side's `Δ`, scans each lower atom by pivot via
  `BuildMaybeScanPartial` gated at `kInNew` (MD §5.1 fixpoint schema, lower
  `j<i` → `InNew`, both signs), folds `∓recursive` into the join's table.
  `is_del`-parameterized (serves OVERDELETE and INSERT).
- `build_claim_round_loop` (~1517): one signed claim-round fixpoint realized
  as an INDUCTION region whose tested `vectors` are exactly the per-round `Δ`
  frontiers, so codegen's `for (changed; …)` IS the Δ-emptiness break (A3),
  not queue-emptiness. Round body: clear Δ → drain+claim (→Δ+accumulated) →
  fire over Δ (join fire + projection re-fire) → retire Δ.
- SCC block (~1598): OVERDELETE loop → output region builds net-removal
  frontiers + REDERIVE → INSERT loop (mirror) → output builds net-addition
  frontiers.
- Round-0 same-SCC internal projection seeds are SKIPPED (they fire only
  inside the loop, over `Δ`); seeding them over the accumulated net frontier
  double-counted every internal derivation per round (real double-decrement
  bug found and fixed during bring-up).
- A4 assert split by emission-site kind, per the owner decision above; the
  stale "no REDERIVE / recursive counter identically zero" doc comment is
  rescoped to single-pass tables.

### Verification evidence

- Build green, all 4 opt modes; no new warnings.
- IR diff: every `+`/`!` line of the corrected CF-IR shape (above, lines
  44–79) accounted for in the emitted tc IR.
- §5.1.1 gate: `tc_mixed_batch` (which IS the §5.1.1 fixture — its .batches
  is exactly `−edge(1,2), +edge(2,5)` on `edge={(1,2),(1,4),(4,5)}`) now
  passes its committed golden BYTE-EXACT in all 4 modes; was GOLDEN-DIVERGE
  at baseline. `t(1,5)` (untouched-support survivor) correctly present.
- Double-derivation gate (the A2 discriminator): scratch fixture below runs
  clean (`t={(1,2)}` final, oracle-agreed). Mutation test: flipping the
  recursive seed's class to kNonRecursive and rebuilding reproduces
  `Assertion failed: (0 <= nr), Table.h:397` — the predicted C_nr→−1 commit
  SIGABRT — proving the gate has teeth; flip reverted (git-verified clean).
- Full OptDiff: 21 red case-names (from 22 at baseline); FIXED =
  {cf15_5, tc_mixed_batch}; NEW regressions = ∅ (comm-diff of red-set names);
  every remaining red byte-identical in per-mode verdict to baseline.
  490/490 oracle+monotone sub-checks OK. ctest identical to baseline
  (MiniDisassembler's pre-existing failure only). data/ corpus: 0 hangs,
  0 crashes, only the 4 known feature-gap diagnostics.

### Deviations from the slice plan — OWNER TO RATIFY at commit time

1. **The INSERT claim-round loop landed in slice 2** (plan said "do not
   touch INSERT beyond the seed"). Forced by coupling: with A2 correctly
   class-R, OVERDELETE emits `-recursive` folds whose derivations only
   INSERT-side recursion ever creates; without the symmetric add loop those
   counters go negative → commit SIGABRT (observed on cf15_5/cf16_3 in fix
   round 2). The add loop is the structural mirror
   (`build_claim_round_loop(is_del=false)`), no new primitives, and is what
   makes tc_mixed_batch's golden actually pass. Alternative (strict split):
   revert A2 to kNonRecursive everywhere = HEAD behavior, abandoning the
   slice's point.
2. **Linearity gate** (~1010, `nonlinear_groups`/`is_linear_recursive`):
   SCCs with ≥2 same-SCC join sides (`p(X,Z):p(X,Y),p(Y,Z)`) stay in
   `recursive_sccs` for scheduling (same-SCC read exemption + shared-stratum
   pinning are termination requirements) but are FOLDED kNonRecursive and
   lowered single-pass (pre-A2 fallback). Without it the scheduling lift
   fixpoint diverged (compiler hang). Consequence: the resolved FLAG-F/H
   matrix semantics are exercised by NO green fixture until slice 4; the
   nonlinear cases match baseline exactly (GOLDEN-DIVERGE/MISSING, no crash).
3. **A4's "reuse the (b) JoinEmission path" held only for the round-0 seed
   join.** The fixpoint fire needed new machinery (`EmitJoinFire`): the seed
   join's `all-in-i`/`all-in-new` section predicates are the wrong read
   discipline for a fire that deltas over `Δ` while reading the lower atom
   at `InNew`.

### Reviewer concerns (approve verdict; carry into slice 3)

- **Frontier ordering (slice 3's first task):** the del-side net-removal
  frontier is built inside the OVERDELETE induction's output region, BEFORE
  the INSERT loop runs, so `NetDeleted = kDel && !kAdd` is evaluated before
  INSERT can set kAdd on a re-added row. Spec §5.0 puts BUILDFRONTIERS after
  INSERT. A row overdeleted-then-re-added within one batch would leak into
  `net_removals`. No observable failure exists today (linear tc never
  overdeletes a re-added row — instrumented: tc_mixed_batch's D_s = {t(1,2)}
  only; the cyclic case that would trip it is behind the linearity gate),
  but it is known-wrong ordering. The add-side frontier is correctly placed
  (after INSERT).
- **INDUCTION-as-loop-vehicle idiom:** the claim-round loop is an INDUCTION
  region with empty `views`/`view_to_add_vec`, carrying only the Δ vectors.
  Verified the CF optimizer leaves it intact (null-init hoist + no-op output
  clear are both no-ops here) and codegen emits the exact loop shape. Worth
  a dedicated loop-until-Δ-empty region if the pattern proliferates.
- **Cyclic self-supporting recursion still over-retains**
  (`transitive_closure_diff2`, `cf16_3` — GOLDEN-DIVERGE, == baseline, no
  crash). Per MD §8 a pure cycle must drain to C_r=0 when external support
  is gone, so this is lowering infidelity (or the linearity gate's fallback),
  not a model hole — but it is NOT solved by slice 2. Owner call whether it
  gates checkpoint (c) or lands with slice 4's matrix.

### Reshaped slice-3 scope (what actually remains)

1. Move del-side BUILDFRONTIERS after the INSERT loop (ordering fix above).
2. Confirm `deep_chain_retract` still emits a `ProgramInductionRegion`
   (slice-2 evidence: Induction.cpp untouched, the re-entry loop survives),
   then delete the add-only re-entry loop; acceptance gate:
   `deep_chain_retract` at N=100000 in constant stack.
3. A LINEAR recursive→downstream-differential fixture for the A4
   `ready_after` seam (`recursive_to_downstream` covers the seam but is
   nonlinear, hence red until slice 4).

### Double-derivation repro (scratch; volatile — preserved here verbatim)

The A2 discriminator fixture (`dbl.dr` / `dbl.batches` / `dbl.main.cpp`,
tc_mixed_batch driver pattern). Worth promoting to a standing OptDiff case
at slice-3/4 bring-up.

`dbl.dr`:
```
#message edge_msg(u64 From, u64 To) @differential.
#local edge(From, To).
#local t(From, To).
#query t_out(free u64 From, free u64 To).
edge(From, To) : edge_msg(From, To).
t(From, To) : edge(From, To).
t(From, To) : t(From, X), edge(X, To).
t_out(From, To) : t(From, To).
```

`dbl.batches` (seed makes `t(1,3)` double-derived — base `edge(1,3)` AND
recursive `t(1,2),edge(2,3)`; the batch removes both supports; expected
final `t={(1,2)}`):
```
batch
+ edge_msg 1 2
+ edge_msg 2 3
+ edge_msg 1 3
end
batch
- edge_msg 1 3
- edge_msg 2 3
end
```

Driver: tc_mixed_batch's `main.cpp` pattern with `t_out_ff()` dumped sorted
after seed and after the batch (`seeded:` / `after:` labels).

### Environment note

`timeout`/`gtimeout` were missing from PATH (macOS; Homebrew at the
non-standard prefix `/Users/pag/Code/.brew` was unlinked). coreutils is now
installed and symlinked (`/Users/pag/Code/.brew/bin/{timeout,gtimeout}`);
diffrun.sh/runall.sh fail with DR-FAIL(127) without it. Fresh shells may
need `export PATH="/Users/pag/Code/.brew/bin:$PATH"`.

## Slice 3 landing record (2026-07-10 — committed; deviations ratified)

The slice-3 diff sits uncommitted: `lib/ControlFlow/Build/Build.cpp` +
`lib/ControlFlow/Build/Stratum.cpp` (+56/−15 across the two), plus two new
standing OptDiff cases with all goldens (6 files under cases/, 6 under
goldens/). Method: two workflows (the
first blocked on a discovered pre-existing defect, the second fixed it) —
opus implementers, opus fixture author, independent opus adversarial
spec-fidelity reviewer, sonnet full-suite gate. Converged round 1:
**review = approve, 0 blockers; gate = pass, zero regressions.**

### What landed

- **T1, frontier ordering (Stratum.cpp SCC block):** the del-side
  net-removal frontier build MOVED out of the OVERDELETE loop's output
  region into the INSERT loop's output region — `del_output` now holds ONLY
  REDERIVE; both signed frontier filters sit together in `add_output`
  (is_del=true then is_del=false), matching spec §5.0
  (OVERDELETE→REDERIVE→INSERT→BUILDFRONTIERS). Closes the slice-2 reviewer's
  "known-wrong ordering" concern: an overdeleted-then-re-added row now has
  kAdd final before outDel's !kAdd test. Reviewer hand-traced t(1,5) in
  tc_mixed_batch (in both D_s and A_s ⇒ excluded from BOTH frontiers,
  correctly stays present) and confirmed no net_removals consumer sits
  between the old and new build points.
- **T2 resolved as FLAG-I already-satisfied + a real routing bug found
  instead.** The re-entry loop was deleted at (a); see the RESOLVED flags
  above. The actual reason `deep_chain_retract` was red: **mis-routing.** In
  `BuildEagerInsertionRegionsImpl` (Build.cpp:754 post-diff), the eager walk
  launched by a MONOTONE receive (`next`) hit the differential recursive
  JOIN successor and — because the cut excluded views with an
  `InductionGroupId` — built a legacy eager `ProgramInductionRegion` whose
  fixpoint is the old monotone `check-member present` loop. That made the
  head's table `TableIsInductionOwned`, so `BuildStratumPhases` skipped it;
  the base seed folded into other tables and the gate table was never
  written ⇒ fixpoint fired 0 times ⇒ count=0. DataFlow partitioning was
  ruled out (identical SET/DEPTH tags vs the working all-differential dbl
  shape); isolation fixtures proved the discriminator is the
  monotone-vs-differential JOIN input, NOT head arity. FIX: widen the cut to
  plain `if (succ_view.CanReceiveDeletions())` — a deletion-capable
  recursive SCC is ALWAYS owned by D/R/I (MD §7), and monotone inductions
  are unaffected (they cannot receive deletions). Only ADDS cuts, never
  removes one.
- **Semi-naive claim-drain (Stratum.cpp `EmitClaimDrain`, VECTORCLEAR at
  ~675):** pre-existing O(N²) — the claim round re-sort-unique-scanned the
  entire never-cleared add/delete queue every round. Now the drained queue
  is destructively cleared inside the claim-round loop (only there;
  acyclic single-pass drains untouched); the fire re-appends only
  newly-crossed rows. Claim-set-identical (CLAIM dedups on table state, so
  diamond re-enqueues stay no-ops). deep_chain at depth 16000: 29s → 0.22s.
- **Fixtures (all goldens authored: compiled + oracle + monotone each,
  compiled only after 4-mode byte-agreement AND oracle match):**
  `linear_rec_downstream` — the LINEAR A4-seam case (one same-SCC atom,
  IR-confirmed live on the claim-round machinery: 94 claim/rederive markers,
  zero legacy induction loops); chain 10→1→2→3→4 plus diamond 1→5→3; its 5
  batches exercise removal-direction drain, addition-direction rederive, and
  the batch-3 no-flap survivor (which is also the runtime discriminator for
  T1). `tc_double_derive` — the notes' double-derivation repro promoted
  verbatim (the A2 discriminator; seeded t={(1,2),(1,3),(2,3)}, after
  t={(1,2)}).

### Verification evidence

- Build green all 4 opt modes, no new warnings.
- **The slice acceptance gate: met.** `deep_chain_retract` green vs its
  untouched committed golden (depth 6000) in all 4 modes; at kChainDepth=
  100000 (scratch driver): `after seed: count=100001 min=0 max=100000`,
  `after retract: count=0`, exit 0, default 8 MB stack, ~0.3 s release /
  0.65 s debug with asserts + `DiffTable::DebugValidateCounts` clean; flat
  scaling 0.24/0.26/0.29 s at depth 25k/50k/100k (drain fix is O(N)).
- **`disassemble` flips green too** (all 4 modes, stable) — a real-world
  program with the same monotone-lower-atom differential recursion.
- Full suite: red set = 19 names = the 21-name slice-2 baseline −
  {deep_chain_retract, disassemble}; **zero new regressions**; every
  remaining red has a per-mode-identical verdict KIND to baseline
  (cond_diff_flipflop still RUN-FAIL(134), tc_nonlinear_diff still
  RUN-FAIL(1)); the 4 deliberate GOLDEN-MISSING fixtures stay red until
  slice 4. Filtered runall.sh over the two new cases: SUITE: PASS, 12/12
  verdicts (4 modes + oracle + monotone each).
- ctest: PointsTo + Runtime pass; MiniDisassembler.DifferentialUpdatesWork
  still fails but is pre-existing on the baseline compiler; the fix ADVANCES
  it (0-vs-1 → 1-vs-6 rows materialized). Corpus: 144 compile runs, only the
  known feature-gap diagnostics + the pre-existing evm_array_parse 134.
- tc_mixed_batch byte-exact all 4 modes; dbl repro debug-clean; cf15_5 and
  two_hop_phantom spot-checked green.

### Deviations — RATIFIED by owner 2026-07-10

1. **The 100k gate required an edit inside slice-2 machinery** (the
   `EmitClaimDrain` destructive clear). Pure performance; reviewer verified
   the clear sits between drain and fire (nothing appends in that window)
   and the claim set is provably identical. Flagged only because it touches
   a slice-2 anchor rather than pure routing.
2. **Do NOT bump the committed deep_chain_retract to 100000 yet.** Its
   golden is depth-dependent (kChainDepth in the driver main.cpp:16). Now
   that the case is green, options: bless a depth-100000 golden, or keep the
   reduced committed depth and carry the 100k driver as the standing
   acceptance-gate artifact (the plan's original intent, .plan.md:362).
3. `disassemble` flips green as a side effect (strictly good news; recorded
   so the red-set arithmetic is explained: 21 − 2 = 19).

### Reviewer concerns (approve verdict; carry into slice 4)

- **Nonlinear seam of the widened cut:** a NONLINEAR differential SCC
  reached via a monotone lower atom now routes to the single-pass
  linearity-gate fallback instead of the eager induction. No failing case
  exists (nonlinear cases unchanged vs baseline), but slice 4's matrix work
  must revisit this path deliberately.
- **The destructive drain assumes no post-loop queue consumer.** True today
  in all emitted IR (the queue is used only inside the claim-round loop);
  revisit if a future emission adds one.
- **Oracle-binary/golden format skew (PRE-EXISTING, suite-wide, not slice
  3):** the current drlojekyll-oracle emits a final
  `INVARIANT: differential-final == monotone-projection (N relations)` line
  that NO committed *.oracle.stdout contains (including tc_mixed_batch's).
  runall.sh's oracle checks still pass, so its comparison tolerates it, but
  a raw byte-compare diverges everywhere. Needs one decision: regenerate all
  oracle goldens with the line, or suppress the line in the oracle.
- **MiniDisassembler follow-up:** the disassemble OptDiff golden is green,
  so the remaining MiniDisassembler driver gap is likely its own
  expectations or a distinct differential path — sort at slice 4 or (d).

### Slice 4 scope (unchanged from the plan, plus carry-forwards)

Nonlinear matrix bring-up (`p(X,Z):p(X,Y),p(Y,Z)`, FLAG-F/G/H predicate
matrix, the third join-section flavor), the `p(1)/p(2)` self-supporting
cycle trace, goldens for the 4 GOLDEN-MISSING fixtures, cyclic
self-support over-retention (transitive_closure_diff2, cf16_3),
tc_nonlinear_diff's driver mismatch, plus the four carry-forward concerns
above.

## Whole-program view: the D/R/I lowering as pseudocode, slice 4 as diffs

Written 2026-07-10 post-slice-3 as the session-handoff ground truth: the
current machinery re-derived from the code (not the plan), so slice 4 can
be expressed as *diffs against this* rather than re-excavated. Code anchors
are post-slice-3 commit 6faf6d9. The executable spec of the slice-4 target
semantics is the oracle's `FixReads` (`bin/Oracle/Main.cpp:1329`); the
hand-trace evidence is under `docs/proposals/StackSafeNegation.evidence/`.

### Row state and the predicate vocabulary (ALL already in IR + codegen)

Per differential-table row (runtime `DiffTable`): counters `C_nr`, `C_r`
(split derivation counters, MD §3); batch bits `kInI` (present at batch
start), `kDel`/`kAdd` (over-deleted / added this batch), and the per-ROUND
frontier bits `kDelNow`/`kAddNow` (claimed this claim-round, cleared by
RETIRE). Ops: `CLAIM(row, sign)` — test-and-set, dedups on table state;
`RETIRE(row, sign)` — clears the Now bit; `UPDATECOUNT(±1, DerivClass)` —
fold, body fires on zero-crossing; COMMITSWEEP at batch end asserts
`0 <= C` (Table.h:397 is the counter-exactness tripwire).

`MembershipPredicate` (include/drlojekyll/ControlFlow/Program.h:611 — the
complete FLAG-F/H vocabulary; slice 4 needs NO new predicates):

    InI                  = kInI                        (frozen batch-start)
    InNew                = (kInI && !kDel) || kAdd     (batch-final-so-far)
    SurvivesSoFar        = kInI && !kDel
    AliveAtClaim         = kInI && (!kDel || kDelNow)
    InNewWithFrontier    = (kInI && !kDel) || kAdd             (≡ InNew, by design — FLAG-F)
    InNewSansFrontier    = (kInI && !kDel) || (kAdd && !kAddNow)
    Present              = C_nr + C_r > 0
    RecursivelySupported = C_r > 0                     (the REDERIVE test)
    NetDeleted           = kDel && !kAdd               (frontier filters)
    NetAdded             = kAdd && !kDel

### The generated program (entry-proc pseudocode, post-slice-3)

```
proc entry(batch):
  ── INGEST (checkpoint b; Procedure.cpp) ──
  for each message receive:
    monotone msg:      fold tuple; eager walk (BuildEagerInsertionRegionsImpl,
                       Build.cpp:754) — the walk CUTS at ANY successor with
                       CanReceiveDeletions()            « slice 3: cut widened;
                       a deletion-capable recursive SCC is ALWAYS D/R/I-owned »
                       (cut boundary accumulates a net-additions frontier
                       vector that the stratum phases consume as a source)
    differential msg:  net removals/additions parked in the view's queues; NO walk

  ── STRATUM PHASES (BuildStratumPhases, Stratum.cpp:1000) ──
  for stratum s ascending:
    # (1) SEED: one-shot rule firings over frontiers finalized at LOWER strata:
    #     branch chains (EmitSeedLoop→EmitChainStep→EmitHeadFold) and
    #     dual-section joins (EmitSectionWalk: added/removed sections with the
    #     §5.1 SEED-position predicates). Fold class = RuleClass(...):
    #     kRecursive iff target table is in a LINEAR recursive SCC and the
    #     rule reads a same-SCC side, else kNonRecursive.
    #     Zero-crossings park in the target table's del/add queues.

    # (2) SINGLE-PASS tables (genuinely acyclic ONLY « slice 4: the
    #     nonlinear-SCC fallback is GONE; every recursive SCC is D/R/I »):
    for t in acyclic_tables@s:               # every fold kNonRecursive ⇒ C_r ≡ 0
      ClaimDrain(t, del); ClaimDrain(t, add) # one drain settles the table
      FrontierFilter(t, del); FrontierFilter(t, add)

    # (3) recursive SCC tables « slice 4: linear AND nonlinear »:
    OVERDELETE := claim_round_loop(del)                       # §5.2
      └ output region: for t in scc: REDERIVE(t)              # between D and I
    INSERT     := claim_round_loop(add)                       # §5.3 mirror
      └ output region: for t in scc: FrontierFilter(t, del)   « slice 3: BOTH
                       for t in scc: FrontierFilter(t, add)     frontiers here,
                                                                after INSERT »

claim_round_loop(sign):        # INDUCTION region; maintained vectors = the Δs,
                               # so codegen's for(changed;…) IS the A3 break
  until no row claimed this round:
    for t in scc: clear Δ_t
    for t in scc:                            # ClaimDrain (Stratum.cpp:606)
      sort-unique queue_t
      for row in queue_t:
        if CLAIM(row, sign):                 # dedups on table state
          append row → accumulated set (D_s / A_s)
          append row → Δ_t                   # per-round claimed frontier
      clear queue_t                          « slice 3: semi-naive drain; the
                                               fire below re-appends only rows
                                               crossing zero THIS round »
    for j in scc_joins:  JoinFire(j, sign)   # fire over Δ — see below
    for SCC-internal projection branches: SeedLoop over Δ, class kRecursive
    for t in scc: Retire(Δ_t, sign)          # clears kDelNow/kAddNow row bits

JoinFire(join, sign):          # EmitJoinFire « slice 4: k same-SCC positions »
  same_pos := [p : side_p table in the join's SCC]   # k ≥ 1, raw JoinedViews() order
  for p in same_pos:                         # k SEPARATE emissions
    for row in Δ of side_p's table:          # loop the CLAIMED frontier
      bind side_p's columns + pivots from row
      scan each OTHER same-SCC side j by pivot, gated CHECKMEMBER
        j < p: (del ? SurvivesSoFar : InNewWithFrontier)   # permissive earlier
        j > p: (del ? AliveAtClaim   : InNewSansFrontier)  # strict later
      nested scan of each lower side by pivot (BuildMaybeScanPartial)
        gated CHECKMEMBER InNew(side row)    # lower read = batch-final, both signs
      UPDATECOUNT(sign∓, kRecursive) into join table
        on zero-crossing: append row → join table's queue_sign
  # k=1 degenerates byte-identically to the slice-2 linear emission.

REDERIVE(t):                   # EmitRederive (Stratum.cpp:712)
  for row in D_s: if RecursivelySupported(row):    # C_r > 0 after OVERDELETE
    append row → add queue     # INSERT re-enters it; C_nr>0-only rows are the
                               # firewall refusals (they simply stay Present)

FrontierFilter(t, del): for row in D_s: if NetDeleted(row): append → net_removals_t
FrontierFilter(t, add): for row in A_s: if NetAdded(row):  append → net_additions_t
```

Scheduling: `ComputeRecursiveSCCs` (Stratum.cpp:128) + the stratum lift
fixpoint place every fold above every table it reads, EXCEPT same-SCC reads
(exempt — they are `InI`-frozen or Δ/InNew-disciplined, never a drain-order
dependency). « slice 4: the linearity gate is deleted; nonlinear SCCs lower
to the same claim-round path. A join whose EVERY side is same-SCC has NO
seed (no lower delta position, MD §5.1) — its seed emission is suppressed
entirely; a MIXED join (lower + ≥2 same-SCC sides sharing the pivot) keeps
the dual-section seed, whose same-SCC reads at seed time are extensionally
InI (flags untouched until the loops run). »

### Slice 4 as diffs against the pseudocode

**Diff 1 — lift the linearity gate** (Stratum.cpp:1021-1052):

    - nonlinear_groups := SCCs with a join having ≥2 same-SCC sides
    - is_linear_recursive(t) := in SCC && SCC ∉ nonlinear_groups   # gate
    - RuleClass folds nonlinear-SCC targets kNonRecursive          # fallback
    + is_recursive(t) := in SCC                                    # gate gone
    + RuleClass folds ALL same-SCC-reading rules kRecursive
    + keep nonlinear_groups only as k>1 marker feeding Diff 2

  Consequence: nonlinear tables move from `acyclic_tables` to `scc_tables`;
  claim-round loops are emitted for them; their seed section walks become
  class-R. This also RESOLVES the slice-3 carry-forward about the widened
  Build.cpp cut (a nonlinear differential SCC fed by a monotone lower atom
  then lands on the claim-round path, not the single-pass fallback).

**Diff 2 — generalize JoinFire to k same-SCC positions** (the FLAG-H
resolved semantics; EmitJoinFire, Stratum.cpp:814):

    - assert exactly one same-SCC side; ONE emission; delta at that side
    + same_pos := [p : side_p table in the SCC]        # k ≥ 1
    + for p in same_pos:                               # k SEPARATE emissions
    +   loop Δ of side_p's table (position p pins the delta)
    +   for each OTHER same-SCC side j (read fixed by STATIC position):
    +     j < p:  CHECKMEMBER (del ? SurvivesSoFar : InNewWithFrontier)  # permissive earlier
    +     j > p:  CHECKMEMBER (del ? AliveAtClaim   : InNewSansFrontier) # strict later
    +   lower sides: CHECKMEMBER InNew                 # unchanged
    +   fold UPDATECOUNT(∓, kRecursive); crossing → queue

  NOT one emission with per-side dynamic dispatch — that double-fires a
  same-round double-claim (both sides in Δ enumerated under both drivers),
  driving C_r negative in OVERDELETE (commit SIGABRT) or inflating
  multiplicity in INSERT (phantom survival). Same-round exactly-once is
  delivered STRUCTURALLY by the strict/permissive asymmetry keyed on
  kDelNow/kAddNow: earlier-as-delta fires via the permissive later-position
  read; later-as-delta does not, the earlier position failing the strict
  read. k=1 must degenerate to today's exact emission (j<p and j>p both
  empty) — the linear cases are the regression net. Builder-only diff; all
  four matrix predicates exist through printer and codegen.

**Diff 3 — seed section walks for k same-stratum atoms** (EmitSectionWalk /
the §5.1 SEED matrix): with Diff 1 the nonlinear seed joins classify R.
Verify the (b) dual-section seed schema against MD §5.1's k-atom cells
(`j<i` / `j>i` seed columns) — the seed reads are `InI`-frozen so this is
expected to already hold, but re-run the §5.1 position-order table by hand
against a `p(X,Z):p(X,Y),p(Y,Z)` seed before trusting it (Risk #2).

**Diff 4 — expected to FALL OUT (verify, don't code): cyclic self-support
drain.** With Diffs 1+2, OVERDELETE cascades around a pure cycle: each
retraction fold decrements the next row's C_r, so a cycle with no external
support drains to C_r=0 everywhere (MD §8) and REDERIVE's C_r>0 test
refuses resurrection (the firewall — machinery already present in
EmitRederive). Acceptance: transitive_closure_diff2, cf16_3 flip green;
firewall_cycle gets its golden. If they do NOT flip, re-run the MD §8 drain
argument by hand before touching code — the model says this is lowering
infidelity, not a model hole.

**Diff 5 — no pseudocode change (bring-up tasks):** author compiled goldens
for the 4 deliberately-GOLDEN-MISSING fixtures (nonlin_tc_both_change,
diamond_reenqueue, firewall_cycle, recursive_to_downstream) from oracle
truth per the standing policy (4-mode byte-agreement + oracle match, never
to silence a red); sort tc_nonlinear_diff's RUN-FAIL(1) driver mismatch;
re-check the destructive-drain assumption (no post-loop queue consumer) on
the new nonlinear IR; decide the oracle INVARIANT-line golden skew.

### Hand-trace obligations BEFORE coding Diff 2 (Risk #2)

1. The nonlinear both-sides-in-Δ same-round traces (batch-2 both-deleted
   p(2,4), batch-3 both-added p(4,6)) — re-run against the PLANNED emitted
   IR: `evidence/flag-h-resolution.md`.
2. The two FLAG-F kill cases (R-iv lost-derivation, R-ii under-count):
   `evidence/flag-f-resolution.md`.
3. The `p(1)/p(2)` self-supporting cycle REDERIVE/firewall trace
   (.plan.md:316-337 region).

### Session bootstrap (fresh-session checklist)

- Read: this file top-to-bottom (slice records are the ground truth where
  docs disagree), then `.plan.md` §(c), then MD §5–§5.3 + §8.
- State « superseded by the slice-4 landing record below »: HEAD commit
  carries slices 0–3; the WORKING TREE carries slice 4 (uncommitted). Suite
  red set = the 11 needs-(d) crossover names only; every slice-4 target and
  fixture is green.
- Environment: `export PATH="/Users/pag/Code/.brew/bin:$PATH"` before any
  test run (see Environment note above). Suite: `DR=build/debug/bin/drlojekyll
  tests/OptDiff/runall.sh <workroot> [jobs] [filter]`.
- The oracle differential path IS the resolved semantics (`FixReads`,
  `bin/Oracle/Main.cpp:1329`); `--project-monotone` is the independent
  positive-only cross-check.

## Slice 4 landing record (2026-07-10 — UNCOMMITTED, pending owner review)

The slice-4 diff sits uncommitted: `include/drlojekyll/Runtime/Table.h`
(+54/−3), `lib/ControlFlow/Build/Stratum.cpp` (481 lines changed), ONE
authorized re-bless (`tests/OptDiff/goldens/transitive_closure_diff.stdout`,
+8/−8), plus 4 new standing OptDiff cases with full goldens and compiled
goldens for the 4 previously GOLDEN-MISSING fixtures. Method: pre-code IR
dump + critical evaluation in-session (found F17 below BEFORE any nonlinear
code), then two workflows — (1) verification: three opus hand-trace agents
(the mandated FLAG-H / FLAG-F / p(1)-p(2)-cycle traces, re-run against the
HIERARCHICAL planned IR) + a sonnet mechanical touch-list audit, all four
sound; (2) implementation: opus implementer, independent opus adversarial
spec-fidelity reviewer, sonnet full-suite gates, opus fixture author.
Converged round 1: **review = approve, 0 blockers; all gates pass, zero
regressions.**

### F17 — the pre-implementation discovery (gates slice 4; fixed first)

Hand-tracing MD §5.1.1 through the LANDED linear IR exposed a real,
pre-existing, all-modes, cross-batch-latent bug (FINDINGS.md F17): the
lowering hoists ALL seeds before OVERDELETE (spec §5.0 interleaves − seeds
into OVERDELETE, + seeds into INSERT), so a phantom pair's `+` up-crossing
enqueues, and `TryClaimAdd` (which tested only `kAdd`) claimed the
count-canceled row and propagated a phantom `+1` witness downstream with no
compensating `−1`. Killed by a 3-batch linear-tc fixture (now standing case
`tc_phantom_claim`): t(1,5)'s witness count drifts 1→2 in the §5.1.1 batch
and the row becomes immortal. FIX (Table.h): **signed claim gates** —
`TryClaimDel` requires `C_nr ≤ 0` at claim time (assert demoted to gate),
`TryClaimAdd` requires `Total > 0`; a stale entry whose crossing was
canceled intra-batch is dropped (any later genuine crossing re-enqueues).
Companion: **NetAdded gained `!kInI`** (frontier = genuine presence gain) —
without it a gate-dropped del claim leaves a spurious net-addition on a
presence-unchanged row (two-base-rule −e1/+e2 flip), double-counting one
stratum down (standing discriminator `two_base_flip_downstream`). Soundness
of both traced adversarially: C_nr is frozen ≥0 before OVERDELETE (every NR
fold is a hoisted seed; loop bodies fold only C_r), so REDERIVE rows always
pass the add gate; the del gate only drops rows the firewall protects
anyway. The gates ALONE flipped `transitive_closure_diff2` and `cf16_3`
green — the "cyclic self-support over-retention" carried since slice 2 was
F17, not a drain infidelity.

### What landed (Diffs 1–5 against the whole-program view)

- **Diff 1 (gate lift):** `nonlinear_groups` and `is_linear_recursive`
  DELETED; `is_recursive` = SCC membership; `RuleClass` lost the nonlinear
  kNonRecursive override; nonlinear SCC tables now enter `scc_tables` and
  the claim-round loops. Scheduling unchanged (nonlinear SCCs were already
  in `recursive_sccs`; no lift divergence).
- **Diff 2 (k-emission JoinFire):** one loop-scan-fold unit per same-SCC
  position in RAW `JoinedViews()` order (never `SortedPredecessors` — its
  EquivalenceSetId tiebreak is not mode-stable), other same-SCC sides read
  by the §5.1 fixpoint matrix keyed on static position (j<p permissive
  `SurvivesSoFar`/`InNewWithFrontier`; j>p strict
  `AliveAtClaim`/`InNewSansFrontier`), lower sides at `InNew`, folds
  ∓recursive into the join's instance table. k=1 degenerates
  byte-identically (verified: linear IR unchanged vs HEAD).
- **Diff 3 (seeds):** all-same-SCC joins (no lower side) get NO seed —
  emission suppressed (was dead-by-construction; now structural). Mixed
  joins (≥1 lower + ≥2 same-SCC sides on one pivot) keep the dual-section
  seed: at seed time same-SCC flags are untouched, so the sections'
  `InNew`/`InI` reads are extensionally the §5.1 seed-schema reads.
  Verified empirically by new case `nonlin_mixed_seed`
  (`p(F,T) : p(F,Y), p(Y,T), gate(Y)` — one join, 2 same-SCC sides + a
  lower differential side; 5 batches incl. both gate signs; 4-mode
  byte-agreement + oracle-agreed, 1435 assertions).
- **Diff 4 (cycle drain): fell out un-coded** as the plan predicted —
  transitive_closure_diff2 + cf16_3 flipped green (via F17's gates),
  firewall_cycle and the diagonal drain traced and green.
- **Diff 5 (bring-up):** tc_nonlinear_diff's RUN-FAIL(1) was NOT a driver
  bug — the driver is a randomized self-checking oracle (in-driver NaiveTC
  recompute) that was correctly reporting the broken nonlinear fallback;
  green now, driver untouched. transitive_closure_diff's stale golden
  (pinned the F16 wrong output, per the FINDINGS F16 forecast) re-blessed
  after 4-mode byte-agreement + oracle verification — the ONE pre-existing
  golden edit, flagged for owner ratification. Compiled goldens authored
  for all 4 GOLDEN-MISSING fixtures from oracle truth.
- **Fixtures added (standing):** `tc_phantom_claim` (F17 kill),
  `nonlin_diag_selfloop` (diagonal p(a,a) self-tie: exactly-once +
  termination + self-support drain), `two_base_flip_downstream` (NetAdded
  `!kInI` discriminator), `nonlin_mixed_seed` (mixed-join seed × k=2 fire),
  each with compiled + oracle + monotone goldens (oracle goldens strip the
  oracle's INVARIANT line, matching all 30 committed oracle goldens).

### Verification evidence

- Full suite red set = EXACTLY the 11 needs-(d) crossover names
  {compare_4, cond_both_polarities, cond_diff_flipflop, insert_4, merge_5,
  negate_1, negate_3, negate_4, negate_5, negate_6, negation_flap}; comm
  against the rebuilt 19-name HEAD baseline (fresh worktree build) shows
  zero new regressions and exactly {cf16_3, transitive_closure_diff2,
  transitive_closure_diff, tc_nonlinear_diff} + the 4 GOLDEN-MISSING
  flipped green. Suite run 3× on fresh workroots, identical.
- Hand-trace obligations (Risk #2) discharged BEFORE Diff-2 code, against
  the PLANNED hierarchical IR: FLAG-H both-deleted p(2,4) / both-added
  p(4,6) + diagonal + cross-round diamond; FLAG-F R-iv and R-ii kill cases;
  the p(1)/p(2) REDERIVE/firewall cycle. All sound. Two findings of note:
  (a) in the hierarchical shape the head's C_r is 0/1 (witness chain), so
  the oracle's flat C_r=#instances equals it only on
  Present()/RecursivelySupported() — never port flat counter assertions to
  table-4 level; (b) the R-ii kill re-localizes to an instance-level DEBUG
  commit assert (an IDB-only oracle cannot see it) — the sep2-style
  discriminator must run debug builds.
- Reviewer hand-verified the emitted nonlinear IR against the planned shape
  element-by-element in all 4 modes, and re-ran the phantom kill against
  BOTH binaries (HEAD baseline retains t(1,5); fixed tree kills it).
- deep_chain_retract at kChainDepth=100000: exit 0 in all 4 modes under
  `ulimit -s 1024` (1 MB stack — constant-stack proven, not just
  no-overflow), ~0.45 s.
- ctest: PointsTo + Runtime pass; MiniDisassembler.DifferentialUpdatesWork
  still red, pre-existing, unchanged failure signature. data/ corpus 4-mode
  sweep: only the known feature-gap diagnostics + pre-existing
  evm_array_parse 134.

### Deviations — OWNER TO RATIFY at commit time

1. **transitive_closure_diff.stdout re-blessed** (a committed golden edit
   outside the --bless flow). The old golden pinned F16's wrong output (per
   the FINDINGS F16 entry, which forecast exactly this re-bless); the new
   output is oracle-verified and 4-mode byte-identical. .oracle/.monotone
   goldens unchanged.
2. **T6 hardening assert skipped** (frozen-C_nr invariant): every in-loop
   UPDATECOUNT already passes a literal `DerivClass::kRecursive`, so the
   invariant is pinned structurally; a runtime assert would need new
   threaded state for no coverage gain.
3. **New oracle goldens strip the oracle's trailing INVARIANT line** to
   match the 30 committed ones — the I9 format-skew decision (regenerate
   all with the line vs suppress it in the oracle) is still open and now
   slightly more entrenched on the strip side.

### Carried-forward concerns (for needs-(d) and beyond)

- **Oracle/runtime claim-semantics split (latent):** the oracle
  (bin/Oracle/Main.cpp:1285) still HARD-FAILS on a del claim with C_nr>0
  while the runtime now gate-drops. They agree everywhere today because the
  oracle uses the spec's interleaved phase order (its queue entries are
  never stale). If the oracle ever adopts the hoisted order, or a case
  produces an oracle-visible stale claim, this surfaces as ORACLE-FAIL.
  Decide at (d) whether the oracle should mirror the gates.
- **cond_diff_flipflop's RUN-FAIL(134)** aborts at the Commit `0 <= nr`
  tripwire — the same stale-crossing family as F17 but on the negation
  crossover path; expected to be fixed by (d)'s crossover machinery (its
  gates-era output diverges earlier than the abort, so (d) has real work).
- **negation_flap.stdout** remains on the (d) reviewed re-bless list
  (ingest-netting decision), per FINDINGS F16's note.
- **MiniDisassembler.DifferentialUpdatesWork**: unchanged by slice 4; its
  gap is its own expectations or a distinct differential path — sort at (d)
  or (e).
