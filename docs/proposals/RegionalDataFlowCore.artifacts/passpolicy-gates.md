# PassPolicy optimization-gate registry — authoritative extraction

Source of truth: `lib/Util/PassPolicy.cpp` (`kAllPasses`, `kDataFlowBody`,
`kControlFlowBody`) + `include/drlojekyll/Util/PassPolicy.h`. Cross-checked
against every live `.Gate("...")` call site (found by
`grep -rn "policy.Gate\|policy->Gate" lib include bin`) and the CLI surface
in `bin/drlojekyll/Main.cpp`.

**8 registered gates total.** `kAllPasses` (PassPolicy.cpp:41-43) is the
*only* authoritative list — it doubles as the parse-time
`MatchesAnyKnownPass` reachability check, so anything not in it cannot be
named on the CLI (`-opt-disable=`/`-opt-only=` reject an unmatched glob as a
clean diagnostic, never a silent no-op).

| # | Registry name | Code site (file:line, function) | What disabling it skips | Default | Composition with the 4 golden modes |
|---|---|---|---|---|---|
| 1 | `df.simplify` | `lib/DataFlow/Build.cpp:2563`, `Query::Build` | `QueryImpl::Simplify(log)` — the very first dataflow simplification pass, run right after clause-building/`TrackDifferentialUpdates`, **before** `ConnectInsertsToSelects` and demand. | Enabled | **NOT gated by either legacy alias.** It is explicitly *not* in `kDataFlowBody`, so it runs identically in all 4 modes (`opt`/`nodf`/`nocf`/`none`). Only reachable via an explicit `-opt-disable=df.simplify` or `-opt-only=` that excludes it. |
| 2 | `df.cse` | `lib/DataFlow/Optimize.cpp:820`, `QueryImpl::Optimize`'s `do_cse` lambda | The whole CSE-to-fixpoint loop (`CSE(this, views)` + the `RemoveUnusedViews`/`TrackDifferentialUpdates` re-derivation inside that loop); `do_cse` is called 3× in `Optimize`. | Enabled | Body-resident (`kDataFlowBody`). Disabled by `-disable-dataflow-opt` and by `none`. Unaffected by `-disable-controlflow-opt`. |
| 3 | `df.canon` | `lib/DataFlow/Optimize.cpp:703`, `QueryImpl::Canonicalize` | Gated **per round** inside the canonicalize fixpoint loop (finest bisect grain in DataFlow) — `break`s the loop on first refusal, so the loop's remaining rounds never run (monotone bisect limit ≈ "run first K rounds"). `Canonicalize` is called 3× from `Optimize` (Optimize.cpp:862, 880, 912), always with a non-null `&policy` — the `const PassPolicy *policy = nullptr` default parameter in `Query.h:1047` is never actually exercised as null anywhere in the current call graph. | Enabled | Body-resident. Disabled by `-disable-dataflow-opt`/`none`. |
| 4 | `df.dfe` | `lib/DataFlow/Optimize.cpp:886`, `QueryImpl::Optimize` | Only `EliminateDeadFlows()` itself (`changed = policy.Gate("df.dfe") ? EliminateDeadFlows() : false;`). The surrounding `RemoveUnusedViews()` on line 883 is **unconditional required hygiene** and never consults the policy (P1 pinned contract §2b, called out in-line) — disabling `df.dfe` does not stop unused-view removal, only taint-based dead-flow propagation. | Enabled | Body-resident. Disabled by `-disable-dataflow-opt`/`none`. |
| 5 | `df.sink` | `lib/DataFlow/Optimize.cpp:837`, `QueryImpl::Optimize`'s `do_sink` lambda | Nothing observable — **the entire lambda body (lines 840-855) is commented out** (union-sinking re-canonicalization). `do_sink()` is called 4× in `Optimize` but every call is a no-op regardless of the gate. | Enabled | Body-resident (`kDataFlowBody`), so it IS toggled by `-disable-dataflow-opt`/`none` — but toggling it has zero observable effect today. **Dead/vestigial gate.** Matches the CLAUDE.md note: "Union sinking (`do_sink`)... is commented out — `lib/DataFlow/Merge.cpp` sinking code is currently unreachable." |
| 6 | `df.ident_join` | `lib/DataFlow/Optimize.cpp:895`, `QueryImpl::Optimize` | The identity-join recognizer block: unconditional `TrackDifferentialUpdates`, the `EliminateIdentityJoins` fixpoint loop (bounded by `joins.Size()+1`), and its trailing re-`Canonicalize`. Landed later than the other 7 (commit `60608468`, "COST-MODEL EPOCH + the identity-join recognizer"). | Enabled | Body-resident — added to `kDataFlowBody` **in the same commit** that introduced it, so it IS disabled by `-disable-dataflow-opt`/`none` today. **See surprise below: the doc comments describing that alias were not updated.** |
| 7 | `cf.regionopt` | `lib/ControlFlow/Optimize.cpp:1286`, `ProgramImpl::Optimize` | Gated **per sweep** at the top of the `for (changed…)` loop — `break`s immediately, so region-flattening/no-op-removal/`Sort(depth_cmp)` never runs for that sweep or any later one. | Enabled | Body-resident (`kControlFlowBody`). Disabled by `-disable-controlflow-opt` (`cf.*`) and by `none`. Unaffected by `-disable-dataflow-opt`. |
| 8 | `cf.procdedup` | `lib/ControlFlow/Optimize.cpp:1395`, `ProgramImpl::Optimize` | An early `return` skips everything below — the procedure-deduplication pass (find/merge structurally-identical procedures). | Enabled | Body-resident. Disabled by `-disable-controlflow-opt`/`none`. |

## Wholesale-skip guards (not gates themselves, but co-authoritative)

Both `Optimize()` entry points are additionally guarded by
`PassPolicy::AnyBodyOptionalEnabled(PassLevel)` so that a policy which
individually gates off every body-resident pass for a level still behaves
exactly like the legacy "don't even call Optimize()" bool it replaced
(byte-identity contract — entering the loop and gating every pass is *not*
equivalent, because e.g. the `parallel_regions.Sort(depth_cmp)` /
`induction_regions.Sort(depth_cmp)` / `series_regions.Sort(depth_cmp)` calls
inside the ControlFlow sweep are emission-visible even on a no-op pass):

- DataFlow: `lib/DataFlow/Build.cpp:2606` — `if (policy.AnyBodyOptionalEnabled(PassLevel::kDataFlow)) { impl->Optimize(log, policy); }`, checked against `kDataFlowBody = {df.cse, df.canon, df.dfe, df.sink, df.ident_join}` (5 names).
- ControlFlow: `lib/ControlFlow/Build/Build.cpp:1588` and `:1597` — both `impl->Optimize(policy)` call sites (Program::Build runs region optimization twice, once before `ExtractPrimaryProcedure`, once after) share the same `AnyBodyOptionalEnabled(PassLevel::kControlFlow)` predicate, checked against `kControlFlowBody = {cf.regionopt, cf.procdedup}` (2 names).

## The two legacy mode aliases (CLI, `bin/drlojekyll/Main.cpp`)

- `-disable-dataflow-opt` / `--disable-dataflow-opt` (Main.cpp:401-405) → `PassPolicy::DisableDataFlowOpt()` (PassPolicy.cpp:100-106) → disables every name in `kDataFlowBody` = **`df.cse, df.canon, df.dfe, df.sink, df.ident_join`** (5 globs, exact names — never `df.*`, deliberately, so `df.simplify` and the unregistered `df.demand` keep running).
- `-disable-controlflow-opt` / `--disable-controlflow-opt` (Main.cpp:408-413) → `PassPolicy::DisableControlFlowOpt()` (PassPolicy.cpp:108-112) → disables the single glob **`cf.*`** (matches both `cf.regionopt` and `cf.procdedup`).
- The 4 golden modes (`tests/OptDiff/runall.sh:162-164`) are literally these two flags combined: `opt` = neither flag, `nodf` = `-disable-dataflow-opt`, `nocf` = `-disable-controlflow-opt`, `none` = both.
- Generic surface: `-opt-disable=<glob,glob,...>` / `-opt-only=<glob,glob,...>` (Main.cpp:417-456) and the shared cross-level `-opt-bisect-limit=<N>` (Main.cpp:457-467; only `-1` "run-all-and-print" or `>= 0` "skip index > N" are CLI-reachable — the internal `-2` "unset" sentinel is only the value of a default-constructed `PassPolicy` when the flag is never passed at all). Every glob on both `-opt-disable=`/`-opt-only=` is validated twice at parse time: `IsValidGlob` (shape: prefix-star-of-a-dotted-stem or exact, no `?`/`[`/`]`) and `MatchesAnyKnownPass` (must hit something in `kAllPasses`) — a malformed or unmatchable glob is always a clean diagnostic, never a silent no-op (this closes the "silent-neuter" trap the P1 Fable review caught: an `-opt-only=` typo used to be able to invert into "disable everything").

## (a) Passes deliberately NOT gated

- **`ApplyDemandTransform` / `df.demand` — confirmed, exactly as the seed doc claims.** `lib/DataFlow/Build.cpp:2587` calls `impl->ApplyDemandTransform(module, log, demand_mode, demand_retract)` completely outside any `PassPolicy` gate. `df.demand` is explicitly absent from `kAllPasses` (PassPolicy.cpp:37-43, comment: "df.demand is deliberately ABSENT: `-demand` is a semantic flag the policy never gates in P1... naming it here would re-open the silent-neuter trap"), and Build.cpp:2582-2586 repeats the rationale in-line: `-demand` is semantics (its own CLI flag), not an optimization — a pass policy or bisect limit silently neutering it would also silently swallow its clean diagnostics (e.g. the `demand_multi_adorn_1` reject class). `RemoveUnusedViews`/`ClearGroupIDs`/`TrackDifferentialUpdates`/`ConnectInsertsToSelects`/`IdentifyInductions`/`Stratify`/`FinalizeDepths`/`FinalizeColumnIDs`/`TrackConstAfterInit`/`BuildEquivalenceSets` in `Query::Build` are likewise all REQUIRED build steps, never gated (matches the header's own framing: "REQUIRED stages (build steps, stratification, DeltaRel derivation/validation, codegen) never consult it").
- `-demand-instance` (the nested keyed-instance lowering) is also OFF the registry entirely — it's a lowering *selector*, not a pass, gated only by its own boolean parameter threaded through `Program::Build` (`lib/ControlFlow/Build/Build.cpp:1310` `demand_instance` param), never a `PassPolicy` name, and Main.cpp:476-477 says so explicitly ("never a registered pass name, never a 5th golden mode").

## (b) Dead/vestigial gates

- **`df.sink`** is registered, wired into the alias set, and toggleable via `-opt-disable=df.sink` — but `do_sink()`'s entire body (Optimize.cpp:840-855) is commented out, so gating it has **zero observable effect** on any current program. This is consistent with the standing CLAUDE.md note that union-sinking is unreachable code.

## Surprises

1. **Stale doc comments undercount `kDataFlowBody` by one name (`df.ident_join`).** Both comments in `include/drlojekyll/Util/PassPolicy.h` — the file-header "byte-identity contract" (lines 12-14: `"-disable-dataflow-opt == -opt-disable=df.cse,df.canon,df.dfe,df.sink"`) and the `AnyBodyOptionalEnabled` doc comment (line 61: `"df body = cse, canon, dfe, sink"`) — list only 4 names. The *actual* `kDataFlowBody` array in `lib/Util/PassPolicy.cpp:33-34` has had 5 names (plus `df.ident_join`) since commit `60608468` (the identity-join recognizer), and `DisableDataFlowOpt()`/`AnyBodyOptionalEnabled` both walk that same array. So today `-disable-dataflow-opt` (and hence the `nodf`/`none` golden modes) really does disable `df.ident_join` too — the header comments just weren't updated when the 5th body-resident pass was added. Not a functional bug (the code is internally consistent — array is the single source both call sites share), but a landmine for anyone trusting the header's prose contract over the array.
2. **`df.demand`'s absence is deliberate and doubly-documented** (PassPolicy.cpp comment + Build.cpp comment), exactly matching the seed pseudocode's claim quoted in the task — good corroboration, not a discrepancy.
3. **The bisect counter (`bisect_counter`) is a single cross-level monotone counter** shared by every DataFlow *and* ControlFlow gate call, ticked in program order — `-opt-bisect-limit=N` therefore truncates "the first N pass *applications* across both IRs combined," not per-IR.
4. **Granularity differs sharply by gate**: `df.canon` and `cf.regionopt` are checked *inside* their fixpoint loops (per-round / per-sweep — finest bisect grain), while the other 6 are checked exactly once per `Optimize()` invocation (coarse, all-or-nothing for that call).
5. `df.dfe`'s disable is narrower than it looks: it stops `EliminateDeadFlows()` only, not the `RemoveUnusedViews()` that brackets it — "required graph hygiene" runs regardless (explicit P1 pinned-contract comment at Optimize.cpp:884-885).
6. The `Canonicalize` function signature still carries a `const PassPolicy *policy = nullptr` defaulted pointer parameter (`Query.h:1047`, guarded with `if (policy && …)` at the call site), but no live caller in the current tree ever passes `nullptr` — all 3 in-`Optimize` call sites pass `&policy`. Effectively vestigial defensive code today.

## Registered-gate name list (8)

```
df.simplify
df.cse
df.canon
df.dfe
df.sink
df.ident_join
cf.regionopt
cf.procdedup
```
