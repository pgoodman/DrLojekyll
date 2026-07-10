# FLAG-F / FLAG-H final resolution report

Branch `derivation-counters`, HEAD 680916c. Snapshot binaries only
(`scratchpad/oracle-slice0`, `scratchpad/drlojekyll-slice0`; the compiler was
NOT run — per owner method, ground truth is the positive-only equivalence
oracle plus hand-trace). Date: 2026-07-09. Method label for the notes:
"positive-only equivalence oracle + hand-trace".

## 1. Adjudication — attack outcome and spot re-traces

**Attack verdict: resolutions survive** (confirmed the input `flag-attack.md`,
did not need to overturn any disputed trace). I independently re-ran the single
most discriminating batch/separator per flag against the snapshot oracle:

- **FLAG-F discriminator (R-i alias vs the three typo variants).** The main
  fixture's batch 3 (both-added → p(4,6)) kills R-iii, batch 4 (mixed
  old-`kInI`/new-`kAdd` earlier sibling) kills R-iv — both inside the 4-batch
  `nonlin_tc_both_change` oracle run (OK, 1410 assertions, monotone byte-agrees,
  `reachable={(4,5),(4,6),(4,7),(5,6),(5,7),(6,7)}`). R-ii survives batches 2–4
  and is separated by the 3-hop same-round chain `+7-8 +8-9 +9-10`: I built and
  ran it — `sep1` holds all 334 per-class assertions with p(7,10) present (C_r
  truth = 2, the R-i reading); `sep2` (append `-8-9`) drains p(7,10) cleanly to
  absent (589 assertions, no negative-counter abort, monotone agrees). Under
  R-ii the per-class counter assert would fire at sep1 (C_r would be 1 ≠ 2) and
  the retraction would drive C_r negative. **R-i (alias) is the unique
  survivor.**

- **FLAG-H discriminator (k emissions vs single dynamic dispatch).** The
  same-round double/triple-claim cases: `nonlin_tc_both_change` batch 2
  (both-deleted p(2,4) via p(2,3),p(3,4), both claimed same round) and batch 3
  (both-added p(4,6)); `diamond_reenqueue` (Δ_D cross-round re-enqueue, OK, 529
  assertions, `q_out={}`, monotone agrees); and `k3_join` (k=3 same-round
  TRIPLE claim, OK, 460 assertions, `reachable={(1,2),(3,4)}`, monotone agrees).
  All hold the per-class full-join (delta=-1) counter cross-assert that a
  Scheme-B double-fire would violate (C_r→negative in OVERDELETE = commit
  SIGABRT; C_r inflated in INSERT = phantom survival). **k emissions with static
  per-position predicate is the unique survivor.**

The two flags are orthogonal (the same-round split is driven by
`InNewSansFrontier`'s `!kAddNow`, unchanged across every FLAG-F reading).

## 2. Settled resolutions

**FLAG-F: intentional ALIAS, not a typo.**
`InNewWithFrontier(id) := (kInI && !kDel) || kAdd`, byte-identical to `InNew`;
implementable as a call-through. It names the INSERT fixpoint same-stratum
`j < i` read site. Both disjuncts are load-bearing there. The same-round
exactly-once bit lives only in the `j > i` cell (`InNewSansFrontier`'s
`!kAddNow`); the `j < i` cell carries no frontier bit. Evidence:
`scratchpad/flag-f-resolution.md` + oracle sep1/sep2 + main fixture.

**FLAG-H: k separate emissions per same-stratum delta position, static
per-position predicate.** Not single dynamic per-side dispatch. Each emission
fires over the per-round CLAIMED frontier (`Δ_D`/`Δ_A`); reads: same `j<p` →
`SurvivesSoFar`/`InNewWithFrontier`, same `j>p` → `AliveAtClaim`/
`InNewSansFrontier`, lower `j` → `InNew`. Same-round exactly-once = the
strict/permissive asymmetry keyed on `kDelNow`/`kAddNow`; orthogonally requires
the `Δ_D`/`Δ_A` claim discipline (A3) against diamond re-enqueue. Evidence:
`scratchpad/flag-h-resolution.md` + nonlin (k=2) + diamond + k3_join (k=3).

## 3. Promoted fixtures (with sensitivity arguments)

Committed to `tests/OptDiff/cases/` (`.dr`+`.batches`+`.main.cpp`) with
`goldens/<name>.oracle.stdout` and `.monotone.stdout` authored from the oracle
and byte-verified against a fresh snapshot-oracle run. All four are distinct,
non-redundant axes:

| fixture | axis / sensitivity | final truth |
|---|---|---|
| `nonlin_tc_both_change` | FLAG-F (R-iii dies batch 3, R-iv dies batch 4, R-ii dies on the appended separator) + FLAG-H (k=2 same-round double-claim, batches 2 & 3). Multiplicities engineered to cross zero exactly once so a mis-fire flips the final SET or SIGABRTs. | `reachable={(4,5),(4,6),(4,7),(5,6),(5,7),(6,7)}` |
| `diamond_reenqueue` | FLAG-H cross-round Δ_D discipline — the re-enqueue axis nonlin does NOT cover; wrong discipline double-decrements C_r(q(5))→negative or strands a phantom. | `q_out={}` |
| `firewall_cycle` | REDERIVE drain vs `C_nr>0` firewall refusal (FLAG-E adjacent, mandated (c) companion); most end-state-sensitive per line — a wrong REDERIVE flips which SCC survives. | `p_out={3,4}` |
| `recursive_to_downstream` | BUILDFRONTIERS producer→consumer seam; a wiring bug corrupts `flagged` invisibly to tc's own counter asserts, so needs an END-STATE oracle. | `flagged_out={4}` |

Dropped as redundant for the standing suite: the ~112-case attack corpus
(k3_mixed, selfloop, cycle2, cycle3fw, reroute, phantom, longchain, stress
seeds) — high value for fuzzing but not small hand-traceable standing cases;
their coverage is subsumed by the four above plus the oracle stress mode.

**Compiled goldens deliberately ABSENT (GOLDEN-MISSING).** These four are
red-until-(c) (recursive-stratum OVERDELETE/BUILDFRONTIERS not yet wired) AND
the drivers dump intermediate per-batch state the oracle final-state truth does
not provide, so a mechanically-correct `goldens/<name>.stdout` cannot be
derived now. They must be authored at (c) slice-2/3 bring-up. `runall.sh` will
report GOLDEN-MISSING on the compiled step for these four until then — an
accepted verdict; the oracle/monotone goldens are the authoritative gate
meanwhile. (A wrong golden was explicitly avoided.)

## 4. Queued proposal edits (owner review; StackSafeNegation.md NOT touched)

Recorded in the checkpoint-c-notes "Queued proposal edit" block: annotate the
§5.1 fixpoint table's INSERT `same j < i` cell as
`` `InNewWithFrontier` (≡ `InNew`; earlier-position add-side read is the plain
final-so-far predicate — the exactly-once bit lives only in the `j > i` cell) ``
and add an under-table paragraph stating the alias identity, the `kAddNow`-only-
in-later-cell symmetry with OVERDELETE, and that dropping `(kInI&&!kDel)` or
narrowing `kAdd` to `kAddNow` is unsound. Owner applies to the authoritative
spec.

## 5. Edits made this session

- `docs/proposals/StackSafeNegation.checkpoint-c-notes.md`: replaced the
  FLAG-F and FLAG-H open-flag stubs with the resolutions (settled semantics,
  evidence pointer, date, method); added the adversarial-recheck note, the
  queued §5.1 proposal edit, and the promoted-fixtures + GOLDEN-MISSING record.
- `tests/OptDiff/cases/`: `nonlin_tc_both_change`, `diamond_reenqueue`,
  `firewall_cycle`, `recursive_to_downstream` — each `.dr` + `.batches` +
  `.main.cpp`.
- `tests/OptDiff/goldens/`: the eight `.oracle.stdout` / `.monotone.stdout`
  goldens (byte-verified). No `.stdout` compiled goldens (intentional).
- Nothing committed; nothing outside `tests/OptDiff/` and the notes file
  touched; the compiled cases were NOT run.

## 6. What remains for running code to confirm

1. Slice-2/3 (c) bring-up must author the four compiled `goldens/<name>.stdout`
   from the corrected generated binary and confirm all 4 opt modes agree.
2. Fidelity of the eventual `lib/ControlFlow/Build/Induction.cpp` lowering
   (the third join-section flavor, `Δ_D`/`Δ_A` discipline, `InNewWithFrontier`
   alias, REDERIVE `kRecursivelySupported`, BUILDFRONTIERS seam) to the resolved
   oracle semantics — the hand-trace validated the SEMANTICS, not the codegen.
3. Multiset/counter exactness over long arbitrary batch streams remains a
   fuzz/differential-vs-oracle property (the oracle `--stress` mode), not a
   fixed fixture.
