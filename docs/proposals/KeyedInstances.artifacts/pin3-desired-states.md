======================================================================
COMMITTED AT THE PIN-3 PRE-DIFF LANDING (2026-07-23; base tip
27aa36b2). THE ADJUDICATED DESIRED-STATES CONTRACT (stage-(d)):
author lane (hand-predicted golden bytes from tip dumps) vs BLIND
worktree-prototype lane (implemented the ruled (ii) semantics
independently, never read the author doc — the R3 blind-lane
precedent) vs grammar-conformance lane (E-71: no new token, render
total, permcheck/seed-class untouched), adjudicated by an xhigh
comparator: ZERO material divergences — predicted goldens, blind
empirical dumps, and the comparator's own mechanical tip-flip are
THREE-WAY BYTE-IDENTICAL for both new fences; the 46-flip inventory
(21 opt / 25 none over 18 cases) tuple-for-tuple identical across
lanes. One LOW divergence adjudicated (D-1: a rationale mislabel on
the disassemble flip — self-producer shape, not sibling-conflict;
no byte moved). The body below is the comparator's ds-final
verbatim: DS-PIN3-1..12.
======================================================================

# PIN-3 stage-(d) — ADJUDICATED DESIRED STATES (pin3-desired-states.md)

Ready to commit as `KeyedInstances.artifacts/pin3-desired-states.md`.

## Banner

- **Slice.** PIN-3 stage-(e) — the standalone `.df`-surface `class=` refinement:
  render `class=` from the **TABLE's** deletion-capability instead of the current
  per-view `CanReceiveDeletions()`-only split. Cures the visible tip defect where a
  single `%table:N` wears two `class=` values (a producer view labels it `monotone`
  while a table-sibling labels it `differential`). One-file edit
  (`lib/DataFlow/Format.cpp`), two new `.df` fences, one spec clause amended, PIN-3
  discharged. Lands as a **standalone PRE-diff before R4** (Q3).
- **Tip.** `27aa36b2186ad4a6e40b04e9948e2ac351aa42cc` (branch keyed-instances,
  clean — repo UNMODIFIED read-only throughout). Debug compiler
  `/Users/pag/Code/DrLojekyll/build/debug/bin/drlojekyll`.
- **Owner rulings (2026-07-23, binding).**
  - **Q1 = candidate (ii) TABLE-LEVEL** `class=`: a view's `class=` renders its
    TABLE's deletion-capability — `differential` iff SOME live/emitted view sharing
    the `%table:<id>` has `CanReceiveDeletions() || CanProduceDeletions()`;
    `monotone` = table present, no live member deletion-capable; `table-less` = no
    table.
  - **Q2 = TWO new standing fences** blessed in-slice:
    `negate_1.df.opt.golden` + `aggregate_1.df.opt.golden` (a `df opt` line added to
    each case's NEW `.irgold` sidecar).
  - **Q3 = standalone PRE-diff** before R4 (its own small commit).
  - **Q4 =** the table-lookup in the emitter uses a loud `fprintf`+`abort`
    tripwire (the DF-BIJECTION/DF-REF idiom); the fold predicate is written
    `CanReceiveDeletions() || CanProduceDeletions()` (crd ⟹ cpd —
    `Differential.cpp:114-117`, VERIFIED below — so this equals bare cpd; keep the
    explicit self-documenting form with a one-line note).
- **Lanes.** AUTHOR (predicted golden bytes + flip inventory by probe/reasoning at
  tip) vs BLIND (independent prototype in a reset worktree, full-corpus regen +
  diff, empirical ground truth) vs GRAMMAR (E-71 token/render conformance). This
  adjudicator re-executed the tip compiler read-only and mechanically constructed
  both predicted goldens from the tip dumps.

## Divergence record

**Zero material divergences.** The author's predicted golden bytes, the blind
prototype's empirical dumps, and this adjudicator's mechanically-flipped tip dumps
are all THREE byte-identical for both `negate_1` and `aggregate_1`. The flip
inventories agree exactly: **21 opt / 25 none = 46 flips across 18 cases**, every
`(case, mode, view_ref, %table:N)` tuple matching between AUTHOR and BLIND. One
LOW, non-blocking descriptive imprecision was found and adjudicated (D-1 below); it
touches no deliverable byte. Grammar lane: PASS (E-71 satisfied), 5 INFO/LOW
findings folded, GO.

- **D-1 (LOW, descriptive, non-blocking).** AUTHOR §2a annotates the `disassemble`
  opt flip (`negate.24 @ %table:26`) as "mixed (this negate crd=0; sibling negate
  #25 crd=1)". Adjudicated at the tip dump: `negate.25` lives at `%table:17` (a
  DIFFERENT table, already `differential`), so it is NOT a `%table:26` sibling. The
  flip of `negate.24 @ %table:26` is nonetheless CORRECT — justified by
  `negate.24`'s OWN `cpd=1` (a non-@never NEGATE producing into its own crossover
  table; a "LIE"/self-producer shape, not a CONFLICT). The flip TARGET matches BLIND
  exactly; only the parenthetical *why* was imprecise. Resolution: the DS-PIN3-3
  inventory below records the shape as **self-producer (own cpd)** for
  `disassemble`, dropping the wrong sibling attribution. No byte moves.

- **Resolved prior-stage discrepancy.** The stage-(a) `a-churn-probe.md`
  double-counted `ex_`/`st_` (`data/examples/`, `data/self_testing_examples/`)
  duplicates to reach 26/30. AUTHOR corrected to 21/25 over the 173-case corpus;
  BLIND independently measured 21/25 over its 159 green cases with a live prototype.
  Adjudicated answer: **21 opt / 25 none**, empirically confirmed.

---

## DS-PIN3-1 — [BYTE] `tests/OptDiff/goldens/negate_1.df.opt.golden`

27 lines, single trailing `\n`, NO trailing blank line (adjudicator-confirmed: tip
dump is 27 lines, terminates `…stratum=6\n`). VERBATIM adjudicated content (the tip
dump with EXACTLY line 23 flipped `class=monotone` → `class=differential`; AUTHOR
§1a == BLIND §3 proto == this adjudicator's mechanical flip, three-way BYTE MATCH):

```
dataflow

select ^select.0 (A:i32, B:i32)                    ; recv #message feed/2
  ATTRIBUTES class=table-less stratum=0
  => ^tuple.4 (A, B)

select ^select.1 (A:i32)                           ; recv #message unsee/1
  ATTRIBUTES class=table-less stratum=1
  => ^tuple.2 (A)

tuple ^tuple.2 (A:i32)
  ATTRIBUTES table=%table:8 class=monotone stratum=2

tuple ^tuple.3 (A:i32, B:i32)
  ATTRIBUTES table=%table:4 class=differential stratum=5
  => ^insert.6 (A, B)

tuple ^tuple.4 (A:i32, B:i32)
  ATTRIBUTES table=%table:11 class=monotone stratum=3
  => ^negate.5 (B, A)

negate ^negate.5 (A:i32, B:i32)                    ; negates ^tuple.2
  ATTRIBUTES table=%table:4 class=differential stratum=4
  => ^tuple.3 (A, B)

insert ^insert.6 (A:i32, B:i32) into %table:4
  ATTRIBUTES class=differential stratum=6
```

Sole change vs tip: **line 23 (`negate.5 @ %table:4`): `monotone` → `differential`**.
Adjudicated rationale: `%table:4` is deletion-capable — member `tuple.3` (crd=1) and
`insert.6` (crd=1) are already `differential`, and `negate.5` itself has
`cpd=1`/`crd=0` (a non-@never NEGATE producing retractions into its own crossover
table). Under (ii) every member renders the table's class; `negate.5` was the sole
member the per-view crd-only rule mislabeled `monotone`. `stratum=4`, `table=%table:4`,
and the `; negates ^tuple.2` header are untouched. This is a VISIBLE-CONFLICT table.

## DS-PIN3-2 — [BYTE] `tests/OptDiff/goldens/aggregate_1.df.opt.golden`

23 lines, single trailing `\n`, no trailing blank line (adjudicator-confirmed:
terminates `…stratum=5\n`). VERBATIM adjudicated content (tip dump with EXACTLY
line 19 flipped; three-way BYTE MATCH):

```
dataflow

select ^select.0 (c1:i32)
  ATTRIBUTES class=table-less stratum=0

select ^select.1 (A:i32, Y:i32)                    ; recv #message pair/2
  ATTRIBUTES class=table-less stratum=1
  => ^tuple.2 (A, Y)

tuple ^tuple.2 (A:i32, c5:i32, Y:i32)
  ATTRIBUTES table=%table:15 class=monotone stratum=2
  => ^aggregate.4 (A, Y)

tuple ^tuple.3 (A:i32, c8:i32, N:i32)
  ATTRIBUTES table=%table:10 class=differential stratum=4
  => ^insert.5 (A, N)

aggregate ^aggregate.4 (A:i32, c11:i32, N:i32)     ; count_i32
  ATTRIBUTES table=%table:5 class=differential stratum=3
  => ^tuple.3 (A, N)

insert ^insert.5 (A:i32, c8:i32, N:i32) into %table:10
  ATTRIBUTES class=differential stratum=5
```

Sole change vs tip: **line 19 (`aggregate.4 @ %table:5`): `monotone` → `differential`**.
Adjudicated rationale: `%table:5` is a PURE-LIE table — its SOLE member is
`aggregate.4` (crd=0, cpd=1). An AGGREGATE seeds `can_produce_deletions=true`
unconditionally (its group-update band emits del/add pairs into `%table:5`), so the
table is deletion-capable via its own producer. `stratum=3`, `table=%table:5`, and the
`; count_i32` functor tag are untouched. NOTE (adjudicator-confirmed): `insert.5`
(line 23) already renders `differential` at tip — INSERT omits its `table=` print but
still reads `TableId()` for the class branch, and `%table:10` (via `tuple.3`, crd=1)
was already differential — so it does NOT flip. The `omit_table` handling is correct
under (ii).

---

## DS-PIN3-3 — [STRUCT] Full-corpus flip inventory (21 opt / 25 none = 46 / 18 cases)

**Adjudicated ground truth** — BLIND ran a live prototype (`Format.cpp` +43/−9,
OR-fold of `CanReceiveDeletions() || CanProduceDeletions()` per `%table:N`) over 159
green cases in opt+none (317 compiles/binary, 0 failures) and diffed vs pristine
tip; AUTHOR predicted the same set by probe/reasoning. The two agree on EVERY tuple.
Every flip is EXACTLY `class=monotone` (tip) → `class=differential`, the two lines
byte-identical after substitution (BLIND: same line count per file, no other class
value ever changed, table-less lines untouched).

**Zero-bystander (i)≡(ii).** BLIND's empirical set == AUTHOR's `crd=0 cpd=1`
producer set: there is NO innocent `crd=0 cpd=0` monotone member of a
deletion-capable table that flips. So the table-level (ii) fold flips EXACTLY the
per-view (i) producers — (i)≡(ii) holds on this corpus.

**By kind** (both lanes agree): opt = 17 NEGATE + 2 AGGREGATE + 2 KVINDEX = 21;
none = 21 NEGATE + 2 AGGREGATE + 2 KVINDEX = 25; total 38 negate + 4 agg + 4 kv = 46.

### OPT flip set — 21

| case | kind | view_ref | table | shape |
|---|---|---|---|---|
| aggregate_1 | AGGREGATE | aggregate.4 | %table:5 | LIE (sole member) |
| average_weight | KVINDEX | kv_index.5 | %table:12 | LIE |
| compare_4 | NEGATE | negate.18 | %table:4 | CONFLICT |
| compare_4 | NEGATE | negate.19 | %table:7 | CONFLICT |
| config_agg_1 | AGGREGATE | aggregate.7 | %table:4 | LIE |
| disassemble | NEGATE | negate.24 | %table:26 | self-producer (own cpd) [D-1] |
| insert_4 | NEGATE | negate.11 | %table:4 | LIE |
| negate_1 | NEGATE | negate.5 | %table:4 | CONFLICT |
| negate_2 | NEGATE | negate.12 | %table:6 | CONFLICT |
| negate_2 | NEGATE | negate.13 | %table:9 | CONFLICT |
| negate_3 | NEGATE | negate.19 | %table:17 | LIE (single-member) |
| negate_3 | NEGATE | negate.20 | %table:21 | LIE (single-member) |
| negate_3 | NEGATE | negate.21 | %table:13 | LIE (single-member) |
| negate_4 | NEGATE | negate.16 | %table:5 | CONFLICT |
| negate_5 | NEGATE | negate.5 | %table:4 | CONFLICT |
| negate_6 | NEGATE | negate.7 | %table:4 | CONFLICT |
| negate_cobatch_mono | NEGATE | negate.5 | %table:4 | CONFLICT |
| negate_downstream_diff | NEGATE | negate.9 | %table:4 | CONFLICT |
| pairwise_average_weight | KVINDEX | kv_index.5 | %table:5 | CONFLICT |
| product_conds | NEGATE | negate.31 | %table:14 | LIE |
| product_conds | NEGATE | negate.32 | %table:18 | LIE |

### NONE flip set — 25

| case | kind | view_ref | table | shape |
|---|---|---|---|---|
| aggregate_1 | AGGREGATE | aggregate.11 | %table:5 | LIE |
| average_weight | KVINDEX | kv_index.16 | %table:18 | CONFLICT |
| compare_4 | NEGATE | negate.35 | %table:4 | CONFLICT |
| compare_4 | NEGATE | negate.36 | %table:7 | CONFLICT |
| config_agg_1 | AGGREGATE | aggregate.15 | %table:4 | LIE |
| deadflowelimination_3 | NEGATE | negate.11 | %table:4 | CONFLICT (none-only) |
| deadflowelimination_4 | NEGATE | negate.39 | %table:4 | LIE (none-only) |
| disassemble | NEGATE | negate.30 | %table:15 | self-producer/mixed [D-1] |
| insert_4 | NEGATE | negate.21 | %table:4 | LIE |
| negate_1 | NEGATE | negate.15 | %table:4 | CONFLICT |
| negate_2 | NEGATE | negate.33 | %table:6 | CONFLICT |
| negate_2 | NEGATE | negate.34 | %table:9 | CONFLICT |
| negate_3 | NEGATE | negate.39 | %table:13 | LIE (single-member) |
| negate_3 | NEGATE | negate.40 | %table:17 | LIE (single-member) |
| negate_3 | NEGATE | negate.41 | %table:21 | LIE (single-member) |
| negate_4 | NEGATE | negate.44 | %table:5 | CONFLICT |
| negate_4 | NEGATE | negate.45 | %table:9 | LIE (single-member) |
| negate_4 | NEGATE | negate.46 | %table:13 | LIE (single-member) |
| negate_5 | NEGATE | negate.16 | %table:4 | CONFLICT |
| negate_6 | NEGATE | negate.26 | %table:4 | CONFLICT |
| negate_cobatch_mono | NEGATE | negate.15 | %table:4 | CONFLICT |
| negate_downstream_diff | NEGATE | negate.22 | %table:11 | CONFLICT |
| pairwise_average_weight | KVINDEX | kv_index.10 | %table:5 | CONFLICT |
| product_conds | NEGATE | negate.47 | %table:14 | LIE |
| product_conds | NEGATE | negate.48 | %table:18 | LIE |

opt→none delta = +4: `deadflowelimination_3` and `deadflowelimination_4` each gain a
flip that opt's dataflow-opt eliminates; `negate_4` gains two (opt fuses three
negates into one at `%table:5`; none keeps all three at t5/t9/t13).

**Adjudicator spot-verifications at tip** (each view renders `class=monotone` at tip
→ genuine flip candidate): negate_1 opt line 23; aggregate_1 opt line 19; negate_3
opt negate.19@t17 / negate.20@t21 / negate.21@t13; negate_4 none negate.44@t5 /
negate.45@t9 / negate.46@t13; compare_4 opt negate.18@t4 / negate.19@t7;
deadflowelimination_3 none negate.11@t4; deadflowelimination_4 none negate.39@t4;
disassemble opt negate.24@t26 (negate.25@t17 already differential — confirms D-1);
product_conds opt negate.31@t14 / negate.32@t18. All confirmed monotone-at-tip.

**Negative controls that must NOT flip** (byte-identical old↔new): already-
differential (`crd=1 cpd=1`) aggregates/negates and pure `crd=0 cpd=0` monotone
tables. `config_agg_2` opt AGGREGATE @ %table:17 is `crd=1 cpd=1` (already
differential — never in the flip set). BLIND's one non-class diff
(`conflicting_constants` none) is a PRE-EXISTING heap-layout column-order flake
present identically in the TIP binary (F20-family; opt 8/8 stable both binaries;
`ATTRIBUTES`-filtered diff EMPTY; `diff|grep -c class=` = 0) — provably class-
neutral, contributes ZERO flips. The DS-PIN3-12b referee catches any control that
flips.

---

## DS-PIN3-4 — [BYTE] The two PRE-EXISTING `.df` pins — byte-identical

`demand_tc_witness.df.opt.golden` + `symrec_tie_1.df.opt.golden` are producer-free
(adjudicator-confirmed at tip: `grep -c class=differential` = **0** in each). Under
(ii) every `%table` in both OR-folds to `false` → every member renders `monotone`
exactly as today. No flip; the change is a strict no-op there (BLIND: both IDENTICAL
against committed golden with the live prototype).

Verification (post-build, refined compiler `$DR`):
```
$DR tests/OptDiff/cases/demand_tc_witness.dr $(cat tests/OptDiff/cases/demand_tc_witness.drflags) -df-out /tmp/dtw.df && cmp tests/OptDiff/goldens/demand_tc_witness.df.opt.golden /tmp/dtw.df   # identical
$DR tests/OptDiff/cases/symrec_tie_1.dr -df-out /tmp/st1.df && cmp tests/OptDiff/goldens/symrec_tie_1.df.opt.golden /tmp/st1.df   # identical
grep -c 'class=differential' tests/OptDiff/goldens/demand_tc_witness.df.opt.golden   # 0
grep -c 'class=differential' tests/OptDiff/goldens/symrec_tie_1.df.opt.golden        # 0
```

## DS-PIN3-5 — [BYTE] ALL SIX `.deltarel` goldens — byte-identical (different emitter)

`booleans, demand_tc_witness, elim-cond-cycle-simple, map_3, merge_2, symrec_tie_1`
`.deltarel.opt.golden`. The `.deltarel` path is `lib/DeltaRel/Format.cpp` and NEVER
calls `attrs_line`. Its `class=` token (`DeltaRel/Format.cpp:765`,
`DerivClassName(op.seed_class)`) is the UNRELATED `kSeedFold` seed-class vocabulary
`{NonRecursive, Recursive}` — grammar-confirmed a distinct production. Adjudicator-
confirmed at tip: all six goldens contain **0** `class=` tokens; BLIND: all six
IDENTICAL under the prototype.

Verification:
```
for g in booleans demand_tc_witness elim-cond-cycle-simple map_3 merge_2 symrec_tie_1; do
  grep -c 'class=' tests/OptDiff/goldens/$g.deltarel.opt.golden   # each 0
done
```

## DS-PIN3-6 — [BYTE] `.h` / `.ir` / `.cpp` anchor-TU corpus-wide — byte-identical

`attrs_line` is reached only from `operator<<(OutputStream&, QueryDF)` via the
`-df-out` drain (`Main.cpp:129-132`), downstream of every id-minting phase; it mints
no DeltaRel op, touches no `Build` path, mutates no VIEW, touches no `next_id`. The
pre-pass is a read-only local `std::unordered_map<unsigned,bool>`. Covers the 676-row
knob-off A/B, post-baseline-4, nested, and `data/` 144-row gates. BLIND:
`fixpoint_stress_1` `.ir` and `.h` proto-vs-tip IDENTICAL (bonus: demand_tc_witness
`.ir`/`.h` opt goldens also IDENTICAL).

Verification (standing A/B gate — compares exit + `.h` + `.cpp` + `.ir`, never `.df`):
`DR=build/<refined>/bin/drlojekyll tests/OptDiff/runall.sh <workroot>` → `SUITE: PASS`;
A/B knob-off 676 + baseline-4 + nested + `data/` 144 → **0-diverged**.

## DS-PIN3-7 — [BYTE] `lib/DeltaRel/Format.cpp:765` seed-class token — untouched

Adjudicator-confirmed at tip: line 765 is
`<< " class=" << DerivClassName(op.seed_class)`, a separate emitter TU and separate
vocabulary from the `.df` `{table-less,monotone,differential}`. The two slice edits
are BOTH inside `lib/DataFlow/Format.cpp`. Verification: the landing `git diff`
touches `lib/DataFlow/Format.cpp` + docs + goldens/sidecars only; `lib/DeltaRel/`
byte-unchanged.

---

## DS-PIN3-8 — [BYTE] `lib/DataFlow/Format.cpp` — replacement comment (retire the PIN)

Adjudicator-confirmed at tip: the current text at the head of PASS 2 (immediately
above the `attrs_line` lambda) is the KNOWN-REFINEMENT-PIN comment
("class= is the spec-pinned per-view CanReceiveDeletions derivation. KNOWN REFINEMENT
PIN … Refine (producer-side / table-level) before blessing any negate-carrying
dump."). DESIRED post-landing: replaced by a plain descriptive comment of the LANDED
(ii) rule with NO "KNOWN REFINEMENT PIN / refine before blessing" language:

```
  // class= is a TABLE property (the peer of table=): a %table:N renders
  // `differential` iff SOME live/emitted view sharing it is deletion-capable
  // -- CanReceiveDeletions() || CanProduceDeletions() -- else `monotone`;
  // table-less means no backing table. The table_is_differential pre-pass
  // (above) OR-folds this once per table so every member renders one
  // consistent class. A producer view -- a non-@never NEGATE / AGGREGATE /
  // KVINDEX (and, latently, an impure MAP) -- PRODUCES deletions into its own
  // table without RECEIVING them, so it makes its whole table differential;
  // deriving class= from per-view CanReceiveDeletions alone mislabeled such a
  // producer `monotone` next to differential table-siblings (the retired
  // PIN-3). crd => cpd (Differential.cpp:114-117), so `crd || cpd` equals bare
  // CanProduceDeletions(); the explicit disjunction is kept self-documenting.
```

The §3a pre-pass keeps its own doc comment; the fold predicate is written
`v.CanReceiveDeletions() || v.CanProduceDeletions()` (Q4); the class branch uses the
loud `fprintf`+`abort` tripwire on a table-id lookup miss — a `DF-CLASS:` message
matching the DF-BIJECTION/DF-REF/DF-JOIN validator idiom.

## DS-PIN3-9 — [BYTE] `t2-dump-spec.md:149-154` — amended clause (verbatim)

The SOLE normatively-amended clause. DESIRED replacement:

```
  - CLASS SEMANTICS PINNED (tc-writer F5, symrec-critic F-D; PIN-3 refined):
    differential = backing table present AND the TABLE is deletion-capable — i.e.
    SOME live/emitted view sharing this `%table:<id>` has `CanReceiveDeletions()`
    OR `CanProduceDeletions()` (table-level, producer-inclusive); monotone = table
    present, NO live member deletion-capable; table-less = no table. A non-@never
    NEGATE, an AGGREGATE / KVINDEX (over any input), and an impure MAP PRODUCE
    deletions into their own table without RECEIVING them; class= reflects the TABLE,
    so such a producer and all its table-siblings render differential — never the
    per-view CanReceiveDeletions-only split that mislabeled the producer monotone.
    RECURSION DOES NOT IMPLY DIFFERENTIAL — a fully monotone recursive program
    (insert-only tc) is class=monotone throughout.
```

Adjudicated changes vs the draft: "SOME view" → "SOME **live/emitted** view" and
"NO member" → "NO **live** member" (grammar-F3 — the §3a fold ranges over
`for_each_df_view`, which skips dead views); "over a monotone input" → "over any
input" and "+ impure MAP" added to the producer list (correctness-F3). Grammar lane
CONFIRMED this is a DERIVATION amendment behind an UNCHANGED three-token vocabulary
(E-71 satisfied — no fourth spelling).

## DS-PIN3-10 — [STRUCT] Amendment-survey addendum (no-edit carriers)

Three further docs state the `CanReceiveDeletions` derivation DESCRIPTIVELY over
producer-free subjects and need NO amendment (grammar lane verified each subject is
producer-free → 0 `differential` → OUTPUT-accurate post-flip):
`d1-desired-states.md:149`, `d2b-desired-states.md:112` (both cite the superseded
per-view derivation but describe `demand_neighborhood_witness`, a non-flipping
case — flag them so a future reader does not mistake them for the LIVE clause, which
is `t2-dump-spec.md:149-154` alone), and `d1-ground-truth-nbhd.md` (6
`class=monotone` excerpts, producer-free). `ir-dump-formats.md:78-79` (the
`differential/monotone/table-less` vocabulary listing) stays valid unchanged. ONLY
`t2-dump-spec.md:149-154` is normatively amended.

## DS-PIN3-11 — [STRUCT] PIN-3 retirement inventory

`grep -rn PIN-3 docs/` finds **12 sites in two files** (`KeyedInstances.md`,
`KeyedInstances.artifacts/rel-arch-pseudocode.md`) — adjudicator-confirmed count 12.
Desired state of each:

- **Discharge (canonical + code/plan pins) — rewrite to DISCHARGED:**
  - `KeyedInstances.md:431-437` (canonical PIN-3 block) → **DISCHARGED**: "landed as
    a standalone `.df`-surface table-level `class=` refinement, zero pre-existing-
    golden churn, `t2-dump-spec.md:149-154` clause amended, two new `.df` fences
    (`negate_1`/`aggregate_1`) blessed."
  - `lib/DataFlow/Format.cpp` PIN comment → replaced by DS-PIN3-8's descriptive text.
  - `rel-arch-pseudocode.md:504-511` (§5 R4 PIN-3 block) → from
    standing-blocker/owed-before-bless/pre-diff-or-fold wording to "DISCHARGED
    (commit `<sha>`) — table-level `class=` landed standalone, zero golden churn;
    negate-carrying `.df`/`.deltarel` dumps now unblocked to bless." Leave the
    M9/E-108/E-109 model-layer notes intact.
- **Update live near-tip status (reads false the instant PIN-3 discharges):**
  - `KeyedInstances.md:3320` ("NEXT: R4 … PIN-3 class= refinement is the standing
    blocker") → drop the standing-blocker / NEXT wording.
  - `KeyedInstances.md:3416` ("… PIN-3 stands") → past-tense / discharged.
  - `rel-arch-pseudocode.md:523` ("PIN-3 MANIFESTS at the model layer …") →
    past-tense / discharged.
- **Frozen history — DECLARE no-touch** (append-only timestamped ledger entries
  recording "PIN-3 open" at past slices): `KeyedInstances.md:425, 1874, 2048, 2195,
  2658, 2775, 3205`. Do NOT edit.

---

## DS-PIN3-12 — [STRUCT] Gate plan as observable states

### 12a. Stability diff (zero-churn proof)
Regenerate `-df-out` opt for `demand_tc_witness` + `symrec_tie_1` under the refined
compiler; byte-diff vs the committed goldens → **EMPTY** (both producer-free,
DS-PIN3-4). BLIND already witnessed both IDENTICAL under a live prototype.

### 12b. Full-corpus self-checking intended-flip referee (gates-F2)
For all 173 cases (resolve `cases/$name.dr`, then `data/examples/`, then
`data/self_testing_examples/`; honor `.drflags`; `kvindex_1` opt-only; skip the 14
expected-diagnostic cases), dump `-df-out` under TIP and REFINED compilers in
opt+none; `diff`. DESIRED: every changed line is `^< .*class=monotone` /
`^> .*class=differential` at the SAME view, and NO OTHER byte moves. The flip TOTAL
falls out — **21 opt / 25 none** (DS-PIN3-3), reported as a secondary sanity check,
NOT the gate. (Caveat, adjudicated from BLIND: `conflicting_constants` none carries a
pre-existing F20-family column-order flake — filter to `ATTRIBUTES` lines or run the
referee opt-only for that case; the flake is class-neutral.)

### 12c. NEW standing fences (Q2, gates-F1 — REQUIRED in-slice)
Add `df opt` to a NEW `tests/OptDiff/cases/negate_1.irgold` and a NEW
`tests/OptDiff/cases/aggregate_1.irgold` (adjudicator-confirmed: neither exists
today), and commit `goldens/negate_1.df.opt.golden` + `goldens/aggregate_1.df.opt.golden`
= the exact DS-PIN3-1/DS-PIN3-2 bytes. Both cases are all-4-modes-clean golden cases
(negate_1 standard; aggregate_1 a 4-mode golden since R3 stage-C, adjudicator-
confirmed it carries a `.batches` sidecar — orthogonal file: `.irgold` ≠ `.batches`,
`.df.opt.golden` ≠ `.oracle.stdout`/`.monotone.stdout`, no collision) → sidecars
permitted. PIN-compliant: the refine text precedes the bless in ONE commit, so the
golden IS refined output (an ADDITION, not a re-bless).

Pre-bless red analysis: `run_irgold` iterates only the surfaces in each sidecar. With
a lone `df opt` line and the golden not yet committed, the compile emits `df.opt.out`,
the golden is absent → **`IRGOLD-MISSING`** (irc=1), one red per case, NOTHING ELSE
(the refinement moves only `.df` bytes, so every `.stdout`/`.ir`/`.h`/`.cpp`/
`.oracle`/`.monotone`/`.deltarel` golden is byte-stable). After `runall.sh --bless`
writes the two goldens, both go GREEN. A clean checkout of the landing commit sees NO
pre-bless red — the two MISSING reds exist only in the transient add-sidecar-before-
bless window.

### 12d. Suite (all 173) — desired: `SUITE: PASS`
With the two new `df opt` pins GREEN; zero `IRGOLD-DIVERGE` on the two pre-existing
`.df` pins (byte-stable); zero `.df` reds elsewhere. A/B knob-off 676 + baseline-4 +
nested + `data/` 144 → **0-diverged** (compares generated code/behavior, never
`.df`). ASAN clean (one `unordered_map<unsigned,bool>` local; no new allocation
shape).

### 12e. Config-invariance + debug==release + 3-run determinism on a flip carrier
`CanProduceDeletions`/`CanReceiveDeletions` are plain non-NDEBUG `impl`-bool reads
(adjudicator-confirmed: `Differential.cpp:114-117` sets `can_produce_deletions` for
any crd view, no NDEBUG guard — clean, unlike the a3-caught `producer` field). The
(ii) OR-fold keys on `unsigned` table id and is order-independent, so:
- **debug==release single-hash**: build under `debug` AND `release`; hash `negate_1`
  `.df` opt AND none → identical hash across presets.
- **3-run determinism**: run the refined compiler 3× on `negate_1` opt+none →
  byte-identical `.df` each run. BLIND witnessed the proto: negate_1 opt 3-run single
  md5 `435133ecbab64eb624d8efdd9503832e`.

### 12f. Pre-diff-or-fold (Q3)
Lands as a **standalone PRE-diff** (its own small commit before R4). The probe
refutes the churn premise (`rel-arch-pseudocode.md:508-511`): the only `class=`-bearing
blessed `.df` dumps are the two producer-free goldens, byte-stable — no existing-
golden churn, no re-bless cycle.

---

## GO / NO-GO for stage-(e) implementation

**GO.** All three lanes converge with ZERO material divergence; the two golden
contents are three-way byte-identical (author prediction == blind prototype ==
adjudicator's mechanical tip-flip); the 46-flip inventory (21 opt / 25 none, 18
cases) is empirically confirmed by a live prototype and matches the author
prediction tuple-for-tuple; (i)≡(ii) zero-bystander holds; the two pre-existing
`.df` pins and six `.deltarel` goldens are provably no-op; the grammar axis is clear
(E-71: no new token, render total, all three values reachable, permcheck untouched,
seed-class production distinct). The single found divergence (D-1) is a cosmetic
rationale imprecision that moves no byte and is resolved in DS-PIN3-3.

Blockers: NONE. Carry-forward notes into stage-(e): (1) D-1 — record `disassemble`
flips as self-producer shape, not sibling-CONFLICT; (2) the referee (12b) must
tolerate the `conflicting_constants` none F20-family column-order flake (filter to
`ATTRIBUTES` or run opt-only for that case); (3) the two `.irgold` sidecars must be
committed in the SAME commit as their goldens to avoid a transient pre-bless
`IRGOLD-MISSING` red on the branch.
