# Stage 3 checkpoint (c) working notes (session scratch, not committed)

Branch: derivation-counters. HEAD 500c16d (after the (b) landing + errata).
This notes file is stage scratch: delete it from the repo when checkpoint
(c) lands (the durable record belongs in StackSafeNegation.plan.md).

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
  carries no `kAddNow` term. Evidence: `scratchpad/flag-f-resolution.md`
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
  of `p`), independent of FLAG-F. Evidence: `scratchpad/flag-h-resolution.md`
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
  `scratchpad/flag-attack.md`. Caveat: the oracle IS the resolved semantics
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
  under (b)'s partition — and therefore whether the re-entry-loop deletion
  is a real task or already-satisfied for the constant-stack gate. Not
  derivable from the docs; grep the built tree.
- **How much of the §(c) deletion inventory is already satisfied at (b)**
  (FLAG-I): `BuildUnknownRecheck` / the 4-phase output region /
  `kInductionRechecks` / the re-entry loop may already be gone by (a)/(b).
  MODESWITCH is confirmed gone; the rest the docs disagree on — grep the
  actual (b) tree to learn how much *smaller* (c)'s real diff is.
- **The A4 `ready_after` decision's downstream effects** on inter-stratum
  ordering when a recursive `t`'s frontiers feed a higher stratum — argued
  sound (the producer→higher edge is how non-recursive producers already
  feed higher seeds) but tc has no higher differential consumer, so the seam
  is untested until a recursive→downstream-differential program runs.
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
