# Test-matrix proposal — RegionalDataFlowCore epoch

Synthesis of the two Phase-5 drafts, folded against their empirical
verification ledgers (`scratchpad/phase5/{covering-array,feature-mixing}-verified.md`,
verifier tip f0c913e0) and the five stage artifacts (`stage-{a,b,c,d}-diff.md`,
`stage-i0-interpreter.md` + the four desired-state docs). Two deliverables:
a **gate×mode covering array** (optimizer-soundness sweep over the 8 PassPolicy
gates) and a **feature-mixing directed corpus** (demand × every language axis).
Both are refereed against *existing* goldens / `bin/Oracle` recomputes — no new
golden is blessed by either instrument.

Every prediction below carries a **verification status** from the empirical
pass. The four **PREDICTION-FAILED** findings are collected in §4 as the
owner's adjudication input; two of them (the `df.dfe` carve-out and the
`:-`/`@barrier` demand fence) change the design and are already folded into
§1/§2 below.

---

# 1. The gate × mode covering array

## 1.1 The eight axes

The 8 registered PassPolicy gates (`lib/Util/PassPolicy.cpp`), each on/off,
CLI-selectable as `-opt-disable=<name[,name…]>`:

| # | Label | Gate name | Body set | Disabling it skips |
|---|-------|-----------|----------|--------------------|
| 1 | **S** | `df.simplify`   | — (never aliased) | `QueryImpl::Simplify` (first DF pass, pre-demand) |
| 2 | **C** | `df.cse`        | kDataFlowBody | the CSE-to-fixpoint loop (×3) |
| 3 | **N** | `df.canon`      | kDataFlowBody | canonicalize fixpoint — **incl. `QueryKVIndexImpl::Canonicalize`, KVIndex.cpp:144** |
| 4 | **D** | `df.dfe`        | kDataFlowBody | `EliminateDeadFlows()` only (RemoveUnusedViews stays) |
| 5 | **K** | `df.sink`       | kDataFlowBody | **nothing — body commented out (vestigial no-op)** |
| 6 | **J** | `df.ident_join` | kDataFlowBody | identity-join recognizer (folds demand raw_seed→d_reader, §20(AV)) |
| 7 | **R** | `cf.regionopt`  | kControlFlowBody | region flatten / no-op removal / depth Sort |
| 8 | **P** | `cf.procdedup`  | kControlFlowBody | structural procedure dedup |

**Verification (V1/V2/V3):** all 8 names accepted exactly as spelled (each
single-off = exit 0 on join_1); `df.bogus` is a clean exit-1 ("matches no
registered pass"). `df.simplify` is confirmed absent from every alias
(gated separately at Build.cpp:2563); the two aliases expand as the lattice
table claims. **VERIFIED-EMPIRICALLY.**

## 1.2 Why the 4 golden modes leave gaps

The 4 modes are four fixed points that flip gates in correlated blocks:

```
             S  C  N  D  K  J  R  P     (1=ON, 0=OFF)
opt          1  1  1  1  1  1  1  1     neither alias
nodf         1  0  0  0  0  0  1  1     -disable-dataflow-opt  (5 df-body OFF as a block)
nocf         1  1  1  1  1  1  0  0     -disable-controlflow-opt (2 cf OFF as a block)
none         1  0  0  0  0  0  0  0     both aliases
```

Structurally uncovered: **S is ON in all four modes** (no alias touches it);
**intra-df-body mixed pairs** (the df alias flips {C,N,D,K,J} as one block, so
`{C0,N1}`, `{N0,D1}`, `{J0,C1}`… are never seen); **the cf mixed pair**
`{R0,P1}`/`{R1,P0}`.

## 1.3 The array — 9 new configs (13 total, ≤15 budget)

Each new config is a single-gate-off delta from `opt` (a divergence points at
exactly one suspect) plus one `all-off` closure. The case's `.drflags` sidecar
composes orthogonally (demand cases keep `-demand`).

```
config          S C N D K J R P   -opt-disable=…        role / isolates
opt        [m]  1 1 1 1 1 1 1 1   (none)                all-on baseline (existing)
nodf       [m]  1 0 0 0 0 0 1 1   (alias)               df-body block off (existing)
nocf       [m]  1 1 1 1 1 1 0 0   (alias)               cf block off (existing)
none       [m]  1 0 0 0 0 0 0 0   (alias)               both blocks off (existing)
s-off           0 1 1 1 1 1 1 1   df.simplify           the S=0 space no mode reaches
cse-off         1 0 1 1 1 1 1 1   df.cse                CSE alone (group_ids guard skipped)
canon-off       1 1 0 1 1 1 1 1   df.canon              *** kvindex_1 split isolator ***
dfe-off         1 1 1 0 1 1 1 1   df.dfe                *** DFE-load-bearing isolator (PF1) ***
sink-off        1 1 1 1 0 1 1 1   df.sink               no-op assertion (§1.4)
ident-off       1 1 1 1 1 0 1 1   df.ident_join         *** demand × ident_join probe ***
regionopt-off   1 1 1 1 1 1 0 1   cf.regionopt          regionopt alone
procdedup-off   1 1 1 1 1 1 1 0   cf.procdedup          dedup alone
all-off         0 0 0 0 0 0 0 0   (all 8)               S=0 both-off closure + total-dark
```

`[m]` = existing golden mode. **9 new configs.** No config is redundant with a
mode: `all-off ≠ none` (differs only in S); `regionopt-off`/`procdedup-off ≠
nocf` (each keeps its sibling ON — the missing mixed `{R,P}` combos); each
`*-off` df config keeps 4 siblings ON where `nodf`/`none` flip all 5.

### Pairwise-coverage argument (strength-2 CA over the 8 axes)

- **{A1,B1}** — `opt`. ✓ (28/28 pairs)
- **{A0,B1}/{A1,B0}** — `A-off` gives (A0,B1); `B-off` gives (A1,B0). Every
  gate has a single-off config ⇒ every mixed combo covered. ✓ (56/56)
- **{A0,B0}** — both ∈{C,N,D,K,J}: `nodf`; {R,P}: `nocf`; one-df/one-cf:
  `none`; {S, anything}: `all-off`. ✓

Every (pair, sign-combo) is hit; the 13 configs are a complete strength-2
covering array. **Verification status (U2):** UNVERIFIABLE-TODAY as a runnable
property — this is a combinatorial argument over the config table (sound on
inspection); the CLI mechanism realizing each row is verified.

### 1.4 `df.sink` (K) kept as a no-op axis

Retained because `do_sink`'s body is commented out (Optimize.cpp:840-855):
`sink-off` asserting byte-identity to `opt` is a standing "union sinking stays
dead" tripwire (CLAUDE.md invariant), and its pairwise obligations are
vacuously satisfiable (K fires nothing, perturbs no bytes). **We do not pretend
the array hunts K-interactions** — its value is the no-op assertion.
**Verification (V4):** `merge_2/sink-off` and `kvindex_1/sink-off` are
byte-identical to opt. VERIFIED-EMPIRICALLY.

## 1.5 THE INVARIANT and its two carve-outs

**Thesis:** all 8 gates are semantics-preserving optimizations (contrast
`-demand`, a *semantic* flag deliberately un-gated). The standing 4-mode
contract `opt==nodf==nocf==none==golden` **extends to every point in the 2⁸
lattice**: each config's stdout is byte-identical to the case's existing
`goldens/<case>.stdout`. **No new golden files.** Referee: primary `cmp -s`
against the existing golden; on any diff, adjudicate with `permcheck.py`
(within-epoch published-delta permutation is benign; any diff outside that
envelope is a genuine FAIL). Dumps (`-df-out`/`-rel-out`/`-ir-out`/`-cpp-out`)
legitimately vary by config and are **never** compared; `.irgold` stays
mode-scoped (the 4 named modes only); `.oracle`/`.monotone`/`.eqgate` are
answer/mode-scoped and are **not** re-run per config.

**Verification (V5/V6):** the invariant holds end-to-end (`cmp -s` golden) for
every non-load-bearing config across the witness corpus, and the full-corpus
(181-case) compile-only sweep shows the 5 never-load-bearing df configs
(s-off/cse-off/sink-off/ident-off) + the 2 cf configs perturb **zero** exit
statuses anywhere. VERIFIED-EMPIRICALLY. Residual (U3): the full
compile-build-run per never-load-bearing config was not run at 180-case scale
(low residual risk — these 5 are compile-identical to opt everywhere).

### Carve-out A — kvindex_1 under `df.canon`-OFF (predicted, confirmed)

`kvindex_1`'s KVINDEX→TUPLE elimination lives in `QueryKVIndexImpl::Canonicalize`
under gate **N**. It is **load-bearing for compilation**: `edge_weight` uses
`add_u32`, which declares no algebra, so if canon does not fold the value-less
KVINDEX to a TUPLE the survivor hits the V-ALGEBRA reject (Build.cpp:1380).

- **N=1 configs** → byte-match `goldens/kvindex_1.stdout`:
  `opt, nocf, s-off, cse-off, dfe-off, sink-off, ident-off, regionopt-off,
  procdedup-off`.
- **N=0 configs** → `expect_diagnostic` (clean exit-1, the V-ALGEBRA reject):
  `nodf, none, canon-off, all-off`.

**Isolating pair:** `canon-off` vs `opt` — the single gate that splits the
outcome. **Verification (V7/V8):** confirmed end-to-end; a full 181-case
`canon-off` sweep shows kvindex_1 is the **only** case whose exit differs from
opt (0→1). VERIFIED-EMPIRICALLY.

### Carve-out B — `df.dfe`-OFF aborts Stratify (PREDICTION-FAILED → new carve-out) — RESOLVED 2026-08-02: F26 landed the hygiene/optimization split (owner-ratified D3.1); dfe-off now golden-matches the full corpus end-to-end and this carve-out is GONE (see covering-array-verified.md Addendum)

**This was not in the draft's expectation ledger.** `dfe-off`
(`-opt-disable=df.dfe`, D0 with C=N=1) **aborts the compiler** — SIGABRT, exit
134, `Assertion failed … Stratify.cpp:420` — on **four** cases:
`deadflowelimination_1`, `deadflowelimination_2`, `deadflowelimination_4`, and
**`recursion`**. `EliminateDeadFlows` (df.dfe) is **load-bearing for the
Stratify precondition** when canonicalization/CSE are active: with dead flows
un-collected, `Stratify.cpp:420`'s per-stratum invariant trips. These four
cases compile **cleanly** under `nodf` and `all-off` (which also disable
canon/cse, keeping the graph Stratify-safe) — the abort surfaces **only** at
the single-gate `dfe-off` point, which no golden mode and not even `all-off`
reaches. This is the sharpest possible confirmation of the array's
fault-localization premise *and* a refutation of the blanket §3 invariant.

**Derived config-dependent expectation for `deadflowelimination_{1,2,4}` +
`recursion`:**
- **dfe-off (D0 while N=1)** → **expect abort** (exit 134), or exclude the cell.
- every other config → byte-match the existing golden.

**Isolating pair:** `dfe-off` vs `nodf` — same D0 in both, but `nodf` also
zeroes C/N and compiles clean, so the pair isolates the abort to *dfe-OFF-while-
canon-ON*. **Verification (PF1/PF2):** PREDICTION-FAILED — the draft's §4.5
hedge ("a later pass … could abort") is exactly what happens, but §3 provided
no carve-out and the array as specified would go RED. Now folded in. Only
`deadflowelimination_3/5/6` compile clean under dfe-off.

### Gate-orthogonal diagnostics — reject under ALL 13 configs

These reject via passes not among the 8 gates (Stratify, Functor.cpp algebra,
lexer/parser, the demand pass, the ControlFlow C-2 pre-pass), so they
`expect_diagnostic` under every config (reuse runall.sh's diagnostic dispatch):
`kvindex_2/3/4`, `agg_in_scc_1`, `kv_in_scc_1`, `evm_func_parse`,
`algebra_dup_1`, `algebra_conflict_1`, `nonascii_1`, `truncated_decl_1`,
`negate_never_diff_1`, `demand_multi_adorn_1`, `demand_multi_adorn_allfree_1`,
`demand_cyclic_1`, `demand_recursive_content_1`.

## 1.6 Predicted latent splits — verification roll-up

| Probe | Prediction | Status |
|---|---|---|
| **canon-off → kvindex_1** compile/reject split (carve-out A) | the sole compilation split | **VERIFIED (V7/V8/V9)** |
| **ident-off × -demand** (THE priority probe, open Q#2) | stdout-invariant; risk = a V-INST-* belt assuming the fold ran | **VERIFIED SAFE (V10)** — `demand_tc_witness`, `demand_neighborhood_mono_witness`, `demand_multi_adorn_witness` all golden-match under `-demand -opt-disable=df.ident_join`; recognizer tolerates raw_seed un-folded, no belt abort |
| **s-off / all-off** (untested S=0 space) | stdout-invariant; abort = unstated simplify precondition | **VERIFIED SAFE on witnesses (V11)** — `demand_tc_witness` golden-matches under s-off/cse-off; `barrier_neck_1` under s-off/all-off |
| **cse-off** (C0,N1; group_ids guard skipped) | stdout-invariant | **VERIFIED (V5/V11)** |
| **dfe-off** (D0) | "stdout stays invariant" | **PREDICTION-FAILED (PF2)** → carve-out B |
| **regionopt-off / procdedup-off** cf mixed pairs | stdout-invariant | **VERIFIED (V12)** — `cf13_1/2/3` golden-match each single cf config; neither perturbs any exit status corpus-wide |

## 1.7 Run harness sketch — `optmatrix.sh` (not a 5th golden mode)

A wrapper `tests/OptDiff/optmatrix.sh <workroot> [jobs] [name-filter]` reusing
runall.sh's per-case worker with the flags string parameterized by the config
table above. Per `(config, case)`: compile `-opt-disable=<OFF gates>` **plus**
the case's `.drflags`; build+run the `.main.cpp`; `cmp -s` against the
**existing** golden; dispatch diagnostics via runall.sh's case-name switch,
**augmented** with the two config-dependent rules (carve-out A: diagnostic iff
`df.canon` OFF; carve-out B: expect-abort iff `df.dfe` OFF while canon ON, for
the four DFE cases). Per-config workroot `<workroot>/<config>/…`; verdicts
aggregate to `SUITE-MATRIX: PASS/FAIL`.

**Why not a 5th mode:** zero new goldens; **no `--bless` path for configs** (a
config can never mint/update a golden); no sidecar re-runs. Tiering (perf-only):
a **smoke tier** (the ~15 §1.6 interaction witnesses + the 4 DFE cases +
kvindex_1 × all 9 configs) and a **full tier** (180×9 ≈ 1620 runs). Run serial
w.r.t. the golden suite, never concurrent with a bench run.

**Verification (U1):** the wrapper is UNVERIFIABLE-TODAY (not built); the
mechanism it relies on (`.drflags`+`-opt-disable` composition, `cmp -s` vs
existing golden) is verified. **Gotcha:** the composed flag string needs
`${=flags}` zsh word-splitting (a single-string flag arg is rejected as one
token).

---

# 2. The feature-mixing directed cases

All 14 crossings (15 `.dr` cases) were compiled in **all 4 optimization modes**
with the draft's flags. **All 9 negatives exit rc=1 with no abort/SIGABRT** —
`expect_diagnostic` passes for every negative regardless of the reject-site
corrections below (message text is not golden-compared).

## 2.1 PRINCIPAL FINDING — the R-BODYWALK reject-site row is over-broad

The draft's vocabulary gave **R-BODYWALK** one message and claimed it fires on
NEGATE/@never/AGG/KV/(some)@product/recursive-content. Empirically the
demanded-body rejects split into **THREE distinct diagnostics**:

| Actual message (verbatim) | Fires on (verified) | Draft mis-named |
|---|---|---|
| `Demand does not propagate through a negation or aggregate (the demand sink); … recompile without -demand` | `!`-NEGATE, `@never`, `over(){}` AGG, config `@recompute` AGG | R-BODYWALK → **the "demand sink" site** |
| `The demanded query must project from a single derived relation under -demand…` | KV `mutable()` body | R-BODYWALK → **the R-MAT family** (`:446`) |
| `Unsupported rule-body shape under -demand…` | `@product`, `@barrier`/`:-` chain, mutual recursive-content | R-BODYWALK (**CORRECT**) |

**Consequence:** crossings 1, 6, 11 (agg/@never/config-agg) hit the **demand
sink** diagnostic; crossing 2's `!` body also hits demand-sink; crossing 5 (KV)
hits **R-MAT**. Only crossings 7/10/12 (@product / barrier / mutual content)
surface the literal R-BODYWALK string. The reject **outcome** is correct in
every case; the *named site* is corrected throughout §2.3. Corrected vocabulary:

| Tag | Site | Fires on |
|---|---|---|
| **R-1BOUND** | Demand.cpp Step 1 `:435` | >1 bound `#query` NAME |
| **R-MAT** | Demand.cpp Step 1 `:446` | ≠1 materialization — incl. a **KV** demanded body ("single derived relation") |
| **R-ALLFREE** | Demand.cpp Loop-1 `:497` | a bound name carrying an all-free sibling adornment |
| **R-SINK** | Demand.cpp body-walk | negation/aggregate demand sink — `!`, `@never`, `over(){}`, config `@recompute` |
| **R-BODYWALK** | Demand.cpp body-walk `:501-783` | `@product`, `@barrier`/`:-`, mutual recursive-content ("Unsupported rule-body shape") |
| **R-CYCLIC** / **R-RECCONTENT** | Build.cpp `:1467-1473` (`-demand-instance` only) | recursive / induction-owned demanded relation |
| **R-STRATIFY / R-VALGEBRA** | Build.cpp `:2632` / C-2 `:1332-1411` (demand-independent) | unstratified agg/neg; algebra-less KV; etc. |

## 2.2 The H-J re-adjudication set

Cases whose reject is a **Stage-C inadmissibility** become an owner-decided
variant flip (E2, stage-c-diff §H-J): **Variant A** = inadmissible extraction
flips to full materialization (compile, answer-identical); **Variant B** =
retain recursion rejects only. Each such case below is marked **[H-J]** with its
A/B fate.

## 2.3 Per-case adjudication

Format: name · axes · flags · expected outcome (derivation) · **status** ·
stage gate → becomes.

### C1 — `demand_agg_body_1` · aggregate inside demanded body · `-demand`
- NEGATIVE. `over(){}` AGG reached by SIP walk → **R-SINK** (corrected from
  R-BODYWALK). **Status: VERIFIED (outcome)** + **PREDICTION-FAILED (site)**.
- Gates **Stage C** admissibility of *stateful/pure* slices (§8.1). **[H-J-
  adjacent, OPEN]** — whether an aggregate slice is admissible is resolved by
  no stage doc; this case forces the question. Variant A/(i): aggregate slice
  admissible → flips to a compiling full extraction (Oracle golden = full
  `indeg` filtered by bound key). Variant (ii): inadmissible → full
  materialization in the observation root (answer-identical). Either way it
  stops being a reject at Stage C. **Author `.batches`/`.oracle.stdout` NOW**
  (`count_i32` is in the Oracle envelope).

### C2a — `demand_negate_body_1` · `!` inside demanded body · `-demand`
- NEGATIVE. `!seen` → **R-SINK** (corrected). **Status: VERIFIED (outcome)** +
  **PREDICTION-FAILED (site)**.
- Gates **Stage C** admissibility (negation is set-definable, not "pure" in the
  effect sense). I0-refereeable. Owner-owed (not in the H-J four).

### C2b — `demand_negate_consumer_1` · downstream consumer negates demanded rel · `-demand`
- NEGATIVE. `!path` is a full-relation reader of the demanded/guarded relation
  → **stray-consumer belt** (`The demanded relation is read by a consumer demand
  cannot guard…` — exactly the predicted message). **Status: VERIFIED
  (outcome + site).** Did NOT compile ⇒ no miscompile (the alt-branch hazard is
  refuted). *Note:* the sketch also trips a prerequisite first
  (`missing/2 … not marked @differential`, because `!path` makes `missing`
  deletion-capable) — mark `missing` `@differential` for a single-diagnostic
  golden; the stray-consumer belt fires either way.
- Gates **Stage C** `RoutedResult` consolidation: a full-relation negation
  consumer must read the ORDINARY pub (H-G.2 shared-pub), which is what makes
  it compile correctly post-cutover.

### C3 — `demand_multi_adorn_diff_1` · multi-adornment × diff-input (flat) · `-demand`
- POSITIVE (composition — VERIFY). **exit 0 all 4 modes.** **Status: VERIFIED
  (compiles).** The R-DUP guard-union ∘ band-(a2') removal-arm composition does
  not reject.
- Gates the **I0 stage** (a diff-input multi-adornment `.batches` referee before
  Stage C touches the multi-adornment guard-union). At Stage C the two
  adornments become two `RequestEdgeRelation` call-sites over one
  `ChildResultRelation` — the D3.a.3 refcounted-union-pub is made OBSOLETE by
  `RoutedResult` (H-F); `.stdout` stays byte-identical, the SHAPE goldens
  change. Oracle-answer + published-delta identity: UNVERIFIABLE-TODAY.

### C4 — `demand_beside_recursion_1` · recursion BESIDE demanded subgraph · `-demand`
- POSITIVE. **exit 0 all 4 modes.** **Status: VERIFIED (compiles).** SIP walk
  from the bound query never reaches the disjoint `path` SCC.
- **Stage B/D boundary witness:** the planner must NOT annex `path` into the
  demanded region. Stage B byte-identical; Stage C extracts `pair` as a pure
  child, leaves `path` in the observation root; Stage D never enters a region
  SCC for `path` (not demand-derived). A regional dump showing `path` in
  `RegionId(0)` and `pair` in a child is the expected shape. I0-refereed.

### C5 — `demand_kv_body_1` · KV merge as demanded producer · `-demand`
- NEGATIVE. `balance` is a KV index in the demanded body → **R-MAT** (corrected
  from R-BODYWALK — "single derived relation"). The `@invertible`-avoids-
  V-ALGEBRA reasoning DOES hold (no V-ALGEBRA reject); the reject simply lands
  one site earlier. **Status: VERIFIED (outcome)** + **PREDICTION-FAILED
  (site)**.
- Same family as C1 (KV = degenerate aggregate). Gates **Stage C** stateful-
  slice admissibility (`StateCellStore`+`GROUP_UPDATE` carried forward
  unchanged). **[H-J-adjacent].** **Author `.batches`/`.oracle.stdout` NOW.**

### C6 — `demand_never_body_1` · `@never` inside demanded body · `-demand`
- NEGATIVE. `@never seen(A)` (a negation form) → **R-SINK** (corrected).
  **Status: VERIFIED (outcome)** + **PREDICTION-FAILED (site)**. *Note:* the `!`
  twin (C2a) and this `@never` case land the **identical** demand-sink message —
  so the two are indistinguishable as demand rejects today (the "must reject
  BOTH, must not CSE them" intent holds at the reject level; "structurally
  distinct rejects" is not observable at the diagnostic).
- Gates **HIGH** — Stage C's H-G.3 makes demand uniformly retractable, so the
  MONO-irrevocability special case disappears; `@never` content extracting
  post-cutover must keep its `Present`-keyed semantics under a retractable
  request edge. Negative fence pinning the boundary before the collapse; stays a
  reject unless the owner explicitly admits `@never` slices. I0 referees the SET
  answer only (@never ordering unrefereed — stage-i0 §2 hole #4).

### C7 — `demand_product_body_1` · `@product` inside demanded body · `-demand`
- NEGATIVE (was OPEN). **Sketch fix applied** (`node(N) : add_node(N).` — the
  draft left `node` unpopulated). `allpairs @product` (zero-pivot JOIN) → exact
  **R-BODYWALK** match. **Status: VERIFIED (outcome + site).** OPEN resolves to
  the REJECT branch.
- Gates **Stage C** admissibility: `@product` is pure (transports no effect), so
  §8.1 would ADMIT a product slice — today's reject likely **FLIPS to a
  compiling extraction** at Stage C (a capability add, Oracle-refereed).

### C8 — `demand_diff_pub_1` · differential-PUBLISHED demand answer · `-demand -demand-retract` · eqgate `+ -demand-instance`
- POSITIVE (drives the unexercised R-DIFF/E1 arm). **exit 0 all 4 modes** flat
  AND **exit 0 all modes** with the `-demand-instance` eqgate arm added.
  **Status: VERIFIED (compiles, flat + instance).** The "answer SHRINKS on
  retract" behavior and the E1 `if (diff)`-arm reachability (`DRInstance::
  differential == true`) are RUNTIME/introspection claims → UNVERIFIABLE-TODAY
  (needs the `.batches` execution + counter inspection).
- Gates **HIGHEST** — this is the **stage-i0 H8 / OG3** witness and the **SOLE
  pre-cutover oracle for the arm Stage C makes the only arm** (H-G.3; stage-c
  E1 declares the Stage-C exit gate DEPENDS on this case or one authored at I0).
  `.batches`: epoch 1 add+probe, epoch 2 remove an in-neighborhood edge while
  demand stands → the answer row retracts through the `@differential nbhd_out`
  tap. If the pub is always provisioned monotone (arm dead), the discovery that
  the `if (diff)` arm is unreachable is itself the finding. **THE top-ranked
  case — author now.**

### C9 — `demand_multi_adorn_diff_nested_1` · diff-input × multi-adorn × nested · `-demand -demand-retract` · eqgate `+ -demand-instance`
- OPEN (triple composition — VERIFY). **exit 0 all 4 modes** flat AND nested.
  **Status: VERIFIED — OPEN resolves to COMPILE.** flat==nested==golden identity:
  UNVERIFIABLE-TODAY (runtime eqgate).
- Gates **Stage C** `RoutedResult` differential realization for the multi-owner
  case; stresses the E1 arm under two adornments (each = a distinct
  `RequestEdgeRelation` owner over one differential `ChildResultRelation`).
  Closest pre-cutover analogue of the §12.3 multiple-owners witness (stage-i0 E3
  says it has NO pre-cutover referee) — worth authoring to shrink that hole.

### C10 — `demand_barrier_body_1` · `@barrier`/`:-` in demanded body · `-demand`
- POSITIVE (VERIFY) → **REJECT all 4 modes**, exact **R-BODYWALK**. **Status:
  PREDICTION-FAILED (headline).** The primary "EXPECTED COMPILE" is false; the
  draft's pre-registered alt is what happened: **`:-`/`@barrier` IS a demand
  fence today** — the SIP walk does not traverse a barrier-staged join chain.
  That is the case's registered finding. The `bin/Oracle` positive-answer plan
  is moot until/unless the fence is lifted.
- Gates **Stage C** local-graph lowering inside a region (barrier staging is a
  local-graph property Stage C carries forward). Becomes: a regional dump showing
  the barrier'd binary-join staging preserved inside the extracted child — *if*
  the fence is lifted.

### C11 — `demand_config_agg_body_1` · config-column `@recompute` agg in demanded body · `-demand`
- NEGATIVE. config `@recompute` AGG → **R-SINK** (corrected). The config/
  `@recompute` arm makes no difference to the reject site. **Status: VERIFIED
  (outcome)** + **PREDICTION-FAILED (site)**.
- Same admissibility question as C1 on the config arm (which lands a
  codegen-emitted per-touched-group seal loop). **[H-J-adjacent].** **Author
  `.batches`/`.oracle.stdout` NOW** (mirror `config_agg_2`).

### C12 — `demand_mutual_content_1` · mutual recursion INSIDE demanded body · `-demand`
- NEGATIVE. `ra` self-reachable via `rb` (two-view SCC) → exact **R-BODYWALK**
  (the shadowed recursive-content belt). **Status: VERIFIED (outcome + site).**
- Gates **HIGH (Stage D)** — the mutual-local-recursion witness (stage-d Part 6).
  Stage C: inadmissible → full materialization (answer-correct global recursion).
  Stage D: recursive_content fence LIFTED (Hunk 1.2 `InstanceClosedSCC`) →
  becomes a per-`(RegionId,InstanceId)`-qualified mutual fixpoint. **Referee gap
  (stage-d E2):** NO pre-cutover tagged binary (it rejects today), so I0 +
  Stage-C full-materialization answer-equivalence is the sole oracle. **[H-J]:**
  Variant B stays a reject; Variant A flips to full materialization. **Author
  `.batches`/`.oracle.stdout` NOW** — the E2 hole this case most needs pinned.

### C13 — `demand_beside_mutual_1` · mutual recursion BESIDE demanded subgraph · `-demand`
- POSITIVE. **exit 0 all 4 modes.** **Status: VERIFIED (compiles).** The positive
  twin of C12; the published `ra`/`rb` SCC is left alone when BESIDE, not INSIDE.
- Stage B byte-identical; Stage C leaves the SCC in the observation root; Stage D
  never enters a region SCC for it. Determinism companion to C4. I0-refereed.

### C14 — `demand_two_queries_1` · two independent bound query NAMES · `-demand`
- NEGATIVE. two distinct bound `#query` names → **R-1BOUND** (`Multiple demanded
  (bound) queries are not yet supported…`). **Status: VERIFIED (outcome +
  site).** Deliberately distinct from `demand_multi_adorn_1` (R-LEFTLIN) and
  `_allfree_1` (R-ALLFREE).
- Gates **Stage C** planner multiplicity: Stage C admits arbitrarily many
  observation roots (each bound query = a root request edge), so this fence is
  **EXPECTED TO LIFT** — two independent root leases + regions, answer-correct.
  Not in the H-J four; a clean capability add. Owner should confirm the
  single-bound-query restriction is a demand-transform artifact, not an
  architectural bound.

## 2.4 Cross-cutting notes

- **Twinning held at the OUTCOME level:** every beside-vs-inside pair (C4↔C12,
  C13) and the agg↔KV↔config-agg trio reject/compile as paired. The one axis
  that did NOT behave as assumed is `!`↔`@never` (C2a↔C6): both land the
  identical demand-sink message, indistinguishable as demand rejects today.
- **`.batches` authoring is REQUIRED** for the positives and the lift-candidates
  — a case whose reject Stage C/D lifts must carry `.batches`+`.oracle.stdout`
  NOW so the answer is pinned by `bin/Oracle` BEFORE the reject lifts (the E2
  hole).
- **Every "Stage gating" paragraph** (Stage B/C/D, I0, Oracle-answer, E1-arm
  reachability, RoutedResult/RequestEdge shape goldens) is future-architecture
  and **UNVERIFIABLE-TODAY** by construction — recorded, not refuted.

---

# 3. THE RANKING

Ordered by what the Stage A→I0→B→C→D diffs most need pinned **before they land**:
blocking-oracle-gap fillers first (anything covering the R-DIFF differential-pub
arm), then mode-split pins, then nice-to-have. **★ = land as an ordinary corpus
case PRE-Stage-A** (pins behavior before the architecture churns / fills a
declared blocking oracle hole).

### Tier 1 — blocking oracle gaps (must exist before their stage lands)

1. **★ C8 `demand_diff_pub_1`** — the R-DIFF/E1 differential-pub arm. stage-c
   E1 (MISSING ORACLE, blocking) + stage-i0 H8/OG3. The SOLE pre-cutover oracle
   for the single most behavior-bearing Stage-C hunk (the arm H-G.3 makes the
   only arm). Compiles today; author `.batches`+`.oracle.stdout`+`.eqgate` now.
2. **★ C12 `demand_mutual_content_1`** — stage-d E2 (MISSING ORACLE): no
   pre-cutover binary ever executed instance-evaluated local recursion. Author
   `.batches`+`.oracle.stdout` NOW so the Stage-D fixpoint-lift flip has a
   referee. **[H-J]** Variant-dependent fate.
3. **★ C1 `demand_agg_body_1`** — forces the Stage-C §8.1 stateful/pure-slice
   admissibility question (not resolved by any stage doc, not in the H-J four).
   Oracle-refereeable; author `.batches`+`.oracle` now so the reject-lift has an
   answer.
4. **★ C5 `demand_kv_body_1`** — the KV (degenerate-aggregate) arm of the same
   admissibility question; `StateCellStore`+`GROUP_UPDATE` carried forward
   unchanged. Author `.batches`+`.oracle` now.
5. **★ C11 `demand_config_agg_body_1`** — the config-column `@recompute` arm of
   the same question (per-touched-group seal loop). Author `.batches`+`.oracle`
   (mirror `config_agg_2`) now.
6. **C9 `demand_multi_adorn_diff_nested_1`** — stresses the E1 arm under two
   adornments; the closest pre-cutover analogue of the §12.3 multiple-owners
   witness (stage-i0 E3's un-refereed hole). Compiles; author to shrink E3.

### Tier 2 — mode-split / carve-out pins (the covering-array core)

7. **kvindex_1 canon-off carve-out (A)** — the sole *compilation* mode-split;
   isolating pair `canon-off` vs `opt`. Already a corpus case; the array adds
   the config-dependent expectation. Highest-certainty prediction (VERIFIED).
8. **dfe-off carve-out (B) on `deadflowelimination_{1,2,4}`+`recursion`** — the
   PREDICTION-FAILED finding: `df.dfe` load-bearing for Stratify. Isolating pair
   `dfe-off` vs `nodf`. Must be encoded as expect-abort in `optmatrix.sh` or the
   array goes RED. New coupling the array is designed to expose.
9. **ident-off × -demand probe** — open Q#2; VERIFIED SAFE but the highest-value
   demand×optimization coupling probe; the config to run first in the smoke tier.
   Retargets at the Stage-C cutover (raw_seed fold disappears with forcing).
10. **s-off / all-off S=0 space** — the entirely mode-untested `df.simplify`
    axis; VERIFIED SAFE on witnesses, but the single largest source of potential
    novel findings at full-corpus scale (an abort = an unstated simplify
    precondition).

### Tier 3 — nice-to-have / determinism companions

11. **★ C14 `demand_two_queries_1`** — pins the R-1BOUND separate-name fence
    (distinct from the two existing multi-adorn rejects); Stage C expected to
    lift it. Cheap negative worth landing now.
12. **★ C4 `demand_beside_recursion_1`** + **★ C13 `demand_beside_mutual_1`** —
    the beside-vs-inside determinism companions; positive twins that prove the
    planner does not annex a disjoint recursive SCC. Stage-B/D boundary
    witnesses; author `.batches` now.
13. **C2a/C2b/C6/C7/C10** — the remaining demand-body negatives (`!`, `@never`,
    `@product`, barrier). Reject-outcome pins; C10 carries the `:-` demand-fence
    finding (PF). Land as directed rejects (no `.batches` needed for the pure
    negatives).
14. **cse-off / regionopt-off / procdedup-off / sink-off** — the remaining
    covering-array configs; VERIFIED stdout-invariant. sink-off = the standing
    "union sinking stays dead" no-op tripwire. Interpretability/coverage, not
    finding-hunting.

**PRE-Stage-A landing set (★, 9 cases):** C8, C12, C1, C5, C11 (with
`.batches`+`.oracle`); C14, C4, C13 (C4/C13 with `.batches`); plus the two
covering-array carve-outs (A already exists, B is a new expectation, not a new
case). These pin the answers the Stage-A→D flips will need a referee for, before
the reject lifts remove the current diagnostic.

---

# 4. PREDICTION-FAILED findings (adjudication input)

Four failures across the two instruments. Two change the design (folded into §1/§2
already); two are site-name corrections that re-target the affected pins.

### PF-1 / PF-2 (covering array) — `df.dfe` is load-bearing for Stratify
`dfe-off` (`-opt-disable=df.dfe`, D0 while C=N=1) **aborts** — SIGABRT exit 134,
`Assertion failed … Stratify.cpp:420` — on `deadflowelimination_1`,
`deadflowelimination_2`, `deadflowelimination_4`, and **`recursion`** (a case
§4.5 did not name). The draft's §3 blanket invariant ("every lattice point →
stdout==golden") is **empirically false as stated**; §4.5's "could abort" hedge
is exactly what happens. **Requires carve-out B** (§1.5): expect-abort at
dfe-off-while-canon-ON for those four cases, or the array goes RED. This is a
genuine previously-unexposed coupling (invisible to nodf/none/all-off, which also
disable canon) — it *validates* the array's fault-localization premise. **This is
also a standalone finding worth a FINDINGS.md entry** independent of the matrix.

### PF-3 (feature-mixing) — the R-BODYWALK reject-site row is over-broad
The demanded-body rejects are **three** distinct diagnostics, not one (§2.1).
The **demand-sink** message fires on `!`/`@never`/`over(){}`/config-`@recompute`
(crossings C1, C2a, C6, C11); the **R-MAT** "single derived relation" message
fires on KV (C5); only `@product`/barrier/mutual-content (C7, C10, C12) surface
the literal R-BODYWALK string. The reject **outcome** is correct everywhere;
only the *named site* was wrong. Vocabulary corrected (added **R-SINK**);
crossings 1, 2a, 5, 6, 11 re-attributed. Sub-note: `!`↔`@never` (C2a↔C6) are
**indistinguishable** by message today (both = demand-sink).

### PF-4 (feature-mixing) — `:-`/`@barrier` IS a demand fence today
C10 `demand_barrier_body_1` was predicted **EXPECTED COMPILE**; it **REJECTs all
4 modes** with R-BODYWALK. The SIP walk does not traverse a barrier-staged join
chain — the draft's pre-registered alt branch. The case's registered finding:
`@barrier`/`:-` is a demand fence. The `bin/Oracle` positive plan is moot until
the fence is lifted; the case remains a valuable Stage-C lift-candidate.

---

# Return summary

## Ranking — top 8 (one-liners)

1. **C8 `demand_diff_pub_1` ★** — the R-DIFF/E1 differential-pub oracle; SOLE
   pre-cutover referee for the arm Stage C makes the only arm (stage-c E1 /
   stage-i0 H8-OG3). Compiles; author `.batches`+`.eqgate` now.
2. **C12 `demand_mutual_content_1` ★** — stage-d E2 blocker; no pre-cutover
   binary for instance-evaluated local recursion. Author `.batches` now. [H-J].
3. **C1 `demand_agg_body_1` ★** — forces the Stage-C §8.1 stateful/pure-slice
   admissibility decision (unresolved, not in the H-J four). Author `.oracle`.
4. **C5 `demand_kv_body_1` ★** — the KV (degenerate-aggregate) arm of the same
   admissibility question. Author `.oracle`.
5. **C11 `demand_config_agg_body_1` ★** — the config-`@recompute` arm of it;
   mirror `config_agg_2`.
6. **kvindex_1 canon-off carve-out (A)** — the sole compilation mode-split;
   isolating pair `canon-off` vs `opt`. Highest-certainty, VERIFIED.
7. **dfe-off carve-out (B)** — the PREDICTION-FAILED coupling; `df.dfe`
   load-bearing for Stratify on 4 cases. Must be encoded expect-abort.
8. **ident-off × -demand probe** — the highest-value demand×optimization probe;
   VERIFIED SAFE; run first in the smoke tier.

## PREDICTION-FAILED list (4)

- **PF-1/PF-2** — `df.dfe` load-bearing for Stratify; `dfe-off` aborts on
  `deadflowelimination_{1,2,4}`+`recursion` (Stratify.cpp:420). New carve-out B.
- **PF-3** — R-BODYWALK over-broad; three distinct sites (added R-SINK; KV=R-MAT);
  `!`↔`@never` indistinguishable by message.
- **PF-4** — `:-`/`@barrier` is a demand fence today; C10 rejects, not compiles.

## Total counts

- **Covering array:** 13 configs (4 modes + 9 new) · complete strength-2 CA over
  8 gates · 2 carve-outs (A kvindex_1/canon-off predicted-confirmed; B dfe-off
  PREDICTION-FAILED) · 12 VERIFIED-EMPIRICALLY, 2 PREDICTION-FAILED, 6
  UNVERIFIABLE-TODAY.
- **Feature-mixing:** 14 crossings / 15 `.dr` cases · 9 negatives + 6 positives ·
  compile/reject OUTCOME: 14 VERIFIED, 1 PREDICTION-FAILED (C10) · reject-SITE:
  4 correct, 5 PREDICTION-FAILED (C1/C2a/C5/C6/C11) · 1 sketch fix (C7).
- **PRE-Stage-A landing set (★):** 9 cases (C8, C12, C1, C5, C11, C14, C4, C13,
  + carve-out B as a new expectation).
- **PREDICTION-FAILED total: 4** (PF-1/PF-2 counted as one root cause on the
  array side; PF-3, PF-4 on the feature-mixing side).
