======================================================================
COMMITTED AT THE OD-13 LANDING (2026-07-23, the Rel epoch, pre-R4;
base tip 3e02db13). THE ADJUDICATED DESIGN CONTRACT for the OD-13
model-set observability diff (the .df partition token; rel-arch
pseudocode §6 D1-D4; owner-directed 2026-07-23, §20(N) NEXT). Ritual
record: stage-(a) folded into the §20(O) OD-13-open fleet (the L3
partition lane's empirics: EQ SET values deterministic, DOT
whole-file pointer-unstable, coverage 7/7 complete — E-116) ->
stage-(b) 2 parallel xhigh lanes (diff designer + E-71 grammar
adjudicator) -> stage-(c) 1 adversarial critic -> xhigh adjudicator
(3 HIGH ruled [placement -> after table=; INSERT -> OMIT; churn
referee -> 41], 3 MED verified-and-folded [corrected negate_1
partition values; corrected symrec witnesses; determinism softened
to "as stable as ForEachView order, D5.4 3-run gate is the guard"],
2 LOW + 1 COSM folded; zero code-shape defects). OWNER RULINGS
(2026-07-23, binding): Q-A = `eqset=` (over model=; names the
computed quantity, matches the DOT "EQ SET" cell — one concept, one
name, one value, same order across both DataFlow surfaces); Q-B =
RAW EquivalenceSetId (over dense; literal DOT cross-surface
identity, zero machinery; sparse ids are cosmetic — a dense
renumber, if ever, lands on BOTH surfaces together). ADJUDICATED
(not owner-open): position after table=/before class= (the DOT
table->eqset order precedent); OMIT on INSERT (rides the F3
model-identity-cluster omission; verified recoverable via the
into-%table:N cross-reference); bare integer, no sigil; DOT floor
ZERO-GOLDEN with NOTHING to mint (D1 proven); the DF-EQSET ~0u belt
included. The body below is the xhigh adjudication verbatim (PART A
= the ruling ledger; PART B = the final diff shape, binding; PART C
= the t2-dump-spec normative clause; PART D = predicted churn; PART
E = the gate battery; PART F = the owner questions as put).
STAGE-(d) THREE-WAY-CONVERGENCE + FABLE-REVIEW RECORDS: see
od13-desired-states.md (committed same-slice).
POST-REVIEW OWNER RULING Q-C (2026-07-23, SUPERSEDES PART B's
INSERT-omit and the HIGH-2 adjudication): eqset= RENDERS ON INSERT
(table= stays omitted). The Fable review (11-agent workflow, high)
REFUTED HIGH-2's decisive "verified recoverable" ground on the
pinned demand_tc_witness carrier: insert.19's %table:4 has ZERO
table-stamped rendered siblings (its set renders only as table-less
eqset=10 on tuple.9/join.16), so the omission severed the textual
set<->table link in both directions — the recoverability was
verified only on negate_1, where siblings exist. Under Q-C the
INSERT line renders "ATTRIBUTES eqset=<id> class=... stratum=..."
beside its `into %table:<id>` header, restoring the one-grep link
in every case; churn 41 -> 45 lines (the four INSERT lines gain the
token); re-blessed via a second ritual cycle. The review's other
verified findings: [2] §20(P)/Fable-record cross-refs discharged by
the ledger entry + records landing SAME-COMMIT (the PIN-3 [1]
family); [4] the "same order" cross-surface claim tightened to the
PAIR order (table= before eqset= == TABLE before EQ SET; the full
field sequences differ by surface); [5] the kDFInsert call-site
comment refreshed. Zero live code-correctness findings.
======================================================================

# OD-13 — THE .df TABLE-SHARING PARTITION TOKEN — ADJUDICATED DESIGN

XHIGH adjudicator pass. Tip 3e02db13, branch keyed-instances. Dump-only (D4),
zero emission change. Every critic finding ruled at the code + the four pinned
`.df.opt.golden`; the two lanes' contradictions reconciled into ONE final diff.

## GROUND TRUTH RE-VERIFIED THIS SESSION (adjudicator, live)

- negate_1 per-view partition, correlated by STRATUM against `-dot-out`:
  **TABLE 11 → EQ SET 1** (tuple.4, stratum 3); **TABLE 8 → EQ SET 2**
  (tuple.2, stratum 2); **TABLE 4 → EQ SET 4** (×3: tuple.3/s5, negate.5/s4,
  insert.6/s6); **select.0/s0 (table-less) → EQ SET 1** (shares tuple.4's
  model); **select.1/s1 (table-less) → EQ SET 2** (shares tuple.2's model).
  md5-stable across runs.
- ATTRIBUTES-line counts (grep, verified): demand_tc_witness **20**,
  symrec_tie_1 **12**, negate_1 **7**, aggregate_1 **6** → **45** total.
  **Exactly 1 INSERT block per golden** → **4** INSERT ATTRIBUTES lines total
  (negate_1:27, aggregate_1:23, symrec_tie_1:60, demand_tc_witness's insert).
- `attrs_line` (Format.cpp:1178-1212) emits, in order: `[table=%table:N]`
  (iff `table_id && !omit_table`) → `class=` (always) → `[stratum=]` →
  `[set= depth=]`. INSERT call site (:1400) passes `omit_table=true`; all 8
  others `false`. `do_table` EQ SET (:65) is unconditional. `with_comment`
  (:831) is applied ONLY at :1248 (edge) / :1263 (header) — NEVER to
  attrs_line; 0 ATTRIBUTES lines carry `;` in any golden.
- symrec_tie_1.df.opt.golden is **60 lines**; its induction members are
  lines 43/51/55 at **stratum=4**; line 60 is its INSERT (insert.11,
  `class=monotone stratum=6`).

---

## PART A — RULING ON EVERY CRITIC FINDING

### HIGH-1 (placement contradiction) — VERIFIED; RULED for grammar.md §1

The lanes hard-fork on table-backed lines: design.md D2.1/D7 puts `eqset=`
**before** `table=`; grammar.md §1 puts it **after** `table=`, before `class=`.
Both produce different golden bytes on every table-backed non-INSERT line.

**ADJUDICATED: `eqset=` renders AFTER `table=`, before `class=`** (grammar.md
§1's "model-identity cluster"). Decisive ground — **the DOT sibling surface
renders `TABLE <id>` then `EQ SET <id>` in that order** (do_table:60-65,
verified). design.md's own decisive argument for RAW ids (D3) is *cross-surface
identity with DOT*; ordering consistency is the same principle applied to
position. Matching DOT's table→eqset order makes the two DataFlow surfaces
agree in BOTH value and sequence. Secondary: `table=` (optional physical face)
and `eqset=` (always-present logical face) are the two faces of one model layer
(the one-union-site fact, Build.cpp:242), and `class=` is a property DERIVED
from the table (PIN-3), so it reads best immediately after the identity cluster
it derives from. design.md's "lead with the total token" is a legitimate but
weaker preference; it is FOLDED. The D7 code's inserted line moves to AFTER the
`table_id` block (see Part B). *This is adjudicated, not owner-open — the DOT
precedent is a principled tiebreaker; the owner may override as pure legibility.*

Table-less lines are UNAFFECTED (no `table=` to sit behind) — both lanes already
agree there byte-for-byte.

### HIGH-2 (INSERT render domain) — VERIFIED; RULED for grammar.md §3 (OMIT)

design.md renders `eqset=` on INSERT ("newly informative"); grammar.md §3 omits
it (rides the `table=` omission).

**ADJUDICATED: OMIT `eqset=` on INSERT.** design.md's "newly informative" is
REFUTED at the code: I verified insert.6 → **EQ SET 4**, identical to its
`%table:4` siblings tuple.3/negate.5 (which render `eqset=4`). An INSERT is a
member view of its target table's DataModel, so its EquivalenceSetId is
DEFINITIONALLY the partition of `into %table:N` — recoverable by the *exact*
cross-reference that already justifies dropping `table=` from INSERT (F3). "eqset
≠ table id" is true but irrelevant: recovery is via the table-id cross-reference,
not by reading the number off the INSERT line. design.md's sole real point
(line-locality) is a legibility preference, not new information. OMIT wins on
(i) verified recoverability, (ii) grammar parallelism — `eqset=` is part of the
model-identity cluster that `omit_table` already suppresses; splitting the
omission (drop `table=`, keep `eqset=`) is arbitrary, (iii) OD-13's value
proposition is untouched — the partition is already obvious on INSERT via the
header table id, so nothing of the "surface the hidden sharing" goal is lost.
Keeps the crisp **INSERT ATTRIBUTES = `class=` + `stratum=`** shape. p5 (line
always present) is satisfied — the LINE still renders, minus the cluster.

### HIGH-3 (D5 churn referee is lane-dependent) — VERIFIED; RESOLVED by HIGH-2

The referee cannot be pre-registered until INSERT is ruled. With HIGH-2 →
OMIT, the churn is **41** ATTRIBUTES lines (45 − 4 INSERT), the 4 INSERT
ATTRIBUTES lines byte-identical. Exact per-golden counts in Part D.

### MED-1 (design.md states a wrong partition value) — VERIFIED; folded correction

design.md D2.2 says "table:11 view carries EQ SET 2; the table:8 view carries a
different EQ SET." **Wrong.** Verified live: **table:11 → EQ SET 1, table:8 →
EQ SET 2.** The directional point survives (EQ SET 1 ≠ 11, EQ SET 2 ≠ 8), but the
concrete number is swapped. The adjudicated motivating table (Part D) carries the
corrected values. This is the load-bearing E-106/E-107 showcase; the correction
propagates into the t2-dump-spec clause below.

### MED-2 (grammar.md §0 mislabels demand_tc lines as symrec) — VERIFIED; folded

grammar.md §0's "symrec:64 … stratum=5 set=0 depth=1" and "symrec:89 …
table=%table:8 … stratum=5" are verbatim **demand_tc_witness:64 and :89**
(symrec is only 60 lines; its induction members are 43/51/55 at **stratum=4**).
grammar.md §6 correctly cites symrec at stratum=4, so §0 self-contradicts. The
adjudicated witnesses (Part D) use genuine symrec lines (43 `class=table-less
stratum=4 set=0 depth=1`; 55 `table=%table:4 … stratum=4 set=0 depth=1`).

### MED-3 (determinism overreach) — VERIFIED; folded softening

`eqset=` determinism is **CONDITIONAL on ForEachView traversal-order stability**,
not an order-free theorem. min-id-wins fixes the SET LABEL only if the mint order
is stable (Build.cpp:2298-2310). F24 (`conflicting_constants` nodf .df run-to-run
AutoVar-order instability) is the live witness that a `.df` dump CAN be unstable;
`eqset=` sits on that same substrate and is not immune to that failure MODE.
It is **orthogonal for the current four carriers** (they are stable; none is
`conflicting_constants`; the D5.4 3-run gate is the guard). Adjudicated framing:
"`eqset=` is as stable as ForEachView order; the D5.4 3-run byte-identical gate
is the guard, not a proof of universal determinism." The four-carrier bless is
byte-stable — that is what OD-13 pins, no more.

### LOW-1 (sparse-id mechanism mis-explained) — VERIFIED; folded clarification

`{1,2,4}` is not a dead-view gap: mints START at 1 (`next_data_model_id = 1u`),
and id 3 WAS minted (7 views → ids 1..7) then **union-collapsed into a lower-id
set** (min-id root), so no set is min-labeled 3. State the mechanism so a reader
does not fear "eqset=3 is a hidden/dead partition." A rendered view's `Find()->id`
CAN be a dead/unrendered view's mint (benign — `eqset` is a SET LABEL, not a view
id; determinism unaffected). `for_each_df_view ⊆ ForEachView`, so every rendered
view has an `equivalence_set` → the `~0u` sentinel is genuinely unreachable at
drain. The optional DF-EQSET tripwire is kept as a belt (Part B).

### LOW-2 (grammar.md §0 negate_1 anchors off-by-one) — VERIFIED; cosmetic

§0 cites "negate_1:11 ATTRIBUTES table=%table:8" but line 11 is the `tuple
^tuple.2` header; the ATTRIBUTES is line **12**. Line 4 IS the table-less
ATTRIBUTES (header at 3). Cite ATTRIBUTES lines uniformly. Cosmetic; folded.

### COSM-1 (divergent spelling menus) — VERIFIED; converged

Both lanes LAND on `eqset=`. The converged owner slate (Q-A below):
**`eqset=` (recommended) | `model=` (runner-up)**; further-out candidates
`share=` / `eq=` / `part=` / `eqclass=` are noted but not recommended.

### AGREEMENTS (verified correct — NOT re-opened)

Spelling recommendation `eqset=`; **bare-integer value, NO `%`-sigil** (peer of
stratum=/set=/depth=; a sigil would falsely imply cross-surface id — verified);
byte-52/p6 NON-interaction (attrs_line never routes through `with_comment`; 0
ATTRIBUTES lines carry `;`); unconditional on non-INSERT (total partition, ~0u
unreachable at Build.cpp:2640); RAW == DOT `EQ SET` cross-surface identity; DOT
floor legitimately zero-golden (node ids embed raw pointers via UniqueId,
whole-file non-reproducible). No pre-pass needed — plain per-view read.

---

## PART B — THE FINAL DIFF SHAPE (adjudicated)

Rendered shapes (spelling `eqset=` per Q-A recommendation; value E = raw
`EquivalenceSetId()` per Q-B recommendation):

    table-backed:      ATTRIBUTES table=%table:8 eqset=E class=monotone stratum=2
    table-less:        ATTRIBUTES eqset=E class=table-less stratum=0
    table-less induct: ATTRIBUTES eqset=E class=table-less stratum=4 set=0 depth=1
    INSERT:            ATTRIBUTES class=differential stratum=6      (UNCHANGED — omit)

- **Token position (HIGH-1):** after `table=`, before `class=`.
- **Render domain (HIGH-2):** every non-INSERT block (table-backed AND
  table-less); OMIT on INSERT.
- **Sigil (agreed):** bare integer, NO `%`-sigil.
- **Id space (Q-B recommendation):** RAW `EquivalenceSetId()`.

### The emitter change (Format.cpp attrs_line, :1178-1212)

Reuse the existing `omit_table` flag to gate the WHOLE model-identity cluster
(both `table=` and `eqset=`) off INSERT — one extra line, no new plumbing:

```cpp
const auto attrs_line = [&](QueryView v, bool omit_table) -> std::string {
  std::string r = "  ATTRIBUTES";
  const auto table_id = v.TableId();
  if (!omit_table) {                                    // model-identity cluster
    if (table_id) {
      r += " table=%table:" + std::to_string(*table_id);
    }
    r += " eqset=" + std::to_string(v.EquivalenceSetId());   // <-- OD-13, after table=
  }
  r += " class=";
  ... // UNCHANGED (class= still keys on table_id, PIN-3 pre-pass untouched)
};
```

Note `class=` still derives from `table_id` regardless of `omit_table`, exactly
as today — the widened gate suppresses only the two cluster tokens. The PIN-3
`table_is_differential` pre-pass and the DF-CLASS abort are UNTOUCHED.

**Optional hardening (recommended cheap belt, LOW-1):** guard the raw read:

```cpp
    const auto es = v.EquivalenceSetId();
    if (es == ~0u) {   // unreachable: BuildEquivalenceSets runs before every dump
      fprintf(stderr, "DF-EQSET: unbuilt equivalence set reached the -df-out drain\n");
      abort();
    }
    r += " eqset=" + std::to_string(es);
```

Net: 1-5 lines in one lambda, dump-only, no pre-pass, no emission change.

### DOT floor (D1) — NOTHING TO MINT

do_table:65 already renders `EQ SET <id>` unconditionally on every view node
(verified: negate_1 7/7). Zero-golden by design (whole-file DOT embeds raw
pointer node ids → not byte-reproducible; partition VALUES are deterministic).
Documentation-only: record that the `.df eqset=` token is the byte-golden'd
counterpart of the already-shipping DOT `EQ SET` cell.

---

## PART C — t2-dump-spec.md NORMATIVE CLAUSE (final text)

Amend the ATTRIBUTES-line clause (currently ~lines 143-148) so the field list
reads `table=` → **`eqset=`** → `class=` → `stratum=` → `set= depth=`, and add
a numbered v3.x pin. Draft:

> - ATTRIBUTES line (only fields that apply), in order: `table=%table:<id>`
>   (TableId — every table-backed non-INSERT view incl. shared-model pairs),
>   **`eqset=<id>`** (EquivalenceSetId — the table-SHARING partition; rendered
>   on every non-INSERT view block, table-backed or table-less; OMITTED on
>   INSERT, riding the same model-identity-cluster omission as `table=`),
>   `class=<differential|monotone|table-less>`, `stratum=<Stratum()>`, and for
>   induction members `set=<merge_set_id> depth=<InductionDepth>`.
>
> - **PARTITION TOKEN SEMANTICS PINNED (OD-13, 2026-07-23):** `eqset=<id>`
>   renders `QueryView::EquivalenceSetId()` RAW as a BARE integer (no `%`-sigil
>   — a peer of `stratum=`/`set=`/`depth=`, NOT a cross-surface ref) — the SAME
>   integer the DOT surface prints as `EQ SET <id>` (cross-surface identity is
>   NORMATIVE: the two DataFlow surfaces name one partition with one value AND
>   in the same order — DOT emits `TABLE` then `EQ SET`, so `.df` emits `table=`
>   then `eqset=`; a dense renumber, if ever adopted, MUST land on both surfaces
>   together). On non-INSERT blocks the token is UNCONDITIONAL — the
>   equivalence-set partition is TOTAL (`BuildEquivalenceSets` runs at
>   Build.cpp:2640 before every dump; the `~0u` sentinel is unreachable at
>   drain). On a table-less view `eqset=` is the first identity token where
>   `table=` would sit — this is how a table-less view of a table-BACKED class
>   (the E-106/E-107 shape: e.g. negate_1 `select.0` table-less shares
>   `eqset=1` with `tuple.4`/`%table:11`) makes its model membership visible in
>   the textual dump, the fact that today needs a worktree `fprintf` + the M12
>   ritual to see. On INSERT the whole model-identity cluster (`table=` AND
>   `eqset=`) is omitted: the INSERT's partition is definitionally that of its
>   `into %table:<id>` target (one-union-site fact) and recoverable from any
>   table-sibling's `eqset=`, so `INSERT ATTRIBUTES = class= + stratum=` stays
>   crisp.
>
> - **DETERMINISM (OD-13):** `eqset=` is as stable as `ForEachView` traversal
>   order — min-id-wins fixes the label given a stable mint order, it does NOT
>   immunize the value against a future view-order regression (the F24 failure
>   mode). The four `.df` carriers are byte-stable; the 3-run byte-identical
>   bless gate is the guard, not a proof of universal determinism.
>
> - **DOT FLOOR (OD-13 D1):** the DOT surface already renders the partition on
>   every view node via the unconditional `EQ SET <id>` cell (Format.cpp:65); no
>   DOT golden is minted (whole-file DOT is not byte-reproducible — node ids
>   embed raw pointers). The `.df eqset=` token is its byte-golden'd counterpart.

---

## PART D — PREDICTED PER-GOLDEN CHURN (exact, under adjudicated OMIT-on-INSERT)

Every NON-INSERT ATTRIBUTES line gains exactly one ` eqset=E` token, inserted
after any `table=`, before `class=`, in place (no line-count change). The 4
INSERT ATTRIBUTES lines are BYTE-IDENTICAL.

| golden | ATTRIBUTES | INSERT (unchanged) | lines changed |
|---|---|---|---|
| demand_tc_witness | 20 | 1 | **19** |
| symrec_tie_1 | 12 | 1 (line 60) | **11** |
| negate_1 | 7 | 1 (line 27) | **6** |
| aggregate_1 | 6 | 1 (line 23) | **5** |
| **TOTAL** | **45** | **4** | **41** |

**41 ATTRIBUTES lines mutate in place → 41 deletions + 41 insertions = 82
unified-diff lines** across the four goldens. No header/edge/body/block-count
line changes; the 4 INSERT ATTRIBUTES lines do not change.

**Adjudicated motivating example — negate_1 (MED-1 corrected):**

| views | eqset (raw) | table= today | what the token adds |
|---|---|---|---|
| tuple.3, negate.5, insert.6 | 4 | %table:4 (insert.6 via header) | confirms the 3-way `%table:4` sharing already visible via `table=`; INSERT omits the token |
| select.0 (table-less), tuple.4 | **1** | select.0 **table-less**, tuple.4 %table:11 | **select.0 today reads `class=table-less` with ZERO hint it shares tuple.4's model** — `eqset=1` reveals it |
| select.1 (table-less), tuple.2 | **2** | select.1 **table-less**, tuple.2 %table:8 | same — the table-less feeder's model membership surfaces via `eqset=2` |

Corrected concrete witnesses (verified live): `%table:11 → eqset=1`,
`%table:8 → eqset=2`, `%table:4 → eqset=4`.

**Adjudicated churn witnesses (MED-1/MED-2 corrected, real golden lines):**

    negate_1:4    ATTRIBUTES eqset=1 class=table-less stratum=0            (select.0)
    negate_1:12   ATTRIBUTES table=%table:8 eqset=2 class=monotone stratum=2   (tuple.2)
    negate_1:19   ATTRIBUTES table=%table:11 eqset=1 class=monotone stratum=3  (tuple.4)
    negate_1:27   ATTRIBUTES class=differential stratum=6                  (insert.6 — UNCHANGED)
    symrec:43     ATTRIBUTES eqset=E class=table-less stratum=4 set=0 depth=1
    symrec:55     ATTRIBUTES table=%table:4 eqset=E class=monotone stratum=4 set=0 depth=1
    symrec:60     ATTRIBUTES class=monotone stratum=6                      (insert.11 — UNCHANGED)
    aggregate_1:11 ATTRIBUTES table=%table:15 eqset=E class=monotone stratum=2
    aggregate_1:23 ATTRIBUTES class=differential stratum=5                 (insert.5 — UNCHANGED)

---

## PART E — THE GATE BATTERY (PIN-3 template)

1. **Pre-registered suite reds = EXACTLY the four .df IRGOLD-DIVERGE.** The only
   .df-referencing goldens are the four `.df.opt.golden` pins (via `.irgold`
   sidecars: demand_tc_witness, symrec_tie_1, negate_1, aggregate_1). All four
   go red pre-bless. A fifth red OR a missing red fails the referee.
2. **A/B corpus is .df-blind → ZERO divergence.** The 173-case suite compares
   generated-C++ stdout; `.df` is emitted only under `-df-out`, never into the
   compiled program. A/B vs frozen debug+release baselines must show zero
   divergence on all non-.df surfaces.
3. **Churn referee (HIGH-3, pre-registerable now HIGH-2 is ruled):** exactly
   **41** ATTRIBUTES lines mutate in place (per Part D: 19/11/6/5); the **4**
   INSERT ATTRIBUTES lines are byte-identical; ZERO header/edge/body/block-count
   changes. Any INSERT-ATTRIBUTES change, any 42nd changed line, or any non-
   ATTRIBUTES touch is a REGRESSION.
4. **Config-invariance:** 3-run byte-identical on each of the four carriers
   (D5.4 — the determinism guard, MED-3); `debug == release` on all four
   (eqset= derives from `EquivalenceSetId()`, config-independent).
5. **Re-bless via the ritual:** after reviewing the run's `.df` outputs,
   `runall.sh --bless <workroot>` filtered to the four carriers. Never auto-bless;
   never to green a red case.

---

## PART F — OPEN OWNER QUESTIONS (minimal; everything else folded)

Everything the critique opened that has a principled tiebreaker is ADJUDICATED
above (placement → after `table=`; INSERT → omit; sigil → bare int; churn → 41;
determinism → softened; DOT → zero-golden; tripwire → include). Two genuine
owner calls remain — both taste/policy the owner alone closes:

**Q-A — SPELLING: `eqset=` vs `model=`.**
Both lanes' JOINT preference is **`eqset=`** (design.md D4, grammar.md §5).
Recommendation: **`eqset=`** — it names the actual computed quantity
(`EquivalenceSetId()`), matches the already-shipping DOT `EQ SET` token (one
concept, one name, one order across both DataFlow surfaces), and avoids importing
CF-layer `DataModel` vocabulary into a DF dump. `model=` is the runner-up (aligns
with CF/M12 "data model" mental model) but asserts a CF identity that has no
printed id of its own — at the DF layer the rendered value IS the DF eqset id, and
`model=` diverges from DOT. Further-out candidates `share=`/`eq=`/`part=`/
`eqclass=` are noted, not recommended (each invents a third name tied to neither
surface). Owner rules; the diff shape is spelling-invariant.

**Q-B — ID SPACE: RAW EquivalenceSetId vs DENSE per-program ordinal.**
Recommendation: **RAW.** The raw id EQUALS the DOT `EQ SET` value — literal
cross-surface identity, zero translation, zero machinery (`attrs_line` already
holds `v`). Already (F)-safe and byte-stable raw. The cost is cosmetic: sparse
ids ({1,2,4} — id 3 minted then union-collapsed, LOW-1, not a dead partition).
DENSE (renumber 0..K-1 in det_seq first-encounter order) fixes only the cosmetic
leak, at the cost of a second id space + a first-encounter pre-pass (breaking
D4/D7 no-pre-pass minimality) AND a DOT divergence — to keep the surfaces
consistent, dense would have to land on BOTH surfaces in one ritual, never
`.df`-only. Owner rules; if dense is ever adopted, it is a both-surfaces change.
