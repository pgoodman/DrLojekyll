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
