# T2b `-deltarel-out` EMITTER GRAMMAR — the pinned adjudication

Adjudicator: T2b GRAMMAR lane (opus). Repo tip `e6264b54` (verified via
`git rev-parse HEAD`). Subject contract: `docs/proposals/KeyedInstances.artifacts/t2-desired-states/t2-desired-deltarel-average_weight.md`
(§1 = lines 117-579). Spelling authority: `lib/DeltaRel/DeltaRel.{h,cpp}`.
Binding spec: `docs/proposals/KeyedInstances.artifacts/t2-dump-spec.md` §2.0-2.4 + v3.1/v3.2 pins.

GROUND FACT (drives everything): **no `-deltarel-out` emitter exists yet.** The
only `deltarel` occurrences in `lib/DeltaRel/DeltaRel.cpp` are two comment lines
(`:3388`, `:3991`); there is no `SetDeltaRelDumpStream`, no `<<` operator, no
enum-spelling function (`grep -rIln "VecRoleName|DROpKindName|PredName|EffKindName"
lib include` returns nothing). Therefore EVERY spelling the contract renders is
either (a) an enum *identifier* the emitter must materialize a spelling table for,
or (b) an id/token read off a stored field. This grammar pins both, and flags the
tokens with NO stored field LOUDLY.

The p8 principle (no-prose in byte blocks) was pinned for `.df` (spec §1.3 p8);
its **deltarel analogue** is this document's job (spec charge to this lane). The
E-70/E-71 precedent (ledger §10): the four `.df` contracts mixed prose into byte
blocks and diverged from the pins; the fix pinned the derivable-comment set and
re-rendered §1 BEFORE code. T2b must not repeat this with the `;;` / `;` lines.

---

## PART 1 — COMMENT-CLASS ADJUDICATION (every comment line in §1)

The contract §1 (lines 117-579) is a fenced block. I walk every `;;` and `;`
line and classify: **BANNER** (emitter-derivable, stays), **PROSE** (moves to
derivation §2/§3, must be stripped from the byte block). Line numbers are the
contract's.

### 1.A The four section banners — BANNER, but MUST be respelled to derivable text

The spec §2.3 format sketch shows NO banner lines at all — it lists `vec …`,
`join.<idx>/branch.<idx>`, `op.<idx> …`, `rounds:`, `deps:`, `census:` as bare
section leads. The contract added decorative `;; ===… NAME (paren-note) ===`
banners. RULING: a section banner is acceptable as an emitter constant ONLY if
its text is a fixed string with NO derived tail. The current banners carry a
PROSE tail in parentheses that is NOT derivable:

| line | current text | verdict |
|---|---|---|
| 120 | `;; ==== VECS (mint order) ====` | BANNER-OK if fixed; the `(mint order)` tail is a fixed literal, allowed. Pin exact bytes. |
| 178 | `;; ==== BRANCHES (flow.branches vector order) ====` | tail is fixed literal — allowed as a constant. |
| 191 | `;; ==== JOINS (flow.joins vector order) ====` | fixed literal — allowed. |
| 200 | `;; ==== OPS (pinned_order = emission) ====` | fixed literal — allowed. |
| 574 | `;; ==== CENSUS ====` | fixed — allowed. |
| 485 | `;; ==== DEPS ====` | fixed — allowed. |

RECOMMENDATION: the section banners MAY stay as fixed emitter constants, but the
spec §2.3 sketch does not mandate them and renders the sections by their bare
lead keyword (`vec`/`op.`/`deps:`/`census:`). To match the p8 spirit and the
`.df` surface (which has NO decorative banners — `lib/DataFlow/Format.cpp` emits
blocks directly), the MINIMAL choice is **drop the decorative `;; ===` banners
entirely**; the `vec`/`branch.`/`join.`/`op.`/`rounds:`/`deps:`/`census:` leads
are self-identifying. If the owner wants banners, pin them as the SIX fixed
strings above with zero derived content. **Adjudication: DROP them (minimal,
p8-aligned); this is re-render item R-1.**

### 1.B The big explanatory `;;` note blocks — ALL PROSE, strip

These are multi-line `;;` blocks that explain derivation. NONE is emitter
output; every one is derivation-prose that belongs in §2. Strip all:

| lines | content (first words) | verdict |
|---|---|---|
| 121-133 | `;; vecs-vector (mint) order = impl->tables id-ascending …` through the `op.16-19` cross-ref table | PROSE — construction-order explanation. STRIP → already in §2.2. |
| 179-184 | `;; First-class per spec §2.3. A DRBranch is one rule chain …` | PROSE. STRIP → §2. |
| 192-195 | `;; First-class per spec §2.3. ONE DRJoin …` | PROSE. STRIP → §2. |
| 202 | `;; --- lead 0: ingest folds (lowered in ^entry, off the ^flow lattice) ---` | PER-STRATUM GROUP BANNER candidate — see 1.C. |
| 210 | `;; --- stratum 1: the KV index edge_weight (@recompute GROUP_UPDATE sc#2) ---` | GROUP BANNER candidate — see 1.C. |
| 244 | `;; --- stratum 2: TUPLE %table:28 (edge_weight->agg input) ---` | GROUP BANNER candidate. |
| 272-275 | `;; --- stratum 3: sum_i32 … ---` + the 3-line `(op.0 keys on group_update_stratum=3 …)` parenthetical | banner line = candidate; the parenthetical = PROSE, STRIP. |
| 308-310 | `;; --- stratum 5: count_i32 …` + parenthetical | banner + PROSE-parenthetical. |
| 342-344 | `;; --- stratum 7: the JOIN … ---` + 2-line explanation | banner + PROSE. |
| 358-359 | `;; join.0 (the DRJoin, rendered first-class above) lowers here …` | PROSE. STRIP. |
| 377 | `;; --- stratum 8: MAP div_i32 -> %table:17 …` | banner candidate. |
| 405-408 | `;; --- stratum 10: MATERIALIZE … ---` + 3-line parenthetical | banner + PROSE. |
| 436-441 | `;; --- lead 2: commit band … ---` + 5-line T2b.0 explanation | banner + PROSE. STRIP the explanation. |
| 466-467 | `;; STATE_SEALs trail at band 10 …` | PROSE. STRIP. |
| 478-482 | `;; ==== ROUNDS … ====` + `;; ZERO rounds …` explanation | banner + PROSE. |
| 485-500 | the entire deps `;;` preamble incl. the LOUD F-9 note | PROSE. STRIP → §3 F-9. |
| 543-572 | the flag-class `;;` explanation + F-10 note | PROSE. STRIP → §3 F-10. |

### 1.C The per-stratum `;; --- stratum <k> --- ` group banners — RE-SPELL or DROP

The `;; --- stratum N: <english> ---` lines are the ONLY plausible in-block
comment beyond the section leads. They ARE partially derivable: `stratum N` is a
lookup the emitter already renders on every op's `stratum=<k>` field. But the
ENGLISH TAIL (`the KV index edge_weight (@recompute GROUP_UPDATE sc#2)`,
`TUPLE %table:28`, `MAP div_i32 -> %table:17`) is NOT derivable from stored fields
— it is a hand English gloss. Two clean options:

  (a) DROP the per-stratum banners entirely. The op lines already carry
      `stratum=<k>`; the grouping is visible from the field. This is the p8-minimal
      choice and it removes ALL the non-derivable english tails at once.
  (b) KEEP a bare derivable banner `;; --- stratum <k> ---` with NO english tail.
      `<k>` is `op_stratum(op)` (DeltaRel.cpp:3276ff) — already computed. But note
      the stratum value is per-OP not a fixed stratum sequence, and the ordering
      is `pinned_order` (banded), NOT stratum-monotone: op.0 at stratum 3 precedes
      op.16 at stratum 4 which precedes op.6 at stratum 2 in mint-id but the LINE
      order is pinned_order. A `;; --- stratum k ---` banner would need to be
      emitted at each *pinned_order* stratum-transition, which is a mechanical
      "emit banner when op_stratum changes from previous op" rule — derivable.

**Adjudication: OPTION (a) — DROP the per-stratum banners.** Rationale: (1) the
`stratum=<k>` field on every op already carries the grouping; (2) the pinned_order
walk is NOT stratum-monotone (band-major, then stratum within the phase lead),
so a `stratum N` transition banner would fire in a non-monotone sequence
(3→4→2→5→6→7→8→10 with the commit band re-entering — confusing and non-obvious);
(3) it eliminates every non-derivable english tail in one stroke, matching p8.
This is re-render item **R-2** (drop lines 202, 210, 244, 272, 308, 342, 377,
405, 436, 478 banner leads AND their prose parentheticals/tails).

### 1.D Trailing `;` comments ON op / branch / vec lines — ALL PROSE, strip

Every trailing `;` comment on a data line is a hand annotation, NONE emitter-
derivable. The `.df` p8 pin is explicit: a byte-block carries ONLY emitter-
derivable comments (provenance, `; cycle`, `; callers:`); the deltarel surface
has NO analogue of those three derivable comment classes defined, so under the
p8 spirit **the deltarel byte block should carry NO `;` trailing comments at
all.** Rulings per line:

| line | comment | verdict |
|---|---|---|
| 185-189 | branch.0..4 `; edge_weight KV -> agg input TUPLE` etc. | PROSE (english gloss of src/tgt). STRIP. src/tgt already on the line as `%table:N`. |
| 203 | op.52 `; lowered in ^entry:41 (ingest-cursor hole; .ir:75). role=kNetAddition` | PROSE. STRIP. (F-6 concern — the "lowered in ^entry" fact is NOT a stored field; it is derivation prose. The dump walks pinned_order and renders op.52 first; that IS the model order, no annotation needed.) |
| 214-216 | op.4 `; column tokens <var-or-cN>:<type> per spec v3 …` (3 lines) | PROSE. STRIP → §2.3. |
| 224 | op.4 spine `; @recompute rescan is a store-internal reduce, no PlanTree` | SEMI-DERIVABLE: the `spine: —` (em-dash) IS derivable (an empty PlanTree renders `—`); the english REASON is PROSE. STRIP the english; keep bare `spine: —`. See PART 2 spine ruling. |
| 226 | op.24 `; TryClaimDel re-tests C_nr<=0 at dequeue` | PROSE. The `gate=kDelGateCnrNonPositive` field already carries this fact derivably. STRIP the english. (The amendment banner lines 10-12 deliberately KEPT this as "the op.52 idiom" standalone `;` line — that decision is REVERSED here under p8: the gate= field is the derivable carrier, the english is redundant prose.) |
| 279-281 | op.0 `; column tokens <var>:<type> …` (3 lines) | PROSE. STRIP. |
| 289 | op.0 spine `; @invertible fold/unfold is the sum_i32_combine/uncombine reduction` | PROSE. STRIP; keep bare `spine: —`. |
| 314-315 | op.2 `; column tokens …; SAME input space as sum …` | PROSE. STRIP. |
| 323 | op.2 spine `; @invertible count_i32_combine/uncombine` | PROSE. STRIP; keep `spine: —`. |
| 347 | op.12 spine `; join-pivot seed: no fold leaf, no target queue` | PROSE. STRIP; keep `spine: —`. |
| 469 | op.1 `; sealed:=working swap (global:rmw)` | PROSE. STRIP. The `kStateFold(%table:4, sign=0)` effect is the derivable carrier. |

**Adjudication: STRIP EVERY trailing `;` comment from the deltarel byte block.**
The gate=/sign=/spine:—/stratum= fields are the derivable carriers; none of the
english is emitter output. Re-render item **R-3**.

### 1.E The PINNED emitter comment set (the p8 deltarel analogue)

**The deltarel byte block carries ZERO comments.** There is no derivable comment
class defined for this surface (unlike `.df`'s p7 provenance / `; cycle` /
`; callers:`). Every `;;` banner and `;` annotation in the contract §1 is either
a fixed decorative constant (drop, R-1) or hand prose (strip, R-2/R-3). If a
future owner wants section leads, the ONLY admissible form is the six bare fixed
`vec`/`branch.`/`join.`/`op.`/`rounds:`/`deps:`/`census:` keyword leads the spec
§2.3 sketch already shows — not decorative `;; === … ===` rules and not english
group banners. **PINNED COMMENT SET = ∅ (empty).**

---

## PART 2 — LINE-GRAMMAR PINS vs DeltaRel.h (token → code source)

The emitter has NO spelling functions yet (grep-confirmed). For every enum the
dump renders, the emitter MUST supply a spelling table; I pin the exact strings
below as the enum *identifier* form the contract uses (e.g. `DROpKind::kSeedFold`
→ `kSeedFold`). The spelling is the bare identifier WITHOUT the `k` prefix in
some contract renders and WITH it in others — this is an INCONSISTENCY the
emitter must resolve. I rule below.

### 2.0 SPELLING-CASE RULING (a cross-cutting inconsistency in the contract)

The contract renders DROpKind WITH the `k` prefix (`kGroupUpdate`, `kSeedFold`,
`kClaimDrain`, `kFrontierFilter`, `kCommitSweep`, `kStateSeal`, `kIngestFold`)
but renders VecRole spellings WITHOUT the `k` in the `$role.idx` sigil
(`$delete-queue`, `$net-removal` — hyphenated lowercase, NOT `$kDeleteQueue`) and
EffKind WITH the `k` (`kVecDrain`, `kStateFold`, `kCounter`, `kInIReadFrozen`)
and Pred WITHOUT (`NetDeleted`, `NetAdded`, `InI` — no `k`). This is THREE
different casings. RULING for the emitter spelling tables:

  - **DROpKind**: render the enum identifier verbatim WITH `k` (`kGroupUpdate`).
    Source: DeltaRel.h:113-138.
  - **EffKind**: render the enum identifier verbatim WITH `k` (`kVecAppend`).
    Source: DeltaRel.h:73-84.
  - **Pred**: the contract drops the `k` (`NetDeleted` not `kNetDeleted`,
    `InI` not `kInI`). Source enum HAS the `k` (DeltaRel.h:94-105: `kNetDeleted`,
    `kInI`). RULING: pin the spelling table to the k-STRIPPED form the contract
    uses ONLY if consistent with `.df`'s Pred rendering; otherwise render WITH `k`
    for uniformity. **Since `reads:` renders Pred spellings and the `.df` p4 pin
    ties the two surfaces, pin Pred spellings to the k-STRIPPED form**
    (`NetDeleted`/`NetAdded`) — this matches the contract. FLAG: this is a
    per-enum spelling-table decision the implementer must hardcode; there is no
    code source for "strip the k" — it is a rendering choice pinned HERE.
  - **VecRole**: the `$role.idx` sigil uses hyphenated-lowercase
    (`delete-queue`, `add-queue`, `overdelete-set`, `addition-set`, `net-removal`,
    `net-addition`, `join-pivots`) — NOT the enum identifier
    (`kDeleteQueue`/`kNetRemoval`/`kJoinPivots`, DeltaRel.h:52-67). This is a
    HAND TRANSLITERATION with no code source. **PIN the VecRole→sigil map
    explicitly** (§2.1 below); the implementer must hardcode it.
  - **UniqueContract**: `sort-unique-at-drain` / `multiset` — hyphenated-lowercase,
    NOT `kSortUniqueAtDrain`/`kMultiset` (DeltaRel.h:70). Hand transliteration;
    pin explicitly.
  - **ElementShape**: the contract does NOT render the enum name; it renders
    `<ids %table:N>` / `<id-cols>` for `kIds`/`kIdCols` (DeltaRel.h:47). Another
    hand map; pin explicitly. `kValues` never appears in this witness.

**LOUD (spelling): NONE of these five spelling tables exists in code.** Every one
is an implementer hardcode. The contract's casing is INCONSISTENT across the five
(k-prefixed DROpKind/EffKind, k-stripped Pred, hyphenated VecRole/UniqueContract/
ElementShape). This grammar pins each table below; the implementer types them in
verbatim with zero further decisions.

### 2.1 The `vec` line

Contract form (line 134): `vec $<role>.<idx>  <shape> uniq=<contract> def=[…] use=[…]`

| token | rendering | code source | verdict |
|---|---|---|---|
| `vec ` | fixed lead | — | OK |
| `$<role>.<idx>` | `$` + VecRole-sigil + `.` + vec vector index | `DRVec.role` (DeltaRel.h:248); index = position in `DRFlowGraph::vecs` (no id field — DeltaRel.h:245-260, identity IS the index) | OK; VecRole→sigil map pinned below |
| `<shape>` | `<ids %table:N>` if `shape==kIds` (N = `debug_table->id`); `<id-cols>` if `kIdCols` | `DRVec.shape` (h:247) + `DRVec.debug_table` (h:252) `->id` (Program.h:129) | OK. NOTE debug_table is "DEBUG annotation only" per h:251-252 but is NOT NDEBUG-gated (compiles every preset — F-8 CLEAN confirmed). |
| `uniq=<contract>` | `uniq=` + UniqueContract-sigil | `DRVec.uniq` (h:249) | OK; sigil map below |
| `def=[…]` | `def=[op.N, …]` comma-joined | `DRVec.defs` (h:258, `std::vector<unsigned>` op indices) | OK. Empty → `def=[]`. |
| `use=[…]` | `use=[op.N, …]` | `DRVec.uses` (h:259) | OK. Empty → `use=[]`. |

**VecRole → `$sigil` map (PINNED, no code source — implementer hardcode):**
`kParam`→`param`, `kNetRemoval`→`net-removal`, `kNetAddition`→`net-addition`,
`kDeleteQueue`→`delete-queue`, `kAddQueue`→`add-queue`,
`kOverdeleteSet`→`overdelete-set`, `kAdditionSet`→`addition-set`,
`kClaimedDel`→`claimed-del`, `kClaimedAdd`→`claimed-add`,
`kJoinPivots`→`join-pivots`, `kProductInput`→`product-input`,
`kTableScan`→`table-scan`, `kMessageOutput`→`message-output`, `kEmpty`→`empty`.
(Source enum: DeltaRel.h:52-67, 14 members. The witness uses only delete-queue/
add-queue/overdelete-set/addition-set/net-removal/net-addition/join-pivots.)

**UniqueContract → sigil (PINNED):** `kMultiset`→`multiset`,
`kSortUniqueAtDrain`→`sort-unique-at-drain`. (DeltaRel.h:70.)

**ElementShape → `<shape>` (PINNED):** `kIds`→`<ids %table:N>` (N=debug_table->id);
`kIdCols`→`<id-cols>`; `kValues`→`<values>` (unused here). (DeltaRel.h:47.)

FLAG (contract line 176): `vec $join-pivots.42 <id-cols> …` renders `<id-cols>`
with NO `%table:N` — correct, `kIdCols` carries no single debug_table referent
and the join-pivots vec's `debug_table` is ∅ (contract §2.2 line 737 confirms
`debug_table=∅`). The emitter must render `<id-cols>` bare (no table) for kIdCols.
The `def=[op.12,op.13,op.14,op.15]` multi-def is A-4 append-accumulation (h:258).

### 2.2 The `branch.<idx>` line

Contract form (lines 185-189):
`branch.<idx> src=%table:N -> tgt=%table:M  ends_at_join=<bool>`
and for a join-terminal branch: `branch.N src=%table:K -> join.0  ends_at_join=true`

| token | rendering | code source | verdict |
|---|---|---|---|
| `branch.<idx>` | `branch.` + position in `DRFlowGraph::branches` | `DRFlowGraph::branches` (h:661); no id field — index IS identity | OK |
| `src=%table:N` | `src=%table:` + `source->id` | `DRBranch.source` (h:289) `->id` | OK |
| `-> tgt=%table:M` | if NOT ends_at_join: `-> tgt=%table:` + `target->id` | `DRBranch.target` (h:292; null for join) | OK |
| `-> join.0` | if ends_at_join: `-> join.<J>` | `DRBranch.ends_at_join` (h:291) true ⇒ target is null; **but WHICH join index?** | **LOUD — see flag below** |
| `ends_at_join=<bool>` | `ends_at_join=true`/`false` | `DRBranch.ends_at_join` (h:291) | OK |

**LOUD no-source flag B-1: `branch.N -> join.0`.** `DRBranch` (h:287-293) has
`source`, `path`, `ends_at_join`, `target`. When `ends_at_join==true`, `target`
is null and there is **NO field naming which DRJoin the branch terminates at**.
The contract renders `-> join.0` but the branch has no `join_index`. The emitter
would have to MATCH the branch to a join by `path.back()==join.join_view`
(`DRBranch.path` h:290 back() = terminal view; `DRJoin.join_view` h:304). That
match is derivable but NOT a stored cross-ref. RULING: pin the emitter to resolve
the join index by scanning `flow.joins` for the join whose `join_view ==
branch.path.back()`; render `-> join.<that index>`. If ambiguous (should not be
— one join per view), abort. This is an EMITTER-SIDE derivation, not a field.

### 2.3 The `join.<idx>` line + section-walk subline

Contract form (lines 196-198):
```
join.0 view=<X,Sum,Count> pivot_vec=$join-pivots.42 targets=[%table:23]
    section-walk: drain $join-pivots.42; point-test %table:4 via %index:149 [X,_],
                  %table:8 via %index:150 [X,_]; fold ± into %table:23
```

| token | rendering | code source | verdict |
|---|---|---|---|
| `join.<idx>` | `join.` + position in `DRFlowGraph::joins` | h:662; no id field | OK |
| `view=<X,Sum,Count>` | col names of the join view | `DRJoin.join_view` (h:304); cols via `QueryView.Columns()` → `QueryColumn.Variable()->Name()` else `c<Id()>` | **PARTIAL — see flag J-1** |
| `pivot_vec=$<role>.<idx>` | `$join-pivots.` + `DRJoin.pivot_vec` | `DRJoin.pivot_vec` (h:305, index into vecs) | OK |
| `targets=[%table:N,…]` | `targets=[%table:` + each `->id` `]` | `DRJoin.targets` (h:306, `std::vector<TABLE*>`) | OK |
| `section-walk: …` | the ENTIRE subline | **NO stored source** | **LOUD — see flag J-2** |

**FLAG J-1 (`view=<X,Sum,Count>`):** the join-view column NAMES are derivable
(`join_view.Columns()` → `Variable()->Name()` per QueryColumn, Query.h:68; fallback
`c<Id()>` per Query.h:81). But the contract renders bare `X,Sum,Count` with NO
type suffix, whereas the v3 column-token rule (spec §1.3, mirrored for deltarel)
is `<var-or-cN>:<type>`. INCONSISTENCY: the join `view=<…>` drops types while the
GROUP_UPDATE `group@{…}` carries `:i32` types. RULING: pin `view=<…>` to render
BARE names (no type) as a view-signature summary — OR render typed for uniformity.
Recommend BARE names (it is a view label, not a column-token position). Pin it
either way; the implementer needs the ruling. **Item: pin as bare names.**

**FLAG J-2 (LOUD — the `section-walk:` subline has NO stored source):** the
subline renders `drain $join-pivots.42; point-test %table:4 via %index:149 [X,_],
%table:8 via %index:150 [X,_]; fold ± into %table:23`. NONE of this is a stored
DRJoin field. Specifically:
  - `%index:149` / `%index:150` — these are `DataIndex.Id()` values (source:
    `lib/ControlFlow/Format.cpp:72` `os << "%index:" << index.Id()`). **DRJoin
    carries NO index ids** (h:302-309: only `join_view`, `pivot_vec`, `targets`).
    Index ids are a *ControlFlow* concept minted at LOWER time by
    `LowerSectionWalk` (Stratum.cpp:1512-1554) — they DO NOT EXIST in the DR-IR
    at the dump's emit point (Stratum.cpp:2166, past `LinearizeAndValidateDRFlow`
    but the section walk is a *lowering* that runs later). **A `%index:N` token
    is UNRENDERABLE from the DR-IR.** This is the single loudest defect in the
    contract's join section.
  - `[X,_]` (the point-test key projection) — not a DRJoin field.
  - `point-test`/`fold ±` — English describing the section walk, not stored.

**RULING J-2:** the `section-walk:` subline as written is NOT emittable. Two clean
options: (a) DROP the subline entirely — `join.0` renders only
`view=/pivot_vec=/targets=` (all three ARE stored, h:304-306); the section-walk
detail is a LOWERING artifact absent from the DR-IR. (b) render only the
DR-derivable facts (`targets`, `pivot_vec`) with NO index ids / no point-test
detail. **Adjudication: DROP the `section-walk:` subline (option a).** The
`%index:` ids cannot be produced at the emit point; keeping them would force the
emitter to reach into ControlFlow lowering state that does not exist yet. This is
re-render item **R-4**.

### 2.4 The `op.<idx>` HEADER line + attribute fields

Common prefix: `op.<idx> <DROpKind> sign=<glyph> ctx=<Ctx> stratum=<k>` then
kind-specific attrs.

| field | rendering | code source | verdict |
|---|---|---|---|
| `op.<idx>` | `op.` + position in `flow.ops` | `DRFlowGraph::ops` (h:651); no id field — **index IS mint identity** (PART 3) | OK |
| `<DROpKind>` | k-prefixed identifier | `DROp.kind` (h:385); table h:113-138 | OK (table 2.0) |
| `sign=<glyph>` | `+`/`-`/`·` | `op_sign` lambda (DeltaRel.cpp:3405-3419); `·`=signless(0) | flag S-1 |
| `ctx=<Ctx>` | `seed`/`eager`/`fixpoint` | `DROp.ctx` (h:391); Ctx h:87, k-stripped-lc | OK |
| `stratum=<k>` | integer | `op_stratum` lambda (DeltaRel.cpp:3276) — **NOT a field** | flag ST-1 |

**FLAG S-1 (`·` glyph):** `op_sign` returns int (−1/0/+1), not a glyph; no code
maps 0→`·`. PIN `{-1→"-",0→"·",+1→"+"}` (implementer hardcode). `·` is UTF-8
0xC2 0xB7 — **byte-verify the golden/OutputStream is multibyte-clean; else use
`0`.** Pin the choice before bless.

**FLAG ST-1 (`stratum=` is a recomputed lambda):** `op_stratum` (DeltaRel.cpp:3276)
is a LOCAL lambda in `LinearizeAndValidateDRFlow`, not a DROp member. Its inputs
(`branch_stratum` h:680, `join_stratum` h:681, `drain_stratum` h:685,
`group_update_stratum` h:684, `crossover_stratum` h:682, `product_stratum` h:683)
ARE stored on `flow` and survive to the emit point (Stratum.cpp:2166). PIN: hoist
`op_stratum`/`op_band` to named helpers callable from both LinearizeAndValidate
and the emitter. Derivable.

**Ctx→spelling (PINNED):** `kEager`→`eager`, `kSeed`→`seed`, `kFixpoint`→`fixpoint`.
**Sign glyph (PINNED):** `-1`→`-`, `0`→`·`, `+1`→`+`.

#### 2.4.a kGroupUpdate (op.0/2/4)
`sc#<n>`=`statecell_id` (h:536) OK. `prov=`=`provenance` (h:532, k-prefixed
`kOver`/`kKv`) OK. `algebra=`=`algebra` (h:533, k-prefixed) OK.
`agg_table=%table:N`=`agg_table->id` (h:534) OK. `config=`=`num_config_cols`
(h:544) OK. `input=%table:N`=`input_view`(h:535)→model table id — OK but see GU-4.
`group@{col:type,…}`=`group_cols` (h:537) each `<var-or-cN>:<type>` via
`.Variable()->Name()`(Query.h:68) else `c<Id()>`(Query.h:81) + `.Type()`(Query.h:69).
`summary@{…}`=`summary_cols` (h:538). See flags:

- **GU-1 (`agg=<name>`):** `agg_view` (h:531) has no unified `.Name()`; branch on
  provenance — kOver→functor name (`QueryAggregate::Functor().Name()`), kKv→KV
  relation name. Derivable via provenance branch + downcast. Verify accessor path.
- **GU-3 (`summary@{Weight:mutable(new_weight_i32)}`):** KV summary renders a
  `mutable(<functor>)` type; aggregates render plain `:i32`. Source: the value
  column's TypeLoc. **The `.ir`/`.df` render mutable as `mut` (§2.0 shows
  `%table:12 [i32,i32,mut]`), NOT `mutable(new_weight_i32)`. The contract's
  expanded form is PREDICTED and likely WRONG — pin to the actual TypeLoc
  spelling at bless.**
- **GU-4 (`input=%table:36`):** table id derivable; but §2.2 (contract lines
  718-726) flags the monotone t36 net-* vecs are NEVER MINTED — affects the
  effects: vec-refs, not this header token (see 2.4.e effects ruling).

#### 2.4.b kSeedFold (op.6-15)
`src=`=`seed_source->id`(h:434); `tgt=`=`seed_target->id`(h:435, omit if null);
`class=`=`seed_class`(h:436, DerivClass h:29, k-STRIPPED `NonRecursive`);
`join_pivot`=bare token iff `join_pivot==true`(h:437), with `tgt=`/`class=`
OMITTED. All derivable.

#### 2.4.c kClaimDrain / kFrontierFilter (op.16-43)
`form=`=`claim_form`(h:466, ClaimForm h:166→`single-pass`/`in-round` k-stripped);
`gate=`=`claim_gate`(h:472, ClaimGate h:186→**k-PREFIXED** `kDelGateCnrNonPositive`/
`kAddGateTotalPositive`); `deferral=`=`deferral`(h:467, Deferral h:171→`immediate`/
`add-loop-output` k-stripped). INCONSISTENCY: form/deferral k-stripped, gate
k-prefixed — matches contract bytes; flag for owner unification.

**ClaimForm→(PINNED):** `kSinglePass`→`single-pass`, `kInRound`→`in-round`.
**Deferral→(PINNED):** `kImmediate`→`immediate`, `kAddLoopOutput`→`add-loop-output`.
**ClaimGate→(PINNED, k-prefixed):** verbatim identifiers.

#### 2.4.d kCommitSweep / kStateSeal (op.44-51, op.1/3/5)
`band=<n>`=`op_band` lambda (flag BD-1); `flavor=`=`sweep_flavor`(h:468,
SweepFlavor h:176→`differential`/`monotone`); `publish_target=`=`publish_target`
(h:478) — flag BD-2; `sc#<n>`=`statecell_id`(h:536) on seals.

- **BD-1:** `band` is a lambda but is a KIND CONSTANT (sweep→9 via `op_band`
  DeltaRel.cpp:3378; seal→10 via `key_of` DeltaRel.cpp:3450-3453). Render as
  kind-derived constant. PIN: lead-1 phase ops render `stratum=<k>`; commit
  sweeps render `band=9`; seals render `band=10`. Derivable.
- **BD-2:** op.51 (monotone) omits `publish_target=` while op.44-50 render it;
  the bool exists on op.51 too (defaults false). PIN uniform rendering (add
  `publish_target=false` to op.51). **Re-render item R-5.**

**SweepFlavor→(PINNED):** `kDifferential`→`differential`, `kMonotone`→`monotone`.

#### 2.4.e kIngestFold (op.52)
`message=<name>/<arity>`=`ingest_message`(h:515)→`Name()`(Parse.h:98)+
`Arity()`(Parse.h:451) → `add_edge/3` OK. `receive=<recv %table:36>` — flag IG-1.
`spine: kFold(...)` — flag IG-2.

- **IG-1 (`receive=<recv %table:36>`):** the `<recv …>` English wrapper has no
  primitive. `receive=` equals `table=` (both = ingest_table->id). PIN: **DROP
  `receive=`** (redundant with `table=`). **Re-render item R-6.**
- **IG-2 (ingest `spine:`):** INGEST_FOLD carries NO arm/PlanTree
  (`MakeMonotoneIngestFold` DeltaRel.cpp:828-841 pushes only kCounter+kVecAppend,
  no DRArm). The contract's `spine: kFold(%table:36,+,NonRecursive)` is a
  SYNTHETIC leaf duplicating the kCounter effect. PIN: `spine: —` for kIngestFold.
  **Re-render item R-7.**

### 2.5 The op SUBLINES: `reads:` / `effects:` / `spine:` / `args:`

#### 2.5.a `reads:` — p4 governs (Pred spellings ONLY)

Spec v3.1 pin (p4, spec §1.3): `reads:` renders Pred spellings ONLY; a
frozen-InI read is `kInIReadFrozen` (EffKind) and lives under `effects:`, NEVER
on `reads:`. Contract's own AMENDMENT (lines 3-16) already deleted the nine
`reads: InI(%table:N)` lines (kInIReadFrozen mis-rendered as a read) and the two
`reads: —` placeholders. What REMAINS on `reads:`:

- FRONTIER_FILTER `reads: NetDeleted(%table:N)` / `reads: NetAdded(%table:N)`
  (op.18/19/22/23/26/27/30/31/34/35/38/39/42/43). Source: these ARE kFlagRead
  effects with `pred==kNetDeleted`/`kNetAdded` (DeltaRel.h:216-218: `DREffect.pred`
  + `read_table`). Pred spellings k-STRIPPED per 2.0: `NetDeleted`/`NetAdded`
  (from `Pred::kNetDeleted`/`kNetAdded`, h:103-104). **KEPT — correct per p4.**

RULING: `reads:` = the op's kFlagRead effects (EffKind::kFlagRead, h:76) rendered
as `<PredSpelling>(%table:<read_table->id>)`. kInIReadFrozen (h:80) is a DISTINCT
EffKind and renders under `effects:`, never `reads:`. The emitter derives the
`reads:` line by filtering `op.effects` for `kind==kFlagRead` and rendering the
`pred`+`read_table`. If NO kFlagRead effect exists, the `reads:` line is OMITTED
entirely (not `reads: —`). This matches the contract's amended state. Derivable.

**Pred→spelling (PINNED, k-stripped):** `kPresent`→`Present`, `kInI`→`InI`,
`kInNew`→`InNew`, `kSurvivesSoFar`→`SurvivesSoFar`, `kAliveAtClaim`→`AliveAtClaim`,
`kInNewWithFrontier`→`InNewWithFrontier`, `kInNewSansFrontier`→`InNewSansFrontier`,
`kRecursivelySupported`→`RecursivelySupported`, `kNetDeleted`→`NetDeleted`,
`kNetAdded`→`NetAdded` (h:94-105, ten members).

#### 2.5.b `effects:` — the EffKind tagged-union render

Contract form: `effects: {<EffKind>(<live fields>), …}` comma-joined, in
`op.effects` VECTOR ORDER (push order — the contract preserves it, e.g. op.4
band(a) drains/folds then band(b) emit/old/counters). Source: `DROp.effects`
(h:388, `std::vector<DREffect>`). Per-EffKind live-field rendering (the tagged
union h:197-226 — only `kind`-selected fields are meaningful):

| EffKind | render | live fields (h) |
|---|---|---|
| `kVecAppend` | `kVecAppend(%table:N, <VecRole>)` | `value_table`(205)+`vec_role`(206) — VecRole k-PREFIXED here (`kNetAddition`/`kDeleteQueue`) |
| `kVecDrain` | `kVecDrain(%table:N, <VecRole>)` | same |
| `kVecClear` | `kVecClear(%table:N, <VecRole>)` | same (unused in witness) |
| `kCounter` | `kCounter(%table:N, <sign>, <DerivClass>)` | `counter_table`(210)+`sign`(211)+`klass`(212); DerivClass k-STRIPPED |
| `kFlagRead` | (rendered on `reads:`, not effects:) | — |
| `kFlagWrite` | `kFlagWrite(%table:N)` or `kFlagWrite(%table:N, <sign>)` | `write_table`(225); `sign` if meaningful |
| `kInIReadFrozen` | `kInIReadFrozen(%table:N, InI, seed)` | `read_table`(216)+`pred`(217)+`ctx`(218) |
| `kStateFold` | `kStateFold(%table:N, <sign>)` | `counter_table`?/`value_table`? + sign — **see flag EF-1** |
| `kStateEmit` | `kStateEmit(%table:N)` | table field — **see flag EF-1** |
| `kStateOld` | `kStateOld(%table:N)` | table field — **see flag EF-1** |

**CROSS-CUTTING SIGN INCONSISTENCY:** inside `effects:` the sign renders as
`+`/`-` on kCounter/kFlagWrite/kStateFold (e.g. `kCounter(%table:12, -, NonRecursive)`,
`kStateFold(%table:4, -)`, `kFlagWrite(%table:12, -)`) but as `sign=0` on the
STATE_SEAL kStateFold (`kStateFold(%table:4, sign=0)`, line 469). So the effect
sign uses `+`/`-` glyphs (NOT `·`) and a literal `sign=0` for the zero case. PIN:
inside effects, `sign` renders `+`/`-`/`sign=0` (or `0`); NOT `·`. **The `·`
glyph is HEADER-only** (the op `sign=` field); effects use `+`/`-`/`0`.
Inconsistent with the header glyph — pin BOTH explicitly:
  - header `sign=` field: `-`/`·`/`+`
  - effect sign arg: `-`/`+`, and `sign=0` where signless (contract line 469).
**FLAG: unify or pin. I pin as the contract renders (header `·`, effect `sign=0`).**

**FLAG EF-1 (kStateFold/kStateEmit/kStateOld table field):** these three EffKinds
(h:81-83) are LIVE since R3 but the `DREffect` struct (h:197-226) has NO dedicated
statecell field — the state effects reuse `counter_table`/`value_table` for the
agg table. Verify WHICH field BuildGroupUpdateOps sets for the state effects
(DeltaRel.cpp:689-740) — the emitter must render the correct one. Contract renders
`%table:N` (the agg table). Derivable once the backing field is confirmed at bless.
**Note the VecRole spelling in effects: is k-PREFIXED** (`kNetAddition`,
`kDeleteQueue`, `kOverdeleteSet`) — UNLIKE the vec-line `$sigil` which is
hyphenated. So VecRole has TWO renderings: `$net-addition` in the vec sigil,
`kNetAddition` in effect args. PIN both (contract does exactly this).

**FLAG EF-2 (the monotone t36 vec-refs, §2.2 lines 718-726 LOUD):** op.52's
`kVecAppend(%table:36, kNetAddition)` and op.4's `kVecDrain(%table:36, kNetRemoval/
kNetAddition)` reference the t36 net-* vecs that are NEVER MINTED (monotone table,
no per-table vec pack). The effect renders `%table:36, kNetAddition` from the
`value_table`+`vec_role` fields (which ARE set on the DREffect regardless of
whether a DRVec was minted). So the EFFECT render is fine (it reads the effect's
own table/role, not a DRVec). The concern is only whether a `$role.idx` vec
REFERENCE is ever rendered for these — it is NOT (effects render `%table:N, kRole`,
not `$role.idx`). **RESOLVED: effect vec-refs render table+role, not a vec index,
so the never-minted vec is invisible here. No re-render needed; the §2.2 LOUD note
is about `TableVec()` lookups the emitter must NOT do for effects.**

#### 2.5.c `spine:` — the PlanNode chain

Contract renders `spine: —` (em-dash) for ops with no PlanTree (GROUP_UPDATE,
join-pivot seeds) and `spine: kFold(%table:N, <sign>, <class>)` /
`spine: kAccess(div_i32 MAP, bbf) -> kFold(...)` for head-chain seed folds.
Source: `DROp.arms` (h:549) → `DRArm.body` (h:372, `std::unique_ptr<PlanNode>`);
PlanNode chain h:332-359 (PlanKind h:325 `{kAccess,kGate,kFold}`).

| PlanKind | render | source |
|---|---|---|
| `kAccess` | `kAccess(<table/functor>, <binding>)` | `PlanNode.table`(337), `lowering`(344), `bound_cols`(341) — **see flag SP-1** |
| `kGate` | `kGate(...)` | `absent`(343)+pred — (none in witness) |
| `kFold` | `kFold(%table:N, <sign>, <class>)` | `fold_table`(353)+`fold_sign`(354)+`fold_class`(355) |

RULING: `spine:` = the arm's PlanNode spine rendered as a `->`-joined chain of
PlanKind nodes. If `arms` is empty OR the sole arm's `body` is null, render
`spine: —`. PlanKind spelling k-PREFIXED (`kAccess`/`kGate`/`kFold`, h:325).

**FLAG SP-1 (`kAccess(div_i32 MAP, bbf)`):** the `div_i32 MAP` name and the `bbf`
binding pattern on op.8/op.9 spine. `PlanNode.table` (h:337) is a TABLE*, not a
functor — a MAP access names a FUNCTOR (`div_i32`) + its binding pattern (`bbf`).
There is no PlanNode field carrying the functor name or `bbf` pattern (h:332-359:
table/bound_cols/pred/absent/lowering/ctx/pivot_col/fold_*). **The `div_i32 MAP,
bbf` text has NO stored source in PlanNode.** RULING: either (a) render the access
by its `table`+`lowering`+`bound_cols` (derivable) and DROP the `div_i32 MAP, bbf`
functor gloss, or (b) confirm a functor field exists on the MAP access path. Since
h:332-359 shows no functor field, **PIN option (a): render `kAccess(%table:N,
<lowering>)` from stored fields; drop the functor-name/binding-pattern gloss.**
**Re-render item R-8** (op.8/op.9 spine: strip `div_i32 MAP, bbf`, render the
derivable access form). NOTE: op.0/op.2/op.4 already have `spine: —` (the
`@invertible`/`@recompute` english was in the STRIPPED trailing comment, R-3).

#### 2.5.d `args:` — table/vec/view refs

Contract form varies: `args: table=%table:N` (claim/filter/sweep/seal),
`args: branch=<%table:N..%table:M> src=%table:N tgt=%table:M` (seed fold),
`args: agg_table=%table:N input=%table:N statecell=sc#N` (group update),
`args: table=%table:N message=… receive=…` (ingest).

| token | source | verdict |
|---|---|---|
| `table=%table:N` | `table_op_table->id`(h:464) or `agg_table`/`ingest_table` per kind | OK |
| `agg_table=%table:N` | `agg_table->id`(h:534) | OK |
| `input=%table:N` | `input_view`(h:535)→table id | OK |
| `statecell=sc#N` | `statecell_id`(h:536) | OK |
| `src=%table:N tgt=%table:M` | `seed_source`/`seed_target`(h:434-435) | OK |
| `branch=<%table:N..%table:M>` | composite of src/tgt | **see flag AR-1** |
| `pivots=$join-pivots.42` | on join-pivot seeds | **see flag AR-2** |

**FLAG AR-1 (`branch=<%table:12..%table:28>` and `branch=<%table:23..%table:17 via
div_i32>`):** the `<A..B>` composite and the ` via div_i32` suffix. The `A..B` is
derivable (src..tgt from seed_source/seed_target). But ` via div_i32` (op.8/op.9)
names the MAP functor on the branch path — same no-source problem as SP-1 (no
functor field). RULING: render `branch=<%table:N..%table:M>` from src/tgt; **DROP
the ` via div_i32` gloss** (no stored source). Alternatively drop `branch=`
entirely (redundant with `src=`/`tgt=` on the same line). **Recommend DROP
`branch=` — it is a redundant composite of src/tgt already present.** Re-render
item **R-9**.

**FLAG AR-2 (`pivots=$join-pivots.42`):** on op.12-15 join-pivot seeds. The pivot
vec is `DRJoin.pivot_vec` but the SEED_FOLD op itself has no direct pivot_vec
field — it has `join_pivot` bool (h:437). The pivot vec index must be resolved via
the branch→join match (B-1) → `join.pivot_vec`. Derivable but indirect. PIN:
render `pivots=$join-pivots.<join.pivot_vec index>` resolved via the branch's
terminal join. OR drop (the vec's own `def=[op.12..15]` already records the
linkage). **Recommend keep `pivots=` (it IS the seed's target) but resolve the
index via the join.** Derivable.

---

## PART 3 — ID-SPACE ADJUDICATION

**Op ids = mint index; op-line ORDER = pinned_order position.** Confirmed from:
- The label is the dense MINT index into `flow.ops` (`DRFlowGraph::ops`, h:651;
  there is NO id field on DROp — DeltaRel.h:383-552 has no `id` member; identity
  IS the vector position). Contract §0 (lines 96-103) + §2.4 (lines 835-841)
  state this explicitly ("op ids are the dense MINT index into flow.ops; the §1
  op LINE order is pinned_order").
- The op SECTION is walked in `pinned_order` (`DRFlowGraph::pinned_order`, h:658,
  the Kahn linearization DeltaRel.cpp:3702-3739). So op.24 (mint 24, stratum 1)
  appears BEFORE op.16 (mint 16, stratum 4) in the section because pinned_order
  places stratum-1 before stratum-4. The labels are NON-MONOTONE by design.

**EXACT RULE (PINNED):** the emitter walks `flow.pinned_order` in order; for each
entry `oi` it renders `op.<oi>` (the mint index = the value stored in
pinned_order, which IS the index into flow.ops) with the op at `flow.ops[oi]`.
Label = mint index (`oi`); order = pinned_order position.

**Vec ids = mint index** into `flow.vecs` (h:646; no id field, position = identity).
Walked in vector (mint) order — §2.3 spec "vecs-vector (mint) order".

**Branch ids = vector index** into `flow.branches` (h:661). **Join ids = vector
index** into `flow.joins` (h:662). Both walked in vector order (spec §2.3
"flow.joins / flow.branches in their own vector order"). No id fields.

**Statecell ids (`sc#N`) = `statecell_id`** field (h:536) = index into
`flow.statecells` (h:647). Stored, not positional-only. Mint order = Aggregates
then KVIndices (DeltaRel.cpp:1057/1085) → sc#0=sum, sc#1=count, sc#2=KV
(contract F-7 CONFIRMED against .ir sc# labels).

**ALL ILLUSTRATIVE IDS PIN AT FIRST EMISSION.** Every `op.<N>`/`vec.<N>`/
`branch.<N>`/`join.<N>` integer in the contract is ILLUSTRATIVE-PREDICTED
(contract §0 lines 96-103, marked `; PRED` in §2). Per the symrec precedent (spec
§1.2 det_seq; the .df id-pinning ritual), the FIRST compiler run pins the exact
integers; STRUCTURE + ORDER are what the contract certifies. The re-render items
below do NOT touch ids (they are pinned by the run, not by this adjudication).

---

## PART 4 — DEPS + CENSUS RULES

### 4.a deps: the canonical sort (F-9 floor) + line grammar

**THE F-9 DETERMINISM DEFECT IS CONFIRMED IN CODE.** `flow.dep_edges` (h:657) is
built by iterating TWO `std::unordered_map`s: `by_vec`
(DeltaRel.cpp:3629, `std::unordered_map<unsigned, std::vector<const VecAccess*>>`)
and `by_flag` (DeltaRel.cpp:3658, `std::unordered_map<TABLE*, …>`). Confirmed by
reading DeltaRel.cpp:3629-3684: `for (auto &[vec_idx, accs] : by_vec)` and
`for (auto &[table, accs] : by_flag)` — both HASH-order traversals. The
`flow.dep_edges` VECTOR ORDER is therefore NOT byte-stable across builds/runs.
Spec §2.3 says "deps: … flow.dep_edges VECTOR ORDER, EXHAUSTIVE" — that vector
order is unstable, so a deps byte-golden blessed from it would flake.

**PINNED EMITTER RULE (the F-9 fix):** the emitter MUST NOT render dep_edges in
vector order. It MUST sort into a canonical order **by (from, to, kind)** before
rendering, where `kind` orders `kRAW < kWAR < kWAW` (DepKind h:605 declaration
order). Ascending `from`, then ascending `to`, then kind. This must be pinned in
spec §2.3 (amend "VECTOR ORDER" → "canonically sorted by (from,to,kind)") and
implemented. Until the emitter sorts, the deps section is UNBLESSABLE (contract
F-9, confirmed). The contract's §1 deps block (lines 502-563) IS already in this
canonical (from,to) order — it is the desired post-sort bytes.

**deps line grammar (PINNED):** `  op.<from> -> op.<to> <DepKind> <DepScope>`
- `op.<from>`/`op.<to>` = `DRDep.from`/`DRDep.to` (h:609-610, op mint indices).
- `<DepKind>` = `RAW`/`WAR`/`WAW` — **k-STRIPPED** (from `DepKind::kRAW` etc.,
  h:605). Contract renders `RAW`/`WAR` (no `k`).
- `<DepScope>` = `epoch`/`round` — k-stripped (from `DepScope::kEpoch`/`kRound`,
  h:606). Contract renders `epoch`.
- `loop_carried` (h:613): rendered ONLY when true, as a trailing token
  `loop-carried` (spec §2.3 "loop_carried rendered here"). This program has NONE
  (acyclic — contract lines 489-490). PIN: append ` loop-carried` iff
  `dep.loop_carried==true`; omit otherwise. NOTE the contract's trailing `;`
  english on every deps line (e.g. `; sum delQ (band-b) -> %table:4 claim drain`)
  is PROSE → STRIP (part of R-3; deps carries no comments).

**F-10 (completeness) is an implementation-time concern, NOT a grammar decision:**
the flag-class edge SET (contract lines 564-572) is a FLOOR — the emitter's actual
`by_flag` FlagAccess enrollment (kCounter writes, claim/filter FlagWrites, sweep
kInI) yields more WAW/WAR edges than the hand-enumerated filter→sweep subset. The
grammar is fixed (the line form above); only the edge COUNT is unpinnable by hand.
PIN: at bless, dump the real sorted dep_edges once and diff against the floor;
bless the actual set. This is a first-run pin, not a re-render item.

### 4.b census: the line grammar (count_kind mold, keys out per E-28)

Contract form (lines 575-578):
```
census: kGroupUpdate=3 kStateSeal=3 kSeedFold=10 kClaimDrain=14
        kFrontierFilter=14 kCommitSweep=8 kIngestFold=1
        (kCrossover=0 kProductArm=0 kFixpointFire=0 kChainFold=0 kRetire=0
         kRederive=0 kNegateGate=0 kPivotAssemble=0)
```

**Source:** the `count_kind` mold (DeltaRel.cpp:2854-2861 — CONFIRMED by read: a
lambda `count_kind(DROpKind k)` counting `op.kind==k` over `flow.ops`). The census
renders PER-KIND COUNTS ONLY. **Keys stay OUT (E-28):** the spec §2.3 says "keys
stay out (E-28)" — the census renders counts, NOT the per-op key multisets that
`ValidateDROps` recomputes (the IngestKey/gu_key multisets, DeltaRel.cpp:2854 area).
So `census:` = `<DROpKind spelling>=<count>` for every kind, space-joined.

**PINNED census grammar:** one `census:` lead, then `<kKind>=<n>` for each of the
15 DROpKind members (h:113-138) in ENUM declaration order, counts via count_kind.
DROpKind spelling k-PREFIXED (per 2.0). The contract splits nonzero kinds onto the
first lines and zero kinds into a trailing `(…)` parenthetical — this is a HAND
formatting choice with no code basis. RULING: pin a MECHANICAL rule. Two options:
  (a) render ALL 15 kinds in enum order, `k<Kind>=<n>` space-joined, wrapping at a
      fixed column — no zero/nonzero split, no parenthetical.
  (b) keep the contract's nonzero-then-`(zeros)` split.
**Adjudication: OPTION (a) — all 15 in enum order, no split, no parenthetical.**
The nonzero/zero partition + parenthetical is non-mechanical (depends on which
kinds happen to be zero for THIS program — a different program partitions
differently, so the byte layout is program-dependent in a non-obvious way). A flat
enum-order list is fully mechanical and program-independent in structure.
**Re-render item R-10** (flatten the census to enum-order, drop the parenthetical).
Line-wrapping: pin a fixed wrap (e.g. wrap at N kinds/line or at a byte column);
the implementer picks one fixed rule — I recommend NO wrap (one long line) or a
fixed 4-per-line, pinned explicitly. **Sub-item: pin the wrap rule; recommend
one line, no wrap, for zero layout ambiguity.**

---

## PART 5 — CONFIG-INVARIANCE PRE-AUDIT

Charge: for every field the grammar renders, check the backing member for
`#ifndef NDEBUG` gating (the a3/producer trap — a config-variant field must NOT
render, else the golden fails under the release preset).

**RESULT: CLEAN.** `grep -n NDEBUG lib/DeltaRel/DeltaRel.h` returns only two hits
(h:714, h:739) and BOTH are comment prose ("survive NDEBUG" in validator doc
comments) — NO struct member is `#ifndef NDEBUG`-gated. Every field this grammar
renders is an always-compiled member:
- DROp fields (kind h:385, ctx h:391, all payload fields h:398-549) — ungated.
- DREffect fields (h:197-226) — ungated.
- DRVec fields incl. `debug_table`/`debug_kind` (h:252-253) — labelled "DEBUG
  annotation only" but NOT NDEBUG-gated; they compile in every preset. The
  `<shape>` render reads `debug_table` — config-invariant.
- DRBranch/DRJoin/DRStateCell/DRDep — all ungated.
- The `*_stratum` maps + pinned_order + dep_edges on DRFlowGraph (h:656-685) —
  ungated.
- The enum spellings — pure switch tables, config-invariant.

**No config-variant field is rendered.** The deltarel golden class is config-clean
like `.ir`/`.h` (and unlike the REJECTED `.df` producer= field, Query.h:531-535,
which WAS NDEBUG-gated — that trap does not recur here). This confirms contract
F-8. No re-render implied.

CROSS-CHECK on the id-space: `TABLE::id` (Program.h:129, `const unsigned id`) is
ungated; QueryColumn/QueryView accessors are ungated. The `%table:N` cross-refs
are config-invariant.

---

## PART 6 — THE RE-RENDER PLAN (concrete §1 edits my pins imply)

Do NOT edit the contract here — the orchestrator applies these. Each item names
the affected §1 lines and the exact change.

**R-1 (comments) — DROP the decorative `;; === … ===` section banners.**
Lines 120, 178, 191, 200, 485, 574. Replace each `;; ==== NAME ====` block with
the bare section lead already present (`vec …` / `branch.0 …` / `join.0 …` /
`op.52 …` / `deps:` / `census:`). Net: remove 6 banner lines (+ their `====`
rules). [If owner keeps banners, pin the 6 fixed strings; default = drop.]

**R-2 (comments) — DROP the per-stratum `;; --- stratum N: … ---` group banners
and their prose parentheticals.** Lines 202, 210, 244, 272-275, 308-310, 342-344,
358-359, 377, 405-408, 436-441, 466-467, 478-482, and the big `;;` prose blocks
121-133, 179-184, 192-195. The `stratum=<k>` field on each op carries the grouping.

**R-3 (comments) — STRIP every trailing/standalone `;` comment from the byte
block.** Lines 185-189 (branch glosses), 203, 214-216, 224, 226, 279-281, 289,
314-315, 323, 347, 469, and every `;` english tail on the deps lines 502-572.
The gate=/sign=/spine:—/class=/field carriers are derivable; the english is prose.
Deltarel byte block = ZERO comments.

**R-4 (join) — DROP the `join.0` `section-walk:` subline.** Lines 197-198.
`%index:149`/`%index:150` are ControlFlow `DataIndex.Id()` values that DO NOT
EXIST in the DR-IR at the emit point (DRJoin h:302-309 carries no index ids).
`join.0` renders only `view=/pivot_vec=/targets=` (all stored, h:304-306).

**R-5 (op header) — ADD `publish_target=false` to op.51 (monotone sweep).**
Line 463. Uniform rendering of the real bool (h:478) on all commit sweeps; the
contract's selective omission has no field backing.

**R-6 (ingest args) — DROP `receive=<recv %table:36>` from op.52 args.** Line 208.
Redundant with `table=%table:36`; the `<recv …>` wrapper has no primitive.
Result: `args: table=%table:36 message=add_edge/3`.

**R-7 (ingest spine) — op.52 `spine:` → `—`.** Line 207. INGEST_FOLD has no arm/
PlanTree; the `kFold(%table:36,+,NonRecursive)` is a synthetic duplicate of the
kCounter effect.

**R-8 (seed spine) — op.8/op.9 spine: strip the `div_i32 MAP, bbf` functor gloss.**
Lines 381, 386. No PlanNode field carries a functor name or binding pattern
(h:332-359). Render `kAccess(%table:N, <lowering>)` from stored fields, or drop
the access node if only the fold is stored. [Confirm the arm's actual PlanNode
shape at bless — the MAP access may lower to a plain fold.]

**R-9 (seed args) — DROP the `branch=<…>` composite (and its ` via div_i32`
suffix).** Lines 249, 254, 348, 351, 354, 357, 382, 387, 413, 418. Redundant with
`src=`/`tgt=` already on the same line; ` via div_i32` has no functor field.
[Or keep `branch=<%table:N..%table:M>` bare without ` via …` — owner's call; I
recommend drop for minimality.]

**R-10 (census) — FLATTEN to enum order, drop the nonzero/`(zeros)` split.**
Lines 575-578. Render all 15 DROpKind counts in h:113-138 enum order,
`k<Kind>=<n>` joined; pin a fixed wrap rule (recommend one line / no wrap). The
zero/nonzero partition is program-dependent layout.

**Non-edits (grammar pins that DON'T change §1 bytes):** the VecRole/UniqueContract/
ElementShape/Ctx/ClaimForm/Deferral/SweepFlavor spelling maps, the Pred/DerivClass
k-strip, the DROpKind/EffKind/ClaimGate/AggProvenance/Algebra/PlanKind k-prefix,
the sign-glyph map, the deps canonical sort (contract §1 already in sorted order),
the id-pinning (first run) — these are IMPLEMENTER pins, contract bytes already
conform (except where an R-item corrects them).

**Re-render list size: 10 items (R-1 … R-10).** R-1/R-2/R-3 are comment strips
(the bulk); R-4 drops one subline; R-5 adds one token; R-6/R-7/R-8/R-9 correct
four op-line over-renders (no-source tokens); R-10 reshapes the census.

---

## APPENDIX — the LOUD no-source flags (fields the contract renders with NO
## stored DR-IR source; each MUST be dropped/re-rendered, never guessed)

| flag | token | why no source | disposition |
|---|---|---|---|
| **J-2** | `%index:149` / `%index:150` (join section-walk) | DataIndex ids are ControlFlow-only (`lib/ControlFlow/Format.cpp:72`); DRJoin (h:302-309) has none; the section walk is a LOWERING that runs after the emit point | R-4 (drop subline) |
| **J-2** | `point-test … [X,_]` / `fold ± into` | section-walk detail, not a DRJoin field | R-4 |
| **SP-1** | `div_i32 MAP, bbf` (op.8/9 spine) | PlanNode (h:332-359) has no functor-name or binding-pattern field | R-8 (strip gloss) |
| **AR-1** | ` via div_i32` (op.8/9 `branch=`) | same — no functor field | R-9 (drop) |
| **IG-1** | `<recv %table:36>` (op.52) | no `<recv …>` primitive; only ingest_table id exists | R-6 (drop, = table=) |
| **B-1** | `branch.N -> join.0` join index | DRBranch (h:287-293) has no join_index; must match by `path.back()==join_view` | emitter-side derivation (not a re-render; pin the match rule) |
| **S-1** | `·` sign glyph | op_sign returns int, no glyph map; and `·` is a multibyte byte in a golden | implementer hardcode + byte-verify |
| **ST-1/BD-1** | `stratum=<k>` / `band=<n>` | recomputed lambdas (DeltaRel.cpp:3276/3354), not DROp fields | hoist lambda; inputs are stored `flow` maps (derivable) |
| **GU-3** | `mutable(new_weight_i32)` summary type | PREDICTED; `.ir`/`.df` render mutable as `mut` — likely mismatch | pin to actual TypeLoc at bless |
| **EF-1** | kStateFold/kStateEmit/kStateOld table field | DREffect (h:197-226) has no statecell field; reuses counter/value_table | confirm backing field at bless |
