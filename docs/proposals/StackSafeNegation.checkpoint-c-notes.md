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
2. **Correctness core — OVERDELETE + REDERIVE for tc:** emit the base-rule
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
3. **INSERT + BUILDFRONTIERS + delete the add-only loop:** mirror the
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

- **FLAG-F**: `InNewWithFrontier` is textually identical to `InNew` — needs
  a 2-same-stratum-atom hand-trace to settle typo-vs-intentional-alias.
  Inert on tc (one same-stratum atom).
- **FLAG-H**: whether the third (fixpoint) join-section flavor needs one
  emission per delta position or dynamic per-side dispatch (k-emissions
  question). Also inert on tc.
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
