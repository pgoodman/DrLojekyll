# D3.a.2 — LANE b1 DESIGN: the fence lifts + input kNetRemovals provisioning + effect/validator regime splits

> **House banner.** Tip **b4d08307** (branch keyed-instances; code bytes ==
> the D3.a.1 landing 33cabcf1 — every commit atop is docs-only, so every anchor
> below is LIVE at the landed binaries; `git rev-parse HEAD` =
> b4d0830797d3eab8530bdbe7f09b1220436b621c). Binding context read END TO END and
> not re-litigated: d3a2-substrate.md §7 (R-A2-TRIGGER: TWO DRAINS, NO RECYCLE;
> gate-set identity; band order a0→a1→a2→a2'-appended; the ungated-late-Recycle
> FORBIDDEN fence; the Present rider), §5 gap ledger (H-1..H-9, H-20 = this
> lane), §4 OB1-OB8, §3 abort chain; d3a-ruling-brief.md (OQ-INPUT / OQ-MODEL /
> OQ-DEATH-VS-REBUILD); d3a1-substrate.md §7 (d2 CO-ACTIVATION — P-STORE/P-DEATH
> NEVER folded) + §8 (e1-e8); KeyedInstances.md §20(AK)-(AM). Mold =
> d3a1-design.md (edit-spec granularity, prediction tables, gate roll-call, the
> d7 L-table idiom).

---

## §0 HEADLINE — what lane b1 owns, and the co-landability fence

Lane b1 lands the **DR-layer admission + provisioning + validator regime
splits** for a `@differential` (deletion-capable) summarized input under
`-demand-instance`. Seven edit clusters, all under §7's R-A2-TRIGGER ruling:

- **E1a** — LIFT FENCE (iii): delete the `diff_input` arm + flag (Build.cpp);
  the cyclic and recursive-content siblings SURVIVE. `demand_diff_input_1`
  flips diagnostic→compiling (the mechanical runall.sh + CLAUDE.md ride; the
  golden/driver disposition is b4's).
- **E1b** — LIFT V-INST-SOLE's differential half; keep the pub-alias half;
  reword + add the acyclic-input defence belt (ADV-1).
- **E1c** — thread a NEW `input_diff` third axis into `InstantiateEffects` (its
  OWN spelling `TableIsDifferential(input_table)`, NEVER folded into `diff` =
  P-STORE); grow the input net-REMOVALS drain leg (the DeathEffects :868 mold).
- **E1d** — V-INST-EFFECT third-axis split: admit the input kNetRemoval drain
  under `input_diff`; totality `input_drains == input_diff?2:1`,
  `drains == input_diff?3:2` (ADV-6).
- **E1e** — V-INST-DRAIN input-arm regime split: `input_diff ? both-sign dr_ok
  : cf_ok` (mirrors the D3.a.1 demand arm; XC-3).
- **E1f** — region `input_removal_frontier` UseRef + accessor + ClassifyVector
  read-set; Procedure.cpp fenced pre-minted fetch (the :314 idiom) for the
  diff-input regime; the input-diff coherence stamp (codegen's selector
  authority); the enrollment ruling (EFFECT-ONLY, no new V-INST-EMITTED row).
- **E1g** — H-20 rider: the two stale `Build.cpp:999` cross-refs → :1110-1114.

**CO-LANDABILITY (the §3 abort chain, binding — b1 is NOT independently
landable).** With E1a+E1b alone a `demand_diff_input_1`-shape program (i) clears
V-INST-SOLE (e2), then (ii) ABORTS at V-INST-DRAIN's input arm (the eager
append was skipped for a diff table, Build.cpp:1110 — the exact XC-3 twin);
with E1e also split it (iii) COMPILES but SILENTLY MISCOMPILES twice
(no a2' removal trigger ⇒ stuck-present; unfiltered rescan ⇒ over-materialize).
Only b3's removal trigger (H-11) + the `Present(s)` rescan conjunct (H-10) close
it, and only b4's eqgate refereees it. **The landable unit is
{E1a,E1b,E1c,E1d,E1e,E1f} ⊕ b2 ⊕ b3 ⊕ b4 as ONE commit** (the D3.a.1
one-commit precedent; the substrate's §3 "landable unit" set). No temporary
fence is designed — none is needed if the four lanes land together.

**The slice serves BOTH regimes** (O-9 / XC-9): diff-input × MONO-demand (the
e5 divergence carrier — P-STORE true, P-DEATH false, NO death op minted) AND
diff-input × diff-demand (the full four-band composition, death present). Every
b1 edit is regime-keyed on the RIGHT axis: the death/removal_frontier machinery
stays P-DEATH (demand) keyed and is ABSENT under mono-demand; the new
input-removal machinery is `input_diff` (input) keyed and PRESENT under both.
Never folded (§7 d2).

---

## §1 FINDINGS (code-anchored at tip b4d08307)

- **F-b1-1 (the `diff_input` flag has exactly one reader — lift is clean).**
  `diff_input` is declared Build.cpp:1516, set at :1530-1532
  (`in.CanReceiveDeletions()`), read ONLY at :1553 (`else if (diff_input)`).
  `grep -n diff_input lib/ControlFlow/Build/Build.cpp` returns those three
  lines and nothing else. Deleting the flag + its set + its else-if arm leaves
  the cyclic (:1546) and recursive_content (:1549) arms untouched, and `in`
  (:1529) survives because :1533-1540 still read it for the recursive-content
  check. The else-if PRIORITY (cyclic > recursive_content > diff_input) means
  a both-cyclic-and-diff-input program keeps rejecting with the cyclic message
  — zero reject-message churn on the surviving fences (substrate F-A load-bear).

- **F-b1-2 (the input ± producers are FREE — ADV-5 confirmed at code).** The
  diff input `pt` is an ordinary acyclic, non-induction-owned differential
  table, so the generic inventory covers it with ZERO new producer code:
  `MintTableVec(flow, pt, kNetRemoval / kNetAddition, …)` at Rel.cpp:1827-1830
  populates `flow.table_vecs[pt]` with both roles; the acyclic mint-loop arm
  `mint_claim(pt,±)` + `mint_filter(pt,±)` at Rel.cpp:2599-2602 mints the
  kFrontierFilter ± producers; the commit-sweep loop at :2606ff adds pt's
  sweep. All at DR-inventory time, BEFORE ValidateDROps (Stratum.cpp:2186).
  This is exactly the flat `-demand` oracle's shape (O-8: `kFrontierFilter=6` =
  a ± pair per DiffTable pt/ans/getpt).

- **F-b1-3 (BOTH CF frontier vecs exist by LowerSubgraphInstances time — the
  XC-3 timing mirrored from the demand arm).** The CF-side
  `VectorKind::kNetAdditions` / `kNetRemovals` vecs for pt are minted inside
  `LowerDRFlow` (Stratum.cpp:2477, called from BuildStratumPhases): the acyclic
  filter band calls `EmitFrontierFilter(…is_del=true…)` (:1637) and
  `EmitFrontierFilter(…is_del=false…)` (:1640), each of which does
  `TableDeltaVector(impl, context, pt, is_del ? kNetRemovals : kNetAdditions)`
  (Stratum.cpp:748-750). BuildStratumPhases runs BEFORE `LowerSubgraphInstances`
  (Procedure.cpp:543, inside the `if (context.dr_flow)` block at :540). So the
  Procedure.cpp:327 input fetch will find BOTH CF vecs PRE-MINTED — exactly as
  the D3.a.1 demand fence at Procedure.cpp:314-321 already relies on for the
  demand kNetAdditions vec. **But at ValidateDROps time (Stratum.cpp:2186 —
  runs BEFORE LowerDRFlow at :2477 in the SAME BuildStratumPhases pass) the CF
  vecs do NOT yet exist**, only the DR-side `table_vecs` + `kFrontierFilter`
  ops do — hence V-INST-DRAIN's input arm MUST use `dr_ok`, never `cf_ok`, for
  a diff input (E1e). This is the D3.a.1 demand-arm regime split verbatim.

- **F-b1-4 (the a2' removal drain is EFFECT-ONLY — no new V-INST-EMITTED
  enrollment).** V-INST-EMITTED enrolls per DROp: Procedure.cpp:453-456 pushes
  `{sid,kSubgraphInstantiate}` + `{sid,kInstanceSeal}`; the death (a separate
  DROp) enrolls `{sid,kInstanceDeath}` at :407-408. The input net-removals
  rebuild drain is a second `kVecDrain` EFFECT of the SAME kSubgraphInstantiate
  op (E1c), not a new op — so it adds NO enrollment row. The V-INST-EMITTED
  multiset for a diff-input × mono-demand store stays `{instantiate, seal}`
  (n_death==0), and the Site-5 cross-check (Procedure.cpp:546-561) balances
  unchanged. Contrast the death op (D3.a.1), which WAS a separate DROp and DID
  earn an enrollment row.

- **F-b1-5 (V-INST-PAIR / V-INST-ORDER / V-INST-DIFF-COHERENCE are INVARIANT —
  ADV-2 / e5).** V-INST-PAIR (Rel.cpp:4405) rejects `n_death > 1u`; `n_death ==
  0` is admitted — exactly the e5 mono-demand store's op set `{instantiate,
  seal}`. V-INST-ORDER (:4826ff) orders death-before-instantiate; with no death
  it is vacuous. V-INST-DIFF-COHERENCE (Procedure.cpp:293-304) stamps
  `inst.differential == TableIsDifferential(pub)`; a diff input forces a diff
  PUB (O-1, the Differential.cpp closure), so `inst.differential==true ==
  TableIsDifferential(pub)==true` — coherent, and the SEPARATE `input_diff`
  axis is not checked here (never folded). No count assumption anywhere keys on
  the input drain count except V-INST-EFFECT (E1d).

- **F-b1-6 (no new DROp kind is needed — the substrate's effect-suffices claim
  holds at code).** The a2' removal trigger is a second `kVecDrain` effect
  (E1c) on the existing kSubgraphInstantiate op; its CF vec is fetched by an
  existing `input_removal_frontier` UseRef; the runtime band (b3) drains it
  through the SAME shared rescan mold. The `VecRole` enum (Rel.h) already
  carries `kNetRemoval`; `VectorKind` already carries `kNetRemovals`. **No new
  DROpKind, no new VecRole, no new EffKind.** (I challenged this at code per the
  house rule: a combined ± drain WOULD need a new role — §7 rejected it; the
  two-drain shape mints zero new enum surface.)

- **F-b1-7 (no new dump spelling — no E-71 grammar note owed by b1).** The new
  input net-removals drain renders on the instantiate's `effects:` line as
  `kVecDrain(<input_tid>, kNetRemoval)` via the existing `emit_effect` arm
  (Format.cpp:492-501) using the already-produced `kVecDrain` (Format.cpp:182)
  + `kNetRemoval` (Format.cpp:69) tokens — the kInstanceDeath block already
  renders a `kNetRemoval` drain since D3.a.1. The instantiate `effects:` line
  gains ONE drain token (2→3) for a diff-input instantiate: a [STRUCT] change,
  no new terminal, **no E-71 note.** (C15's optional `input`-adjacent
  input-diff MARKER is a separate stage-(b) render choice, OUT OF SCOPE for
  b1; b1 introduces no marker, so no E-71 note there either.)

---

## §2 EDIT SPECS

Every spec: file, function, insertion anchor at tip, verbatim new text where
feasible, and a per-surface [BYTE]/[STRUCT] verdict with the gate family named.

### E1a — LIFT FENCE (iii) (e1 / H-1; the `diff_input` arm + flag)

**File** `lib/ControlFlow/Build/Build.cpp`, in the `if (demand_instance)`
feature-gap block (`Program::Build` head), the per-forcing-group loop
:1515-1557. Three surgical deletions (the flag has one reader, F-b1-1):

1. **Delete the `diff_input` local** — at :1516 change
   ```cpp
         bool diff_input = false, recursive_content = false, cyclic_demand = false;
   ```
   to
   ```cpp
         bool recursive_content = false, cyclic_demand = false;
   ```
2. **Delete the flag set** — remove :1530-1532 verbatim:
   ```cpp
           if (in.CanReceiveDeletions()) {
             diff_input = true;
           }
   ```
   `const QueryView in = jl[1];` (:1529) STAYS — :1533-1540 read `in` for the
   recursive-content check.
3. **Delete the diagnostic arm** — remove :1553-1555 verbatim:
   ```cpp
         } else if (diff_input) {
           log.Append() << "Demanded subgraphs over deletable (differential) "
                           "inputs are not yet supported under -demand-instance";
   ```
   so the else-if chain closes at the `recursive_content` arm; the trailing `}`
   of the `if (cyclic_demand) … else if (recursive_content) { … }` remains.

**What replaces the reject: nothing** (admission). No new code at the fence.
The admitted shape is a differential, ACYCLIC, non-induction-fed input — the
surviving `recursive_content` fence (:1533-1540, incl. the predecessor
induction walk) guarantees acyclicity, which is load-bearing for OB8 (counters
final at band time). The `cyclic_demand` (recursive DEMAND) fence also survives.

**demand_diff_input_1 mechanical flip (coordinated with b4 — disposition is
b4's; b1 owns ONLY the mechanical list edit).** After E1a the case COMPILES
under `-demand -demand-instance`. IF b4 promotes it to a compiling golden case:

- **runall.sh** — remove the `demand_diff_input_1` alternative from the
  all-4-modes-diagnostic regex, line 361:
  ```
  …|truncated_decl_1|demand_multi_adorn_1|demand_cyclic_1|demand_recursive_content_1|demand_diff_input_1)
  ```
  →
  ```
  …|truncated_decl_1|demand_multi_adorn_1|demand_cyclic_1|demand_recursive_content_1)
  ```
- **runall.sh header comment** — the case-expectations block (:20-24) names
  `demand_diff_input_1` among "the D2.c nested-lowering fences … cyclic +
  diff_input compile under plain -demand and reject only under
  -demand-instance". Re-spell to drop diff_input from the -demand-instance-only
  fence set (b4 owns the exact prose since it depends on the promote-vs-witness
  disposition).

b4 owes: the `goldens/demand_diff_input_1.stdout` (NONE exists today) + a real
driver or the empty-driver golden, and the stale `Build.cpp:1344` comment in
the `.dr` header (ADV-8, H-17). **b1's spec ends at the runall.sh regex + the
mechanical comment-count.** (If b4 instead keeps it a diagnostic under a
different flag, E1a's runall.sh edit is void and only the .dr disposition moves
— b1 flags the dependency, does not decide it.)

**CLAUDE.md ride (H-1).** The "two `-demand-instance` … fences" prose at
CLAUDE.md:97-99 must drop the diff-input member (only `demand_cyclic_1` remains
a compile-under-plain-`-demand`, reject-under-`-demand-instance` fence). Exact
edit — CLAUDE.md:97-100:
```
  via its `.drflags` sidecar), `demand_cyclic_1`/`demand_diff_input_1` (two
  `-demand-instance` nested-lowering feature-gap fences — recursive demand and
  a @differential summarized input; both COMPILE under plain `-demand` and
  reject only under `-demand-instance`) and `demand_recursive_content_1` (a
```
→
```
  via its `.drflags` sidecar), `demand_cyclic_1` (a
  `-demand-instance` nested-lowering feature-gap fence — recursive demand;
  COMPILES under plain `-demand` and rejects only under `-demand-instance`; the
  @differential-input fence LIFTED at D3.a.2, `demand_diff_input_1` now
  compiles) and `demand_recursive_content_1` (a
```
And the keyed-instance section prose at CLAUDE.md:483-486:
```
compile fences: recursive demand (`demand_cyclic_1`) and a @differential
summarized input (`demand_diff_input_1`) reject at the Program::Build nested
pre-pass (Build.cpp:1336-1346) only under `-demand-instance` (both compile
under plain `-demand`); a recursive-content demanded body
```
→
```
compile fences: recursive demand (`demand_cyclic_1`) rejects at the
Program::Build nested pre-pass (Build.cpp:1504-1556) only under
`-demand-instance` (compiles under plain `-demand`); the @differential
summarized-input fence was LIFTED at D3.a.2 (`demand_diff_input_1` compiles);
a recursive-content demanded body
```
(the ":1336-1346" anchor is itself stale — D3.a.1 drift; the lift corrects it
to :1504-1556.) **[STRUCT]** on both files; no gate (docs). Coordinate the
"differential input" bullet under `## The keyed-instance nested lowering` with
b4 (it may add the new witness's name).

**Verdict.** Build.cpp: **[STRUCT]** (an accepted program's ControlFlow is
unchanged — the fence block only ever appended diagnostics; the sole behavior
change is one program family stops being rejected). Gate: the diagnostic
verdict-line set (SUITE `demand_diff_input_1` line flips diagnostic→green under
b4's golden) + the surviving `demand_cyclic_1` / `demand_recursive_content_1`
lines stay **[BYTE]** diagnostic. No id/op mint on any OTHER program (the block
runs only under `-demand-instance` and only appends to `log`).

### E1b — LIFT V-INST-SOLE's differential half + reword + acyclic belt (e2 / H-2 / ADV-1)

**File** `lib/Rel/Rel.cpp`, `ValidateDROps`, the V-INST-EFFECT
`kSubgraphInstantiate` case, :4336-4341. Replace the one two-forbiddance
conjunction:
```cpp
          if (op.input_table != nullptr &&
              (TableIsDifferential(op.input_table) ||
               op.input_table == op.table_op_table)) {
            ValidatorFail("V-INST-SOLE: an instantiate's summarized input is "
                          "differential or aliases the published table");
          }
```
with the surviving pub-alias reject (half 2, reworded truthful) PLUS the
narrowed differential-acyclic defence belt (ADV-1 "split into two"):
```cpp
          // [D3.a.2 e2] V-INST-SOLE half 2 SURVIVES: a summarized input that
          // ALIASES the published table is still forbidden (a self-summarizing
          // instantiate is nonsense). Reworded — the differential forbiddance
          // (half 1) LIFTED: a @differential input is now ADMITTED (fence (iii)
          // lift), its deletion machinery living in the V-INST-DRAIN input-arm
          // regime split (dr_ok, :4545ff), the InstantiateEffects removal leg,
          // the input_removal_frontier fence, and the band's Present rescan.
          if (op.input_table != nullptr &&
              op.input_table == op.table_op_table) {
            ValidatorFail("V-INST-SOLE: an instantiate's summarized input "
                          "aliases its published table");
          }
          // [D3.a.2 e2/ADV-1] the differential half's TEETH re-pointed, not
          // dropped: an admitted @differential input MUST be acyclic /
          // non-induction-owned. The surviving F-A recursive-content fence
          // (Build.cpp:1533-1540) guarantees it upstream; this DR-layer belt
          // catches an F-A regression before the band trusts "counters final at
          // band time" (OB8 — a fixpoint-refired input breaks the Present
          // rescan). Cheap; always-on.
          if (op.input_table != nullptr &&
              TableIsDifferential(op.input_table) &&
              TableIsInductionOwnedDR(context, op.input_table)) {
            ValidatorFail("V-INST-SOLE: a differential summarized input is "
                          "induction-owned (recursive content must stay "
                          "F-A-fenced)");
          }
```
`context` is in scope (ValidateDROps param, Rel.cpp:3336); `TableIsDifferential`
(static) and `TableIsInductionOwnedDR(context, …)` (Rel.cpp:79) are both
callable here (the mint-loop at :2542 uses the same pair). The two new strings
are the "split into two" (ADV-1): (1) the pub-alias reject, truthful for its
sole condition; (2) the differential-acyclic belt, replacing the lifted
differential forbiddance with a NARROWER always-on check.

**Verdict.** **[BYTE]** on every landed program (V-INST-SOLE never fired on any
accepted case; the pub-alias string is unreachable on the corpus — no
self-summarizing instantiate exists, and no monotone-input case trips the
deleted differential half). Gate: V-INST-SOLE / V-INST-EFFECT family (the
D3.a.1 pinned `.rel` census `kSubgraphInstantiate` blocks regenerate
byte-identical; validated by the 20/20 pinned-regen gate + the eqgate). The
NEW witness (b4) is the first program to exercise the lifted admission.

### E1c — the `input_diff` third axis into InstantiateEffects + the removal leg (e3 / H-6 / ADV-6, C3)

**Three coordinated edits, all `lib/Rel/Rel.cpp`. `input_diff` is its OWN
spelling `TableIsDifferential(input_table)`; it is NEVER folded into `diff` (=
P-STORE = TableIsDifferential(pub)) nor into P-DEATH — the §7 d2 anti-fold
discipline extended to the input axis.**

**(1) Signature** — `InstantiateEffects` head, :781-782:
```cpp
static std::vector<DREffect> InstantiateEffects(bool diff, TABLE *pub,
                                                TABLE *demand, TABLE *input) {
```
→
```cpp
static std::vector<DREffect> InstantiateEffects(bool diff, bool input_diff,
                                                TABLE *pub, TABLE *demand,
                                                TABLE *input) {
```

**(2) The removal drain leg** — INSIDE `InstantiateEffects`, immediately after
the existing `edge_drain` push (:799, `fx.push_back(edge_drain);`) insert:
```cpp
  // [D3.a.2 e3 / R-A2-TRIGGER] the input(edge) net-REMOVALS rebuild drain —
  // the band-(a2') arm. Present ONLY under a DIFFERENTIAL input (input_diff,
  // the THIRD predicate axis; NEVER folded into `diff`=P-STORE per §7 d2). The
  // DeathEffects kNetRemoval drain (:868) is the effect-declaration mold; the
  // producer is FREE (generic both-sign mint, :2599-2602 — F-b1-2/ADV-5).
  // Pushed THIRD (after the two net-additions drains) for push-order
  // determinism; no reader keys on drain order (ADJ-R9).
  if (input_diff) {
    DREffect edge_del_drain;
    edge_del_drain.kind = EffKind::kVecDrain;
    edge_del_drain.value_table = input;
    edge_del_drain.vec_role = VecRole::kNetRemoval;
    fx.push_back(edge_del_drain);
  }
```
The rederive leaf (`kFlagRead`/`kPresent`/`kSeed` on input, :806-811) STAYS
UNCHANGED (C4 — Present is meaningful on a DiffTable; codegen's honoring of it
is b3's rescan-Present conjunct). The `if (diff)` pub-side block (:829-857) is
UNTOUCHED (input differentiality never touches pub counters).

**(3) The call site + the axis** — at :1055 there is already `const bool diff =
TableIsDifferential(pub_table);`; directly after it (new line at :1056) add:
```cpp
    // [D3.a.2 e3] the input differentiality axis — its OWN spelling, threaded
    // to InstantiateEffects for the removal leg. NEVER folded into `diff`
    // (P-STORE) or P-DEATH (§7 d2). input_table non-null on the recognized
    // single-monotone-hop shape (the HP-4 refusal belt at :1049 pins it).
    const bool input_diff = input_table && TableIsDifferential(input_table);
```
and change the call at :1097-1098 from
```cpp
    inst.effects =
        InstantiateEffects(diff, pub_table, demand_table, input_table);
```
to
```cpp
    inst.effects = InstantiateEffects(diff, input_diff, pub_table,
                                      demand_table, input_table);
```
(`input_table` is the local `TABLE *` at :1043.)

**Verdict.** **[BYTE]** on every monotone-input program (`input_diff` false ⇒
`fx` byte-identical; the signature change is source-only). **[STRUCT]** on the
diff-input witness: the instantiate `effects:` line gains one
`kVecDrain(<input_tid>, kNetRemoval)` (F-b1-7, no new token). Gate:
V-INST-EFFECT (E1d hand-counts the same totality — a mint/validator drift
aborts) + the 20/20 pinned-regen [BYTE].

### E1d — V-INST-EFFECT third-axis split (H-3 / ADV-6, C5)

**File** `lib/Rel/Rel.cpp`, `ValidateDROps`, the `kSubgraphInstantiate` case.
Three edits mirror the mint (E1c) — the V-AGG-EFFECT hand-count that aborts on
mint/validator drift.

**(1) Compute the axis** — after `const bool diff =
TableIsDifferential(op.table_op_table);` (:4271) add:
```cpp
          const bool input_diff =
              op.input_table && TableIsDifferential(op.input_table);
```

**(2) Admit the input net-removals drain** — replace the drain case body
:4279-4300 (the `case EffKind::kVecDrain:` through its `break;`). New body
(braced — it needs a local):
```cpp
              case EffKind::kVecDrain: {
                ++drains;
                // [D3.a.2 e3] the input net-REMOVALS rebuild drain is admitted
                // ONLY under a differential input (input_diff). Everything else
                // must be a net-additions drain of the demand or input frontier.
                const bool input_del =
                    (fx.vec_role == VecRole::kNetRemoval &&
                     fx.value_table == op.input_table && input_diff);
                // Source-aware: a count alone cannot tell "demand + edge" from
                // "demand twice" (the R-a2 Fable hazard — the dump renders these
                // value_tables verbatim).
                if (!input_del &&
                    (fx.vec_role != VecRole::kNetAddition ||
                     (fx.value_table != op.demand_table &&
                      fx.value_table != op.input_table))) {
                  ValidatorFail("V-INST-EFFECT: an instantiate kVecDrain is "
                                "not a net-additions drain of the demand or "
                                "input frontier, nor a differential input's "
                                "net-removals rebuild drain");
                }
                if (fx.value_table == op.demand_table) {
                  ++demand_drains;
                } else {
                  ++input_drains;
                }
                break;
              }
```
(The input net-removals drain increments `input_drains` — its `value_table ==
op.input_table`. So under `input_diff` a correct instantiate has
`input_drains == 2u`.)

**(3) Totality** — replace the `drains == 2u && … && input_drains == 1u &&`
prefix of the `ok` expression (:4324-4325):
```cpp
          const bool ok =
              drains == 2u && demand_drains == 1u && input_drains == 1u &&
```
→
```cpp
          const bool ok =
              drains == (input_diff ? 3u : 2u) && demand_drains == 1u &&
              input_drains == (input_diff ? 2u : 1u) &&
```
(the remaining conjuncts — `demands==1u && leaves==1u && rebuilds==1u &&
rebuild_sign==1 && emits==1u && olds==1u && (diff ? … : …)` — UNCHANGED: the
pub-side `diff` split is orthogonal, and the failure string at :4333 stays).

**Anti-fold note (ADV-6 / L3-style which-predicate).** The regime axis here is
`TableIsDifferential(op.input_table)` — a THIRD axis, distinct from `diff` (the
pub axis, :4271) and from P-DEATH (the demand axis, never read here); NOT folded
into a shared bit. On the e5 carrier `diff` and `input_diff` are co-true (a diff
input forces a diff pub, O-1) but stay SEPARATELY spelled (the §7
theorem-not-invariant posture; D3.a.3+ could break co-truth, both sites already
handle it).

**Verdict.** **[BYTE]** on every monotone-input program (`input_diff` false ⇒
the drain case and totality reduce to today's exact arithmetic). **[STRUCT]** on
the diff-input witness (drains 2→3, input_drains 1→2 accepted). Gate:
V-INST-EFFECT self (always-on fprintf+abort surviving NDEBUG). d7 rows L-E1d-1
(accept), L-E1d-2 (mint an input net-removals drain but DON'T bump the totality
→ abort).

### E1e — V-INST-DRAIN input-arm regime split (H-4 / C6)

**File** `lib/Rel/Rel.cpp`, `ValidateDROps`, the V-INST-DRAIN block. Replace
the input arm :4542-4548:
```cpp
    // [R-REBUILD-a2] input(edge) arm UNCHANGED: monotone this slice (a
    // differential input is V-INST-SOLE-rejected upstream, :4335-4340;
    // D3.a.2 owns the split here).
    if (!cf_ok(op.input_table, VectorKind::kNetAdditions)) {
      ValidatorFail("V-INST-DRAIN: an instantiate's input(edge) net-additions "
                    "frontier was never provisioned (OD-4/R-a2 gap)");
    }
```
with the regime split (mirrors the demand arm :4534-4541; F-b1-3):
```cpp
    // [D3.a.2 C6] input(edge) arm REGIME-SPLIT (mirrors the demand arm above):
    //  - MONOTONE input: the eager boundary append (Build.cpp:1110-1114)
    //    provisioned the ControlFlow kNetAdditions VECTOR during the walk — it
    //    must exist NOW (cf_ok).
    //  - DIFFERENTIAL input: the ± frontiers are commit-band products whose
    //    ControlFlow VECTORs first mint inside LowerDRFlow, AFTER this validator
    //    (XC-3, F-b1-3) — check the DR-side vec + the signed kFrontierFilter
    //    producer for BOTH signs, minted by the DR inventory before validation.
    //    The eager append is SKIPPED for a diff table (Build.cpp:1110), so
    //    re-enabling cf_ok here would double-provision (laneB B14) — FORBIDDEN.
    const bool input_ok =
        (op.input_table && TableIsDifferential(op.input_table))
            ? (dr_ok(op.input_table, VecRole::kNetAddition, +1) &&
               dr_ok(op.input_table, VecRole::kNetRemoval, -1))
            : cf_ok(op.input_table, VectorKind::kNetAdditions);
    if (!input_ok) {
      ValidatorFail("V-INST-DRAIN: an instantiate's input(edge) frontier(s) "
                    "were never provisioned (OD-4/R-a2 gap; a differential "
                    "input needs BOTH signs' DR vec + frontier-filter "
                    "producer)");
    }
```
`dr_ok` (:4517-4528) already checks `flow.table_vecs[t].count(role)` AND a
matching signed `kFrontierFilter` op — both present for pt by F-b1-2. `cf_ok`
(:4514-4516) = `HasTableDeltaVector`. The monotone abort string is preserved in
spirit; the differential arm gets a distinct both-signs string.

**Verdict.** **[BYTE]** on every monotone-input program (the ternary's false
arm IS the old `cf_ok` + a string that fires on the same condition). **[STRUCT]**
on the diff-input witness (the differential arm passes by construction). Gate:
V-INST-DRAIN family (the D3.a.1 `rel_validators_test` binary + the always-on run
on every Program::Build). d7 rows L-E1e-1 (accept), L-E1e-2 (a diff input with a
suppressed `-` kFrontierFilter producer → abort).

### E1f — region `input_removal_frontier` + fenced fetch + coherence stamp + ClassifyVector (H-7 / H-8 / C9 / C10)

> Substrate §5 tabled H-7/H-8 to lane b2; the orchestrator's b1 charter (item
> 5) folds them here so the provisioning story lands in ONE lane. b2's death
> wiring is adjacent — the merge note (§4) flags the shared function.

**(1) The region member** — `lib/ControlFlow/Program.h`, class
`ProgramSubgraphInstanceRegionImpl`, after `input_frontier` (:1175) add a peer:
```cpp
  UseRef<VECTOR> input_removal_frontier;  // BAND (a2') drain source (the input
                                          // net-REMOVALS rebuild keys). NULL
                                          // unless the summarized input is
                                          // @differential (D3.a.2, input_diff).
                                          // Presence == codegen's a2'/Present
                                          // selector authority (keyed on the
                                          // MEMBER, not a folded bit — d2).
```
Hash/Equals UNTOUCHED: `input_removal_frontier` is a pure function of the Equals
key (V-INST-SOLE pins one input table per store; the frontier is that table's
kNetRemovals vec) — the same argument the D3.a.1 `removal_frontier`/queues/
`demand_table` members use (recorded in the commit message).

**(2) The public accessor** — `include/drlojekyll/ControlFlow/Program.h`, after
`InputFrontier()` (:861), mirroring `RemovalFrontier`'s optional shape (present
only under a differential input):
```cpp
  // BAND (a2') input(edge) net-REMOVALS rebuild frontier (D3.a.2). Present ONLY
  // when the summarized input is @differential; absent under a monotone input.
  // Its PRESENCE is codegen's authority for emitting the a2' removal drain +
  // the shared rescan's Present(s) conjunct (b3) — keyed on the member, never a
  // folded input-diff bit (§7 d2).
  std::optional<DataVector> InputRemovalFrontier(void) const noexcept;
```
and the impl in `lib/ControlFlow/Program.cpp`, after `InputFrontier` (:759-761),
mirroring `RemovalFrontier` (:762-768):
```cpp
std::optional<DataVector>
ProgramSubgraphInstanceRegion::InputRemovalFrontier(void) const noexcept {
  if (auto vec = impl->input_removal_frontier.get()) {
    return DataVector(vec);
  }
  return std::nullopt;
}
```

**(3) The fenced pre-minted fetch** — `lib/ControlFlow/Build/Procedure.cpp`,
`LowerSubgraphInstances`, replace the UNFENCED mint-on-miss input fetch
:325-329:
```cpp
    // [R-REBUILD-a2] the SAME memoized edge frontier the eager cut-successor
    // append (Build.cpp:999-1004) writes into — a2 drains it, provisions none.
    VECTOR *const input_front =
        TableDeltaVector(impl, context, op->input_table,
                         VectorKind::kNetAdditions);
```
with the regime-split fenced fetch (the :314-321 demand-fence idiom; H-20's
comment respell folded in — see E1g):
```cpp
    // [R-REBUILD-a2] the memoized input net-additions frontier.
    //  - MONOTONE input: the eager cut-successor boundary append
    //    (Build.cpp:1110-1114) minted it during the walk — the mint-on-miss
    //    fetch resolves it and provisions nothing new.
    //  - DIFFERENTIAL input (input_diff): the eager append was SKIPPED
    //    (Build.cpp:1110 `!TableIsDifferential`), so both frontiers are
    //    commit-band products minted by LowerDRFlow's frontier-filter lowering
    //    (Stratum.cpp:1637-1642) BEFORE this pass (F-b1-3). A mint-on-miss here
    //    would hand band-(a2) a producer-less always-empty frontier — the
    //    silent-orphan hazard the fence catches (A2.6 idiom).
    const bool input_diff =
        op->input_table && TableIsDifferential(op->input_table);
    if (input_diff &&
        (!HasTableDeltaVector(context, op->input_table,
                              VectorKind::kNetAdditions) ||
         !HasTableDeltaVector(context, op->input_table,
                              VectorKind::kNetRemovals))) {
      std::fprintf(stderr,
                   "error: orphan-mint fence: differential input +/- frontier "
                   "not pre-minted (store %u)\n", sid);
      std::abort();
    }
    VECTOR *const input_front =
        TableDeltaVector(impl, context, op->input_table,
                         VectorKind::kNetAdditions);
    VECTOR *const input_removal_front =
        input_diff ? TableDeltaVector(impl, context, op->input_table,
                                      VectorKind::kNetRemovals)
                   : nullptr;
```
and Emplace it beside `input_frontier` (after :336
`si->input_frontier.Emplace(si, input_front);`):
```cpp
    if (input_removal_front) {
      si->input_removal_frontier.Emplace(si, input_removal_front);  // a2'
    }
```
**Monotone byte-identity (REQUIRED).** For a monotone input, `input_diff` is
false: the fence is skipped, `input_front` is the unchanged mint-on-miss fetch,
`input_removal_front` is null, and no Emplace runs — the monotone lowering is
BYTE-IDENTICAL (the a2' member stays absent, mirroring how `removal_frontier`
stays absent under R-MONO). This is the d5-selector discipline: the monotone
path must not move a byte.

**(4) The input-diff coherence stamp (C10 — codegen's checked selector
authority).** After the Emplaces, add a peer of V-INST-DIFF-COHERENCE
(:293-304) asserting the member's presence equals the input's differentiality —
so a drifted mint (member absent but input differential, or vice-versa) aborts
LOUD rather than silently disabling the a2' arm:
```cpp
    // V-INST-INPUT-COHERENCE [ALWAYS-ON]: input_removal_frontier is present iff
    // the summarized input is @differential — the invariant codegen's a2'/
    // Present selector relies on. A drift (F-A regression admitting a diff
    // input the DR layer didn't provision, or a stale mint) would silently
    // drop the removal rebuild. fprintf+abort, survives NDEBUG (the
    // V-INST-DIFF-COHERENCE mold).
    if ((si->input_removal_frontier.get() != nullptr) != input_diff) {
      std::fprintf(stderr,
                   "error: SUBGRAPHINSTANCE store %u: input_removal_frontier "
                   "presence (%d) != TableIsDifferential(input)=%d\n",
                   sid, si->input_removal_frontier.get() != nullptr,
                   input_diff);
      std::abort();
    }
```

**(5) ClassifyVector read-set** — `lib/ControlFlow/Build/Procedure.cpp`,
`ClassifyVector`, the `kSubgraphInstance` arm, after the `input_frontier` read
insert (:201-203) add (the A2.1 hazard: an unclassified member survives only by
the frontier-filter written-by-primary coupling — a producer relocation would
silently repoint the drain to a fresh-empty local):
```cpp
        if (vec == si->input_removal_frontier.get()) {
          read.insert(vec);  // [D3.a.2 a2'] input net-removals drain (read-only)
        }
```
(read-only, exactly like `input_frontier`/`removal_frontier` — the band DRAINS
it, writes nothing.)

**(6) ENROLLMENT RULING (item-5 question, proven at code — F-b1-4).** The a2'
drain needs NO new `{sid,…}` enrollment row for V-INST-EMITTED. V-INST-EMITTED
enrolls per DROp (Procedure.cpp:453-456 push `{sid,kSubgraphInstantiate}` +
`{sid,kInstanceSeal}`; :407-408 push `{sid,kInstanceDeath}` only when the death
op — a SEPARATE DROp — exists). The a2' removal drain is a second `kVecDrain`
EFFECT of the SAME kSubgraphInstantiate op (E1c), not a new op, so it is
**effect-only**: the enrolled multiset for a diff-input × mono-demand store
stays exactly `{instantiate, seal}` and the Site-5 cross-check
(Procedure.cpp:546-561) balances unchanged. (Contrast the D3.a.1 death, which
was a new DROp and DID earn a row.)

**Verdict.** Program.h / include Program.h / Program.cpp: **[STRUCT]** (new
member + accessor; no accepted program's emitted regions change unless the
input is differential). Procedure.cpp lowering: **[BYTE]** on every
monotone-input program (the whole diff branch is `input_diff`-gated; the
coherence stamp is `!input_diff ⇒ (nullptr!=nullptr)==false==input_diff`, no
abort, no code). **[STRUCT]** on the diff-input witness (one new member
Emplaced; the a2' member surfaces in `.ir`). Gate: the 20/20 pinned-regen
[BYTE] (no pinned case has a differential input), V-INST-INPUT-COHERENCE
always-on, the eqgate on the witness. d7 rows L-E1f-1 (accept),
L-E1f-2 (perturb the coherence stamp: force the member null under input_diff →
abort), L-E1f-3 (drop the orphan-mint fence + suppress the CF mint → the
band-(a2) silently under-rebuilds, caught by the eqgate not a validator —
records WHY the fence is load-bearing).

### E1g — H-20 rider: stale `Build.cpp:999` cross-refs respelled (cosmetic)

Two in-source comments cite the pre-drift `Build.cpp:999` (the eager append is
live at :1110-1114, XC-6). Line-count-preserving comment edits (both fit ≤80
cols on one line):

1. `lib/Rel/Rel.cpp:4507`:
   ```cpp
   //  - MONOTONE demand/input: the eager boundary append (Build.cpp:999)
   ```
   →
   ```cpp
   //  - MONOTONE demand/input: the eager boundary append (Build.cpp:1110-1114)
   ```
2. `lib/ControlFlow/Build/Procedure.cpp:326` — SUPERSEDED by E1f(3) (that edit
   already respells the whole comment block to name Build.cpp:1110-1114). If E1f
   lands, this standalone edit is void; listed for completeness in case the
   lowering fetch edit is deferred.

**Verdict.** **[BYTE]** everywhere (comment-only). No gate (docs-in-source).

### H-5 / ADV-2 statement (no code — the death asymmetry, stated once)

The demand-removal DEATH pieces have NO input twin. Input removals NEVER mint a
`kInstanceDeath`, never drain a `removal_frontier`, never enroll
`{sid,kInstanceDeath}`, and `CheckInstanceDeathFrontier` (Rel.cpp:4789-4813) is
UNTOUCHED (it iterates `kInstanceDeath` ops only, over `op.demand_table`). An
input net-removal is a REBUILD trigger (band-(a2'): RecycleCurrent is NEVER
called — §7 R-A2-TRIGGER(3); the shared rescan re-derives the key's NET
neighborhood, and band-(b)'s (T,F) drop scan publishes the retractions), NOT a
death (OQ-DEATH-VS-REBUILD: death = the DEMAND key leaving; a shrunken input
for a still-demanded key is a smaller-but-live instance). So on the e5 carrier
(diff input, mono demand) ZERO death machinery is minted or lowered
(V-INST-PAIR n_death==0, F-b1-5) — the P-STORE∧¬P-DEATH divergence goes live
with the input-removal REBUILD path as the ONLY deletion surface.

---

## §3 PRE-REGISTERED PREDICTIONS (per surface, gate family named)

### 3.1 Monotone-path [BYTE] surfaces (b1 must not move a byte)

| surface | count | prediction | why |
|---|---|---|---|
| non-witness, non-flip case stdouts × 4 modes | (177 − new witness − demand_diff_input_1) × 4 | **[BYTE]** | no other case carries `-demand-instance` over a diff input; every b1 code path is `input_diff`-gated and `input_diff` is false on all of them; the signature/totality changes reduce to today's arithmetic |
| 20 pinned dump surfaces (demand_tc_witness ×4, 11 `.rel` pins, `.irgold`/`.df`/`.h` regen set) | 20 | **[BYTE]** | none has a differential input; `kSubgraphInstantiate` census + `effects:` lines regenerate identical; V-INST-SOLE/EFFECT/DRAIN monotone legs keep their exact abort strings |
| `demand_neighborhood_mono_witness` nested arm (bare `-demand`, R-MONO) | 1 (×4 modes + eqgate) | **[BYTE]** | monotone input × monotone demand: `input_diff` false, `diff` false — the a2' member absent, no removal leg, no Present conjunct; the pre-slice bytes stand |
| `demand_neighborhood_witness` nested arm (`-demand -demand-retract`, diff DEMAND, MONO input) | 1 (×4 + eqgate) | **[BYTE]** | VERIFIED at code: its input is `#message add_edge(u64 From, u64 To).` — NO `@differential`, so MONOTONE (`demand_neighborhood_witness.dr:30`); only the demand (`-demand-retract`) and the `nbhd_out` OUTPUT tap (:44) are differential. `input_diff` false ⇒ b1 byte-neutral. The D3.a.2 witness (b4) is a SEPARATE, NEW diff-input case — it does not perturb this one |
| data/ corpus × 4 modes | 36 × 4 | **[BYTE]** | no `-demand-instance`; the fence block never runs |
| existing diagnostic verdict lines (14 + kvindex_1 split) minus demand_diff_input_1 | — | **[BYTE]** | surviving fences (`demand_cyclic_1`, `demand_recursive_content_1`, `demand_multi_adorn_1`, `negate_never_diff_1`, …) keep their exact diagnostics |

### 3.2 New-witness [STRUCT] surfaces (shape stated; absolutes are b4's, at the (d0) baseline)

| surface | prediction shape |
|---|---|
| `demand_diff_input_1` verdict line | **[STRUCT]** diagnostic→compiling (b4's golden/driver disposition; b1 owns the runall.sh regex + CLAUDE.md prose flip only) |
| the b4 differential-INPUT eqgate witness, `.rel` instantiate block | **[STRUCT]**: `effects:` line gains one `kVecDrain(<input_tid>, kNetRemoval)` (drains 2→3); NO new token (F-b1-7); census `kSubgraphInstantiate=1`, `kInstanceDeath=`(0 for the e5 mono-demand carrier / 1 for the diff-demand carrier) |
| the b4 witness `.ir` | **[STRUCT]**: the SUBGRAPHINSTANCE region gains an `input_removal_frontier` drain member (a2'); render token is b3's/dump-lane's — b1 introduces NO new spelling |
| the b4 witness generated header (nested arm) | **[STRUCT]**: the store's input tables flip `Table<>`→`DiffTable<>` (O-1 closure); the a2' removal drain + b3's Present rescan surface — particulars are b3/b4's |
| goldens churn | b4-owned; b1 predicts **ZERO** golden churn from b1's edits alone (every b1 surface is [BYTE] modulo the fence-flip that b4 dispositions) |

**Line-level absolutes are re-anchored at the stage-(d) (d0) baseline** (the
D3.a.1 protocol): compile the b4 witness `-demand -demand-instance`
(+`-rel-out`/`-ir-out`) with b1's edits applied ALONE-ON-TIP is NOT possible
(co-landability, §0) — so the (d0) baseline is the WHOLE-SLICE first green
compile; b1's [STRUCT] deltas are the binding arithmetic (drains 2→3,
input_drains 1→2, one new region member), absolutes fall out at (d0).

---

## §4 GATE ROLL-CALL + CROSS-LANE COORDINATION

**Validator/belt inventory touched by b1 (all always-on fprintf+abort, survive
NDEBUG — house rule honored; none argued NDEBUG-only):**

| validator | b1 change | invariant-preserved? |
|---|---|---|
| V-INST-SOLE (Rel.cpp:4336) | half-1 lifted; half-2 reworded; +acyclic belt | teeth re-pointed, not dropped (E1b) |
| V-INST-EFFECT (Rel.cpp:4272) | third-axis drain admit + totality split | hand-count still aborts on mint drift (E1d) |
| V-INST-DRAIN input arm (Rel.cpp:4542) | regime split dr_ok/cf_ok | mirrors the demand arm (E1e) |
| V-INST-INPUT-COHERENCE (Procedure.cpp, NEW) | member-presence == input_diff | new always-on belt (E1f-4) |
| orphan-mint fence (Procedure.cpp:325, NEW arm) | diff-input ± pre-mint check | mirrors the demand fence :314 (E1f-3) |
| V-INST-PAIR / V-INST-ORDER / V-INST-DIFF-COHERENCE / V-INST-EMITTED / CheckInstanceDeathFrontier | **UNTOUCHED** | INVARIANT — F-b1-4/F-b1-5, ADV-2 |

**Gate roll-call (b1's half; the slice's full gate set is the merged design's):**
SUITE PASS (177 + b4's witness delta) ×4 modes debug + release, error-grep 0
across trees; ASAN full suite + ctest + eqgate (the diff-input rescan is
rebuild terrain — hard gate, 2 sweeps); eqgate LIVE refereeing the diff-input
witness (answer + sorted published-delta identity, per O-7); 20/20 pinned regen
[BYTE] ×3; config-invariance single-hash ×3 + release==debug on the witnesses;
E-62 clean; Q5 MUST RUN (the slice moves bytes — no byte-identity waiver);
permcheck N/A (driver-sorted flushes). The b1-specific pre-bless expectation:
the ONLY monotone-surface reds allowed are ZERO — any [BYTE] surface moving is
STOP (b1 is byte-neutral off the diff-input path by construction).

**Cross-lane coordination (declared expectations b1 provides / consumes):**

| # | expectation | b1 ↔ lane | resolution |
|---|---|---|---|
| Y1 | `input_removal_frontier` member + `InputRemovalFrontier()` accessor exist for the band | b1 → b3 | PROVIDED (E1f-1/E1f-2); b3 keys the a2' removal drain + the shared-mold Present conjunct on `InputRemovalFrontier().has_value()` (member-presence, never a folded bit) |
| Y2 | `input_diff` selector authority for codegen | b1 → b3 | PROVIDED via the member presence + the V-INST-INPUT-COHERENCE stamp (E1f-4); NO region `bool` added (keyed on the member, d2 discipline) |
| Y3 | the a2' drain is EFFECT-only (no new enrollment/op) | b1 → b3/b2 | RULED F-b1-4; b3's band drains the member, mints no op |
| Y4 | `demand_diff_input_1` golden/driver + `.dr` ADV-8 comment | b1 → b4 | b1 flips the runall.sh regex + CLAUDE.md prose; b4 owns the golden, driver, and the stale `Build.cpp:1344` comment (H-17/ADV-8) |
| Y5 | the D3.a.1 witness input differentiality | b1 (self, verified) | RESOLVED: `demand_neighborhood_witness` + `_mono_witness` both take `#message add_edge(u64,u64)` MONOTONE (no `@differential`) — `input_diff` false, both [BYTE] under b1. The D3.a.2 witness is a NEW, separate diff-input case (b4) |
| Y6 | the H-7/H-8 region+lowering edits land in b1 (not b2) | b1 ↔ b2 | the orchestrator folded them into b1 (item 5); b2's death wiring shares `LowerSubgraphInstances` — the merge inserts b1's diff-input branch beside b2's death branch, no overlap (b1 owns the input_* members/fetches; b2 owns removal_frontier/death) |
| Y7 | NO new DROp/VecRole/EffKind/dump-token | b1 → all | RULED F-b1-6/F-b1-7; the a2' drain reuses kVecDrain+kNetRemoval; no E-71 note owed by b1 |

---

## §5 d7 LIVENESS-BY-PERTURBATION TABLE (b1's rows; run at stage (d), texts recorded, all reverted)

Ritual amendments honored (banked D3.a.1): **WIP-commit the prototype worktree
BEFORE perturbation cycles**; perturbation vehicles are the **NESTED arm** (the
diff-input witness compiled `-demand -demand-instance`, the four modes are the
flat arm); **never-minted roles are GENUINELY never-minted for the flow** — the
kProductInput class (the L3 amendment; a diff input now OWNS the queue/frontier
roles, so those are NOT never-minted — kProductInput is a product-family role
the demand slice never mints); background shells use **absolute paths**.

| # | belt / arm | procedure (scratch worktree, revert after) | expected |
|---|---|---|---|
| L-b1-1 | V-INST-SOLE half-2 (pub-alias, surviving) | scratch: force a mint where `input_table == table_op_table` (perturb the ResolvedInstance re-resolution to alias pub as input); compile the witness nested | SIGABRT "V-INST-SOLE: an instantiate's summarized input aliases its published table" — proves half-2 still guards |
| L-b1-2 | V-INST-SOLE acyclic belt (ADV-1 new) | scratch: make `TableIsInductionOwnedDR` return true for the diff input (or point the input at an induction-owned diff table); compile witness nested | SIGABRT "V-INST-SOLE: a differential summarized input is induction-owned" — proves the re-pointed teeth fire (the OB8 precondition belt) |
| L-b1-3 | V-INST-EFFECT third-axis (E1d) | scratch: in InstantiateEffects, mint the input net-removals drain but LEAVE the totality at `input_drains==1` (i.e. perturb E1d-3 back); compile witness nested | SIGABRT "V-INST-EFFECT: a SUBGRAPH_INSTANTIATE effect set is not the §3.3 regime-split totality" — proves the count split runs |
| L-b1-4 | V-INST-EFFECT drain-role admit (E1d-2) | scratch: mint the removal drain with `input_diff` forced FALSE at the validator only (mint keeps it) | SIGABRT "V-INST-EFFECT: an instantiate kVecDrain is not a net-additions drain … nor a differential input's net-removals rebuild drain" — proves the role gate is input_diff-keyed |
| L-b1-5 | V-INST-DRAIN input diff arm (E1e) | scratch: suppress the `-` kFrontierFilter producer for the diff input (make `mint_filter(pt,-1)` skip, a never-minted-role-equivalent perturbation); compile witness nested | SIGABRT "V-INST-DRAIN: an instantiate's input(edge) frontier(s) were never provisioned … a differential input needs BOTH signs" — proves the dr_ok both-sign check runs from ValidateDROps |
| L-b1-6 | V-INST-DRAIN input MONOTONE arm (E1e cf_ok) | scratch: on the R-MONO mono witness, delete the eager append (Build.cpp:1110 gate flipped) so cf_ok fails; compile mono witness nested | SIGABRT "V-INST-DRAIN: an instantiate's input(edge) frontier(s) were never provisioned" (cf_ok arm) — proves the monotone leg is unbroken by the split |
| L-b1-7 | V-INST-INPUT-COHERENCE (E1f-4 new) | scratch: force `si->input_removal_frontier` null while `input_diff` true (skip the Emplace); compile witness nested | SIGABRT "SUBGRAPHINSTANCE store N: input_removal_frontier presence (0) != TableIsDifferential(input)=1" — proves codegen's selector authority is checked |
| L-b1-8 | orphan-mint fence, diff input (E1f-3 new) | scratch: suppress the CF frontier mint for the diff input (make EmitFrontierFilter skip pt's kNetRemovals) so the fetch would mint-on-miss; compile witness nested | SIGABRT "orphan-mint fence: differential input +/- frontier not pre-minted (store N)" — proves the silent-orphan hazard is fenced |
| L-b1-9 | fence (iii) lift end-to-end (E1a, NEGATIVE liveness) | LANDED: compile `demand_diff_input_1` (and the b4 witness) `-demand-instance` on the slice tree | exit 0 (admission) — the removed diagnostic no longer fires; the surviving `demand_cyclic_1`/`demand_recursive_content_1` still reject (their L-rows unchanged) |
| L-b1-10 | the co-landability chain (§0, XC-3) | scratch: apply ONLY E1a+E1b (lift both fences) WITHOUT E1e; compile the b4 witness nested | SIGABRT at V-INST-DRAIN's input arm (the eager append skipped for a diff table) — proves b1 is NOT independently landable, the §3 ABORT-2 recorded |

L-b1-3/L-b1-4/L-b1-5/L-b1-8 use the kProductInput-class never-minted-role
discipline where a role must be faked; L-b1-5/L-b1-8 fake by SUPPRESSING a real
mint (the genuinely-observable perturbation, not a synthetic role the flow
would never carry).

---

## §6 SUMMARY — landable unit + open coordination

b1 = the DR-layer admission for a `@differential` input: two fence lifts (E1a
Build.cpp, E1b V-INST-SOLE), the `input_diff` third axis threaded to the effect
mint + its removal drain leg (E1c), two validator regime splits (E1d
V-INST-EFFECT, E1e V-INST-DRAIN), the region member + fenced fetch + coherence
stamp + ClassifyVector (E1f), and the H-20 comment respell (E1g). Every edit is
regime-keyed on the RIGHT axis (`input_diff` for the input machinery, P-DEATH/
`diff` untouched) and NEVER folds the three predicates (§7 d2). No new
DROp/VecRole/EffKind/dump-token; no new E-71 note. Monotone-path
byte-identical by construction (the d5 discipline). **NOT independently
landable** — it co-lands with b2 (death/removal_frontier — no input twin,
ADV-2), b3 (the a2' removal trigger + the Present rescan conjunct — the silent
miscompiles b1 alone would ship), and b4 (the witness + eqgate + the
demand_diff_input_1 golden disposition) as ONE commit, per the §3 abort chain.

OPEN for the orchestrator/critics: the runall.sh/CLAUDE.md prose
final wording (b4 co-owns since it names the new witness); whether the optional
C15 `.rel` input-diff render marker is worth a stage-(b) E-71 note (b1 says
NO — the member/`.ir` already surfaces it, and F-b1-7 keeps the effects line
token-clean).
