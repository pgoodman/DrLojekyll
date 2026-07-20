# P1 PINNED IMPLEMENTATION CONTRACT — cross-IR pass harness, stage P1

Consolidated from judge-alias / judge-api / judge-predictions, adjudicated at
code. Repo tip **8239657e** (keyed-instances; T2a/T2b/T3 landed, IR-golden gate
live). All line refs verified at tip.

**VERDICT: GO-WITH-AMENDMENTS.** The two byte-identity predictions (§5) are
sound *only under the amendments below*. Three DRAFT statements are WRONG and
are corrected here as hard pins: (1) "~10 driver call sites" — the driver has
**2** consumption sites (Main.cpp:63, :75); the gate sites are **library-
internal**; (2) "`-disable-dataflow-opt` == `-opt-disable=df.*`" — WRONG,
`df.*` must NOT sweep `df.simplify`/`df.demand`; (3) the bisect narrative (§0/
§2/§3) needs INTERIOR gate points that §4 does not scope — resolved below by
splitting the alias endpoints (outer guard) from the bisect counter (interior,
default-allow).

---

## 1. THE ALIAS RULE — wholesale-skip preservation (PINNED)

**All three judges converge on the same rule via independent evidence: the
legacy flags gate at the CALL, and P1 MUST preserve that exact branch. Do NOT
"enter Optimize() with every pass gated."**

### Evidence (why enter-and-gate-all is NOT byte-equal to never-enter)

The `Optimize()` bodies do NON-PASS structural work between/around their
nameable passes; that work runs iff the body is entered:

- **DataFlow** (`lib/DataFlow/Optimize.cpp:777-845`): `do_cse` re-runs
  `RemoveUnusedViews` + `TrackDifferentialUpdates(log,true)` per CSE round
  (:786-791); `CSE()` re-stamps `det_seq` over ALL views (not just Simplify's
  SELECT set) and runs `RelabelGroupIDs`/`ClearGroupIDs`; `Canonicalize`
  marks every view `is_canonical=false` and tail-calls `RemoveUnusedViews`
  (:733); the driver's own tail `RemoveUnusedViews()` at :844. These leave
  residue (det_seq/group_ids/is_canonical) that differs from never-entering.
  Mostly overwritten downstream, but NOT provably byte-identical by static
  reading — so it must not be relied on.

- **ControlFlow** (`lib/ControlFlow/Optimize.cpp:1253-1466`): DECISIVE. The
  four `.Sort(depth_cmp)` list reorderings (parallel/induction/series/
  operation regions at :1288, :1302, :1312, :1322) are **inside the
  `for(changed…)` sweep loop body** (loop head :1283) — verified at code.
  They are a **durable, emission-visible** reordering of the DefLists the
  Stratum/codegen walks iterate. `remove_unused` sweeps (:1386, :1466) also
  fire per entry. Entering-and-gating-all would run the sorts; never-entering
  leaves build order. Region-list order reaches the emitter → this alone
  refutes the naive reading.

Because the sorts + sweeps live *inside* the loop body (which is inside the
entered `Optimize()`), keeping the outer wholesale-skip guard is BOTH
necessary AND sufficient: in `none`/`nocf`/`nodf` the body is never entered,
byte-identical to today by the SAME single branch.

### The pinned rule

For each optimization LEVEL (df, cf) PassPolicy exposes a derived predicate
`AnyBodyOptionalEnabled(level)` = "does the requested policy leave ≥1 of that
level's **body-resident** optional passes enabled?". The call sites keep their
guard, rewritten:

```cpp
// lib/DataFlow/Build.cpp:2584
if (policy.AnyBodyOptionalEnabled(Level::kDataFlow)) impl->Optimize(log);
// lib/ControlFlow/Build/Build.cpp:1370 AND :1377 — BOTH, same predicate
if (policy.AnyBodyOptionalEnabled(Level::kControlFlow)) impl->Optimize();
```

- **Body-resident df passes = {df.cse, df.canon, df.dfe, df.sink}.** These are
  the only passes inside `QueryImpl::Optimize`. `AnyBodyOptionalEnabled(df)` =
  (df.cse ∨ df.canon ∨ df.dfe enabled). (df.sink is a commented no-op —
  irrelevant to output but counts as body-resident for the predicate.)
- **Body-resident cf passes = {cf.regionopt, cf.procdedup}.**
  `AnyBodyOptionalEnabled(cf)` = (cf.regionopt ∨ cf.procdedup enabled). BOTH
  Build sites use this predicate — the cf alias MUST reach BOTH (:1370 AND
  :1377), else `nocf`/`none` stdout churns (judge-predictions attack 2c).
- **`df.simplify` and `df.demand` are NOT body-resident** — they run
  UNCONDITIONALLY, OUTSIDE the `if(optimize)` guard (Build.cpp:2559 Simplify,
  :2576 demand), in all 4 modes today. They are EXCLUDED from
  `AnyBodyOptionalEnabled(df)` and from the `df.*` alias glob.

### The corrected legacy-alias globs (DRAFT §2 is WRONG here)

```
-disable-dataflow-opt   == -opt-disable=df.cse,df.canon,df.dfe,df.sink
                           (NOT df.* — df.* would sweep df.simplify + df.demand)
-disable-controlflow-opt == -opt-disable=cf.*    (= {cf.regionopt,cf.procdedup},
                                                   both == whole ProgramImpl::Optimize)
```

`cf.*` is fine because both cf passes ARE body-resident and are exactly what
`optimize==false` skips today. `df.*` is NOT fine. Implement these as
`PassPolicy::DisableDataFlowOpt()` / `DisableControlFlowOpt()` factories that
produce EXACTLY these enumerated globs; Main.cpp's legacy branches
(:386-393) call the factories.

**4 golden modes map exactly, byte-identity by construction (no corpus run
needed to TRUST the endpoints — only to catch a wiring bug):**
- `opt`  → AnyBody(df)=T, AnyBody(cf)=T  (empty policy)
- `nodf` → AnyBody(df)=F, cf=T
- `nocf` → df=T, cf=F
- `none` → both F.

**Adjudication note (alias grain):** judge-alias framed this as "(a) per-pass
equivalence vs (b) wholesale-skip" and picked (b). judge-api and judge-
predictions independently reach the same enumerated-globs conclusion. PINNED:
**(b) wholesale-skip via `AnyBodyOptionalEnabled`, with the df alias
enumerated (not `df.*`).** No dissent survives.

---

## 2. THE TRUE CALL-SITE LIST (DRAFT "~10 driver sites" corrected)

The driver (`bin/drlojekyll/Main.cpp`) has **2 consumption sites** (:63, :75)
— it only PARSES flags and constructs the policy. The gate sites are
**library-internal**. Two GRAINS, adjudicated below.

### 2a. Outer wholesale-skip guards (MANDATORY, byte-identity endpoints)

| # | site | file:line | predicate |
|---|---|---|---|
| 1 | DataFlow Optimize entry | lib/DataFlow/Build.cpp:2584 | AnyBody(df) |
| 2 | ControlFlow Optimize entry #1 | lib/ControlFlow/Build/Build.cpp:1370 | AnyBody(cf) |
| 3 | ControlFlow Optimize entry #2 | lib/ControlFlow/Build/Build.cpp:1377 | AnyBody(cf) |

These 3 guards give the 4 golden modes their byte-identity. **This is the
irreducible P1 core.**

### 2b. Interior per-application gate/counter sites (for bisect + -opt-only)

To make `-opt-only`, `-opt-disable` of a SINGLE body pass, and
`-opt-bisect-limit` mean anything, gate calls sit INSIDE the drivers. Grain =
**one registered NAME per logical pass**; a name may tick the bisect counter
multiple times (one per application):

| name | gate site | applications/compile |
|---|---|---|
| df.simplify | Build.cpp:2559 (entry) | 1 |
| df.demand | Build.cpp:2576 (entry; also self-gates on demand_mode) | 0 or 1 |
| df.cse | head of `do_cse` lambda, Optimize.cpp:780 | 3 (do_cse() called :814, :820, :842) |
| df.canon | per-round inside Canonicalize's fixpoint loop | N rounds (across entries :817, :835×max_depth) |
| df.dfe | Optimize.cpp:839 (EliminateDeadFlows) | 1 per max_depth iter |
| df.sink | do_sink head (no-op body) | nominal |
| cf.regionopt | sweep-loop head, Optimize.cpp:1283 | N sweeps ×2 (Optimize called twice) |
| cf.procdedup | procdedup block entry, Optimize.cpp:1389 | 1 ×2 |

**df.dfe == `EliminateDeadFlows` ONLY.** `RemoveUnusedViews` is a REQUIRED
graph-hygiene primitive (runs unconditionally at Build.cpp:2552/2593; called
many places inside Optimize) — it is NEVER gated. The DRAFT §1 "df.dfe /
RemoveUnusedViews" naming is imprecise; PINNED: gate EliminateDeadFlows, leave
RemoveUnusedViews always-on. If df.dfe is skipped, read
`changed = EliminateDeadFlows()` as `changed=false` (loop terminates; sound).

**Skip semantics for interior passes (sound termination):** a gated-off
interior pass returns as if it did nothing / made no change — canon skips its
round, dfe returns false, regionopt sweep is not entered, cse loop does not
run. The always-on V-* validators + oracle referee every under-optimized
config; an invalid config ABORTS loudly, never miscompiles.

### 2c. THE ADJUDICATED SCOPE SPLIT (resolves judge-predictions attack 1c vs judge-api)

judge-api recommends wiring 2b in P1. judge-predictions attack 1c says 2b
contradicts §4's scope and the bisect narrative is P2/P3. **RESOLUTION — both
can be P1 WITHOUT breaking the emission-neutral prediction, because the two
concerns are ORTHOGONAL:**

- The **byte-identity endpoints** (4 golden modes) are delivered ENTIRELY by
  the 3 outer guards (2a). They do NOT depend on the interior gates.
- The **interior gates (2b)** are wired in P1 but **default to ALLOW** (empty
  policy → every `Gate()` returns true, no ordering-visible side effect). At
  default config the interior gates are provably inert (see §4 purity pin), so
  emission-neutrality holds BY CONSTRUCTION even with them wired.

**PINNED SCOPE for P1:** wire BOTH 2a (mandatory) AND 2b (the registry +
`Gate()` calls + the monotone bisect counter). This is the honest reading of
DRAFT §4 ("-opt-bisect-limit at the … call sites"). The DRAFT's "~10 driver
call sites" is corrected to: **3 outer guards + ~8 interior gate NAMES, all
library-internal; 2 Main.cpp consumption sites.** The bisect counter DOES
reach interior rounds in P1 (contra attack 1c's pessimistic reading) — the
counter is cheap and pure; what P1 does NOT ship is `-opt-counter` (P3) and
`-passes` (P4). If the owner prefers the minimal endpoint, 2b can defer to P2,
but the recommendation is to land 2b now since it is inert-at-default and
unlocks the bisect workflow the owner asked for (directive ii).

---

## 3. PASSPOLICY HOME / SHAPE / THREADING (PINNED)

**HOME:** new header `include/drlojekyll/Util/PassPolicy.h` + a small `.cpp`
folded into an EXISTING lib target (no new library). Util is neutral ground
both `Query::Build` and `Program::Build` already include (DefUse.h,
EqualitySet.h). NOT Parse/.

**SHAPE (minimal, deterministic, no globals):**

```cpp
namespace hyde {
enum class Level { kDataFlow, kControlFlow };
class PassPolicy {
 public:
  // Empty policy == "run everything" == today's default pipeline.
  std::vector<std::string> disabled_globs;   // -opt-disable
  std::vector<std::string> only_globs;        // -opt-only (empty => all)
  int64_t bisect_limit = -2;                  // -2 unset, -1 print-all, >=0 skip idx>N
  mutable uint64_t bisect_counter = 0;        // monotone application index

  bool Enabled(std::string_view name) const;  // pure predicate, NO counter tick
  bool AnyBodyOptionalEnabled(Level lvl) const;  // the wholesale-skip predicate
  bool Gate(std::string_view name, std::string_view desc = {}) const;  // the one call per site

  static PassPolicy DisableDataFlowOpt();     // {df.cse,df.canon,df.dfe,df.sink} globs
  static PassPolicy DisableControlFlowOpt();  // {cf.*} glob
};
}  // namespace hyde
```

**Gate(name, desc) EXACT semantics:**
1. `enabled = Enabled(name)` (glob filter, §5).
2. if `!enabled`: return false, **NO counter tick** (filtered-out passes do
   not consume bisect indices — matches LLVM OptBisect; keeps binary search
   sound and index-stable across `-opt-disable` configs).
3. `idx = bisect_counter++`.
4. if `bisect_limit == -1`: print stderr line (§6), return true.
5. if `bisect_limit >= 0 && idx > bisect_limit`: return false (skipped) — the
   tick still happened (index space stays stable).
6. return true.

**Enabled(name) logic (optionals only; required passes never call Gate):**
```
if only_globs non-empty:  enabled = ANY only_glob matches name   // -opt-only inverts default
else:                     enabled = true
enabled = enabled AND NO disabled_glob matches name              // -opt-disable subtracts
return enabled
```
`-opt-only` present ⇒ baseline "all optionals OFF, turn on the matched ones".
`-opt-disable` always subtracts and WINS on conflict (subtract after select).
**Alias→glob expansion and this predicate evaluate AFTER all flags parse** (so
`-opt-disable X -opt-only Y` is order-independent — judge-predictions attack
1d). Malformed glob = clean CLI diagnostic in Main.cpp, never silent no-match.

**THREADING — REPLACE the `optimize` bool, do not add-alongside** (two sources
of truth is a bug). New signatures:
```cpp
Query::Build(const ParsedModule&, const ErrorLog&,
             const PassPolicy&, bool demand_mode=false)
Program::Build(const Query&, const ErrorLog&, unsigned first_id, const PassPolicy&)
```
The `optimize` bool is DERIVED internally: the guard `if(optimize)` becomes
`if(policy.AnyBodyOptionalEnabled(level))`. `demand_mode` STAYS a separate
DataFlow-only param (Program::Build has no demand_mode — verified at
Program.h:1359). Current defaults `optimize=true` map to an empty (default)
PassPolicy.

**CALLERS THAT MUST CHANGE (grep-verified — only these two binaries):**
1. `bin/drlojekyll/Main.cpp:63` `Query::Build(...)` — pass PassPolicy from new
   flags; keep `gDemand` as the demand_mode arg.
2. `bin/drlojekyll/Main.cpp:75` `Program::Build(...)` — pass the companion
   PassPolicy.
3. `bin/Oracle/Main.cpp:739` `Query::Build(*module_opt, error_log,
   /*optimize=*/false)` — MUST pass `PassPolicy::DisableDataFlowOpt()`. Oracle
   deliberately runs UNoptimized DataFlow for independence (Oracle/Main.cpp:7).
   **Oracle does NOT call Program::Build** — no cf change there.
No test/tool other than these two binaries calls either Build.

**Lifetime (judge-predictions attack 1e):** the mutable `bisect_counter` must
be RESET at the head of each `CompileModule` (or the policy passed const in the
allow-everything path). Corpus is one-module-per-invocation today so default
config is safe, but pin the reset to avoid a multi-module carry-over trap.

---

## 4. Gate() ALLOW-PATH PURITY (PINNED — the emission-neutrality guarantee)

**The single most important byte-identity pin (judge-predictions attack 1a):**
in the default/allow path (`bisect_limit == -2`, name enabled) `Gate()` MUST be
free of any side effect that ANY ordering decision or dump can read:
- The `bisect_counter++` is fine ONLY if no emission/iteration order reads it
  (it doesn't — it is a diagnostic index). Confirm no pass keys behavior on it.
- `desc` MUST be lazy/never-captured in the allow path — do NOT eagerly format
  a string that captures pointer identity (would be nondeterministic across
  runs and could leak into a dump).
- **No NDEBUG-conditional logic in the Gate path** (judge-predictions attack
  2e) — T3 just made .ir/.h config-invariant (E-72); a debug-only stats line
  would re-open that divergence. debug==release must re-hold on all 4 surfaces
  of both irgold carriers.

---

## 5. THE GLOB MATCHER (PINNED — minimal, not fnmatch)

Prefix-star + exact ONLY. Glob `G` matches name `N` iff:
- `G` ends `".*"` ⇒ `N == G[:-2]` OR `N` starts with `G[:-1]` (the dotted
  prefix incl. the dot). `df.*` → {df.cse, df.canon, df.dfe, df.sink,
  df.simplify, df.demand}; `cf.*` → {cf.regionopt, cf.procdedup}.
- `G == "*"` ⇒ everything.
- else ⇒ `N == G` exact.
No `?`, no bracket classes, no mid-string `*` (those are clean CLI
diagnostics). Deterministic, order-independent. Reject full fnmatch
(POSIX-locale surprises, over-engineering).

**Note on `df.*` the GLOB vs the df ALIAS:** the *matcher* `df.*` DOES match
df.simplify/df.demand (it is a pure string matcher). The *legacy alias* does
NOT use `df.*` — it uses the enumerated `df.cse,df.canon,df.dfe,df.sink`
(§1). Keep these distinct: a user typing `-opt-disable=df.*` disables ALL six
(their choice); the alias factory never emits `df.*`.

---

## 6. BISECT SEMANTICS + PRINT FORMAT (PINNED)

- SINGLE monotone cross-level counter on the PassPolicy (mutable
  `bisect_counter`), shared DataFlow+ControlFlow, ticked in program order.
  Deterministic: compiler single-threaded, post-(F) iteration id-ordered →
  same input yields same index sequence → sound binary search.
- Tick once per ENABLED application (Gate step 3). Filtered-out (glob-disabled)
  passes do NOT tick (index space stable across configs — you bisect WITHIN a
  config). Skipped-by-limit (idx > N) still ticks.
- `bisect_limit`: `-2` unset (no limit, no print), `-1` run-all-and-print,
  `>=0` skip applications with idx > N.
- **-1 stderr line (LLVM-style, stderr NEVER stdout — keeps goldens clean):**
  ```
  BISECT: running pass (<idx>) <pass-name> on <unit-desc>
  ```
  e.g. `BISECT: running pass (2) df.canon on round 0 (bottom-up)`.

**CF two-call granularity caveat (judge-predictions attack 1b):** because
`ProgramImpl::Optimize` is called twice (Build.cpp:1370, :1377), cf passes tick
twice per compile and a bisect boundary CAN land between the two calls,
producing a half-optimized program the legacy bool-pair cannot express. This is
INTENDED (finer bisect granularity), NOT a default-config bug. Document it; it
is invisible at default config.

---

## 7. THE GATE BATTERY (PINNED — standing + the two NEW P1 gates)

Standing (T3 pattern, ledger §16):
- **[G1]** SUITE PASS (169) with irgold arms LIVE — `runall.sh` ends `SUITE:
  PASS`. Covers stdout all-4-modes + the 6 irgold surfaces (h/ir/df/deltarel on
  demand_tc_witness@opt + ir/df on symrec_tie_1@opt).
- **[G2]** 676-row A/B byte-identical vs frozen baseline (169×4×{exit,.h,.ir}) —
  the DIRECT emission-neutrality proof at DEFAULT config (prediction 1). Zero
  churn.
- **[G3]** ctest 3/3 (MiniDisassembler, PointsTo, Runtime).
- **[G4]** Q5 progsize/timing ABABAB interleaved release — report-only, noise
  band; guards a Gate() call landing on a hot path.
- **[G5]** data/ 36-file A/B clean (compile-all, default config).

NEW, this diff UNIQUELY needs (G1/G2 use the legacy spelling only — they pass
even if the new vocabulary is subtly wrong):
- **[G6] ALIAS-EQUIVALENCE A/B (MANDATORY — prediction 2's only real test).**
  Run the FULL suite (4 modes, stdout + all 6 irgold surfaces) TWICE: once with
  `flags_of()` emitting the LEGACY spellings (as-is), once with a temporary
  `flags_of()` emitting the NEW vocabulary (`-opt-disable=df.cse,df.canon,
  df.dfe,df.sink` for nodf, `-opt-disable=cf.*` for nocf, both for none).
  Byte-compare the two workroots row-for-row. Must include nodf/nocf/none
  (attack 2c: cf alias reaches BOTH Optimize calls) and demand_tc_witness@opt
  (attacks 2a/2d).
- **[G7] df.* / df.demand ORTHOGONALITY probe (targeted G6 sub-case).** Compile
  demand_tc_witness under `-demand -opt-disable=df.cse,df.canon,df.dfe,df.sink`
  (the df alias) and assert stdout + all 4 dumps byte-identical to `-demand`
  alone — pins that the df alias does NOT sweep df.demand (the highest-risk
  interaction; attack 2d).

Recommended:
- **[G8]** PassPolicy unit: empty policy ⇒ `Gate` allows every registered
  PassId, `AnyBodyOptionalEnabled(df)==AnyBodyOptionalEnabled(cf)==true`, and
  the -1 print path touches only stderr. Pins attack 1a/1f mechanically.

**flags_of("opt") MUST stay exactly "" under P1** (no redundant explicit
`-opt-only` "for clarity") — any non-empty opt spelling rides the demand-ON
pipeline touching all 4 dump surfaces and risks a 1-byte irgold shift
(attack 2a). Leave flags_of untouched for opt; only the G6 A/B run substitutes.

---

## 8. VERDICT: GO-WITH-AMENDMENTS

Sound-in-spirit; the two byte-identity predictions hold under the amendments.
The three DRAFT errors (~10 driver sites; `df.*` alias; the interior/bisect
scope ambiguity) are corrected above as hard pins. No REDESIGN needed — the
harness architecture is right; P1 needs the enumerated-globs alias, the
wholesale-skip guards, and G6/G7.
