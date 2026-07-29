# D3.a.2 — BINDING DESIGN (stages (b)/(c) adjudicated) — differential input: fence lifts, the a2' removal arm, the Present rescan, the witness 2×2

> **House banner.** XHIGH ADJUDICATION at tip **b4d08307** (verified
> `git rev-parse HEAD`; code bytes == the D3.a.1 landing 33cabcf1 — every
> commit atop is docs-only, so every code anchor below is live at the landed
> binaries and the frozen-A compiler
> `/private/tmp/claude-502/-Users-pag-Code-DrLojekyll/1c4f7baa-aa01-4834-b5c8-3a50396dd83f/scratchpad/frozenA/drlojekyll-debug`).
> Inputs: b1/b2/b3/b4-design.md + the four critiques (read in full); EVERY
> load-bearing critique finding RE-VERIFIED AT CODE by this adjudicator
> (grounds in §7; two adjudicator probes archived at
> `.../scratchpad/d3a2b/adjprobe/` — the CMP-input reject and the MERGE-input
> no-instance fallback, §2/A2.3). Binding context honored, NOT re-litigated:
> d3a2-substrate.md §7 (**R-A2-TRIGGER: TWO DRAINS, NO RECYCLE; gate-set
> identity; band order a0→a1→a2→a2'-APPENDED; Present spelling RULED; the
> ungated-late-Recycle FORBIDDEN fence**), §3 abort chain, §4 OB1-OB8, §5 gap
> ledger; d3a-ruling-brief.md (OQ-INPUT / OQ-MODEL / OQ-DEATH-VS-REBUILD);
> d3a1-substrate.md §7 (the d2 CO-ACTIVATION ruling — P-STORE at Rel.cpp:1055,
> P-DEATH at Rel.cpp:1140, NEVER folded; the input axis extends the anti-fold
> discipline, never joins it) + §8 (e1-e8); KeyedInstances.md §20(AK)-(AM).
> House rules: always-on validators = fprintf+abort surviving NDEBUG; new dump
> tokens need E-71 notes (this slice owes ZERO — R-4); the eqgate flat==nested
> answer + sorted published-delta identity is the standing oracle; per-surface
> [BYTE]/[STRUCT] pre-registered in §6.
>
> The four lane designs travel with this draft as ANNEXES
> (`.../scratchpad/d3a2b/b{1,2,3,4}-design.md`). This draft BINDS: where it
> amends a lane, the amendment wins; where silent, the lane's edit spec is
> adopted verbatim (with its critique's cosmetic anchor fixes applied).
> **THE WHOLE SLICE LANDS AS ONE COMMIT** (the substrate §3 abort chain: e1
> alone aborts at V-INST-SOLE; +e2 aborts at V-INST-DRAIN's input arm; +the
> validator splits alone SILENTLY MISCOMPILES twice — stuck-present +
> over-materialization; only the full set {b1 ⊕ b2 ⊕ b3 ⊕ b4} is landable).

---

## §0 HEADLINE SHAPE + ADJUDICATOR RATIFICATIONS

**The merged slice** = ONE commit, four sub-diffs:

- (i) **b1** — the DR-layer admission: FENCE (iii) lifted (Build.cpp
  `diff_input` arm + flag deleted; cyclic + recursive-content siblings
  SURVIVE — load-bearing for OB8); V-INST-SOLE's differential half lifted with
  the pub-alias half reworded truthful + a NEW induction-owned-input defence
  belt (ADV-1); the `input_diff` THIRD predicate axis
  (`TableIsDifferential(input_table)`, its own spelling, never folded — §7 d2
  extended) threaded into `InstantiateEffects` with the second
  `kVecDrain{input, kNetRemoval}` leg; V-INST-EFFECT third-axis split
  (`drains == input_diff?3:2`, `input_drains == input_diff?2:1`) + the
  adjudicator-folded **O-1 closure belt** (`input_diff && !diff` aborts —
  A1.4/R-3); V-INST-DRAIN input-arm regime split (both-sign `dr_ok` for a diff
  input, `cf_ok` monotone — the XC-3 twin of the D3.a.1 demand arm); the
  region `input_removal_frontier` UseRef + `InputRemovalFrontier()` accessor +
  ClassifyVector read arm + the fenced pre-minted ± fetch (the :318 idiom) +
  the NEW always-on **V-INST-INPUT-COHERENCE** stamp (member presence ==
  input_diff); two new fork/waitpid TESTs in `rel_validators_test` (A1.8/R-7);
  the H-20 stale-comment rider.
- (ii) **b2** — the codegen band (Database.cpp `EmitSubgraphInstance` ONLY):
  E2a hoists `input_removal = region.InputRemovalFrontier()` + `input_diff =
  input_removal.has_value()` (the codegen selector = member presence, belt-
  checked by b1's coherence stamp); E2b grows the ONE shared rescan mold's
  key-filter with the `input.Present(s)` conjunct under `input_diff` (the
  RULED Present spelling; rides ALL THREE sources — a1 birth, a2 edge-adds,
  a2' edge-removals); E2c appends **band-(a2')** — the edge net-REMOVALS
  drain, a character-for-character gate-set clone of band-(a2) (iid → nested
  demand Find+Present → !TouchedFlag → the mold), **NO RecycleCurrent**
  anywhere in it, landed a0/a1/a2 bytes untouched ahead (R-A2-TRIGGER §7(4)).
- (iii) **b3** — the ARGUMENT lane (comments only, [BYTE] on every generated
  artifact): the e4 lemma (L-EDGE / L-MONO-irrevocability / L-DEMAND /
  L-COMBINED-documentary-fence) discharging the in-source D3.a.2 RIDER at
  Database.cpp:2494-2495 (E3a); the e5 divergence-goes-live audit (A1-A12,
  ZERO edits needed — the d2 anti-fold payoff); the N-1 close (InstanceStore.h
  comment edits E3b/E3c); the pinned FIVE-WAY input-quiescence coupling
  (OD-15 idiom, a0 wording amended per A3.2).
- (iv) **b4** — the witness 2×2 (D-b4-1): NEW flagship
  `demand_diff_neighborhood_witness` (diff-input × MONO-demand — the e5
  carrier, first P-STORE∧¬P-DEATH program, census `kInstanceDeath=0` beside
  `kSubgraphInstantiate=1`) + `demand_diff_input_1` REPURPOSED
  diagnostic→golden as the diff×diff composition witness (E-F1/E-F2/E-F3);
  both with `.batches` (the oracle SEES input retractions — stronger than
  D3.a.1) + `.eqgate`; suite 177→**178**, eqgate carriers 2→**4**; the
  a1-Present discriminator REPAIRED cross-batch (A4.1 — the b4C-1 HIGH);
  runall.sh/CLAUDE.md flips; the consolidated gate plan + d7 L-table.

### Ratifications (adjudicator rulings within the granted frame; none escalate)

- **R-1 (b1C-1): the lane decomposition is CORRECTED, not shrunk.** b1's §0/Y6
  described a "b2" owning death wiring — a stale read of the SUBSTRATE §5 lane
  proposal, superseded by the §20(AM) recut this fleet ran under. The REAL b2
  (b2-design.md) is the codegen-band lane with genuine content (E2a/E2b/E2c);
  the death machinery (removal_frontier, kInstanceDeath, band-(a0),
  CheckInstanceDeathFrontier) LANDED at D3.a.1 and is UNTOUCHED — input
  removals never mint death (ADV-2/H-5; OQ-DEATH-VS-REBUILD). The landable
  unit is **b1 ⊕ b2 ⊕ b3 ⊕ b4 as ONE commit**. b3's §0 "NOT OWNED" lane
  attributions (which assigned the effects leg / region member / classify arm
  to "b2") are corrected the same way: those are b1's E1c/E1f (verified: no
  edit is specified twice and none is orphaned — §5 X4).
- **R-2 (b1C-2 + H-17): the demand_diff_input_1 disposition is b4's D-b4-1,
  RATIFIED, and the suite flip is MANDATORY AT CO-LAND.** After e1 the case
  COMPILES (its `.drflags` verified `-demand -demand-instance`; the fence
  string verified at Build.cpp:1553-1555), so leaving it in runall.sh:361's
  all-4-modes-diagnostic list reds the suite — b1's "IF b4 promotes …
  otherwise void" conditional is struck. b4 repurposes it as the diff×diff
  composition witness (E4g-E4j: header rewrite discharging ADV-8, `.drflags`
  → `-demand -demand-retract`, the inert stub driver → the E-F1/E-F2/E-F3
  runtime driver, goldens ×3); b1 cuts the :361 regex; b3's E3d (the same
  header's comment edit) is **SUPERSEDED by E4g** — one owner, no double edit
  (A3.6).
- **R-3 (b1C-4 + b2C-5, adjudicator extension): the O-1 closure BELT.**
  E1d additionally aborts on `input_diff && !diff` (a deletable input feeding
  a MONOTONE pub — no delete side to retract into; structurally impossible via
  the Differential.cpp closure, O-1). Verified at code: `diff` is computed at
  the case head (Rel.cpp:4271), the conjunct is two lines. This turns b1's
  "D3.a.3+ could break co-truth — both sites handle it" (over-claim) and b2's
  F2a "provably unreachable else branch" (empirically-grounded reliance) into
  a CHECKED invariant. Always-on; [BYTE] flag-off. The d3a1 A2.1 extension
  precedent.
- **R-4 (C15 + E-71): NO input-diff dump marker this slice; E-71 notes owed =
  ZERO.** All four lanes converge (b1 F-b1-7, b2 §6, b4 D-b4-4, both critics
  confirm): the removal leg renders as `kVecDrain(<input_tid>, kNetRemoval)`
  from existing tokens (first live PRODUCTION on a kSubgraphInstantiate — the
  D3.a.1 kInstanceDeath precedent, production not spelling); input-diff is
  already dump-visible via the `input=` tid + the effects line + the flagship
  census shape `kSubgraphInstantiate=1 ∧ kInstanceDeath=0`. A C15 marker
  would mint a new token (an E-71 note) for zero diagnostic gain — REJECTED.
- **R-5 (b2C-3, resolved BY ADJUDICATOR PROBE): the "derived acyclic diff
  input" branch of OB8(i) is UNREACHABLE at tip.** Two probes at frozen-A
  (archived `.../scratchpad/d3a2b/adjprobe/`): (a) a CMP-filtered demanded
  body (`edge(F,T) : add_edge(F,T), F != T.`) is rejected UPSTREAM by the
  plain-`-demand` body-walk — "Unsupported rule-body shape under -demand"
  (adjprobe/cmpin.log); (b) a MERGE-fed input (two clauses for `edge`)
  compiles but mints NO instance (`kSubgraphInstantiate=0`, flat guard-web
  fallback — adjprobe/mrgin.rel census). And the landed witness's nested dump
  proves the admitted single-clause single-hop shape's input model is
  INGEST-written (mono_nested.rel: op.4 kIngestFold writes %table:11 =
  input_table directly; kSeedFold=0). So every D3.a.2-admissible summarized
  input has counters written by explicit folds in the ingest proc — OB8(i)'s
  trivial branch. Consequences: b2's E2b comment and b3's lemma keep the
  derived-branch sentence as FORWARD-LOOKING with a reachability note (A2.3);
  the (d0) protocol RECORDS the input-writer census for both witnesses ×
  modes (kSeedFold must be 0 on the input tid); a NAMED OBLIGATION binds any
  future widening of the demand body-walk or the instance recognition to
  re-derive OB8(i)'s derived branch WITH a directed witness first. No new
  fence (the upstream reject + fallback already are the fence); no new
  witness case owed this slice.
- **R-6 (b4C-1 HIGH): the flagship a1-Present discriminator is REPAIRED
  cross-batch (fix (i), the critic's preferred).** NetBatch annihilates a
  same-batch ± of one row BEFORE any fold (Vec.h:176ff, verified; the
  design's own O-6/m2 step proves it empirically), so the as-designed m3
  (`send_pm({{5,12}},{{5,12}})`) never creates the dead-but-present row the
  a1-source Present conjunct exists to skip — L-b4-D was a non-firing
  perturbation and the "flagship covers BOTH sources" claim false. A4.1
  splits it into two epochs. L-b4-D now fires (conjunct deleted ⇒ p5 = {12},
  triply red). The composition's E-F2/p1b remains the diff-demand a1 witness.
- **R-7 (R-5-of-d3a1 precedent): ctest packaging.** b4 §4.4's two negative
  TESTs (`InputArmRejectsMissingRemovalProducer`,
  `EffectCountRejectsWrongInputDrainRole`) are ASSIGNED to b1 (A1.8), landing
  in the EXISTING `rel_validators_test` binary via the shared
  `tests/DrTest/DeathHarness.h` fork/waitpid mold. ctest stays **6/6
  binaries** (+2 TESTs). No InstanceStore runtime unit is owed (OQ-MODEL:
  the store stays a monotone set island; the D3.a.1 death-rebirth unit
  stands).
- **R-8 (the (d0) protocol, merged): whole-slice-first-green baseline.**
  Co-landability forbids a witness-alone-on-tip baseline (the abort chain),
  so (d0) = the first green whole-slice compile in the prototype worktree,
  BEFORE bless: record both witnesses' nested `.rel`/`.ir`/`.h` (+ flat),
  re-anchor every absolute id/census numeral, EYEBALL the flagship census
  `kInstanceDeath=0 ∧ kSubgraphInstantiate=1` (F-b4-4) and the R-5
  input-writer census (kSeedFold=0 on the input tid, both witnesses, spot ×
  modes), recompute the oracle [COMPUTED] goldens (incl. the A4.1 batch
  split). The repurposed `demand_diff_input_1`'s baseline = its archived
  pre-slice all-4-modes-diagnostic behavior (frozen-A logs,
  `.../scratchpad/d3a2b/{nested,compo_n}.log`); it had NO goldens, so churn
  is all-new. Then b4's Phase-A exact-red-set run → manual flat==nested cmp →
  `--bless` filtered → Phase-B zero-red (§6.2).
- **R-9 (prose single-owner): b4 owns runall.sh + CLAUDE.md.** b1's E1a
  proposed the same CLAUDE.md:97-100/:483-486 and runall.sh edits b4's
  E4k/E4l specify — ONE owner (b4), b1's drafted text folds in as input
  (b1C-7's `:1504-1558` anchor correction applied; the replacement prose
  names both new witnesses). One edit per file at the landing commit.

---

## §1 SUB-DIFF (i) — b1 ADOPTED, WITH AMENDMENTS

b1-design.md §1-§2 edit specs **E1a-E1g** are ADOPTED (every anchor
critic-verified exact and re-verified by this adjudicator: Build.cpp:1504 gate
/ :1516 flag / :1529-1531 `in = jl[1]` + `CanReceiveDeletions` / :1553-1555
arm; Rel.cpp:4336-4341 V-INST-SOLE; :4271 `diff` / :4279-4300 drain case /
:4324-4331 totality; :4542-4548 input arm + :4514-4529 cf_ok/dr_ok;
InstantiateEffects :781-782 sig / :795-799 edge_drain / :1055 `diff` local /
:1097-1098 the ONLY call; Procedure.cpp:196-211 ClassifyVector / :293-304
coherence mold / :314-321 demand fence / :325-336 input fetch+Emplace;
Program.h:1175; include Program.h:861/:866; Program.cpp:759-768;
`TableIsInductionOwnedDR` at Rel.cpp:79 with `context` in scope). The three
core findings stand verified: **F-b1-2/F-b1-3** (the diff input's ± producers
are FREE and both CF vecs pre-mint inside LowerDRFlow, Stratum.cpp:748-750/
:1637-1642/:2477, BEFORE LowerSubgraphInstances at Procedure.cpp:543 but AFTER
ValidateDROps at :2186 — hence dr_ok, never cf_ok, the XC-3 twin), **F-b1-4**
(the a2' drain is EFFECT-ONLY — no new DROp, no V-INST-EMITTED row; the e5
store enrolls/emits {instantiate, seal} = 2 vs 2), **F-b1-6/F-b1-7** (no new
DROpKind/VecRole/EffKind/dump token).

**A1.1 (R-1 / b1C-1): §0 + Y6 lane map REPLACED.** The co-landing set is
"{E1a..E1g} ⊕ b2 (the codegen band E2a-E2c) ⊕ b3 (arguments) ⊕ b4
(witnesses) as ONE commit". Every mention of a b2 "death wiring" deliverable
is struck — the death machinery landed at D3.a.1, has NO input twin
(ADV-2/H-5), and no lane touches it. b1's H-5/ADV-2 statement stands as
written (it is correct); only the lane bookkeeping moves.

**A1.2 (R-2 / b1C-2): the demand_diff_input_1 flip is UNCONDITIONAL.** b1's
"IF b4 promotes … E1a's runall.sh edit is void" paragraph is struck. b1 cuts
the runall.sh:361 regex member `|demand_diff_input_1` (verified present,
unique, list-final) as a mandatory co-land edit; b4 supplies the repurposed
case content (E4g-E4j). Rider verified: the case's `.main.cpp` today is an
inert stub whose comment says "this driver is inert / never compiled" — b4's
E4i rewrite replaces it wholesale, so no stale-comment residue.

**A1.3 (b1C-3): d7 row L-b1-10 is pinned to E1a+E1b ONLY** (E1c, E1d, AND
E1e all withheld). Only then is the effect set unchanged, V-INST-EFFECT passes,
and V-INST-DRAIN's input arm is GENUINELY the first abort (substrate §3
ABORT-2). Recorded rider: the natural-looking "E1a+E1b+E1c+E1d minus E1e"
variant aborts EARLIER at V-INST-EFFECT's totality (drains==3 vs the unpatched
2u) — an operator observing that must not flag chain divergence.

**A1.4 (R-3, adjudicator extension): E1d gains the O-1 closure belt.** In the
`kSubgraphInstantiate` case, immediately after E1d-(1)'s `input_diff` decl:

```cpp
          // [D3.a.2 R-3] O-1 closure belt: a @differential summarized input
          // FORCES a differential pub (the lib/DataFlow/Differential.cpp
          // closure). input_diff && !diff would be a deletable input feeding
          // a MONOTONE pub — no delete side to retract into; never a legal
          // mint. Keeps the three-axis separation CHECKED, not assumed
          // (the b2 F2a else-arm deadness and the §7-d2 divergence audit
          // both rely on this implication). Always-on; survives NDEBUG.
          if (input_diff && !diff) {
            ValidatorFail("V-INST-EFFECT: a differential summarized input "
                          "over a MONOTONE published table (the O-1 closure "
                          "is broken)");
          }
```

[BYTE] flag-off (`input_diff` false on every landed program). d7 row L-b1-11
(§6.4) is its teeth.

**A1.5 (b1C-5): §4 gains the model↔emission statement.** The a2' drain's
model-effect (E1c) and region member (E1f) are belt-checked per hop, but the
EMITTED band's actually-draining-the-vector link is eqgate-referred — the
landed instance-band architecture (Database.cpp is outside V-PRED-XCHECK's
Emit*-template net; the substrate §3 "no third abort" class). No reviewer
should expect an always-on emission cross-check; Lb2-2/L-b4-B are the
standing referees.

**A1.6 (b1C-6): the selector authority is TWO independent belts, stated.**
Model-drain ⟺ input_diff (E1c mint + E1d count, DR layer) and member ⟺
input_diff (E1f-4 V-INST-INPUT-COHERENCE, CF layer) — no single belt spans
model-drain ↔ member; neither may be deleted on the assumption the other
covers it. (The commit message records both couplings.)

**A1.7 (b1C-7 + b3C-4): anchors + prose.** The fence block extent is
Build.cpp:1504-1558; every design-body "Build.cpp" citation is file-qualified
**lib/ControlFlow/Build/Build.cpp** (the lib/DataFlow/Build.cpp collision is
real — b3's critic landed in the wrong file at the same line numbers).
CLAUDE.md/runall.sh prose ownership moves to b4 per R-9; b1's drafted
replacement text is the input, with the new-witness names added.

**A1.8 (R-7): b1 owns the two validator TESTs** (spec'd in b4 §4.4, adopted
verbatim): `TEST(RelValidators, InputArmRejectsMissingRemovalProducer)` (a
diff-input instantiate flow with the kNetRemoval frontier-filter producer
absent → `RunInForkedChild` SIGABRT citing the V-INST-DRAIN input arm;
positive twin clean) and `TEST(RelValidators,
EffectCountRejectsWrongInputDrainRole)` (wrong-role input drain → V-INST-EFFECT
SIGABRT; `input_drains==2` twin clean). Existing binary, DeathHarness.h mold,
ctest 6/6.

Interface notes locked: the Emplace of `input_removal_frontier` is gated
EXACTLY on `TableIsDifferential(op->input_table)` and stamped by
V-INST-INPUT-COHERENCE — this IS b2's F2c precondition, now belt-checked, and
the commit message states the coupling. No new dump spelling; no E-71 note
owed by b1 (R-4).

---

## §2 SUB-DIFF (ii) — b2 ADOPTED, WITH AMENDMENTS

b2-design.md §0-§9 (**E2a/E2b/E2c**) are ADOPTED — the critique's verdict
"code-correct, well-anchored, honours R-A2-TRIGGER §7 and the d2 anti-fold
discipline" is re-verified by this adjudicator: E2a insertion :2350/:2351;
E2b old-text at :2380-2391 verbatim; E2c insertion after the `// edge drain`
close at :2522-2523 before the band-(b) comment :2525; the E2c gate fork is a
character-for-character emitted-string clone of band-(a2) :2486-2510 (gate-set
identity BY CONSTRUCTION OF COPY — R-A2-TRIGGER §7(2)); `table_member` values
are `std::string` so the E2b concatenation type-checks; PushIndent/PopIndent
balance 3/3 diff, 2/2 else; `emit_instance_rescan` has exactly two callers
today and NO sibling input scan exists, so the ONE-mold conjunct rides
a1/a2/a2' by construction. The NO-RECYCLE discipline, the F2a retained-else
(now belt-attested via A1.4), the clone-not-helper choice (landed a0/a1/a2
bytes untouched — §7(4) diff minimality; the ~24-line gate duplication
recorded as a D3.a.3 [F]-dedup candidate), and F2d's three-axis separation
are all RATIFIED.

**A2.1 (b2C-1): X-b1-2 is a NAMED BLOCKING CONTRACT + an accepted-risk
line.** b2 must not merge before b1's E1f-3 orphan-mint fence (the
`(input, kNetRemovals)` + `(input, kNetAdditions)` pre-mint check) is in
place — an unfenced empty `*input_removal` is the silent stuck-present
under-rebuild (OB1) no belt catches. ACCEPTED RISK, recorded: like band-(a0),
the band itself trusts its optional member with no runtime orphan belt; the
mint fence (b1) + Lb2-2 are the referees, and **Lb2-2 runs against the REAL
b1 producer**, never a hand-mocked vec. (Moot-in-practice under the one-commit
landing; binding for any staged prototype work.)

**A2.2 (b2C-2): the diff-input ±2 symmetry claim is EXPLICITLY
eqgate-contingent on a NEW flat path, and the acceptance line is named.** The
E-D doubling (flat NetDeleted del-arm −1 ⊕ band-(b) drop-scan −1) was never
refereed by a landed eqgate (D3.a.1's retraction path was diff-DEMAND). The
b4 flagship's **d1 (E-D) phase with the sorted `nbhd_out` delta** is the
referee: flat == nested == golden on the published `-(1,2)` — a b4 ACCEPTANCE
CRITERION (§4/§6), not an implication of "the eqgate referees it".

**A2.3 (R-5 / b2C-3): the derived-input sentence gains the reachability
note.** E2b's comment clause "a derived acyclic input's seed folds run in the
ready_after-lifted stratum ahead of the band — the OB8 lemma, b3" is KEPT
(the stratum-lift ground is real: Rel.cpp:1170-1173 `1 + max(ready_after(
demand), ready_after(input))`) and gains: "(currently unreachable: the
plain-`-demand` body-walk rejects non-plain demanded bodies and a MERGE-fed
input mints no instance — every admitted input model is ingest-written; any
widening of the body-walk or the recognition re-derives this branch with a
directed witness FIRST)". Same note lands in b3's OB8 lemma text (§3) and the
(d0) protocol records the input-writer census (R-8). Probes archived
(adjprobe/cmpin.log, adjprobe/mrgin.rel); the named obligation goes in the
landing-record residuals.

**A2.4 (b2C-4): §4's e5-gate wording re-graded.** The mono-demand gate path
(diff-pub probing a monotone `Table` member) is compile-verified but has ZERO
landed runtime coverage until the flagship runs — the §4 audit reads "safe
once Lb2-6 / L-b4-A run green", and Lb2-6 is a HARD landing gate (the first
executed e5 emission).

**A2.5 (b2C-5): F2a's "provably" is grounded + now checked.** The else-arm
deadness rests on O-1's structural closure over the ADMITTED shape (no
differentiality-absorbing node between input and pub — a fence precondition);
A1.4's belt makes the implication validator-attested, so the retained else
arm is dead-by-checked-invariant, kept for the §7(2) char-identity audit.

**A2.6 (b2C-6, cosmetics applied):** band-(b) born loop closes :2634, belt
:2635-2645; E2b's trailing `sep = " && ";` is a deliberate symmetry-preserving
dead store (matches the landed loop's); §7 row 1's [BYTE] set explicitly
carves out `demand_diff_input_1` (a diagnostic pre-slice, repurposed by b4 —
never in the [BYTE] stdout set).

The §5 TouchedFlag/epoch-coupling facts and the §3 E-D/E-F2/E-F3 walks are
ADOPTED as the mechanism half of e4 (b3 owns the lemma statement; the
interlock is quoted in §3 below). E-71 notes owed by b2: NONE (R-4).

---

## §3 SUB-DIFF (iii) — b3 ADOPTED, WITH AMENDMENTS (the e4 lemma binds)

b3-design.md §1-§7 (**E3a/E3b/E3c**; E3d superseded — A3.6) are ADOPTED. The
critic's crux confirmation is re-verified here: the e5 carrier EMITS the
diff-arm gate against a MONOTONE demand `Table` member and it COMPILES
(`Table::Find` inherited from RowStore; `Table::Present(uint32_t)` =
assert+`return true` at Table.h:261-266; `DiffTable::Present` = counts>0 at
:418-424) — the emission TEXT is regime-invariant, only the member's resolved
C++ type differs; and `region.DemandTable()` is non-null for e5 because the
Emplace (Procedure.cpp:368, verified in the `if (inst.differential)` block)
keys on P-STORE, which is TRUE for e5. b3 correctly stays a comment lane.

The e4 lemma is adopted **verbatim** as the slice's soundness spine —
L-EDGE (edge add OR retract epochs leave demand frozen; T-3 + T-5),
L-DEMAND (demand epochs leave both input frontiers empty — per-epoch-fresh
`::hyde::rt::Vec<...>` proc locals, Database.cpp:1840-1843, verified), and
L-COMBINED (the hypothetical combined demand+input entry pinned as a
DOCUMENTARY fence — exactly what R-A2-TRIGGER §7(4) left to e4). The
IRREVOCABILITY sentence is quoted verbatim (adjudicator-verified against
Table.h:261-266 and the XC-9 ruling; this is the NEW e4-normative sentence
the substrate demanded):

> **THE IRREVOCABILITY SENTENCE (XC-9, e4-normative).** When the demand table
> is monotone, a key's demand, once forced, is NEVER retracted; a bound iid
> therefore always implies a live demand and the demand row that minted it persists
> monotonically (so `Find` succeeds and `Present` is unconditionally true).
> The gate is sound not by QUIESCENCE (the demand table has no retract epoch
> to be quiescent across) but by IRREVOCABILITY — there is no dead-key-still-
> binds-iid hazard because there are no dead keys. The zombie-rebirth the R-3
> gate defends against (D3.a.1 HIGH-1) is UNREACHABLE under monotone demand;
> the emitted `Present`/`Find` conjuncts are correct-but-redundant belts, not
> load-bearing filters.

**E3a** (the RIDER discharge at Database.cpp:2494-2495 — anchor re-verified
verbatim, a pure source comment above the arm's first `cc <<` at :2496ff) is
adopted with its replacement text VERBATIM, plus one appended sentence from
A2.3 (after "…each writing a single channel."): `// (The L-EDGE derived-input
branch is FORWARD-LOOKING: today every admitted summarized input's model is
ingest-written — the plain-demand body-walk and the instance recognition
exclude seed-fold-written inputs; a widening re-derives OB8(i) first.)`

**The §2 e5 divergence audit (A1-A10) is RATIFIED — ZERO edits needed** on
the P-STORE side under n_death==0. F3a (V-INST-EMITTED balances 2-vs-2,
op-presence-keyed, NOT edit-needed) re-verified at Procedure.cpp:546ff +
:407-408; F3c (the d2 anti-fold payoff — the divergence absorbed with no new
branch) is the audit's headline, RECORDED. F3b's classify-arm dependency is
RE-ATTRIBUTED to b1 E1f-5 (R-1); L3b stays its teeth.

**A3.1 (b3C-1): the audit table gains two rows + one intro note** so "every
site re-read" is literally true (all three verified vacuous by this
adjudicator):
- **A11 — lib/ControlFlow/Format.cpp:663** `if (auto removal =
  region.RemovalFrontier(); removal)` — the `.ir` render of the death drain;
  region-member-null-guarded (the A5 class, dump surface); e5: not rendered.
  **OK.**
- **A12 — Rel.cpp:4410-4415** — V-INST-PAIR's second check (`death_per_store`
  orphan-death loop); e5: empty map, vacuous. **OK.**
- Intro note: the InstantiateEffects fork also carries the diff-independent
  `kInstanceRebuild`/`kStateEmit`/`kStateOld` legs (Rel.cpp:813-828) — present
  for e5, death-free by construction, counted by E1d's unchanged
  rebuilds/emits/olds conjuncts.

**A3.2 (b3C-2): the FIVE-WAY coupling's mechanism 2 is respelled** (the block
is the copy-paste OD-15 idiom; the imprecision must not propagate):
"Whichever arm (a0 death, a1 birth, a2 edge-add, a2' edge-removal) first
touches key K does the ONE full Present-filtered rescan of K's net content
**(or, for a0, the RecycleCurrent recycle-to-empty that lets band-(b) retract
K's whole frozen set — a0 Touches and EMPTIES, it never rescans)**; every
later arm skips." Verified at Database.cpp:2435 (RecycleCurrent) +
InstanceStore.h:216-219 (Touch + Reset). The rest of the coupling block —
NETTING / V-INST-FRESH / the Present conjunct / the demand-liveness gate +
the per-epoch-shape interlock + C-REC — is adopted verbatim; C-REC's fence
anchor is file-qualified per A1.7 (lib/ControlFlow/Build/Build.cpp:1533-1540,
re-verified surviving the E1a deletion set).

**A3.3 (b3C-3): anchor ranges corrected** — E3b's old block is
InstanceStore.h:**21-26** (line 27 is a bare `//` separator; re-verified);
E3d's (now-moot) header quote was `.dr` lines **3-9**. The quoted old-texts
are exact; only the labels move.

**A3.4 (b3C-4): file-qualified Build.cpp citations** throughout §0/§4/§5
(design body); the E4g `.dr` header (which absorbed E3d) cites the fence
without a bare line-number spelling.

**A3.5 (b3C-5): E3a's verdict wording** — "[BYTE] on every GENERATED artifact
and every DRIVER binary (the codegen source comment is preprocessor-stripped);
the drlojekyll compiler binary's -g line tables shift and no gate hashes it."

**A3.6 (R-2): E3d is SUPERSEDED** by b4's E4g header rewrite (one owner for
the demand_diff_input_1 header; ADV-8 discharged there). b3's corrected
anchor/disposition content served as E4g review input; no separate b3 edit to
that file remains.

The N-1 close (**E3b/E3c**) is adopted verbatim (C-CONF-5 re-verified:
`cur.TryAdd` at Database.cpp:2398 is the sole grower; Seal/RecycleCurrent the
sole emptiers; `TouchCurrent` :130-133 does not reset) — the close is RULED
sound under OQ-MODEL + R-A2-TRIGGER(3) NO-RECYCLE; it reopens only if a2 ever
becomes an incremental shrink.

---

## §4 SUB-DIFF (iv) — b4 ADOPTED, WITH AMENDMENTS

b4-design.md §0-§7 are ADOPTED: the D-b4-1 witness 2×2 (R-2), D-b4-2 the
edge-driven-death observable + the oracle-sees-retractions upgrade (F-b4-2,
critic-verified empirically at frozen-A), D-b4-3 the FORBIDDEN-fence catcher
(E-E `send_pm` different-rows ± — critic-verified `+(1,11) -(1,3)` publishes,
non-annihilated), D-b4-4 zero E-71 notes (R-4), D-b4-5 the H-18 discharges
(G-INPUT-NEG by the a2' band; G-STALE by keyed-frontier subsumption, with the
flagship O-5 step as the no-over-revisit witness), E4a-E4m, the §4 gate plan,
§5 L-table, §6 bless ritual. The critique's empirical confirmations carry:
both driver molds COMPILE and RUN flat at frozen-A with exactly the predicted
answers and 4-mode byte-identity; E-F1/E-F3 silent teeth hold;
`getpt_bf_retract` exists under `-demand-retract`; the Phase-A red set
mechanics (7 lines/case) verified against diffrun.sh/runall.sh.

**A4.1 (R-6 / b4C-1 HIGH): the a1-Present discriminator goes CROSS-BATCH.**
E4e's `m3` step and E4c's final batch are REPLACED:

- Driver (replacing `send_pm({{5,12}}, {{5,12}}, "m3");`):
  ```
  // ---- a1-Present-conjunct: CROSS-batch add-then-retract BEFORE demand ----
  // (same-batch +/- would NetBatch-annihilate pre-fold and never create the
  //  dead-but-PHYSICALLY-PRESENT row the a1 conjunct exists to skip — b4C-1.)
  send_silent({{5,12}}, "m3a");          // 5 undemanded: nothing published.
  retract_edges_silent({{5,12}}, "m3b"); // (5,12) counters -> 0; row stays at
                                         //   RowAt(s) (no compaction, XC-7).
  probe(5, {}, "p5");                    // a1 birth-rescan meets the DEAD
                                         //   (5,12); Present(s) MUST skip it.
  ```
- `.batches`: the final batch splits into two (`+ add_edge 5 12` / then
  `- add_edge 5 12`), comment updated to say CROSS-batch.
- The §1 discriminator roll-call and F-b4-3 now truthfully claim flagship
  coverage of the a1 source; **L-b4-D fires** (conjunct deleted ⇒ p5 = {12}:
  HP-5 abort + eqgate + golden). The composition's E-F2/p1b stays the
  diff-demand a1 witness (both regimes covered).
- Oracle/monotone [COMPUTED] goldens and the assertion count recompute at
  (d0) (the batch count changes 7→8); golden-churn count (6 files) and the
  Phase-A red-set line count (14) are UNCHANGED.

**A4.2 (b4C-2): the silent-lambda amendment names BOTH twins.** The flagship
needs `retract_edges_silent` (rem-only — d2/m3b) AND `send_silent` (add-only —
m3a); the composition needs `retract_edges_silent`. Both are the hard-abort
(`std::abort`, NDEBUG-surviving) pre-flush-empty check; the D3.a.1 A4.3
teeth idiom.

**A4.3 (b4C-3): E4k prose fixes.** The "`Add negate_never_diff_1` stays"
non-edit sentence is struck; the runall.sh header rewrite is scoped to
**:20-25** (verified: the fence-inventory sentence naming demand_diff_input_1
+ the ":23-24 cyclic + diff_input compile under plain -demand" clause), and
the rewrite makes `demand_cyclic_1` the sole compile-under-`-demand`,
reject-under-`-demand-instance` fence with `demand_recursive_content_1` the
upstream one; the two new golden cases are auto-discovered, unlisted.

**A4.4 (R-8): the (d0) protocol as consolidated in §0** binds b4's §4.2/§6
ritual: whole-slice-first-green regeneration + census eyeballs (flagship
`kInstanceDeath=0`; input-writer kSeedFold=0 both witnesses) + [COMPUTED]
recompute BEFORE Phase-A; the repurposed case's baseline = the archived
pre-slice diagnostic logs.

**A4.5 (R-9): b4 owns the CLAUDE.md + runall.sh prose**, folding b1's drafted
text (suite 177→178; the diagnostic-list sentence drops demand_diff_input_1
and names it the diff×diff composition witness; the keyed-instance section
gains the D3.a.2 paragraph — fence lifted, the a2' trigger + Present rescan,
the e5 P-STORE∧¬P-DEATH divergence, the two new witnesses; the nested
pre-pass anchor respelled :1504-1558).

---

## §5 CROSS-LANE RECONCILIATION (every declared sibling expectation, resolved)

| # | expectation | holder → provider | resolution |
|---|---|---|---|
| X1 | `InputRemovalFrontier() -> std::optional<DataVector>`, present IFF `TableIsDifferential(input_table)` | b2 (E2a selector + drain source) → b1 (E1f-1/2) | MATCHED + BELT-CHECKED: the Emplace is `input_diff`-gated and V-INST-INPUT-COHERENCE stamps member==input_diff (E1f-4) — b2's F2c precondition is validator-attested, and the commit message states the coupling |
| X2 | the `(input, kNetRemovals)` + `(input, kNetAdditions)` CF vecs fence-guarded pre-minted (never a silent orphan) | b2 (E2c drains) → b1 (E1f-3) | MATCHED; elevated to a NAMED BLOCKING CONTRACT + accepted-risk line (A2.1); Lb2-2 runs against the real producer |
| X3 | InstantiateEffects removal leg + V-INST-EFFECT `input_drains==2` + V-INST-DRAIN both-sign input arm | b2 §7 `.rel` predictions / b4 I-b4-3 → b1 (E1c/E1d/E1e) | MATCHED — b1 provides all three; the effects line grows exactly one `kVecDrain(<input_tid>, kNetRemoval)` (no new token, R-4); + the A1.4 O-1 belt |
| X4 | ClassifyVector read arm for `input_removal_frontier` | b3 F3b (said "b2's H-7") / b2 X-b1-1 → **b1 E1f-5** | RESOLVED (R-1): b1 owns it; b3's attribution corrected; L3b is its teeth. Sweep verified: NO edit is specified by two lanes and none is orphaned (effects leg = E1c only; member/accessor/classify/fences = E1f only; Present conjunct = E2b only; a2' arm = E2c only) |
| X5 | gate-set IDENTITY a2' == a2 (R-A2-TRIGGER §7(2)) | b3 §1 lemma precondition → b2 (E2c) | MATCHED BY CONSTRUCTION OF COPY (char-identical fork); the d7 identity perturbation (Lb2-4) is the standing referee; a divergence voids b3 §1 |
| X6 | the `input.Present(s)` conjunct on ALL THREE rescan sources (a1 incl.) | b3 coupling mech-4 / b4 L-b4-C/D → b2 (E2b, the ONE mold) | MATCHED — one mold, three call sites (verified no sibling scan exists); witness teeth REPAIRED for the a1 source (A4.1/R-6) |
| X7 | the recursive-content fence survives F-A's lift (C-REC — OB8(i)'s precondition) | b3 → b1 (E1a keeps :1533-1540) | MATCHED + double-belted: E1b's DR-layer induction-owned-input reject re-points the lifted forbiddance's teeth (L-b1-2) |
| X8 | the e4 lemma + RIDER rewrite; E2c's interim gate comment defers to it | b2 X-b3-1 → b3 (E3a) | MATCHED — E3a owns the discharge text (+ the A2.3 reachability sentence); E2c's header comment cites the lemma, does not restate it |
| X9 | witness coverage: both regimes, E-D delta tap, E-F2 rebirth, FORBIDDEN-Recycle catcher | b2 F2b/b2C-2 → b4 (D-b4-1, E4a-E4j) | MATCHED — flagship d1 = the diff-input E-D sorted-delta referee (A2.2, named acceptance line); composition p1b = E-F2; flagship m1/p1c = the E-E catcher; flagship m3a/m3b/p5 = the mono-regime a1 conjunct (A4.1) |
| X10 | demand_diff_input_1 disposition + its header/runall/CLAUDE.md mechanics | b1 E1a / b3 E3d / b4 E4g-E4m (THREE claimants) | RESOLVED (R-2/R-9/A3.6): b4 owns the case content + all prose; b1 owns the runall.sh:361 regex cut (mandatory at co-land); b3's E3d superseded; ADV-8 discharged by E4g |
| X11 | NO new DROp/VecRole/EffKind/dump token; E-71 notes = 0; census kinds unchanged | b4 I-b4-3 → b1/b2 | MATCHED all lanes (F-b1-6/F-b1-7, b2 §6, D-b4-4); the C15 marker REJECTED (R-4); the 11 `.rel` pins keep their census lines [BYTE] |
| X12 | ctest: the input-arm validator TESTs, 6/6 binaries | b4 §4.4 → b1 (A1.8) | RESOLVED (R-7): 2 TESTs in `rel_validators_test`, DeathHarness.h mold |
| X13 | the (d0) baseline protocol under co-landability + the repurposed case | b1 §3.2 / b2 §7 / b4 §4.2 | RESOLVED (R-8): whole-slice-first-green (d0) with census eyeballs + [COMPUTED] recompute; pre-slice diagnostic logs are the repurposed case's baseline; all absolute ids re-anchor at (d0) |
| X14 | the lane decomposition + the death machinery | b1 §0/Y6 (stale) vs the §20(AM) recut | RESOLVED (R-1/A1.1): b1 ⊕ b2(band) ⊕ b3(args) ⊕ b4(witnesses), ONE commit; death landed, untouched, NO input twin (ADV-2/H-5) |
| X15 | the frozen D3.a.1 witnesses stay [BYTE] | all → all | MATCHED + verified: both take monotone `#message add_edge(u64,u64)` (no `@differential`) ⇒ `input_diff` false ⇒ every new code path inert; the mono/diff-demand goldens and eqgates are the standing regression anchors |

The d2 discipline is honored slice-wide and EXTENDED: P-STORE (Rel.cpp:1055),
P-DEATH (Rel.cpp:1140), and the NEW `input_diff` axis are three separately
spelled predicates at every site; no shared helper is minted; the e5 carrier
is the first program where they diverge (P-STORE ∧ input_diff ∧ ¬P-DEATH) and
the divergence is absorbed with ZERO death-side edits (b3 §2 audit) — plus
the one CHECKED implication input_diff ⇒ P-STORE (A1.4), which is O-1's
closure theorem, not a fold.

---

## §6 GATE FAMILY + PRE-REGISTERED PREDICTIONS

### 6.1 Per-surface predictions

| surface | prediction |
|---|---|
| 176 non-witness case stdouts × 4 modes | **[BYTE]** — no other case carries a differential summarized demanded input; every new code path is `input_diff`- or `-demand-instance`-gated; E1c/E1d/E1e reduce to today's exact arithmetic flag-off |
| 20 pinned dump surfaces (demand_tc_witness ×4, 11 `.rel` pins, 14 `.irgold`, `.df`/`.h` regen set) | **[BYTE]** — no pinned case has a diff input; census kinds unchanged (the a2' drain is EFFECT-level); V-INST-SOLE/EFFECT/DRAIN monotone legs keep their exact abort strings on every pinned regen |
| `demand_neighborhood_witness` (diff-demand × MONO-input) ×4 + oracle + monotone + eqgate ×4 | **[BYTE]** — FROZEN; input monotone (verified), `input_diff` false; its goldens are the D3.a.1 regression anchor |
| `demand_neighborhood_mono_witness` (mono × mono) ×4 + eqgate | **[BYTE]** — FROZEN |
| data/ corpus (36 × 4) | **[BYTE]** |
| diagnostic verdict lines | **[BYTE]** except `demand_diff_input_1` MOVES diagnostic→golden/eqgate (R-2); `demand_cyclic_1` / `demand_recursive_content_1` / `demand_multi_adorn_1` / `negate_never_diff_1` keep their exact strings |
| flagship generated header, FLAT arm (`-demand`) | **[STRUCT]** — mono demand `Table<>` beside `DiffTable` edge/nbhd (the e5 shape, frozen-A-verified); `add_edge_2` two-Vec + NetBatch; `nbhd_out` transmit both signs; NO retract entry |
| flagship generated header, NESTED arm | **[STRUCT]** — plus: the a2' removal-drain `for` loop (FindInstance + the R-3 Find/`Table::Present`/!TouchedFlag nest, NO Recycle); `.Present(s)` at ALL THREE rescan call sites; `instance_<id>(allocator_, false)`; the (T,F) drop scan + born arm + V-INST-PARTITION belt (diff pub); NO band-(a0), NO removal_frontier (P-DEATH false) |
| composition (`demand_diff_input_1`) NESTED arm | **[STRUCT]** — the FULL four-band a0→a1→a2→a2' (diff demand + diff input); the a2' bytes REGIME-INVARIANT vs the flagship's (the gate forks on PUB-keyed `diff`, true in both) |
| both new witnesses' `.rel` (unpinned; eyeball + census) | **[STRUCT]** — instantiate `effects:` + one `kVecDrain(<input_tid>, kNetRemoval)` (drains 2→3, input_drains 2 accepted); census KINDS unchanged; flagship `kInstanceDeath=0` (the divergence dump-visible), composition `kInstanceDeath=1`; input-writer census kSeedFold=0 on the input tid (R-5 check) |
| both new witnesses' `.ir` (unpinned) | **[STRUCT]** — the region's vector list gains the a2' member; no new IR token |
| goldens churn | **EXACTLY 6 NEW files** (§6.3), zero existing golden changes, via filtered `--bless` |
| ctest | **6/6 binaries** (+2 TESTs in `rel_validators_test` — A1.8) debug + ASAN |
| E-71 grammar notes | **0** (R-4) |

Line-level absolutes (ids, census numerals, effect counts) re-anchor at the
(d0) whole-slice-first-green baseline (R-8); the deltas above are the binding
arithmetic.

### 6.2 Expected pre-bless red set (EXACT — any deviation = STOP, never bless)

**PHASE A** (code landed, goldens absent): EXACTLY the b4 §4.2 **14-line**
set — per new case {opt,nodf,nocf,none} GOLDEN-MISSING + oracle
GOLDEN-MISSING + monotone MONO-MISSING + ONE EQGATE-GOLDEN-MISSING (run_eqgate
early-returns on an absent golden). Any DR-FAIL on either witness = fence-lift
bug, STOP. Any other red = STOP. THEN the manual flat==nested per-mode `cmp`
ritual (b4 §6 step 2) — all four modes byte-identical to each other AND
flat==nested, oracle/monotone == the [COMPUTED] blocks. **PHASE B**
(post-bless): **SUITE PASS(178)**, eqgate LIVE ×8 all OK (+ the 2 frozen
eqgates ×4), zero residual reds.

### 6.3 Golden churn — EXACT file list (all NEW)

```
goldens/demand_diff_neighborhood_witness.stdout
goldens/demand_diff_neighborhood_witness.oracle.stdout   [COMPUTED]
goldens/demand_diff_neighborhood_witness.monotone.stdout [COMPUTED]
goldens/demand_diff_input_1.stdout
goldens/demand_diff_input_1.oracle.stdout                [COMPUTED]
goldens/demand_diff_input_1.monotone.stdout              [COMPUTED]
```
`git status` after bless shows EXACTLY these 6 additions + the source edits.
No eqgate golden is ever blessed.

### 6.4 Gate roll-call

SUITE PASS(178) ×4 modes debug (+ release), error-grep 0 across trees; ASAN
×2 hard gate (full suite + ctest + BOTH new eqgates — the a2' rescan over a
DiffTable's dead-row log while pub retracts is use-after-free terrain); eqgate
LIVE ×8 refereeing answer + sorted `nbhd_out`/`getpt_out` delta identity
(incl. the A2.2 diff-input E-D acceptance line); 20/20 pinned regen [BYTE]
×3; config-invariance 3-run single-hash + release==debug on BOTH new witness
arms (flat + nested, `.h` + `.rel`/`.ir`) + demand_tc_witness; **Q5 MUST RUN**
(progsize@128 release ABABAB; baseline A = tip b4d08307 snapshot; >2% = STOP —
bytes move in Build.cpp/Rel.cpp/Procedure.cpp/Database.cpp); E-62 clean;
permcheck N/A (driver-sorted flushes; no pinned delta order moves); Lb2-6 /
L-b4-A (the first executed e5 emission) is a HARD gate (A2.4).

### 6.5 THE CONSOLIDATED d7 L-TABLE (run at stage (d); prototype worktree
### WIP-COMMITTED FIRST; vehicles = the NESTED arms of the two new witnesses;
### never-minted roles = kProductInput-class; abort texts recorded; ALL
### reverted)

| # | belt / claim | lane | procedure | expected |
|---|---|---|---|---|
| L1 | V-INST-SOLE half-2 (pub-alias, surviving) | b1 | scratch: alias pub as input at the mint | SIGABRT "…aliases its published table" |
| L2 | V-INST-SOLE acyclic belt (ADV-1 re-pointed teeth) | b1 | scratch: point the input at an induction-owned diff table | SIGABRT "…induction-owned (recursive content must stay F-A-fenced)" — the OB8/C-REC belt |
| L3 | V-INST-EFFECT totality split | b1 | scratch: mint the removal leg, revert the totality bump | SIGABRT "…not the §3.3 regime-split totality" |
| L4 | V-INST-EFFECT role admit (input_diff-keyed) | b1 | scratch: force validator-side input_diff false, mint keeps the leg | SIGABRT "…nor a differential input's net-removals rebuild drain" |
| L5 | **A1.4 O-1 closure belt (NEW)** | b1 | scratch: force validator-side `diff` false with input_diff true | SIGABRT "…differential summarized input over a MONOTONE published table" |
| L6 | V-INST-DRAIN input diff arm (both-sign dr_ok) | b1 | LANDED TEST (`InputArmRejectsMissingRemovalProducer`, A1.8) + scratch producer suppression | SIGABRT citing the both-signs string; positive twin clean |
| L7 | V-INST-DRAIN input MONOTONE arm intact | b1 | scratch on the R-MONO witness: skip the eager append | SIGABRT (cf_ok arm) — the split didn't break the monotone leg |
| L8 | V-INST-INPUT-COHERENCE (NEW) | b1 | scratch: skip the a2' Emplace under input_diff | SIGABRT "input_removal_frontier presence (0) != TableIsDifferential(input)=1" |
| L9 | orphan-mint fence, diff-input ± (NEW) | b1 | scratch: suppress the CF kNetRemovals mint (EmitFrontierFilter skip) | SIGABRT "orphan-mint fence: differential input +/- frontier not pre-minted" — records WHY the fence is load-bearing (the silent-orphan under-rebuild) |
| L10 | fence-(iii) lift end-to-end (NEGATIVE) | b1 | LANDED: both witnesses compile nested; `demand_cyclic_1`/`demand_recursive_content_1` still reject | exit 0 / unchanged diagnostics |
| L11 | co-landability chain (ABORT-2) | b1 | scratch: **E1a+E1b ONLY** (E1c/E1d/E1e all withheld — A1.3); compile flagship nested | FIRST abort = V-INST-DRAIN's input arm (the eager append skipped for a diff table); the E1c+E1d-withheld variant aborts earlier at V-INST-EFFECT (recorded, not the expectation) |
| L12 | E2b mold Present conjunct, a2' source | b2/b4 | scratch: drop the `if (input_diff)` conjunct clause; run flagship E-D/E-E | p1b/p1c re-materialize retracted rows: HP-5 abort + eqgate + golden (Lb2-1 == L-b4-C, merged) |
| L13 | E2b Present conjunct, a1 BIRTH source | b2/b4 | scratch: drop the conjunct from the a1 call site only; run flagship m3a/m3b/p5 | p5 yields {12} not {}: HP-5 abort + eqgate (**fires only under the A4.1 cross-batch repair** — L-b4-D live) |
| L14 | E2c a2' arm existence (OB1) | b2/b4 | scratch: comment out the `if (input_removal)` block; run E-D | stuck-present: eqgate GOLDEN-DIVERGE + HP-5 (Lb2-2 == L-b4-B negative; runs against the REAL b1 producer — A2.1) |
| L15 | **THE FORBIDDEN ungated-late-Recycle (R-A2-TRIGGER §7(3) negative)** | b2/b4 | scratch: INSERT an unconditional `RecycleCurrent(iid)` in the a2' arm AFTER the rescan, outside `!TouchedFlag`; run flagship E-E (m1/p1c) | SILENT full-retract of key 1's frozen set; **V-INST-PARTITION does NOT abort** (0+0==0, frz balances) and **V-INST-FRESH never fires**; ONLY eqgate + HP-5 catch it — demonstrating why the fence is a named DESIGN FENCE with no landed belt (Lb2-3 == L-b4-E, merged) |
| L16 | gate-set identity a2' vs a2 | b2 | scratch: drop `Present(dq)` from the a2' gate ONLY; run composition E-F3 | dead-key edge-removal re-materializes on the removal path: `retract_edges_silent` abort + eqgate — the two-arm divergence the ruling names a design ERROR |
| L17 | classify-arm soundness (X4) | b3/b1 | scratch: remove the E1f-5 read-classify arm; run flagship E-D | silent mis-threaded/empty frontier ⇒ stuck-present ⇒ eqgate diverges, NO validator abort (L3b — why the arm is soundness-critical) |
| L18 | V-INST-PARTITION under a death-free shrink (e5) | b3 | scratch: drop `++dropped;` in the drop scan; run flagship E-D | generated V-INST-PARTITION abort on the first e5 retract epoch — the belt is P-STORE-armed, live with n_death==0 (L3a) |
| L19 | HP-7 disarm sanity (e5 negative) | b3 | LANDED: debug flagship run (d1/m1/m3a/m3b) | no monotone Seal-belt abort; DebugValidate green — the N-1 close's standing liveness (L3e) |
| L20 | R-3 dead-key gate-close, a2' arm (diff demand) | b4 | LANDED: composition rd1/d1 (E-F1) + d2 (E-F3), every pass | silent (hard-abort teeth armed); E-F2 rebirth p1b = {20} |
| L21 | e5 divergence end-to-end + census | b4 | LANDED: flagship every pass + (d0) census eyeball | eqgate ×4 OK; census `kSubgraphInstantiate=1 ∧ kInstanceDeath=0`; V-INST-EMITTED balances 2-vs-2 (F3a positive, L3d) |
| L22 | oracle-nets-retractions cross-check | b4 | LANDED: both witnesses' oracle+monotone every pass | NET rows == [COMPUTED] goldens (the demand-independent referee, F-b4-2) |

DOC-only discharges get NO L-row (E3a/E3b/E3c; the L-MONO degeneration —
nothing perturbable; the L-COMBINED documentary fence — no vehicle exists,
its teeth are the coupling block binding future APIs).

---

## §7 ADJUDICATION RECORD (finding → severity → disposition → verified-at)

Every finding re-verified at code/probe by this adjudicator at tip b4d08307.

| finding | sev | disposition | verified at |
|---|---|---|---|
| b4C-1 a1-Present discriminator defeated by NetBatch | **HIGH** | CONFIRMED → FOLDED A4.1/R-6 (cross-batch repair, fix (i)) | Vec.h:176-206 NetBatch per-side dedup + both-bits fold; the critic's empirical m2 run (`b4crit/out_opt.txt`: same-row ± publishes nothing); Table.h dead-row persistence (XC-7) |
| b1C-1 stale "b2 = death wiring" lane map | MED | CONFIRMED-AS-BOOKKEEPING → FOLDED A1.1/R-1 (the "phantom b2" half CORRECTED: the real b2 lane owns E2a-E2c per the §20(AM) recut; b3's mirror-image attributions fixed the same way) | b2-design.md content vs b1 §0/Y6 vs d3a2-substrate §5 vs §20(AM); Procedure.cpp:360-410 (death wiring landed) |
| b2C-1 kNetRemovals provisioning = the one silent net | MED | CONFIRMED → FOLDED A2.1 (named blocking contract + accepted-risk + real-producer Lb2-2) | Procedure.cpp:325-329 unfenced mint-on-miss vs :314-321 fenced demand idiom; substrate §3 C10/H-8 |
| b2C-2 diff-input E-D doubling is a NEW unrefereed flat path | MED | CONFIRMED → FOLDED A2.2 (named b4 acceptance line: flagship d1 sorted-delta) | O-4 flat retraction path vs the D3.a.1 eqgate's diff-demand-only coverage; b4 E4e d1 phase |
| b2C-3 derived-acyclic diff input unwitnessed | MED | CONFIRMED → **RESOLVED BY PROBE** A2.3/R-5 (the class is EMPTY at tip: CMP body rejected upstream by plain-`-demand`; MERGE input mints no instance; admitted inputs ingest-written) + (d0) writer census + named widening obligation | adjprobe/cmpin.log ("Unsupported rule-body shape under -demand"); adjprobe/mrgin.rel (kSubgraphInstantiate=0); mono_nested.rel (op.4 kIngestFold writes the input tid; kSeedFold=0); Rel.cpp:1170-1173 ready_after lift |
| b1C-2 demand_diff_input_1 flip forced; stub driver exists | MED→LOW | CONFIRMED → FOLDED A1.2 + R-2 (mandatory at co-land; b4's repurpose supersedes the compile-only ambiguity) | cases/demand_diff_input_1.{drflags,main.cpp} read (inert stub + comment); runall.sh:361 |
| b1C-3 L-b1-10 abort-point ambiguity | LOW | CONFIRMED → FOLDED A1.3 (pinned E1a+E1b only; the earlier-abort variant recorded) | Rel.cpp:4324-4331 unpatched totality would fire first on drains==3 |
| b1C-4 `input_diff ∧ ¬diff` un-belted; "handled" over-claim | LOW | CONFIRMED → FOLDED A1.4/R-3 (the O-1 closure belt, adjudicator extension) | Rel.cpp:4271 `diff` in scope; O-1 empirical (flat header: pt/ans/getpt DiffTable) |
| b2C-4 e5 gate path zero runtime coverage | LOW | CONFIRMED → FOLDED A2.4 (re-graded "safe once Lb2-6/L-b4-A green"; hard gate) | Table.h:261-266/:418-424 compile-safety re-verified; no landed diff-pub × mono-demand program exists (fence iii) |
| b2C-5 F2a "provably" leans on the admitted shape | LOW | CONFIRMED → FOLDED A2.5 (grounded + now belt-checked via A1.4) | same as b1C-4 |
| b3C-1 audit missing 3 vacuous sites | LOW | CONFIRMED → FOLDED A3.1 (rows A11/A12 + intro note) | lib/ControlFlow/Format.cpp:663 (grep-verified); Rel.cpp:4410-4415; Rel.cpp:813-828 |
| b3C-2 coupling mis-describes a0 as a rescan | LOW | CONFIRMED → FOLDED A3.2 (recycle-to-empty respell) | Database.cpp:2435 RecycleCurrent; InstanceStore.h:216-219 Touch+Reset |
| b4C-2 silent-lambda naming (retract-shaped, not add-shaped) | LOW | CONFIRMED → FOLDED A4.2 (BOTH twins — A4.1's m3a additionally needs `send_silent`) | b4 §1 phase skeleton vs amendment #5 |
| b3C-4 bare "Build.cpp" file ambiguity | COSM/LOW | CONFIRMED → FOLDED A1.7/A3.4 (file-qualified) | lib/DataFlow/Build.cpp:1525ff is unrelated negation code at the cited lines |
| b1C-5 a2' model↔emission link eqgate-referred | LOW (note) | CONFIRMED → FOLDED A1.5 | Database.cpp outside V-PRED-XCHECK; substrate §3 no-third-abort class |
| b1C-6 two-hop selector-authority belts | COSM | CONFIRMED → FOLDED A1.6 | E1c/E1d vs E1f-4 spans |
| b1C-7 fence-block extent :1504-1558 | COSM | CONFIRMED → anchor fixed (A1.7) | Build.cpp:1504-1558 read |
| b2C-6 born-arm range / dead `sep` / [BYTE] carve-out | COSM | CONFIRMED → FOLDED A2.6 | Database.cpp:2590-2645 |
| b3C-3 E3b/E3d range labels off-by-one | COSM | CONFIRMED → FOLDED A3.3 (21-26 / 3-9; E3d moot) | InstanceStore.h:19-28 read |
| b3C-5 E3a "[BYTE] on every binary" over-reach | COSM | CONFIRMED → FOLDED A3.5 | -g line-table reasoning; no gate hashes the compiler binary |
| b4C-3 E4k prose + runall header :20-25 scope | COSM | CONFIRMED → FOLDED A4.3 | runall.sh:20-25 read |

Tally: **21 lane-critique findings — 21 CONFIRMED (0 refuted outright; 1
premise corrected within a confirmed finding, b1C-1's "phantom"), all
folded/resolved; 1 adjudicator extension (A1.4, the O-1 closure belt) + 2
adjudicator probes (R-5) + 4 ownership ratifications (R-2/R-7/R-8/R-9);
0 ESCALATED.**

---

## §8 OWNER-ESCALATION

**NONE.** No finding or amendment conflicts with §7 R-A2-TRIGGER, the OQ
rulings, or the d2 predicate discipline:
- **R-A2-TRIGGER held everywhere**: two drains reusing landed roles (E1c/E2c;
  no combined VecRole, no new DROp kind — challenged at code by b1 and b2,
  both confirmed effects suffice); gate-set identity BY COPY (E2c) with the
  d7 identity perturbation (L16); RecycleCurrent stays DEATH-ONLY — no lane
  emits it in any input arm, and the UNGATED-late shape is encoded as the
  FORBIDDEN design fence with its directed catcher (L15, belt-invisible by
  demonstration); band order a0→a1→a2→a2'-appended with landed bytes
  untouched (the E2c clone-not-refactor choice); the Present rider on the ONE
  mold, all three sources, input_diff-gated (E2b).
- **OQ-INPUT / OQ-MODEL / OQ-DEATH-VS-REBUILD honored**: any-sign input
  change for a live-demanded key fires the full rescan and band-(b) publishes
  the net retractions; the store stays a predicate-free monotone set island
  (no new runtime unit needed); input removals never mint death (ADV-2 stated
  in b1, audited in b3 §2, census-witnessed by the flagship's
  kInstanceDeath=0).
- **The d2 anti-fold discipline EXTENDED, not bent**: three separately
  spelled axes; the A1.4 belt checks the one-directional O-1 closure THEOREM
  (input_diff ⇒ P-STORE) without folding any predicate into another — the
  same posture as V-INST-DIFF-COHERENCE (a drift guard, not a unification).
- The L-COMBINED documentary fence is exactly the "fence-or-proof" the ruling
  clause (4) delegated to e4; e4 answers "documentary", with the coupling
  block binding any future multi-message batch API.

RESIDUALS OPENED for the landing record: (i) the R-5 named obligation — any
widening of the plain-`-demand` body-walk or the instance recognition
re-derives OB8(i)'s derived-input branch with a directed witness first;
(ii) the E2c ~24-line gate duplication joins the D3.a.3 [F] dedup sweep
candidate list; (iii) the b1C-4 forward note is CLOSED by A1.4 (no residual).

END d3a2-design-draft.md
