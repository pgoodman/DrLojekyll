# Feature-mixing directed cases — EMPIRICALLY VERIFIED

Verifier pass, 2026-08-02, tip f0c913e0. Compiler:
`build/debug/bin/drlojekyll`. Every `.dr` sketch below was written out and
compiled in **all 4 optimization modes** (opt / nodf / nocf / none) with the
draft's flags. Probes live in
`scratchpad/phase5/probe-feature-mixing/` (`*.dr`, `out/*.log`). No repo file,
test, or golden was touched.

Verdict vocabulary:
- **VERIFIED-EMPIRICALLY** — the draft's compile/reject OUTCOME reproduced in
  all 4 modes (clean exit-1, no crash, for rejects).
- **PREDICTION-FAILED** — reality diverged from the draft's stated prediction;
  both prediction and reality are kept below.
- **UNVERIFIABLE-TODAY** — a future-stage (Stage B/C/D / I0 / Oracle-answer /
  runtime-behavior) claim not decidable by compiling today.

## Counts

- **Compile/reject OUTCOME (15 cases):** 14 VERIFIED-EMPIRICALLY, 1
  PREDICTION-FAILED (`demand_barrier_body_1`).
- **Reject-SITE / message-shape sub-prediction (9 reject cases):** 4 correct
  (`product`, `mutual_content`, `two_queries`, `negate_consumer`), **5
  PREDICTION-FAILED** on the named site (`agg_body`, `negate_body`,
  `never_body`, `config_agg_body`, `kv_body`) — the R-BODYWALK vocabulary row
  is over-broad (see the finding below).
- **Stage-B/C/D gating, Oracle-answer, E1 differential-arm, nested==flat
  runtime equivalence:** UNVERIFIABLE-TODAY across every case (future-stage /
  runtime; noted per case, not re-counted).
- **Sketch fixes applied:** 1 (`demand_product_body_1`, added a missing
  `node(N) : add_node(N).` rule — see that case).

All 9 reject cases exit with rc=1 and no abort/SIGABRT — the
`expect_diagnostic` referee (exit-1 + no-crash + all-4-modes) passes for every
negative regardless of the site-name findings, because message text is not
golden-compared (draft's own cross-cutting note).

---

## PRINCIPAL FINDING — the R-BODYWALK reject-site row is over-broad

The draft's vocabulary table (§ "reject-site vocabulary") gives **R-BODYWALK**
one message — *"Unsupported rule-body shape under -demand"* — and claims it
fires on *"a NEGATE / @never / AGG / KV / (some) @product / recursive-content
node"*. Empirically the demanded-body rejects split into **THREE distinct
diagnostics**, not one:

| Actual message (verbatim) | Fires on (verified) | Draft named it |
|---|---|---|
| `Demand does not propagate through a negation or aggregate (the demand sink); this shape is not supported under -demand; recompile without -demand` | `!`-NEGATE, `@never`, `over(){}` AGG, config `@recompute` AGG | R-BODYWALK (WRONG) |
| `The demanded query must project from a single derived relation under -demand; recompile without -demand` | KV `mutable()` body | R-BODYWALK (WRONG — this is the **R-MAT** family, "≠1 materialization", Step-1 `:446`) |
| `Unsupported rule-body shape under -demand; recompile without -demand` | `@product`, `@barrier`/`:-` join chain, mutual-recursive content | R-BODYWALK (CORRECT) |

Consequence for the design: cases whose derivation says "AGG/NEGATE/@never/KV
node → R-BODYWALK" are pinning the wrong diagnostic. The negation/aggregate
family hits a dedicated **"demand sink"** diagnostic; KV hits the **R-MAT
single-derived-relation** diagnostic. Only `@product` / barrier / mutual
recursive-content actually surface the literal R-BODYWALK string. The reject
OUTCOME is correct in every case; the named site must be corrected in the
vocabulary table and in crossings 1, 2, 5, 6, 11 (and the KV-family note in 5).

---

## Per-case adjudication

### Crossing 1 — `demand_agg_body_1` (NEGATIVE, `-demand`)
- Predicted: REJECT all 4 modes → **R-BODYWALK** ("Unsupported rule-body shape").
- Reality: REJECT all 4 modes, rc=1, no crash. Message =
  `Demand does not propagate through a negation or aggregate (the demand sink)…`
- **Verdict: VERIFIED-EMPIRICALLY (outcome)** + **PREDICTION-FAILED (site)** —
  fires at the distinct "demand sink" diagnostic, not the R-BODYWALK string.
- Stage-C admissibility adjudication / I0-Oracle answer: UNVERIFIABLE-TODAY.

### Crossing 2 — `demand_negate_body_1` (NEGATIVE, `-demand`)  — `!` inside body
- Predicted: REJECT all modes → R-BODYWALK.
- Reality: REJECT all 4 modes, rc=1. Message = the "demand sink" diagnostic.
- **Verdict: VERIFIED-EMPIRICALLY (outcome)** + **PREDICTION-FAILED (site)** —
  same "demand sink" mis-attribution as crossing 1.

### Crossing 2 — `demand_negate_consumer_1` (NEGATIVE, `-demand`) — downstream negating consumer
- Predicted: REJECT all modes; stray-consumer belt; message ≈ "the demanded
  relation is read by a consumer demand cannot guard". Alt branch: if it
  COMPILES it is a miscompile finding.
- Reality: REJECT all 4 modes, rc=1. TWO diagnostics:
  1. `Message 'missing/2' can produce deletions but is not marked with the '@differential' attribute` (a prerequisite, because `!path` makes `missing` deletion-capable),
  2. `The demanded relation is read by a consumer demand cannot guard (a sibling query or another rule) under -demand; recompile without -demand` — **exactly the predicted stray-consumer message.**
- **Verdict: VERIFIED-EMPIRICALLY.** Did NOT compile ⇒ no miscompile. Note for
  the corpus author: the sketch also trips the `@differential`-required check
  first; mark `missing` `@differential` if a single-diagnostic golden is wanted
  (the stray-consumer belt still fires either way).

### Crossing 3 — `demand_multi_adorn_diff_1` (POSITIVE, `-demand`)
- Predicted: COMPILE (multi-adornment × diff-input composition).
- Reality: **exit 0 in all 4 modes.**
- **Verdict: VERIFIED-EMPIRICALLY (compiles).** The R-DUP guard-union ∘ a2'
  removal-arm composition does not reject. Oracle answer + published-delta
  identity: UNVERIFIABLE-TODAY (runtime referee).

### Crossing 4 — `demand_beside_recursion_1` (POSITIVE, `-demand`)
- Predicted: COMPILE (demand touches only `pair`; `path` disjoint SCC).
- Reality: **exit 0 in all 4 modes.**
- **Verdict: VERIFIED-EMPIRICALLY (compiles).** Stage-B/C/D region-placement of
  `path`: UNVERIFIABLE-TODAY.

### Crossing 5 — `demand_kv_body_1` (NEGATIVE, `-demand`)
- Predicted: REJECT all modes → **R-BODYWALK** (KV in demanded body); the
  `@invertible` is present so it is NOT R-VALGEBRA.
- Reality: REJECT all 4 modes, rc=1. Message =
  `The demanded query must project from a single derived relation under -demand…`
- **Verdict: VERIFIED-EMPIRICALLY (outcome)** + **PREDICTION-FAILED (site)** —
  this is the **R-MAT** ("≠1 materialization", Step-1 `:446`) family, NOT
  R-BODYWALK. The `@invertible`-avoids-R-VALGEBRA reasoning does hold (no
  V-ALGEBRA reject observed); the reject simply lands one site earlier than
  predicted (the KV column makes `balance` not a single plain derived relation
  the demanded query can project). Site correction owed.

### Crossing 6 — `demand_never_body_1` (NEGATIVE, `-demand`)
- Predicted: REJECT all modes → R-BODYWALK (`@never` NEGATE node).
- Reality: REJECT all 4 modes, rc=1. Message = the "demand sink" diagnostic.
- **Verdict: VERIFIED-EMPIRICALLY (outcome)** + **PREDICTION-FAILED (site).**
  The `!`-twin (crossing 2) and this `@never` case land the SAME "demand sink"
  message — so, empirically, the two do NOT surface as distinguishable demand
  rejects (the draft's "must reject BOTH and not fold them" intent holds at the
  reject level, but the two are indistinguishable by message today).

### Crossing 7 — `demand_product_body_1` (NEGATIVE/OPEN, `-demand`)
- **Sketch fix:** the draft sketch left `#local node(u64 N).` with no defining
  rule (`node` unpopulated). Added `node(N) : add_node(N).`. Recorded here.
- Predicted: EXPECTED REJECT (OPEN) → R-BODYWALK; alt branch = compiles as a
  bound-side product (then Oracle-refereed positive).
- Reality: REJECT all 4 modes, rc=1. Message =
  `Unsupported rule-body shape under -demand` — **exact R-BODYWALK match.**
- **Verdict: VERIFIED-EMPIRICALLY.** OPEN resolves to the REJECT branch at the
  correctly-named site. (Result robust to the fix — an unpopulated `node` would
  also not change the body-shape classification.)

### Crossing 8 — `demand_diff_pub_1` (POSITIVE, `-demand -demand-retract`; eqgate `+ -demand-instance`)
- Predicted: COMPILE, answer shrinks; drives the E1 `DRInstance::differential`
  arm question.
- Reality: **exit 0 all 4 modes** with `-demand -demand-retract`, AND **exit 0
  all modes** with the `-demand-instance` eqgate arm added.
- **Verdict: VERIFIED-EMPIRICALLY (compiles, flat + instance).** The
  "answer SHRINKS on retract" behavior and the E1 `if (diff)`-arm reachability
  question are RUNTIME/introspection claims → UNVERIFIABLE-TODAY (needs the
  `.batches` execution + counter inspection).

### Crossing 9 — `demand_multi_adorn_diff_nested_1` (OPEN, `-demand -demand-retract`; eqgate `+ -demand-instance`)
- Predicted: OPEN — compile OR clean diagnostic at an unnamed composition fence.
- Reality: **exit 0 all 4 modes** flat (`-demand -demand-retract`) AND **exit 0
  all modes** nested (`+ -demand-instance`).
- **Verdict: VERIFIED-EMPIRICALLY — OPEN resolves to the COMPILE branch.** The
  triple composition (diff-input × multi-adornment × nested) does not reject.
  flat==nested==golden answer identity: UNVERIFIABLE-TODAY (runtime eqgate).

### Crossing 10 — `demand_barrier_body_1` (POSITIVE — VERIFY, `-demand`)
- Predicted (primary): **EXPECTED COMPILE** (barrier is answer-transparent;
  question was whether the SIP walk crosses a `:-`-staged body). Pre-registered
  alt: "if the SIP walk cannot classify a barrier-separated body it would
  R-BODYWALK-reject … the barrier is a demand fence and THAT is the finding."
- Reality: **REJECT all 4 modes, rc=1.** Message =
  `Unsupported rule-body shape under -demand` (R-BODYWALK).
- **Verdict: PREDICTION-FAILED (headline).** The primary "EXPECTED COMPILE"
  prediction is false; the draft's own alt branch is what happened: **`:-` /
  `@barrier` IS a demand fence today** — the SIP walk does not traverse a
  barrier-staged join chain. This is the case's registered finding. (The
  `bin/Oracle` positive-answer plan is moot until/unless the fence is lifted.)

### Crossing 11 — `demand_config_agg_body_1` (NEGATIVE, `-demand`)
- Predicted: REJECT all modes → R-BODYWALK (config `@recompute` AGG node).
- Reality: REJECT all 4 modes, rc=1. Message = the "demand sink" diagnostic.
- **Verdict: VERIFIED-EMPIRICALLY (outcome)** + **PREDICTION-FAILED (site)** —
  same "demand sink" mis-attribution as crossing 1; the config/`@recompute`
  arm makes no difference to the reject site.

### Crossing 12 — `demand_mutual_content_1` (NEGATIVE, `-demand`)
- Predicted: REJECT all modes → R-BODYWALK ("Unsupported rule-body shape"), the
  shadowed recursive-content belt.
- Reality: REJECT all 4 modes, rc=1. Message =
  `Unsupported rule-body shape under -demand` — **exact R-BODYWALK match.**
- **Verdict: VERIFIED-EMPIRICALLY.** Stage-D mutual-fixpoint lift + Variant
  A/B fate: UNVERIFIABLE-TODAY.

### Crossing 13 — `demand_beside_mutual_1` (POSITIVE, `-demand`)
- Predicted: COMPILE (mutual SCC published, disjoint from demand).
- Reality: **exit 0 all 4 modes.**
- **Verdict: VERIFIED-EMPIRICALLY (compiles).** Oracle three-sink answer:
  UNVERIFIABLE-TODAY.

### Crossing 14 — `demand_two_queries_1` (NEGATIVE, `-demand`)
- Predicted: REJECT all modes → **R-1BOUND** (>1 bound query name).
- Reality: REJECT all 4 modes, rc=1. Message =
  `Multiple demanded (bound) queries are not yet supported under -demand; recompile without -demand`
  — matches the R-1BOUND site.
- **Verdict: VERIFIED-EMPIRICALLY (outcome + site).**

---

## Cross-cutting verification notes

- **Twinning discipline held at the OUTCOME level:** every beside-vs-inside pair
  (4↔12 compile/reject; 13 beside-mutual compiles) and the agg↔KV↔config-agg
  trio all reject/compile as paired. The one axis that did NOT behave as the
  draft assumed is the `!`↔`@never` pair (crossings 2↔6): both land the
  identical "demand sink" message, so they are indistinguishable as demand
  rejects today (the draft's "must not CSE / must reject both" holds; "are
  structurally distinct rejects" is not observable at the diagnostic).
- **No stderr/lint dependence and no crashes:** all 9 negatives are clean
  exit-1 diagnostics; `expect_diagnostic` passes for all.
- **Every "Stage gating" paragraph** in the draft (Stage B/C/D, I0, Oracle,
  H-J Variant A/B, E1 arm reachability, RoutedResult/RequestEdge shape
  goldens) describes future-architecture behavior and is **UNVERIFIABLE-TODAY**
  by construction — recorded, not refuted.
