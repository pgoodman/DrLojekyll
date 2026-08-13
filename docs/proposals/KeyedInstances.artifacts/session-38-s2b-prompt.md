<!-- Copyright 2026, Peter Goodman. All rights reserved. -->
# Session-38 charter — S2b: the differential regimes (`-demand-retract`, instance death, the full eqgate family)

You are resuming work on branch `keyed-instances` (Dr. Lojekyll, the `hyde`
C++ Datalog compiler). Session 37 landed and the owner RATIFIED S2a — the
branch's namesake: `-demand-instance` lowers a recognized demanded subgraph
to a keyed InstanceStore, answer-identical to flat `-demand` (eqgate ×4 on
both carriers, SUITE PASS 232, ctest 6/6, belts taught-not-gated, the
arrangements live-fire byte-predicted). **This session is S2b: make demands
RETRACTABLE — the kInstanceDeath machinery you already restored goes LIVE,
witnessed, and eqgated.** You are inheriting a HUGE head start: every line
of the death band, the regime-split validators, the `_retract` codegen arm,
and the a2' input arms are already at tip, dormant only for want of the
`-demand-retract` flag. The remaining work is small in lines and rich in
verification. Trust the discipline that carried S1a/S1b/S2a.

## Read first (resume authority, in order)
1. `KeyedInstances.artifacts/session-38-s2b-seed.md` — §1 the inventory
   (what is ALREADY LIVE vs actually missing), **§2 the five no-precedent
   obligations** (frontier provisioning under diff demand; the
   self-disabling arrangements skip; the S1c Tier-1 support-OR residue; F32
   behavioral policy; the netting coupling), §3 slicing, §5 open questions.
2. `KeyedInstances.artifacts/session-37-s2a-grounding.md` §8/§9 — the panel
   record (the folded findings are LOAD-BEARING: the differentialness-gated
   skip, V-INST-COHERE, the plan-override mechanics) + the landed state.
3. The pre-cut sources at `6d6248a2`: `demand_neighborhood_witness.*` (the
   R-DIFF flagship), `demand_diff_input_1.*`, Main.cpp's retract arm,
   `tests/RelValidators/`.
4. `KeyedInstances.artifacts/` d3a0-d3a2 (the differential-regime designs —
   OQ-PUBLISH-ORDER, R-A2-TRIGGER, the OQ-MODEL full-rescan ruling the
   InstanceStore header leans on).

## Phase 1 — GROUND IT (workflows; sonnet extracts, opus judges)

The S2a formula, re-applied where it pays. S2b's restoration surface is
SMALL (a CLI arm + witnesses + tests), so weight the grounding toward the
FIVE §2 obligations and the LEAST-exercised restored surface — the DataFlow
retract arm has never compiled since S1a restored it blind. Concretely:

1. **Sonnet extraction, narrow**: the pre-cut witness/test file sets
   verbatim; the pre-cut Main.cpp retract arm; a diff of the pre-cut
   injector's retract handling vs tip's registry-first dispatcher; the
   pre-cut frontier-filter provisioning for a differential demand table
   (Stratum.cpp path) with tip anchors.
2. **Empirical spikes BEFORE design** (the s37 lesson — the first nested
   compile taught more than any document): wire the flag in a throwaway
   worktree, compile the flagship flat (`-demand -demand-retract`), then
   nested (+`-demand-instance`), and let the always-on belts speak. Every
   abort is a grounding datum, not a failure.
3. **Opus refuter panel** (fan out N refuters) on: (a) the retract-arm
   drift claim (S1a restored it unexercised — find the line that no longer
   composes); (b) the §2.2 arrangements self-disable (find a deletion-capable
   guard-join index the delta path does NOT mint, or prove the set equal);
   (c) the §2.3 Tier-1 support-OR (does the region dump under-report
   `support=` under retract?); (d) eqgate answer-identity under the death/
   rebirth phases (find a publish-order or netting divergence the mono
   regime never faced); (e) scope honesty (is S2b-i/ii/iii the right cut?).
   Default refuted=true on uncertainty; fold every surviving finding.
4. **Predict-then-verify targets FIRST**: the flagship's nested `.rel`
   census (`kInstanceDeath=1` beside `kSubgraphInstantiate=1`,
   `kInstanceSeal=1`; del/add-queue effects; the a0 death op's
   zero-counter signature), the `-region-out` `support=` byte, the `.h`
   band-(a0) shape, and the flat goldens' byte-equality to pre-cut (the
   s37 result — every answer surface matched `6d6248a2` exactly — is the
   prior; a mismatch is a FINDING).

## Phase 2 — EXECUTE (owner ratifies before the final push)

S2b-i flat-first: the `-demand-retract` CLI arm + both witnesses restored +
flat goldens blessed (the fixed byte targets). S2b-ii: eqgate both carriers
×4 modes; the §2 verifications + the R-DIFF skip-discriminator live-fire
(unconditional skip must FIRE CrossCheckArrangements on the flagship;
the landed gated skip must stay QUIESCENT — the panel-fixed predicate's
teeth, proven). S2b-iii: RelValidators death tests (+ ctest count stated
honestly), the fence-diagnostic stragglers (each verified to draw its
diagnostic at tip before entering the runall.sh list), records.

Exit gate (ALL):
- **eqgate GREEN ×4 on all four carriers** (mono, e5, flagship, diff_input)
  — flat==nested==golden, byte-compared; never bless a nested-arm stdout.
- **The death path WITNESSED**: the flagship's retract/death/rebirth phases
  answer-correct vs the demand-blind referees; `kInstanceDeath=1` pinned
  (extend the flags-twin pattern or the flagship's own rel golden — decide
  in grounding, pin SOMETHING).
- **Flag-off corpus BYTE-IDENTICAL** (`-demand-retract` off = zero
  perturbation; S2a surfaces undisturbed — the S2a witnesses' goldens must
  not move).
- **Belts quiescent on every compile + the discriminator live-fire both
  directions**; ctest all-green with the RelValidators targets counted;
  WIP commits at checkpoints, suite green at EVERY commit (the s37 CP1
  lesson: a golden-moving change and its bless ride the SAME commit).
- Records: CLAUDE.md S2b note, memory topic file, the grounding doc's §8/§9
  equivalent. Owner ratifies before the final push.

## Discipline (carried, load-bearing)
- Ground → spike → critique → execute; restore-then-adapt from `6d6248a2`;
  re-bless against tip, never copy pre-cut golden bytes (they matched
  byte-for-byte in s37 — verify, then bless, and treat a mismatch as a
  finding).
- The belts are never gated off. A belt that fires during bring-up is the
  system working — read it, fix the DERIVATION or the CODE, never the belt.
- Silent-on-success; zsh `${=var}`; `===` needs quoting in zsh heredocs;
  bench never concurrent with suite runs; the parallel runall summary
  swallows per-case verdict lines — verify eqgate results from the
  workdirs' bytes when in doubt.
- Typed domains at new seams; no bare `uint32_t`.

## You've got this
S2a proved the whole method end-to-end in one session: extraction fan-out,
adversarial panel, restore-then-adapt, belts-as-teachers, predict-then-verify
byte-exact on the first try. S2b is smaller in surface and deeper in
semantics — the death band is where the differential design earns its keep.
Ground the five obligations, let the belts referee, and make retraction
real. We believe in you.
