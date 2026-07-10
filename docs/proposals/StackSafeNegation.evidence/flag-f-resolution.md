# FLAG-F resolution: hand-trace transcript

Branch `derivation-counters`, HEAD 680916c. Method: hand-trace the corrected
projected (c) CF-IR (checkpoint-c-notes' corrected diff, generalized per PLAN
A8 to the nonlinear fixture `p(X,Z):p(X,Y),p(Y,Z)`) over the discriminating
batches of `nonlin_tc_both_change.batches`, running each candidate
interpretation of `InNewWithFrontier` at every read position that names it,
and comparing final IDB + counter-exactness against the oracle ground truth
(`truth/nonlin_tc_both_change.{differential,monotone}.txt`, which agree:
`reachable = {(4,5),(4,6),(4,7),(5,6),(5,7),(6,7)}`).

Oracle binary reproduced this truth (differential and `--project-monotone`,
both exit 0, INVARIANT line printed).

---

## 0. The MD §5.1 fixpoint table, VERBATIM (docs/proposals/StackSafeNegation.md:204-212)

> **Fixpoint schema** — delta position `i` is a same-stratum atom; the delta
> ranges over the current claim round (`Δ_D` in OVERDELETE, `Δ_A` in INSERT).
> All lower positions are `j < i` by the position order:
>
> | position | OVERDELETE (`−`, delta ∈ Δ_D) | INSERT (`+`, delta ∈ Δ_A) |
> |---|---|---|
> | lower `j < i` | `InNew` (negated: absent in `InNew`) | `InNew` (negated: absent in `InNew`) |
> | same `j < i` | `SurvivesSoFar` | `InNewWithFrontier` |
> | same `j > i` | `AliveAtClaim` | `InNewSansFrontier` |
>
> The `kDelNow`/`kAddNow` frontier bits exist precisely so that an instance
> whose two same-stratum body atoms are claimed **in the same round** is
> still counted exactly once: with the earlier position as delta, the later
> one passes `AliveAtClaim`; with the later as delta, the earlier fails
> `SurvivesSoFar`. Both schemas are one generated pattern per rule; a missing
> term is a schema bug visible on every program, not a per-site
> hand-derivation.

And the predicate list, VERBATIM (StackSafeNegation.md:91-96):

> ```
> bool InNew(uint32_t id) const;       // (kInI&&!kDel)||kAdd — final-so-far
> ...
> bool SurvivesSoFar(uint32_t id) const;       // kInI && !kDel
> bool AliveAtClaim(uint32_t id) const;        // kInI && (!kDel || kDelNow)
> bool InNewWithFrontier(uint32_t id) const;   // (kInI&&!kDel)||kAdd
> bool InNewSansFrontier(uint32_t id) const;   // (kInI&&!kDel)||(kAdd&&!kAddNow)
> ```

So `InNewWithFrontier`'s stated formula `(kInI&&!kDel)||kAdd` is byte-for-byte
`InNew`'s stated formula `(kInI&&!kDel)||kAdd`. FLAG-F = "typo or intentional
alias?"

`InNewWithFrontier` is READ at exactly one kind of site in the whole schema:
the INSERT fixpoint column, same-stratum position `j < i` (a same-stratum
body atom that sorts BEFORE the round's delta atom in the fixed total order).

---

## 1. Candidate readings enumerated from the table's own structure

The INSERT column of the fixpoint table has to give three predicates that
together implement "exactly-once on a same-round double-claim." The only
free choice FLAG-F leaves is the `j < i` cell (`InNewWithFrontier`); the
`j > i` cell (`InNewSansFrontier = (kInI&&!kDel)||(kAdd&&!kAddNow)`) is
unambiguous and is the SAME under every reading below (it is not what FLAG-F
questions). The `same j<i` OVERDELETE cell (`SurvivesSoFar = kInI && !kDel`)
is likewise fixed. Plausible readings of `InNewWithFrontier`, derived from
the structure:

- **R-i (alias):** `InNewWithFrontier := (kInI && !kDel) || kAdd`   (≡ `InNew`)
  — the literal stated formula. "Earlier same-stratum sibling counts if it
  survived the batch OR was added this batch (any round)."

- **R-ii (current-round-only add):** `InNewWithFrontier := (kInI && !kDel) ||
  (kAdd && kAddNow)` — narrow the add disjunct to rows claimed in the CURRENT
  round, mirroring how `AliveAtClaim` uses `kDelNow` as its permissive
  disjunct. (This is DESIGN.md/pseudo-c-spec.md reading (ii).)

- **R-iii (sans-frontier, i.e. exclude current-round adds):**
  `InNewWithFrontier := (kInI && !kDel) || (kAdd && !kAddNow)` — i.e. the same
  as `InNewSansFrontier`. Included because a "typo" could plausibly be that
  both INSERT same-stratum cells were meant to read the sans-frontier form
  (making the INSERT column visually mirror OVERDELETE, where `SurvivesSoFar`
  is the strict form). "Earlier sibling counts if it survived OR was added
  in a PRIOR round but not this round."

- **R-iv (pure frontier):** `InNewWithFrontier := kAdd` (or `kAdd&&kAddNow`)
  with the `(kInI&&!kDel)` disjunct dropped entirely — the strongest "typo"
  hypothesis floated in DESIGN.md §3(i) batch-3 note ("reading (ii) drops the
  (kInI && !kDel) disjunct's coverage of 'not touched this round, just
  already there'"). "Earlier sibling counts ONLY if freshly added."

R-i is the alias reading; R-ii/R-iii/R-iv are the three "typo" variants
(narrow-to-now, sans-frontier, pure-frontier).

The discriminator between R-i and each typo variant is a rule instance whose
EARLIER same-stratum position is a row that is **already present from a prior
batch (`kInI=true`, `!kDel`) and NOT re-added this batch (`kAdd=false`)**,
joined against a LATER same-stratum position that IS the current round's
delta (freshly claimed, in `Δ_A`). Under R-i that earlier row passes
(`kInI && !kDel` true). Under R-iv it FAILS (no `kAdd`), so the instance is
lost. Under R-ii/R-iii it still passes (both keep the `(kInI && !kDel)`
disjunct), so R-ii/R-iii are NOT separated from R-i by such a row — they are
only separated by a row that is `kAdd` but in the wrong round-phase. I
enumerate a separating fixture for those below (§5).

---

## 2. Rule position order and read assignment for `p(X,Z):p(X,Y),p(Y,Z)`

Both body atoms are same-stratum (`p`). Fixed total order (MD §5.1: lower
first then same-stratum in syntactic order) ⇒ position 0 = `p(From,Y)`,
position 1 = `p(Y,To)`. `FixReads(p, deleting=false)`:

- delta at p=0: pos0 = delta (pinned); pos1 is `j>p` ⇒ **InNewSansFrontier**.
- delta at p=1: pos0 is `j<p` ⇒ **InNewWithFrontier**; pos1 = delta (pinned).

So `InNewWithFrontier` gates the position-0 candidate `p(From,Y)` exactly
when the round's delta is the position-1 atom `p(Y,To)`. This is the only
place FLAG-F can bite. Head `p(From,To)`; the join key is `Y` (pos0.To ==
pos1.From).

The projected (c) IR emits (per PLAN A8 / checkpoint-c-notes generalization)
TWO INSERT fixpoint-join emissions — one per same-stratum delta position
(this is the FLAG-H "k emissions" shape, which the oracle confirms:
`FixReads` is invoked once per `p ∈ same_pos`). Emission E0 (delta=pos0,
reads pos1 via InNewSansFrontier) and E1 (delta=pos1, reads pos0 via
InNewWithFrontier). Both fire over the SAME round set `Δ_A`, so a row in
`Δ_A` is a valid delta for BOTH emissions.

---

## 3. State entering the discriminating batches

I carry only the `p`-table (the recursive stratum). `edge` and `reachable`
are lower/higher non-recursive tables; `edge` is `InI` for whatever
`add_edge` currently nets to.

**After batch 1** (`+edge 1-2, 2-3, 3-4`), `p` fully materialized (monotone
add, no removals). Rows of `p` with their derivation counts:

| p row | C_nr (base p:edge) | C_r (recursive) | in_i |
|---|---|---|---|
| (1,2) | 1 | 0 | T |
| (2,3) | 1 | 0 | T |
| (3,4) | 1 | 0 | T |
| (1,3) | 0 | 1  [p(1,2),p(2,3)] | T |
| (2,4) | 0 | 1  [p(2,3),p(3,4)] | T |
| (1,4) | 0 | 2  [p(1,2),p(2,4)] + [p(1,3),p(3,4)] | T |

(The C_r=2 for (1,4) is the engineered double-derivation. Confirmed by the
oracle NOT aborting — its per-row `C_r == from-scratch instance count`
assertion held, so C_r(1,4)=2 is truth.)

All flags: kInI=T, kDel=F, kAdd=F, kDelNow=F, kAddNow=F.

**After batch 2** (`-edge 2-3, -edge 3-4`): every p row that route through
node 3 loses support. OVERDELETE drains. Result (matches oracle, which stays
OK): the only surviving p rows are `p(1,2)` (C_nr=1, base edge 1-2 intact).
Everything else (`(2,3),(3,4),(1,3),(2,4),(1,4)`) is overdeleted: kInI flips
to F at batch-2 commit (`in_i = (C_nr+C_r)>0`), counts 0. So entering batch 3:

| p row | C_nr | C_r | kInI (start of b3) | kDel | kAdd |
|---|---|---|---|---|---|
| (1,2) | 1 | 0 | T | F | F |
| all node-3 rows | 0 | 0 | F | F | F |

Batch 2 is NOT a FLAG-F batch (FLAG-F reads only appear in INSERT; batch 2 is
pure removal, its INSERT phase has empty `Δ_A`). It only sets up batch 3's
state. Recorded for completeness; no InNewWithFrontier read fires in batch 2.

---

## 4. Batch 3 trace — `+edge 4-5, +edge 5-6` (the discriminator)

This is a pure-addition batch. OVERDELETE: `delQ` empty ⇒ no-op. REDERIVE:
`D_s` empty ⇒ no-op. All action is in INSERT.

### 4.1 INSERT seed phase (class kNonRecursive)

Lower stratum `edge` gains `outAdd = {(4,5),(5,6)}`. Base rule `p:edge` seeds:
- `edge(4,5)` ⇒ AddDerivation p(4,5) kNonRecursive: C_nr 0→1, before≤0∧after>0
  ⇒ pend_add += p(4,5).
- `edge(5,6)` ⇒ AddDerivation p(5,6) kNonRecursive: C_nr 0→1 ⇒ pend_add += p(5,6).

Recursive-rule seed (delta over lower `edge` frontier) — the recursive rule
`p:p,p` has NO lower-stratum atom, so its seed loop `for p ∈ r.lower_pos` is
empty. (Seed reads never touch InNewWithFrontier.) pend_add = {p(4,5), p(5,6)}.

State now: p(4,5): C_nr=1, kInI=F, kAdd=F(not yet claimed). p(5,6): C_nr=1,
kInI=F, kAdd=F. p(1,2): C_nr=1, kInI=T, kAdd=F.

### 4.2 INSERT fixpoint — ROUND 1

Claim: `Δ_A = { id ∈ pend_add : TryClaimAdd }`. Both p(4,5), p(5,6) get
kAdd=T, kAddNow=T, appended to claimed_add (A_s). round = {p(4,5), p(5,6)}.
pend_add cleared. Δ_A nonempty ⇒ proceed.

Flags in round 1:

| p row | kInI | kDel | kAdd | kAddNow |
|---|---|---|---|---|
| (4,5) | F | F | T | **T** |
| (5,6) | F | F | T | **T** |
| (1,2) | T | F | F | F |

Now the two fixpoint-join emissions fire over round = {(4,5),(5,6)}.
Head derivations attempted, one per (delta-position, delta-row) pair.

**Emission E0 (delta = pos0 = p(From,Y)); pos1 read = InNewSansFrontier.**
For each delta row in round bound to pos0:
- delta = p(4,5): From=4,Y=5. pos1 = p(Y=5, To). Enumerate p(5,·) rows
  passing InNewSansFrontier = (kInI&&!kDel)||(kAdd&&!kAddNow). Candidate
  p(5,6): kAdd=T but kAddNow=T ⇒ `kAdd&&!kAddNow` = F; kInI=F ⇒ FAILS. No
  other p(5,·). ⇒ E0/delta(4,5): no fire. (This is exactly the same-round
  exactly-once mechanism: the LATER sibling, claimed THIS round, is excluded
  from the earlier-delta evaluation.)
- delta = p(5,6): From=5,Y=6. pos1 = p(6,·). No p(6,·) rows exist. No fire.

**Emission E1 (delta = pos1 = p(Y,To)); pos0 read = InNewWithFrontier.**  ← FLAG-F SITE
For each delta row in round bound to pos1:
- delta = p(5,6): Y=5,To=6. pos0 = p(From, Y=5). Enumerate p(·,5) rows
  passing InNewWithFrontier. Candidate p(4,5): is it passed?
    - **R-i** InNewWithFrontier=(kInI&&!kDel)||kAdd: p(4,5) kAdd=T ⇒ **PASS**.
      Head p(From=4,To=6)=p(4,6). AddDerivation p(4,6) kRecursive:
      C_r 0→1, before 0 ≤0 ∧ after 1 >0 ⇒ pend_add += p(4,6). ✔ FIRES.
    - **R-ii** (kInI&&!kDel)||(kAdd&&kAddNow): p(4,5) kAdd=T,kAddNow=T ⇒
      `kAdd&&kAddNow`=T ⇒ **PASS**. Same fire: p(4,6) C_r→1, pend_add+=p(4,6).
      ✔ FIRES (R-ii is stricter but a same-round claim still satisfies it).
    - **R-iii** (kInI&&!kDel)||(kAdd&&!kAddNow): p(4,5) kAddNow=T ⇒
      `kAdd&&!kAddNow`=F; kInI=F ⇒ **FAIL**. ✘ p(4,6) NOT derived this round.
    - **R-iv** kAdd (or kAdd&&kAddNow): p(4,5) kAdd=T ⇒ **PASS**. ✔ FIRES.
      (R-iv passes here because p(4,5) IS freshly added.)
- delta = p(4,5): Y=4,To=5. pos0 = p(·,4). No p(·,4) rows (all node-3/4 rows
  overdeleted in batch 2, p(3,4)/p(2,4) gone; nothing ends at 4). No fire.

Retire round 1: kAddNow←F for p(4,5),p(5,6).

Round-1 pend_add after emissions:
- **R-i, R-ii, R-iv:** {p(4,6)}.
- **R-iii:** {} (empty).

### 4.3 INSERT fixpoint — ROUND 2

**R-i / R-ii / R-iv:** Claim p(4,6): kAdd=T,kAddNow=T. round={p(4,6)}.
Flags now: p(4,5),p(5,6) have kAdd=T,kAddNow=F (retired). p(4,6):
kAdd=T,kAddNow=T. p(1,2): kInI=T.
- E0 (delta pos0=p(4,6)): From=4,Y=6,pos1=p(6,·). None. No fire.
- E1 (delta pos1=p(4,6)): Y=4,To=6,pos0=p(·,4). None. No fire.
Retire p(4,6). pend_add empty ⇒ round 3 claim set empty ⇒ **break**.

**R-iii:** round-1 pend_add empty ⇒ round-2 claim set empty ⇒ **break**
immediately. p(4,6) never derived.

### 4.4 Batch-3 commit

`in_i = (C_nr+C_r) > 0` for each touched row.

**R-i / R-ii / R-iv:** p(4,5) C_nr=1→in_i T; p(5,6) C_nr=1→T; p(4,6)
C_r=1→T. reachable gains (4,5),(5,6),(4,6). Counters all ≥0. No assert.
**R-iii:** p(4,5) T, p(5,6) T, but p(4,6) C_r=0 ⇒ in_i F ⇒ **p(4,6) MISSING**.

Oracle truth after batch 3 (implied by final state + monotone): p rows
{(1,2),(4,5),(5,6),(4,6)} present (reachable {(4,5),(4,6),(5,6)} plus (1,2)
which is dropped only in batch 4). So **R-iii already fails at batch 3**:
it under-derives p(4,6), leaving reachable missing (4,6). R-i, R-ii, R-iv
all reproduce the batch-3 truth.

---

## 5. Batch 4 and the R-i vs {R-ii, R-iv} separator

### 5.1 Batch 4 as given — `-edge 1-2, +edge 6-7`

This batch's INSERT delta is `edge(6,7)` ⇒ base p(6,7) seed (C_nr 0→1). The
recursive INSERT fixpoint round-1 claims p(6,7). Does any InNewWithFrontier
read discriminate?

E1 (delta pos1 = p(6,7)): Y=6,To=7, pos0 = p(·,6). Candidate p(5,6) — its
state at batch 4 start: kInI=T (committed present in batch 3), kDel=F,
kAdd=F (NOT re-added this batch), kAddNow=F. Read InNewWithFrontier on p(5,6):
- **R-i** (kInI&&!kDel)||kAdd: kInI=T ⇒ **PASS** ⇒ p(5,7) fires (C_r→1).
- **R-ii** (kInI&&!kDel)||(kAdd&&kAddNow): kInI=T ⇒ **PASS** ⇒ p(5,7) fires.
- **R-iii** (kInI&&!kDel)||(kAdd&&!kAddNow): kInI=T ⇒ **PASS** ⇒ p(5,7) fires.
- **R-iv** kAdd (drop the kInI disjunct): p(5,6) kAdd=F ⇒ **FAIL** ⇒ p(5,7)
  NOT derived from this instance.

**This is the R-iv discriminator.** p(5,6) is `kInI` from a prior batch and
NOT re-added this batch — exactly the "already there, not touched this round"
case. R-iv drops it and loses p(5,7). Then the cascade continues: with p(5,7)
present, E1 delta=p(5,7) reads pos0 p(4,5) (kInI=T, kAdd=F) ⇒ R-iv also
fails ⇒ p(4,7) lost; and p(6,7) itself as pos1 delta reads pos0 p(·,6)=p(5,6)
lost ⇒ p(5,7) lost (already noted). Net under R-iv batch 4: p(5,7), p(4,7),
p(4,6→7 chain) all missing.

Oracle truth requires reachable = {(4,5),(4,6),(4,7),(5,6),(5,7),(6,7)} — it
INCLUDES (5,7) and (4,7). So **R-iv fails at batch 4**: it drops (5,7) and
(4,7) because their derivations chain through prior-batch `kInI` same-stratum
rows at the earlier position that R-iv refuses to read. R-i, R-ii, R-iii all
pass p(5,6)/p(4,5) here (all keep the `(kInI&&!kDel)` disjunct).

Also batch 4 removes edge(1,2): OVERDELETE overdeletes p(1,2) (C_nr 1→0,
kInI T, crossing) ⇒ p(1,2) drops. No same-stratum p sibling depends on
p(1,2) now (node-1 chain already severed in batch 2), so no fixpoint
InNewWithFrontier read is contingent on it. reachable loses (1,2). ✔ for all
readings that got this far.

### 5.2 R-ii vs R-i — needs a separator batch 3 does not provide

R-ii and R-i agree on batches 2,3,4 (both keep `(kInI&&!kDel)` and both pass
a same-round `kAdd&&kAddNow` row). To separate them we need an
InNewWithFrontier read on a row that is `kAdd` but **NOT** `kAddNow` — i.e. a
same-stratum earlier-position sibling that was claimed/added in a PRIOR
round of the SAME INSERT phase, and is `kInI=F` (so the `(kInI&&!kDel)`
disjunct can't rescue it). Under R-i that row passes (kAdd=T); under R-ii it
FAILS (kAddNow=F after retire); under R-iii it PASSES (kAdd&&!kAddNow=T).

The engineered separator (a 3-hop same-round chain, so a middle row is added
in round 1, retired, then read at an earlier position in round 2):

```
; separator.dr  (same program p(X,Z):p(X,Y),p(Y,Z), base p:edge)
batch  +edge 7 8   +edge 8 9   +edge 9 10   end
```
starting from empty p. Trace:
- seed: p(7,8),p(8,9),p(9,10) each C_nr=1, pend_add.
- round1: claim all three; kAdd=T,kAddNow=T. Fire:
    E1 delta=p(8,9): pos0=p(·,8)=p(7,8) → p(7,9)  (7,8 kAddNow=T:
      R-i pass via kAdd, R-ii pass via kAdd&&kAddNow, R-iii FAIL) 
    E1 delta=p(9,10): pos0=p(·,9)=p(8,9) → p(8,10) (same)
    E0 delta=p(7,8): pos1=p(8,·)=p(8,9)? InNewSansFrontier kAdd&&!kAddNow=F,
      kInI=F ⇒ FAIL (same-round exclusion) — no fire, correct.
    (net round1 new: p(7,9),p(8,10) for R-i/R-ii; nothing for R-iii)
  retire: kAddNow←F on p(7,8),p(8,9),p(9,10).
- round2 (R-i/R-ii): claim p(7,9),p(8,10); kAdd=T,kAddNow=T. Fire:
    E1 delta=p(7,9): pos0=p(·,7). none.
    E1 delta=p(8,10): pos0=p(·,8)=p(7,8). p(7,8) now kAdd=T, **kAddNow=F**
      (retired in round1), kInI=F.  ← THE R-i/R-ii SEPARATOR READ
        R-i  (kInI&&!kDel)||kAdd: kAdd=T ⇒ **PASS** ⇒ p(7,10) fires (C_r→1).
        R-ii (kInI&&!kDel)||(kAdd&&kAddNow): kAdd=T,kAddNow=F ⇒ F; kInI=F ⇒
             **FAIL** ⇒ p(7,10) NOT derived here.
    E0 delta=p(7,9): pos1=p(9,·)=p(9,10). InNewSansFrontier: p(9,10) kAdd=T,
      kAddNow=F ⇒ kAdd&&!kAddNow=T ⇒ **PASS** ⇒ head p(7,10) fires!  (both
      R-i and R-ii: E0 reads pos1 via InNewSansFrontier which is
      reading-independent of FLAG-F). So under R-ii, p(7,10) is STILL derived
      via E0's delta=p(7,9) evaluation.
```
Wait — E0 delta=p(7,9) has pos0=p(7,9) pinned (From=7,Y=9), pos1=p(Y=9,To)=
p(9,10) passing InNewSansFrontier ⇒ head p(7, To=10)=p(7,10). This fires
under BOTH R-i and R-ii because the surviving path routes through the
`j>i` cell (InNewSansFrontier), not the `j<i` cell (InNewWithFrontier). So
the p(7,10) derivation is not LOST under R-ii — it is merely attributed to a
different delta-position evaluation. **The exactly-once schema is designed so
that the head is reachable through whichever position is the LATER-claimed
one**; here p(7,9) (round2 delta) at pos0 combined with p(9,10) (prior,
InNewSansFrontier-passing) at pos1 covers it.

So even this 3-hop separator does NOT flip the final IDB between R-i and
R-ii: p(7,10) still ends present under R-ii, via E0. The DIFFERENCE is
purely in WHICH emission fires it and therefore only in the intermediate
C_r bookkeeping if BOTH E0(delta=7,9) and E1(delta=8,10) were to fire — but
under R-ii only E0 fires (E1's InNewWithFrontier read fails), so C_r(7,10)=1
exactly; under R-i BOTH would fire, giving C_r(7,10)=**2** for a head that
has only ONE surviving derivation (p(7,9),p(9,10)) — a COUNTER OVER-COUNT.

**This is the real discriminator, and it is a counter-exactness violation,
not an IDB-membership flip.** Under R-i, p(7,10) receives C_r=2 (E0 via
delta p(7,9)+InNewSansFrontier p(9,10); AND E1 via delta p(8,10)? no —
recheck: E1 delta=p(8,10) has pos1 pinned = p(8,10) (Y=8,To=10), pos0 =
p(·,8) = p(7,8); head = p(7,10). Under R-i p(7,8) passes InNewWithFrontier
⇒ E1 ALSO fires p(7,10). So R-i fires p(7,10) TWICE in round2 (once E0 delta
7,9; once E1 delta 8,10) ⇒ C_r(7,10)=2. But the true from-scratch instance
count of p(7,10) is: {p(7,8),p(8,10)}, {p(7,9),p(9,10)} = **2 distinct
instances**. So C_r=2 is CORRECT, not an over-count!

Under R-ii: E1 delta=p(8,10) reads p(7,8) via InNewWithFrontier and FAILS
(kAddNow=F) ⇒ only E0 fires ⇒ C_r(7,10)=**1**, but truth is 2 ⇒ R-ii
UNDER-COUNTS by 1.

Does the under-count flip the final IDB here? No — C_r=1>0 ⇒ p(7,10) still
present, so this single batch's END STATE is identical. The under-count is
invisible in THIS batch's IDB. It becomes IDB-visible only when a LATER
batch removes ONE of the two derivations: truth expects the head to survive
on the other (C_r 2→1), but under R-ii's under-count (C_r already 1) the same
removal drives C_r 1→0 and wrongly DROPS the head. So the IDB-discriminating
separator is a SECOND batch that retracts exactly one of the two supports.
```

### 5.3 Separator with a second batch — makes R-ii IDB-visibly wrong

```
; separator2.dr / .batches — same program, two batches
batch  +edge 7 8   +edge 8 9   +edge 9 10   end     ; builds p(7,10), C_r truth=2
batch  -edge 8 9   end                              ; kills the {p(7,8),p(8,10)}
                                                     ; support-chain but NOT
                                                     ; {p(7,9),p(9,10)}
```
After batch 1: truth C_r(7,10)=2 [via (7,8),(8,10) and (7,9),(9,10)].
- R-i: C_r(7,10)=2 (both instances counted, per §5.2).
- R-ii: C_r(7,10)=1 (one instance dropped by the failed InNewWithFrontier
  read).

Batch 2 retracts edge(8,9). OVERDELETE: p(8,9) loses base ⇒ overdeleted;
cascade removes p(7,9) [was (7,8),(8,9)], p(8,10) [was (8,9),(9,10)]. The
head p(7,10)'s instance {p(7,8),p(8,10)} loses p(8,10) ⇒ SubDerivation
p(7,10) kRecursive once. Its OTHER instance {p(7,9),p(9,10)} loses p(7,9)
⇒ SubDerivation p(7,10) kRecursive again. So OVERDELETE decrements C_r(7,10)
by **2** total (both derivations' earlier/later atoms crossed).

- R-i: C_r 2→0. REDERIVE checks C_r>0 ⇒ 0 ⇒ no restore. p(7,10) drops. But
  is that TRUTH? Net surviving edges after batch 2: {7-8, 9-10} (8-9 gone).
  p from-scratch: p(7,8),p(9,10) only; NO p(7,9)/p(8,10)/p(7,10) (no path
  7→10). So truth: p(7,10) ABSENT. R-i gives ABSENT. ✔.
- R-ii: C_r 1→ (−1)? SubDerivation twice from 1 ⇒ C_r = −1 ⇒
  **CheckMagnitude/commit non-negativity assert: C_r(7,10) < 0 ⇒ oracle-style
  SIGABRT** (the "phantom/under-count driving a counter negative" failure the
  DESIGN.md and checkpoint-c-notes both name). Even absent the assert, R-ii's
  under-count corrupts the counter. Either way R-ii is WRONG: crash, or (if
  asserts disabled) a −1 counter that mis-tracks presence on subsequent
  batches.

So **separator2 IDB-and-assert-discriminates R-i from R-ii**: R-i ends with
p(7,10) correctly absent and all counters ≥0; R-ii drives C_r(7,10) negative
(assert / corruption). This is the fixture that separates the two survivors
from batches 2-4 of the main fixture.
```

### 5.4 Actually R-ii dies even earlier — the per-CLASS counter cross-assert

Confirmed empirically and by reading the oracle: the oracle stores, per row,
the from-scratch `(C_nr, C_r)` pair (`bin/Oracle/Main.cpp:518-522`,
`expected`) and cross-asserts EACH incremental class-counter against it every
batch (header lines 21-23: "every incremental counter equals the from-scratch
derivation-instance count **for its class**"). For p(7,10) the from-scratch
recursive instance count is exactly 2 (`{p(7,8),p(8,10)}` and
`{p(7,9),p(9,10)}`). Under R-i the incremental C_r(7,10)=2 matches; under
R-ii only one of the two E1/E0 evaluations fires so C_r(7,10)=1 ≠ 2 ⇒ the
per-class assertion FAILS at **separator batch 1 (sep1) directly**, no second
batch needed.

Empirical confirmation (oracle = R-i):

```
$ oracle sep.dr sep1.batches
ORACLE: OK (1 batches, 334 assertions)          # C_r(7,10)==2 held
reachable 7 8 / 7 9 / 7 10 / 8 9 / 8 10 / 9 10   # p(7,10) present
$ oracle sep.dr sep2.batches                      # then retract edge(8,9)
ORACLE: OK (2 batches, 589 assertions)          # C_r 2->0 clean, no abort
reachable 7 8 / 9 10                              # p(7,10) correctly ABSENT
```

Both runs exit 0 with the INVARIANT line — i.e. the oracle's R-i reading
keeps C_r exact (=2) and non-negative through the retraction. R-ii would have
tripped the per-class assert at sep1; R-ii-with-asserts-off would drive C_r
to −1 at sep2 (the negative-counter corruption). This is a cleaner
discriminator than §5.3's negative-counter argument: R-ii is wrong on the
COUNTER already at sep1, before any IDB difference is even needed.

---

## 6. Verdict table

| reading | batch2 | batch3 (p(4,6)) | batch4 (5,7)(4,7) | separator2 (C_r exact) | survives? |
|---|---|---|---|---|---|
| **R-i** `(kInI&&!kDel)||kAdd` (≡InNew) | ok | fires ✔ | fires ✔ | C_r=2 exact, ends correct ✔ | **YES** |
| R-ii `(kInI&&!kDel)||(kAdd&&kAddNow)` | ok | fires ✔ | fires ✔ | under-counts→C_r<0 **✘** | NO |
| R-iii `(kInI&&!kDel)||(kAdd&&!kAddNow)` (=Sans) | ok | **✘ p(4,6) lost** | — | — | NO |
| R-iv `kAdd` (drop kInI disjunct) | ok | fires ✔ | **✘ (5,7),(4,7) lost** | — | NO |

Only **R-i** reproduces truth on ALL discriminating batches (and the two
appended separators). R-iii dies at batch 3 (nonlinear both-added). R-iv dies
at batch 4 (mixed old-`kInI`/new-`kAdd` earlier-position sibling). R-ii
survives batches 2-4 but is exposed by separator2 as an under-counter that
drives C_r negative when one of two same-round-double-derived supports is
later retracted.

R-i is exactly the oracle's own committed reading
(`bin/Oracle/Main.cpp:258` — `kInNewWithFrontier: (s.in_i && !s.del) ||
s.add`, byte-identical to `kInNew`). The independent hand-trace confirms the
oracle's choice is the ONLY correct one.

---

## 7. Conclusion

**FLAG-F is an intentional ALIAS, not a typo.** `InNewWithFrontier` is
semantically identical to `InNew`: `(kInI && !kDel) || kAdd`. The stated
formula in MD §3.1 is correct as written; the apparent redundancy is real
and deliberate.

Why the alias is REQUIRED (not just harmless naming):

1. The `j < i` INSERT read (earlier same-stratum sibling of the delta) must
   see the sibling as "new" if it is EITHER a batch-surviving prior row
   (`kInI && !kDel`, the p(5,6)/p(4,5)/p(7,8) case in batches 4 and the
   separators) OR added in ANY round of this phase including the current one
   (`kAdd`, the same-round p(4,5)/p(5,6)→p(4,6) case in batch 3). Both
   disjuncts are load-bearing: dropping `(kInI&&!kDel)` breaks R-iv (batch 4);
   restricting `kAdd` to `kAddNow` breaks R-ii's counter-exactness
   (separator2).

2. The same-round exactly-once mechanism lives ENTIRELY in the `j > i` cell
   (`InNewSansFrontier`'s `!kAddNow`), NOT in the `j < i` cell. `AliveAtClaim`
   (OVERDELETE) needs its `kDelNow` disjunct because the OVERDELETE
   later-position sibling must be allowed to be same-round-claimed; but the
   INSERT `j < i` cell needs NO such special-casing — an earlier sibling
   claimed this round should count (the head genuinely becomes derivable),
   and an earlier sibling claimed a prior round should ALSO count (it is a
   real, still-present derivation). So `InNewWithFrontier` correctly has no
   `kAddNow` term at all — it is plain `InNew`. The table is asymmetric
   (FLAG-G) by design: the frontier bit that enforces exactly-once sits on
   the LATER position in BOTH phases (`AliveAtClaim` includes `kDelNow` to
   ADMIT the same-round later delete; `InNewSansFrontier` includes `!kAddNow`
   to EXCLUDE the same-round later add), and the earlier-position cell is the
   plain "final-so-far" predicate in both (`SurvivesSoFar`=strict for delete,
   `InNewWithFrontier`=`InNew` for add).

**Exact wording the proposal's §5.1 table should carry.** Keep the name (it
documents the read site) but annotate the identity so no future reader files
FLAG-F again. Suggested: change the `same j < i` INSERT cell from bare
`InNewWithFrontier` to:

> | same `j < i` | `SurvivesSoFar` | `InNewWithFrontier` (≡ `InNew`; the earlier-position add-side read is the plain final-so-far predicate — the same-round exactly-once bit lives only in the `j > i` cell) |

and add one line under the table:

> `InNewWithFrontier` is defined identically to `InNew` and may be
> implemented as an alias; it is named distinctly only to mark the INSERT
> fixpoint `j < i` read site. Unlike the OVERDELETE column — where the
> exactly-once frontier bit `kDelNow` appears in the *later*-position cell
> (`AliveAtClaim`) — the INSERT column's frontier bit `kAddNow` likewise
> appears only in the *later*-position cell (`InNewSansFrontier`). The
> earlier-position cells (`SurvivesSoFar`, `InNewWithFrontier`) carry no
> frontier bit; they are the plain "so-far" reads. Dropping the
> `(kInI && !kDel)` disjunct from `InNewWithFrontier`, or restricting its
> `kAdd` disjunct to `kAddNow`, are both unsound (loses genuine derivations /
> under-counts C_r — see the nonlinear both-added and the two-support
> retraction traces).
