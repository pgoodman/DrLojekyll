======================================================================
COMMITTED AT THE PIN-3 PRE-DIFF LANDING (2026-07-23, the Rel epoch,
pre-R4; base tip 27aa36b2). THE ADJUDICATED DESIGN CONTRACT for the
PIN-3 class= refinement (KeyedInstances.md:431-437 discharged by this
slice). Ritual record: stage-(a) 2 lanes (subsystem build-out +
worktree churn probe, the M12 fprintf ritual) -> stage-(b) xhigh
designer -> stage-(c) 3 fresh critics (correctness/grammar/gates) ->
xhigh adjudicator (13 findings: 12 CONFIRMED all record-accuracy or
gate-hardening, 1 REFUTED; zero code-shape defects). OWNER RULINGS
(2026-07-23, binding): Q1 = candidate (ii) TABLE-LEVEL class= (a
view's class= renders its TABLE's deletion-capability: differential
iff SOME live member view of the %table:<id> has CanReceiveDeletions
|| CanProduceDeletions); Q2 = TWO new standing fences blessed
in-slice (negate_1.df.opt + aggregate_1.df.opt goldens — the FIRST
producer-carrying .df pins; pin-compliant refine-then-bless in one
commit); Q3 = standalone PRE-diff before R4 (the probe refuted the
rel-arch:508-511 churn premise — the pre-existing .df pins are
producer-free); Q4 = loud fprintf+abort checked lookup (the
DF-REF/DF-JOIN idiom) + the explicit crd||cpd fold spelling (crd
implies cpd, Differential.cpp:114-117). The body below is the xhigh
adjudication verbatim (PART 2 = the amended design, binding; PART 1
= the adjudication ledger; PART 3 = the ruling list as put to the
owner). FABLE-REVIEW RECORD (at the landing, 8-agent workflow, high
effort): 2 verified findings, ZERO live correctness. [1] CONFIRMED —
the discharge edits' §20(N) cross-references were dangling in the
working tree; discharged by the §20(N) ledger entry landing in the
SAME commit. [2] CONFIRMED cleanup — the §2.1/§3a pre-pass
try_emplace-plus-explicit-OR four-liner simplified to
`table_is_differential[*tid] |= crd||cpd` (operator[]
value-initializes the accumulator; the checked-lookup discipline is
the EMISSION-side rule where an insert would mask a live/dead skew —
in the pre-pass, insertion IS the job; this contract pins the
crd||cpd predicate spelling and the emission-side abort, NOT the
map-insertion mechanics, so the landed shape supersedes §2.1's
try_emplace sketch). Fix proven DUMP-NEUTRAL on all 10 pinned
surfaces; post-fix suite PASS (173) + A/B re-run clean.
======================================================================

# PIN-3 stage-(c) ADJUDICATION — every critic finding ruled at the code

Tip `27aa36b2186ad4a6e40b04e9948e2ac351aa42cc` (branch keyed-instances, clean —
`git status` empty). Repo UNMODIFIED. Every disputed fact below re-checked at the
code / with the tip debug compiler `build/debug/bin/drlojekyll`, read-only. The
three critics' full-corpus `.df` dumps under `scratchpad/pin3/allmodes/` were
verified GENUINE (I regenerated `negate_1`, `aggregate_1`, `negate_6` in opt+none
myself and byte-diffed → MATCH), then I re-counted every number from scratch with
my own scripts before ruling.

---

## PART 1 — ADJUDICATION OF EVERY FINDING

### correctness-F1 — the 26/30 flip count — **CONFIRMED (HIGH) → amendment**
The design's pre-registered flip count (§1, §4.2 referee, §5 `[STRUCT]`
prediction) is an **overcount**. Verified independently at the tip:

The flip set = table-bearing, currently-`class=monotone` views whose block kind is
a deletion PRODUCER (non-@never NEGATE ∪ AGGREGATE ∪ KVINDEX). I counted this
directly from the genuine tip dumps (awk pairing each block header keyword with its
`ATTRIBUTES` line):

| mode | negate | aggregate | kv_index | **TOTAL** | design claimed |
|---|---|---|---|---|---|
| opt  | 17 | 2 | 2 | **21** | 26 |
| none | 21 | 2 | 2 | **25** | 30 |

- Robustness of my count: the ONLY `@never` negate block corpus-wide (both modes)
  is `negate_6 ^negate.8`, and it is `class=table-less` (verified: zero
  `; never negates` blocks are table-bearing). So *every* table-bearing monotone
  negate is a non-@never producer that genuinely flips — my "count the monotone
  table-bearing negates" method has no @never false-positive. (The other three
  `@never` grep hits in `.dr` sources are in COMMENTS / an optimized-away
  `map_5` negate — no negate block emitted.)
- Across-modes decomposition: tip = **38 negate + 4 agg + 4 kv = 46** (= 21+25);
  the design's "44 AND-NOT + 8 KVINDEX + 4 AGGREGATE = 56" is wrong on negate
  (44→38) and DOUBLES kvindex (8→4; there are only 2 kv views per mode).
- Visible/invisible mislabel split: I compute **12 visible + 9 invisible (opt)**,
  **15 visible + 10 invisible (none)** — matches the correctness critic exactly and
  REFUTES the design's "14 visible + 18 … + 12/12" line.

Why HIGH: the design elevates 26/30 to the §4.2 acceptance referee ("expect
EXACTLY … Any extra changed byte falsifies the design") and the §5 `[STRUCT]`
prediction. A faithful implementation emits 21/25 and would trip the design's own
gate. **Amendment:** replace 26/30 with 21/25 (and the decomposition) everywhere
the number appears; and — per gates-F2 below — make the count a DERIVED report, not
an asserted magic total.

### correctness-F2 — config_agg_2 negative-control rationale — **CONFIRMED (LOW) → wording fix**
`QueryAggregateImpl` seeds `can_produce_deletions = true` UNCONDITIONALLY
(`Aggregate.cpp:18`; re-asserted `Differential.cpp:71-73`). So `cpd=0` is
impossible for an aggregate. I dumped `config_agg_2` (opt): its aggregate renders
`table=%table:17 class=differential` — i.e. it is a **crd=1 cpd=1
already-differential** control, NEVER the `crd=0 cpd=0` case the design's §4.2
prose states. The CONCLUSION (byte-identical old↔new) is right; the stated REASON
is a semantic misread. **Amendment:** correct §4.2 to say config_agg_2 is a
negative control because its aggregate is *already differential* (crd=1), not
because cpd=0 (which no aggregate can be).

### correctness-F3 — impure MAP omitted from producer enumeration — **CONFIRMED (LOW) → clause note**
`QueryMapImpl` sets `can_produce_deletions = !functor.IsPure()` (`Map.cpp:18`), so
an impure MAP is a **fourth** `crd=0 cpd=1` producer class the design's enumeration
and amended clause omit (they list only NEGATE/AGG/KV). Latent, not live: impure
functors are a clean-diagnostic feature gap (CLAUDE.md) rejected at ControlFlow
build, and the `-df-out` drain runs post-`Program::Build` (`Main.cpp:129-132`), so
no impure map reaches the dump — confirmed by construction (no map view flips in my
count; the 21/25 are exactly the ACTUAL flips on this corpus). **Amendment:** the
formula `CanReceiveDeletions() || CanProduceDeletions()` already flips it correctly;
add impure MAP to the clause's producer list for completeness (or note impure maps
cannot reach the drain). The gates-F2 full-corpus referee catches any impure-map
flip regardless of kind, so this is a spec-completeness note, not a gate hole.

### grammar-F1 — the recommendation leans on a FABRICATED pin quote — **CONFIRMED (MEDIUM) → strike the framing**
PIN-3 verbatim (`KeyedInstances.md:431-437`):
> class= is per-view CanReceiveDeletions; a non-@never NEGATE's own table is
> deletion-capable via its crossover while the negate view does not receive
> deletions, so a negate block labels its own table monotone while table-sharing
> views say differential. **Refine (producer-side / table-level)** before any
> negate-carrying dump is blessed. In-code comment at attrs_line carries it.

The pin contains **no** phrase "must no longer label a deletion-capable table
monotone ANYWHERE" (design lines 104-105, 160 quote it as the pin's literal
requirement). The pin explicitly names **"producer-side"** — candidate **(i)** —
as an acceptable direction, listed FIRST, co-equal with "table-level". Both (i) and
(ii) satisfy the pin *as written*; the in-code comment (`Format.cpp:1160-1161`) and
the task-head restatement say the same ("producer-side / table-level"). **Amendment:**
strike §2 rationale #1's "only PROVABLE cure of the pin's *literal requirement*" and
#2's implied mandate. The (ii) recommendation STILL STANDS on rationale #3 (category
error: `class=` is the peer of `table=`, a table property — the genuinely strongest
argument) and #4 (zero measurable downside + forecloses the latent bystander (i)
leaves open). Re-state honestly: "the pin sanctions BOTH (i) and (ii); (ii) is
preferred on category-error / peer-of-`table=` grounds and additionally forecloses a
future same-table disagreement."

### grammar-F2 — amendment survey under-counts the derivation docs — **CONFIRMED (MEDIUM, partial) → enumerate**
Beyond `t2-dump-spec.md:149-154`, three further artifacts state the
CanReceiveDeletions derivation, verified at tip:
- `d1-desired-states.md:149` — "`TableId()/CanReceiveDeletions()` (Format.cpp:1162-1185)".
- `d2b-desired-states.md:112` — "`TableId()/CanReceiveDeletions()`" (in a
  ControlFlow/Format.cpp DefineTable context — a slightly different layer note).
- `d1-ground-truth-nbhd.md` — 6 literal `class=monotone` `.df` excerpts for
  `demand_neighborhood_witness`.
These are DESCRIPTIVE landed-slice desired-state records, not the pinned contract
clause, so they need no *amendment*. And they do NOT go stale: I confirmed
`demand_neighborhood_witness` is producer-free (grammar critic's negate/agg/kv count
0; it holds no flipping view). **Amendment:** the §2 survey must add a one-line
audit note enumerating these three as "descriptive, producer-free carrier, no edit
needed," rather than assert t2-dump-spec is the sole derivation statement.

### grammar-F3 — amended clause domain imprecision — **CONFIRMED (LOW) → clause wording**
The §3a pre-pass folds over `for_each_df_view`, which SKIPS dead views
(`Format.cpp:750`, `!v.impl->is_dead`), so the aggregate ranges over LIVE/emitted
views only. The amended clause (design 206-207) says "SOME view sharing this
`%table:<id>`" — wider than the code. **Amendment:** say "some **live/emitted** view
sharing this `%table:<id>`". Practically moot (dead views aren't emitted) but the
E-71 bar is exact-and-total; the contract text must not describe a wider domain than
the emitter.

### grammar-F4 — ir-dump-formats citation off by ~2 lines — **REFUTED**
The critic claims the `(differential/monotone/table-less)` token sits on line 81 and
the design's 78-79 cite is short. At tip the ATTRIBUTES-class bullet is at **line 78**
and the vocabulary token `(differential/monotone/table-less)` is on **line 79** —
INSIDE the design's cited `78-79` range. The design's citation is ACCURATE; the
critic miscounted. No amendment.

### grammar-F5 — permcheck.py interaction left implicit — **CONFIRMED (LOW/info) → one sentence**
`tests/OptDiff/permcheck.py` has zero `.df`/`class=`/attrs handling (grep empty) —
it is a stdout published-delta referee only. **Amendment:** add one sentence to §4.4:
"permcheck.py is stdout-only and never parses `.df`; `class=` lives only on the
strict-byte-compare IR-golden surface." Closes the brief's question; no risk.

### gates-F1 — no standing regression fence for the refined producer path — **CONFIRMED (HIGH) → bless a producer .df golden IN THIS SLICE**
Verified the coverage void three ways:
1. The 676-row A/B gate compares exit + `.h` + `.cpp` + `.ir`; `class=` lives ONLY
   in `.df`, never in the A/B surface set → A/B predicts 0-diverged whether the
   refinement is right OR wrong (inert for this slice, not reassurance).
2. Both standing `.df` goldens (`demand_tc_witness`, `symrec_tie_1`) are
   producer-free → they never execute the changed producer branch → SUITE stays
   green regardless of producer-path correctness.
3. R4's recommended golden trio (`rel-arch:526-528`:
   `negate_6 + negate_1 + d5_recursive_negate`) is a **`.deltarel`** trio, and the
   pin block itself (`rel-arch:508`) scopes to "`.deltarel` bless". `.deltarel` is a
   DIFFERENT emitter (`DeltaRel/Format.cpp`, the unrelated seed-class token) that
   never calls `DataFlow/Format.cpp attrs_line`. So R4 does NOT acquire a standing
   golden for the refined `.df` producer `class=` — not this slice, not R4.

Net: the slice discharges a *standing* observability contract with only a one-shot,
uncommitted §4.2 referee. A later `attrs_line` edit or `CanProduceDeletions` drift
silently reintroduces the PIN-3 contradiction with zero coverage. **Amendment
(required):** in THIS slice bless at least one producer `.df opt` golden — add a
`df opt` line to `negate_1.irgold` and commit `goldens/negate_1.df.opt.golden`
generated by the REFINED compiler (recommended: `negate_1`, the visible-`%table:4`
conflict, shows the cure directly; optionally also `aggregate_1` for the agg
producer family). This is PIN-compliant (refine textually precedes bless in one
commit → the golden is refined output), deterministic (the `.df` bijection witness
`Format.cpp:766-796` fixes view numbering), and converts the only proof into a
standing `run_irgold` fence that makes A/B's blindness harmless. *Owner-flag:* this
mints the FIRST negate-carrying `.df` golden — which is exactly what the pin gates
on the refinement landing, so doing it in the refine commit is the natural discharge,
but the owner should confirm the slice's deliverable expands from "zero new golden"
to "one/two new `.df` goldens" (still an ADDITION, not a re-bless).

### gates-F2 — the §4.2 E-77 referee is not executable — **CONFIRMED (MEDIUM) → full-corpus self-checking**
Two executability defects, both verified:
1. Open list: the carrier set ends "… etc. — the 22 cases from a-churn-probe §4"
   with a magic total; a 23rd flipping carrier inside "etc." either mismatches
   (false red) or goes unchecked (false green).
2. Path skew: `conditions_to_bools` exists ONLY at
   `data/examples/conditions_to_bools.dr` (NOT `cases/`); `disassemble` exists in
   BOTH `data/self_testing_examples/` and `cases/` — a literal `cases/$name.dr` loop
   breaks/ambiguates.
**Amendment:** replace §4.2 item 2 with a FULL-CORPUS self-checking referee — for
every one of the 173 cases (resolve `cases/` first, `data/` fallback), dump
`-df-out` under tip and refined compilers in opt+none, `diff`, assert EVERY changed
line is `^< .*class=monotone` / `^> .*class=differential` at the same view and no
other byte moves anywhere. The flip TOTAL then FALLS OUT of the run (report
21/25 as a secondary sanity check, not the gate). This closes the "etc." hole, makes
the negative-control claim self-verifying (gates-F5), and removes the reliance on any
hand-classified crd/cpd tally (correctness-F1).

### gates-F3 — PIN-3 retirement inventory incomplete — **CONFIRMED (MEDIUM) → enumerate all sites**
`grep -rn PIN-3` over `docs/**/*.md` (excluding scratchpad/worktree) finds 12 sites;
§4.6 lists only 3. Adjudicated per-site:
- **Discharge (canonical + code/plan pins):** `KeyedInstances.md:431-437` (canonical
  block), `Format.cpp:1156-1161` (in-code PIN comment), `rel-arch:504-511` (§5 block).
  [§4.6 covers these.]
- **Update live near-tip status (WILL read false the instant PIN-3 discharges):**
  `KeyedInstances.md:3320` ("NEXT: R4 … PIN-3 class= refinement is the standing
  [blocker]"), `KeyedInstances.md:3416` ("… PIN-3 [stands]"), `rel-arch:523`
  ("PIN-3 MANIFESTS at the model layer …" — present-tense, must go past-tense/
  discharged). [§4.6 MISSES all three.]
- **Frozen history — declare no-touch:** `KeyedInstances.md:425, 1874, 2048, 2195,
  2658, 2775, 3205` (append-only timestamped ledger entries recording "PIN-3 open"
  at past slices). [§4.6 silent; rule them frozen.]
**Amendment:** §4.6 enumerates all 12 and rules each (discharge / update-live /
frozen-history). "All sites listed" is a checklist the design did not run.

### gates-F4 — (i)/(ii) forced by whether R4 pins a table-consistency invariant — **CONFIRMED (LOW) → fold into Q1, flag ordering**
Sharpened and correct: (i) and (ii) are byte-identical on every current-corpus
carrier (my zero-bystander check below confirms set-identity), so R4's concrete
goldens are the same under either. (ii)'s sole advantage is the *structural*
guarantee that no future graph renders a same-`%table:N` `class=` disagreement.
If R4's desired-states pin that table-consistency invariant (natural, given PIN-3's
whole framing is a same-table contradiction), (ii) is not merely recommended but
**required** — (i) cannot honor it on an unseen graph. **Amendment:** state the
coupling explicitly in Q1 — you cannot pick (i) now and later ask R4 to pin an
invariant only (ii) supports; settle it before landing. This REINFORCES the (ii)
recommendation.

### gates-F5 — negative-control classification unverifiable without the build — **CONFIRMED (LOW), subsumed by gates-F2**
The curated negative-control set (`d5_recursive_negate, merge_5, negation_flap,
cond_both_polarities, cond_diff_flipflop, config_agg_2`) rests on a hand crd/cpd
classification. The gates-F2 full-corpus self-checking referee removes the reliance:
a mis-classified control that flips is caught automatically. No standalone
amendment beyond adopting gates-F2.

### (independently re-verified, NOT attack surface)
- **(i) ≡ (ii), zero bystanders — CONFIRMED at code.** My own scan across all 173
  cases both modes: **0** tables retain a monotone non-producer member on a
  deletion-capable table after the producer flip. The design's central set-identity
  claim holds.
- **`crd ⟹ cpd` universal** (`Differential.cpp:114-117`) — so
  `CanReceiveDeletions() || CanProduceDeletions()` ≡ `CanProduceDeletions()`. The
  `|| crd` disjunct is dead-but-harmless; formula correct. (Optional: the amended
  design MAY simplify the branch to a single `CanProduceDeletions()` call and note
  the equivalence — a legitimate clarity win, not required.)
- **Layer handling sound** (no E-106/E-107 trap): the pre-pass groups by CF-stamped
  integer `TableId()` at DF render time; a table is deletion-capable iff some DF
  member has the `cpd` bit the CF crossover already set. The QueryView-helper refusal
  (§3c, M12 fence) is correct.
- **Abort-tripwire domain-match real:** emission (`Format.cpp:1241`) and the §3a
  pre-pass both drive the same dead-skipping `for_each_df_view` (`:748-764`) → the
  guaranteed-present-key claim holds; the abort never false-fires.
- **Blessed goldens byte-stable, config-invariance clean, DeltaRel seed-class scope
  guard honored, INSERT `omit_table` handling correct, (F) determinism law clean,
  E-71 not fired by (i)/(ii)** — all re-verified, all PASS.
- **The pre-diff-or-fold premise (rel-arch:508-511) is REFUTED by the probe** — the
  only existing `class=`-bearing `.df` goldens are producer-free/byte-stable, so
  there is no existing-golden churn and no re-bless cycle. Standalone PRE-diff is
  correct (design §4.5 sound).

---

## PART 2 — THE AMENDED DESIGN (confirmed amendments folded in)

Recommendation UNCHANGED: **candidate (ii) TABLE-LEVEL** — but now argued from the
category-error / peer-of-`table=` ground (grammar-F1), the bystander-foreclosure,
and the R4-invariant coupling (gates-F4), NOT from a fabricated "literal
requirement". The pin sanctions both (i) and (ii); (i) remains the acceptable
minimal fallback with byte-identical corpus behavior.

### 2.1 The diff pseudocode (form (ii)) — UNCHANGED from b-design §3
The two-edit diff to `lib/DataFlow/Format.cpp` inside `operator<<(OutputStream&,
QueryDF)` stands exactly as b-design §3a/§3b wrote it (the pre-pass
`table_is_differential` OR-fold over `for_each_df_view` using
`CanReceiveDeletions() || CanProduceDeletions()`; the §3b checked-lookup class
branch with the loud `fprintf`+`abort` tripwire). No code-shape amendment survived
adjudication — every critic finding on the code itself (layer, determinism,
domain-match, config-invariance) cleared. *Optional clarity win:* since
`crd ⟹ cpd`, the fold predicate MAY be written `v.CanProduceDeletions()` alone with
a one-line "(crd⟹cpd, Differential.cpp:114-117)" note; keep the `crd || cpd`
disjunction if the owner prefers the explicit, self-documenting form.

### 2.2 The amended contract clause (`t2-dump-spec.md:149-154`) — grammar-F3 domain fix folded
> CLASS SEMANTICS PINNED (tc-writer F5, symrec-critic F-D; PIN-3 refined):
> differential = backing table present AND the TABLE is deletion-capable — i.e.
> SOME **live/emitted** view sharing this `%table:<id>` has `CanReceiveDeletions()`
> OR `CanProduceDeletions()` (table-level, producer-inclusive); monotone = table
> present, NO live member deletion-capable; table-less = no table. A non-@never
> NEGATE, an AGGREGATE / KVINDEX (over any input), **and an impure MAP** PRODUCE
> deletions into their own table without RECEIVING them; class= reflects the TABLE,
> so such a producer and all its table-siblings render differential — never the
> per-view CanReceiveDeletions-only split that mislabeled the producer monotone.
> RECURSION DOES NOT IMPLY DIFFERENTIAL — a fully monotone recursive program
> (insert-only tc) is class=monotone throughout.

(Changes vs b-design's draft: "SOME view" → "SOME **live/emitted** view" and "NO
member" → "NO live member" [grammar-F3]; added impure MAP to the producer list
[correctness-F3]. If the owner picks (i), collapse to the per-view producer wording
per b-design §2's parenthetical.)

### 2.3 Amendment survey addendum (grammar-F2)
Audit note to add to §2: three further docs state the CanReceiveDeletions
derivation descriptively and need NO edit (all producer-free carriers, verified
non-flipping): `d1-desired-states.md:149`, `d2b-desired-states.md:112`,
`d1-ground-truth-nbhd.md` (6 `class=monotone` excerpts for the producer-free
`demand_neighborhood_witness`). `ir-dump-formats.md:78-79` (vocabulary) stays valid
unchanged. Only `t2-dump-spec.md:149-154` is normatively amended.

### 2.4 The gate / bless plan (amended)
1. **Stability diff (zero-churn proof):** regenerate `-df-out` opt for
   `demand_tc_witness` + `symrec_tie_1` under the refined compiler, byte-diff vs the
   committed goldens → EMPTY. [unchanged, confirmed correct]
2. **Full-corpus self-checking intended-flip referee [gates-F2, REPLACES the open
   "22 carriers + etc." referee]:** for all 173 cases (resolve `cases/` first,
   `data/` fallback), dump `-df-out` under tip and refined compilers in opt+none;
   `diff`; assert every changed line is `class=monotone`→`class=differential` at the
   same view and no other byte moves. The flip TOTAL falls out — expect **21 (opt) /
   25 (none)**; report it as a secondary sanity check, NOT the gate.
3. **NEW standing fence [gates-F1, REQUIRED]:** bless one (recommend two) producer
   `.df opt` golden IN THIS SLICE, generated by the refined compiler — add `df opt`
   to `negate_1.irgold` + commit `goldens/negate_1.df.opt.golden` (visible-`%table:4`
   cure), and optionally `aggregate_1` (agg producer family). Converts the throwaway
   referee into a standing `run_irgold` suite fence; makes A/B's `.df`-blindness
   harmless.
4. **Config-invariance + debug==release single-hash + 3-run determinism** on a flip
   carrier (`negate_1` opt+none) — unchanged, confirmed clean.
5. **Suite:** predict `SUITE: PASS` (all 173); with the new `negate_1`/`aggregate_1`
   `df opt` pins now GREEN under the refined compiler (they are blessed FROM it).
   Zero `IRGOLD-DIVERGE` on the two pre-existing `.df` pins (byte-stable). A/B
   knob-off 676 + baseline-4 + nested + `data/` 144 → **0-diverged** (compares
   generated code, never `.df`). ASAN clean (one `unordered_map<unsigned,bool>`).
6. **pre-diff-or-fold:** standalone PRE-diff (probe refutes the churn premise). Its
   own small commit before R4.

### 2.5 PIN retirement — the FULL inventory (gates-F3)
- Discharge: `KeyedInstances.md:431-437`, `Format.cpp:1156-1161` (replace with a
  plain descriptive comment of the landed table-level/producer-inclusive rule),
  `rel-arch:504-511`.
- Update live near-tip status: `KeyedInstances.md:3320` + `3416` (drop the
  standing-blocker/NEXT wording), `rel-arch:523` (present-tense "MANIFESTS" →
  discharged/past-tense).
- Declare frozen history, no touch: `KeyedInstances.md:425, 1874, 2048, 2195, 2658,
  2775, 3205`.

### 2.6 Amended [BYTE]/[STRUCT] predictions
| surface | prediction | basis |
|---|---|---|
| `.h` / `.cpp` anchor TU / `.ir`, corpus-wide | **[BYTE] identical** | `attrs_line` unreachable from codegen/CF; no op minted, no Build path touched; change is downstream of every id-minting phase |
| `.deltarel` (6 goldens) | **[BYTE] identical** | different emitter; `DeltaRel/Format.cpp:765 class=` is the unrelated seed-class token (scope guard) |
| `.df` `demand_tc_witness.opt` + `symrec_tie_1.opt` (2 pre-existing pins) | **[BYTE] identical** | producer-free; aggregate false everywhere |
| `.df` producer carriers (unpinned today), full corpus | **[STRUCT] change: EXACTLY 21 lines flip `monotone`→`differential` (opt) / 25 (none)**, summed over all 173; opt = 17 neg + 2 agg + 2 kv, none = 21 neg + 2 agg + 2 kv; every other byte identical (no new token, no reorder) | tip-verified count, three ways |
| `.df` `negate_1.opt` (+ `aggregate_1.opt`) NEW goldens | **NEW committed golden, GREEN from refined compiler** | gates-F1 standing fence |
| A/B knob-off (676 + baseline-4 + nested + `data/`) | **0-diverged** | compares generated code/behavior, not `.df` |

---

## PART 3 — MINIMAL OWNER-RULING LIST

- **Q1 (the one real semantics ruling — M12 mental model).** Is `class=` a
  per-VIEW or per-TABLE attribute? → (i) vs (ii). **Recommend (ii) table-level**,
  now on legitimate grounds: `class=` is the peer of `table=` (a table property);
  (ii) forecloses any future same-`%table:N` disagreement; and if R4's
  desired-states pin a table-consistency invariant, (ii) is REQUIRED (you cannot
  pick (i) now and pin that invariant later — gates-F4). The pin sanctions BOTH
  directions; (i) is the acceptable minimal one-line fallback with byte-identical
  corpus output. NOT decided by any "literal requirement" — that framing was
  withdrawn (grammar-F1).

- **Q2 (NEW — required-amendment confirm, gates-F1).** Bless a producer `.df opt`
  golden IN THIS SLICE (recommend `negate_1`, optionally `aggregate_1`) as the
  standing fence? **Recommend YES.** Owner-flag: this mints the FIRST negate-carrying
  `.df` golden — pin-compliant (refine-then-bless in one commit) and the natural
  discharge, but it expands the slice's deliverable from "zero new golden" to one/two
  ADDED `.df` goldens.

- **Q3 (confirm, near-rhetorical).** Pre-diff-or-fold → **standalone PRE-diff**
  (probe refutes the churn premise). Owner confirm it lands as its own small commit
  before R4.

- **Q4 (idiom, trivial).** (ii) checked-lookup: loud `fprintf`+`abort` tripwire
  (recommended — matches this file's DF-BIJECTION/DF-REF/DF-JOIN validators) vs
  `.at()`. Recommend the abort. Sub-option: write the fold predicate as bare
  `CanProduceDeletions()` (crd⟹cpd) vs the explicit `crd || cpd` — cosmetic.

---

## PART 4 — GO / NO-GO

**GO** to stage-(d) desired-states, with the confirmed amendments folded.

The design's APPROACH is sound and was verified at code on every load-bearing axis
(the live `%table:4` contradiction is real; (i)≡(ii) with zero bystanders;
producer-free pinned goldens byte-stable; layer/determinism/config-invariance
clean; the (ii) diff shape needs no change). No finding blocks the (ii) refinement.

All confirmed defects are **record-accuracy and gate-hardening, not code-shape**:
- correctness-F1 (HIGH): number-of-record 26/30 → **21/25** (dissolved anyway by the
  gates-F2 derive-don't-assert referee).
- gates-F1 (HIGH): add a standing `.df` producer golden in-slice — a harness
  ADDITION, fully compatible with the scope guard.
- gates-F2/F3, grammar-F1/F2/F3/F5, correctness-F2/F3 (MED/LOW): referee
  executability, PIN-inventory completeness, honest recommendation framing, clause
  precision, survey completeness.
- grammar-F4: REFUTED (the cite is accurate).

Two items genuinely need the owner before code (Q1 semantics, Q2 the new required
in-slice golden); Q3/Q4 are confirmations. Stage-(d) should encode the amended gate
plan (full-corpus self-checking referee + the new `negate_1`/`aggregate_1` `.df opt`
fence) and the full PIN-retirement inventory as desired states.
