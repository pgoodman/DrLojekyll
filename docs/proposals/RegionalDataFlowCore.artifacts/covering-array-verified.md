# Covering-array draft — empirical verification ledger

Verifier: empirical probe against `build/debug/bin/drlojekyll` (tip f0c913e0).
All probes in `phase5/probe-covering-array/`; no repo/golden/tests changes.
Method: CLI acceptance checks + full-corpus (181-case) compile-only sweeps
under each single-off config + end-to-end compile→build→run→`cmp -s` against
the committed `goldens/<case>.stdout` for a witness set.

Categories: **VERIFIED-EMPIRICALLY** / **PREDICTION-FAILED** (both prediction
and reality kept) / **UNVERIFIABLE-TODAY** (future-stage or out-of-scope-to-run).

---

## Counts

- **VERIFIED-EMPIRICALLY: 12**
- **PREDICTION-FAILED: 2** (one root cause: `df.dfe` is load-bearing for
  compilation on 4 cases — a second carve-out the draft omits)
- **UNVERIFIABLE-TODAY: 6** (wrapper not built; combinatorial proofs; whole-
  corpus stdout equality for the 5 never-load-bearing configs not run at full
  compile-RUN scale; Stage-C forward-compat)

---

## §0 / §2 — CLI surface & gate inventory

**[V1] The `-opt-disable=<name[,name…]>` surface and all 8 gate names.**
VERIFIED-EMPIRICALLY. All 8 names accepted exactly as spelled
(`df.simplify df.cse df.canon df.dfe df.sink df.ident_join cf.regionopt
cf.procdedup`), each single-off compile exit 0 on join_1. An unregistered
name (`df.bogus`) is a clean exit-1 error ("matches no registered pass") —
so the wrapper's config table must use exact registered names.

**[V2] `df.simplify` is not in any alias / not body-resident (row 1, §0 block).**
VERIFIED-EMPIRICALLY (source): `lib/Util/PassPolicy.cpp:33-34`
`kDataFlowBody = {df.cse, df.canon, df.dfe, df.sink, df.ident_join}` —
`df.simplify` absent; gated separately at `Build.cpp:2563`. The four legacy
mode aliases therefore never touch S, as the §0 lattice table asserts.

**[V3] The two aliases expand as the §0 table claims** (`-disable-dataflow-opt`
= df.cse,df.canon,df.dfe,df.sink[,+ident via body]; `-disable-controlflow-opt`
= cf.*). VERIFIED-EMPIRICALLY (source `include/.../PassPolicy.h:12-15` +
behavior: `nodf` on kvindex_1 rejects exactly as `canon-off` does — canon is
in the df block).

## §1 — `df.sink` kept as a no-op axis

**[V4] `sink-off` (`-opt-disable=df.sink`) is observationally identical to opt.**
VERIFIED-EMPIRICALLY. `merge_2/sink-off` and `kvindex_1/sink-off` both behave
byte-identically to opt (merge_2: golden-match end-to-end; kvindex_1: compiles,
exit 0, unlike canon-off). The "sinking stays dead" tripwire is realizable.

## §3 — THE INVARIANT (the core claim under test)

**[V5] The invariant holds for every NON-load-bearing config × the witness
corpus.** VERIFIED-EMPIRICALLY. End-to-end compile→build→run→`cmp -s` golden:
- `merge_2` × {cse-off, canon-off, sink-off, s-off, regionopt-off,
  procdedup-off, ident-off, dfe-off, all-off} → all golden-match.
- `join_1` × {cse-off, all-off}, `agg_distinct_1` × {all-off},
  `barrier_neck_1` × {s-off, all-off}, `cf13_1/2/3` × {regionopt-off,
  procdedup-off} → all golden-match.

**[V6] Full-corpus blast-radius: the 5 configs s-off/cse-off/sink-off/
ident-off + the 2 cf configs perturb NO compilation outcome anywhere.**
VERIFIED-EMPIRICALLY (compile-only sweep, all 181 cases, each vs opt exit):
zero exit-status diffs for s-off, cse-off, sink-off, ident-off, regionopt-off,
procdedup-off. Only `all-off` diffs, and only on kvindex_1 (its canon leg).

**[PF1] §3 blanket statement: "for EVERY config and EVERY corpus case … stdout
is byte-identical to the case's existing golden." PREDICTION-FAILED.**
> Prediction (§3): all 8 gates are semantics-preserving optimizations, so every
> point in the 2⁸ lattice yields stdout == golden for every case; the only
> derived exception carved out is kvindex_1 under `df.canon`-OFF.

> Reality: `dfe-off` (`-opt-disable=df.dfe`, i.e. D0 with C=N=1) **aborts the
> compiler** — SIGABRT, exit 134, `Assertion failed: (stratum_num_views[s]==1u
> || stratum_has_inductive_merge[s] || stratum_has_io_seam[s]), Stratify.cpp:420`
> — on **four** cases: `deadflowelimination_1`, `deadflowelimination_2`,
> `deadflowelimination_4`, **and `recursion`**. These produce no stdout at all,
> so they can neither golden-match nor `expect_diagnostic` (a clean exit-1). The
> array as specified would go **RED** on these `(dfe-off, case)` cells, with no
> derived carve-out to catch it.

This is a genuine coupling the array is *designed* to expose and a real finding
in its own right: **`EliminateDeadFlows` (df.dfe) is load-bearing for the
Stratify pass's precondition when canonicalization/CSE are active** — a second
"load-bearing-for-compilation" gate exactly analogous to kvindex_1/df.canon,
but for a *different* set of cases and gate, that the draft's §3 does not
enumerate. Adjudication consequences:
- The draft's derived-expectation policy needs a **second carve-out**: under
  `df.dfe`-OFF-while-df.canon-ON, `deadflowelimination_{1,2,4}` and `recursion`
  are expected to **abort** (or the array must special-case them). Absent that,
  the "observational soundness at every lattice point" thesis (§3) is
  empirically false as stated.
- The abort is *not* a clean diagnostic: it is an internal invariant
  (`Stratify.cpp:420`) tripping, i.e. the compiler's own belt. §4.5 anticipated
  the *mechanism* (see PF2) but §3's expectation ledger does not.

**Why this is uniquely an array finding (design-validating):** the same four
cases compile **cleanly** under `nodf` (exit 0) AND under `all-off` (exit 0) —
because both also disable `df.canon`/`df.cse`, keeping the graph in a
Stratify-safe shape. The abort surfaces **only** at the single-gate `dfe-off`
point (dfe OFF while canon/cse ON), which no golden mode and not even the
all-off closure reaches. This is the sharpest possible confirmation of the
array's "intra-df-body mixed pair" motivation (§0) — and simultaneously the
sharpest refutation of the blanket §3 invariant.

**[V7] The kvindex_1 derived exception (§3 "The one derived EXCEPTION").**
VERIFIED-EMPIRICALLY end-to-end. N=0 configs {nodf, none, `canon-off`,
`all-off`} → clean exit-1 diagnostic ("Mutable-attributed merge functor must
declare an algebra…", the V-ALGEBRA reject). N=1 configs {opt, nocf, s-off,
cse-off, dfe-off, sink-off, ident-off, regionopt-off, procdedup-off} → compile,
exit 0. `canon-off` is exactly the single gate that isolates the split, as
claimed.

**[V8] "df.canon OFF splits ONLY kvindex_1 in the whole corpus" (§3.5).**
VERIFIED-EMPIRICALLY at full scale: compile-only sweep of all 181 cases under
`-opt-disable=df.canon` — the *only* case whose exit status differs from opt is
`kvindex_1` (0→1). No other case runtime-/compile-diverges under canon-off.

## §4 — Predicted latent splits & priority probes

**[V9] Probe #1 — `canon-off`→kvindex_1.** VERIFIED-EMPIRICALLY (= V7/V8).

**[V10] Probe #2 — `ident-off` × `-demand` (THE priority probe / open Q#2).**
VERIFIED-EMPIRICALLY, resolves to the *safe* branch the draft hoped for.
`demand_tc_witness`, `demand_neighborhood_mono_witness`,
`demand_multi_adorn_witness` all compiled, ran, and **golden-matched** under
`-demand -opt-disable=df.ident_join`. No `V-INST-*` belt abort, no divergence:
`ResolveLiveRecognition` / the demand recognizer **tolerates** `df.ident_join`
OFF (raw_seed un-folded). Open question #2 is answered empirically: no belt
assumes the fold ran, for these witnesses.

**[PF2] Probe #5 — `dfe-off` (§4 item 5) primary prediction "stdout stays
invariant." PREDICTION-FAILED (same root cause as PF1).**
> Prediction (§4.5): "[dead flows survive]… It is unsatisfiable (0 rows) so
> stdout stays invariant, but a later pass assuming a DFE-cleaned graph could
> abort. Witnesses: deadflowelimination_1..6."

> Reality: the "could abort" hedge is what actually happens — `dfe-off` aborts
> (Stratify.cpp:420) on `deadflowelimination_1/2/4` and additionally on
> `recursion` (a case §4.5 does NOT name). It is NOT stdout-invariant on these;
> stdout is never produced. Only `deadflowelimination_3/5/6` compile clean
> under dfe-off.

Kept as PREDICTION-FAILED (not VERIFIED) because the *stated primary
expectation* ("stdout stays invariant") is contradicted for 3 of the 6 named
witnesses plus one un-named case, and §3's ledger provides no carve-out. The
draft's own hedge correctly identified the mechanism and family — credit noted
— but the array's pass/fail rule (§3, §5) would fire RED here, so the design
must promote the hedge into an explicit config-dependent expectation.

**[V11] Probes #3 (`s-off`), #4 (`cse-off`) on demand + the sharp-edge cases.**
VERIFIED-EMPIRICALLY. `demand_tc_witness` golden-matches under both
`-demand -opt-disable=df.simplify` and `-demand -opt-disable=df.cse`;
`barrier_neck_1` golden-matches under `s-off` and `all-off`. No abort, no
divergence — the demand SIP walk and the group_ids-guard-skipped path are
stdout-invariant on these witnesses, as predicted.

**[V12] Probe #6 — cf pair (`regionopt-off` / `procdedup-off`) on cf-targeted
corpus.** VERIFIED-EMPIRICALLY. `cf13_1/2/3` golden-match under each single cf
config; full-corpus compile-only sweep shows neither cf config perturbs any
case's exit status. The mixed `{R,P}` combos compile and stay answer-invariant.

## §5 / §6 / Open questions — design & wrapper

**[U1] The `optmatrix.sh` wrapper (§5) and its per-config-workroot,
`SUITE-MATRIX` aggregation, no-bless-path structure.** UNVERIFIABLE-TODAY —
not built; a future implementation deliverable. The underlying mechanism it
relies on (compose `.drflags` + `-opt-disable`, `cmp -s` vs existing golden)
is verified (V5/V10/V11); the composition needs `${=flags}` word-splitting in
zsh (the CLAUDE.md gotcha) — a single-string flag arg is rejected as one token.

**[U2] "Complete strength-2 covering array over the 8 axes / all 28 pairs ×
4 sign-combos" (§2 proof).** UNVERIFIABLE-TODAY as an empirical claim — it is a
combinatorial argument over the config table, not a runnable property. The CLI
mechanism that would realize each row is verified; the coverage *math* is
sound on inspection but not empirically testable.

**[U3] Whole-corpus stdout==golden for the 5 never-load-bearing single-off
configs at full compile-RUN scale.** UNVERIFIABLE-TODAY (scope). Verified:
(a) none of s-off/cse-off/sink-off/ident-off/regionopt-off/procdedup-off change
any case's *compile* outcome across all 181 cases (V6), and (b) golden-match
end-to-end on the witness set (V5/V10/V11/V12). NOT run: the full 180-case
compile-build-run-compare for each of these configs. Given PF1 showed a
compile-clean config is not automatically answer-clean is *not* at issue here
(these 5 are compile-identical to opt everywhere), the residual risk is low but
the full run is deferred.

**[U4] permcheck.py within-epoch-permutation adjudication envelope (§3).**
UNVERIFIABLE-TODAY — no config in the witness set produced a within-epoch
delta reorder, so the permcheck adjudication branch was never exercised. Its
existence/behavior is unverified by this pass.

**[U5] Tiering / ~1620-run cost / ctest-vs-manual (§5, open Q#3,#4).**
UNVERIFIABLE-TODAY — design/operational decisions, no empirical content.

**[U6] Stage-C forward-compat (§4.5 open Q#5): ident_join raw_seed fold
disappears when request edges replace forcing.** UNVERIFIABLE-TODAY —
future-stage architecture not yet in tree.

---

## Adjudication summary for the owner

The array's central thesis — "the 4-mode golden contract extends to every point
in the 2⁸ gate lattice; every config's stdout == the existing golden" — is
**empirically false as stated**, for one concrete reason the array itself is
built to find: **`df.dfe` is load-bearing for compilation** on
`deadflowelimination_{1,2,4}` and `recursion`, aborting the Stratify pass
(`Stratify.cpp:420`) when disabled while canonicalization is on. This is a
genuine, previously-unexposed coupling (invisible to nodf/none/all-off because
those also disable canon), so it *validates* the array's fault-localization
premise — but the draft's §3 derived-expectation ledger and §4.5 primary
prediction must be corrected: alongside the kvindex_1/`df.canon` carve-out,
add a **`df.dfe`-OFF carve-out** for those four cases (expect abort, or exclude
the cell), or the matrix goes red on a "semantics-preserving" gate. Everything
else the draft predicts that is testable today — the CLI surface, all gate
names, the kvindex_1 split and its whole-corpus uniqueness, the no-op sink
axis, the ident-off×demand safety (open Q#2), and stdout-invariance of the
five never-load-bearing configs on the witness set — is VERIFIED-EMPIRICALLY.

---

## Addendum — 2026-08-02, post-F26 (the df.dfe split landed)

PF1/PF2's root cause is FIXED (FINDINGS.md F26, owner-ratified D3.1): the
`df.dfe` gate now selects WHICH dead-flow pass runs (`EliminateDeadFlows`
vs the new REQUIRED `CollectDeadCycles` hygiene), and Stratify's debug
assert is the always-on V-SCC-SEAM validator. Re-verified on the fixed
binary:

- **dfe-off full-corpus compile+build+RUN sweep** (the U3 gap, closed for
  this config): 182 cases → 166 golden-match + 15 expected diagnostics +
  `product_in_scc_diff_1` rejecting. ZERO aborts. `deadflowelimination_1/
  2/4` + `recursion` golden-match end-to-end — the four directed witnesses
  of the split. **Carve-out B is GONE.**
- **Whose-absence-breaks-whom audit** (the D3.1 follow-on, all 8 single-off
  configs × 182 cases, compile-only exit-status diff vs base): the ONLY
  divergence in the entire lattice is carve-out A (`kvindex_1` 0→1 under
  `df.canon`-OFF — the pinned mode-split CLEAN diagnostic). df.simplify /
  df.cse / df.dfe / df.sink / df.ident_join / cf.regionopt / cf.procdedup:
  zero divergences. The distinction that separates A from the F26 disease:
  canon-off's coupling is an ACCEPTANCE difference drawn as a rendered
  user-facing diagnostic (KVINDEX→TUPLE elimination is genuinely an
  optimization whose absence leaves an unsupported node), while dfe-off's
  was an INTERNAL INVARIANT ABORT — no remaining gate hides a coordination
  invariant.
- F23's recorded exit-139 no longer reproduces (clean diagnostic all 4
  modes); pinned as `product_in_scc_diff_1` (suite is 182 cases).
