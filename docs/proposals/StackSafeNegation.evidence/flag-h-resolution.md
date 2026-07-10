# FLAG-H resolution: k emissions per delta position vs single dynamic dispatch

Branch `derivation-counters`, HEAD 680916c. Snapshot binaries only.
Oracle: `scratchpad/oracle-slice0`. Fixture: `flag-fixtures/nonlin_tc_both_change.{dr,batches}`.
Ground truth: `flag-fixtures/truth/nonlin_tc_both_change.differential.txt`.

---

## 0. The two verbatim §5.1 passages (from `docs/proposals/StackSafeNegation.md`)

### 5.1 header + position order (MD:192-202, verbatim)

> ### 5.1 The delta schemas (exactly-once firing enumeration)
>
> Each rule body has a fixed total position order: **lower-strata atoms first**
> (in stratum order, then syntactic order), **then same-stratum atoms** in
> syntactic order. Negated atoms always occupy lower-stratum positions
> (guaranteed by Stratify). There are two enumeration schemas, both instances
> of the telescoped signed-delta identity
> `New(B₁…Bₙ) − Old(B₁…Bₙ) = Σᵢ New^{<i} ⊗ Δᵢ ⊗ Old^{>i}` where
> `Δᵢ = Newᵢ − Oldᵢ` is a **signed** set:

### The fixpoint schema table (MD:204-212, verbatim)

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
> whose two same-stratum body atoms are claimed **in the same round** is still
> counted exactly once: with the earlier position as delta, the later one
> passes `AliveAtClaim`; with the later as delta, the earlier fails
> `SurvivesSoFar`. Both schemas are one generated pattern per rule; a missing
> term is a schema bug visible on every program, not a per-site hand-derivation.

### The Σᵢ note in the telescoped identity (MD:192, above)

The identity is `Σᵢ New^{<i} ⊗ Δᵢ ⊗ Old^{>i}` — an explicit **sum over
delta positions i**. Each term of the sum is one join evaluation with the
delta pinned at position `i`, earlier positions reading `New`-flavored state,
later positions reading `Old`-flavored state. This Σᵢ is the textual source
of the "k emissions" reading: the schema IS a sum indexed by delta position.

### Predicate definitions (MD:93-96, verbatim)

>   bool SurvivesSoFar(uint32_t id) const;       // kInI && !kDel
>   bool AliveAtClaim(uint32_t id) const;        // kInI && (!kDel || kDelNow)
>   bool InNewWithFrontier(uint32_t id) const;   // (kInI&&!kDel)||kAdd
>   bool InNewSansFrontier(uint32_t id) const;   // (kInI&&!kDel)||(kAdd&&!kAddNow)

(`InNew(id) := (kInI&&!kDel)||kAdd`, MD §3.1 — identical to `InNewWithFrontier`;
that is FLAG-F, not FLAG-H, and is settled elsewhere as reading (i)/alias.)

### OVERDELETE / INSERT pseudocode fixpoint loops (MD:230-274, verbatim excerpt)

>   loop:                                   # recursive rules only
>       Δ_D := { id ∈ N_D : TryClaimDel(id) };  N_D := ∅   # sets kDel|kDelNow
>       if Δ_D empty: break
>       D_s += Δ_D
>       for each fixpoint-schema firing (OVERDELETE column), delta over Δ_D:
>           d := SubDerivation(head, kRecursive)
>           if d.crossed: N_D += d.id
>       RetireDelFrontier(Δ_D)              # clear kDelNow; swap vectors

Note the pseudocode line: **"for each fixpoint-schema firing ... delta over
Δ_D"** — "each fixpoint-schema firing" is precisely one firing per
(rule, delta-position, delta-row) triple, and "delta over Δ_D" restricts the
delta-row to the per-round claimed frontier, not the raw drained queue.

---

## 1. What the two candidate schemes are

The rule under test:

```
p(From, To) : p(From, Y), p(Y, To).      -- both body atoms are same-stratum p
```

Total position order: no lower-stratum atoms; two same-stratum positions
`p_0 = p(From,Y)` (position 0) and `p_1 = p(Y,To)` (position 1).

**Scheme A (k emissions per delta position — my reading of §5.1).**
For a k-same-stratum-atom rule, the generated code emits **k** distinct
fixpoint-join sections, one per choice of "which same-stratum atom is the
delta this evaluation." For k=2:

- Emission E_0 (delta = position 0): position 0 pinned to a Δ_D/Δ_A row;
  position 1 (`j > 0`) read with the **later**-position predicate
  (`AliveAtClaim` for OVERDELETE, `InNewSansFrontier` for INSERT).
- Emission E_1 (delta = position 1): position 1 pinned to a Δ_D/Δ_A row;
  position 0 (`j < 1`) read with the **earlier**-position predicate
  (`SurvivesSoFar` for OVERDELETE, `InNewWithFrontier` for INSERT).

Each emission is statically specialized: the read-predicate assignment per
position is fixed at compile time by the position's static before/after
relationship to that emission's delta position. This is exactly the oracle's
`for (unsigned p : r->same_pos) { reads = FixReads(r, p, deleting); ... }`
loop, with `FixReads` returning the per-position predicate vector as a pure
function of `p` (`Main.cpp:1329-1343`, `Overdelete`:1384-1400, `InsertPhase`:1454-1470).

**Scheme B (single emission, dynamic first-changed dispatch).**
A single fixpoint-join section per rule, structurally like (b)'s
`added_body`/`removed_body`: fires per combination in which "all sides pass
some per-side predicate AND ≥1 side is in the current Δ", with the per-side
predicate chosen *dynamically per scanned row* (e.g. "is THIS side the delta
side / the first-changed side"), no compile-time-fixed delta position. This
is the "(b) increment-4 style generalized to rounds" the task names.

---

## 2. Discriminating batch trace — BATCH 2 (both-deleted crossing)

### 2.1 State at end of batch 1 (the from-scratch materialization)

Edges (base `p` seed via `p:edge`): 1→2, 2→3, 3→4.
Recursive `p` closure over the chain 1→2→3→4:

```
p rows and their derivation counters at batch-1 commit (kInI=true for all):
  p(1,2)  C_nr=1 (edge 1,2)                                  C_r=0
  p(2,3)  C_nr=1 (edge 2,3)                                  C_r=0
  p(3,4)  C_nr=1 (edge 3,4)                                  C_r=0
  p(1,3)  C_r=1   [ p(1,2),p(2,3) ]
  p(2,4)  C_r=1   [ p(2,3),p(3,4) ]
  p(1,4)  C_r=2   [ p(1,2),p(2,4) ]  AND  [ p(1,3),p(3,4) ]   <-- double-derived
```

`p(1,4)` has multiplicity 2 (two distinct same-stratum join instances). This
is the multiplicity the exactly-once machinery must decrement to exactly 0,
not −1 or +1, when its supports vanish.

### 2.2 Batch 2 ops: `− edge(2,3)`, `− edge(3,4)`. OVERDELETE.

**Seed phase** (`− seeds`, lower position = `edge`, class kNonRecursive).
edge's net-removals frontier = {(2,3),(3,4)}. The base rule `p:edge` seed
fires `SubDerivation(p(2,3), NR)` and `SubDerivation(p(3,4), NR)`:

```
  p(2,3): C_nr 1→0 ; kInI && C_nr<=0  → enqueue delQ[p]
  p(3,4): C_nr 1→0 ; kInI && C_nr<=0  → enqueue delQ[p]
```

delQ[p] = {p(2,3), p(3,4)} entering the fixpoint loop.

**OVERDELETE fixpoint round R1.** Drain + claim:
`Δ_D(R1) = { p(2,3), p(3,4) }` (both have C_nr=0 so TryClaimDel succeeds;
sets kDel|kDelNow on both). D_s = {p(2,3), p(3,4)}.

Now the fixpoint firing. **THIS is the discriminating step: both p(2,3) and
p(3,4) are same-stratum delta rows in the SAME round.** We trace both schemes.

Relevant candidate instances of `p(From,Y),p(Y,To)` that involve a claimed
row this round. The claimed rows are p(2,3) and p(3,4). Enumerate which
join instances have a claimed row at position 0 or position 1:

- Instance `p(1,2),p(2,3)` → head p(1,3). p(2,3) at position 1.
- Instance `p(1,3),p(3,4)` → head p(1,4). p(3,4) at position 1.
- Instance `p(2,3),p(3,4)` → head p(2,4). p(2,3) at pos 0, p(3,4) at pos 1
  — **BOTH sides claimed this round** (the double-claim case).
- Instance `p(2,4)` derivations: p(2,4)=[p(2,3),p(3,4)] is the one above.
- Instance `p(1,2),p(2,4)` → head p(1,4). p(2,4) NOT claimed this round
  (its C_r is still 1 at this point; it only gets enqueued later). Skip for R1.
- Instance `p(1,3),p(3,4)` counted above (head p(1,4)).

#### Scheme A, round R1

Two emissions per rule; delta ranges over Δ_D(R1) = {p(2,3), p(3,4)}.

**Emission E_1 (delta = position 1, later position is delta; position 0 read
`SurvivesSoFar = kInI && !kDel`).** Delta row iterates {p(2,3), p(3,4)}:

- delta = p(2,3) at pos1 ⇒ From free, Y=2 pinned by delta's From-col... trace
  the join key: position1 is p(Y,To) with Y bound to delta.From=2, To=delta.To=3.
  Position0 is p(From,Y=2). Scan p(_,2) with `SurvivesSoFar`: only p(1,2)
  (kInI, kDel=false). Instance p(1,2),p(2,3) → head p(1,3).
  `SubDerivation(p(1,3), R)`: C_r 1→0, kInI, → enqueue delQ. **fires once.**
- delta = p(3,4) at pos1 ⇒ Y=3, To=4. Position0 p(From,3) with SurvivesSoFar:
  p(1,3) (kInI, kDel=false — not yet claimed) and p(2,3)? p(2,3) has kDel=true
  now (claimed this round) so `SurvivesSoFar` = kInI && !kDel = FALSE → excluded.
  So only p(1,3),p(3,4) → head p(1,4). `SubDerivation(p(1,4), R)`: C_r 2→1.
  Not crossed (still >0). **fires once (this is one of p(1,4)'s two supports).**

**Emission E_0 (delta = position 0, earlier position is delta; position 1
read `AliveAtClaim = kInI && (!kDel || kDelNow)`).** Delta iterates {p(2,3),p(3,4)}:

- delta = p(2,3) at pos0 ⇒ From=2, Y=3. Position1 p(3,To) with AliveAtClaim:
  p(3,4) — kInI, kDel=true BUT kDelNow=true this round → AliveAtClaim = TRUE.
  Instance p(2,3),p(3,4) → head p(2,4). `SubDerivation(p(2,4), R)`: C_r 1→0,
  kInI → enqueue delQ. **fires once.** (This is the same-round double-claim
  instance, fired via the EARLIER position as delta, exactly as MD:212 says.)
- delta = p(3,4) at pos0 ⇒ From=3, Y=4. Position1 p(4,To) with AliveAtClaim:
  no p(4,_) exists. No fire.

Round R1 firings summary (Scheme A): p(1,3)↓0, p(1,4)↓(2→1), p(2,4)↓0.
New crossings enqueued: delQ = {p(1,3), p(2,4)}.

**Critical: the double-claim instance p(2,3),p(3,4)→p(2,4) fired EXACTLY
ONCE** — in E_0 (earlier=delta, later passes AliveAtClaim), and NOT in E_1
(later=delta p(3,4)@pos1, earlier p(2,3)@pos0 read via SurvivesSoFar which is
FALSE because p(2,3).kDel is set this round). This is the same-round
exactly-once mechanism: `SurvivesSoFar` has NO kDelNow escape hatch, so the
"later as delta" evaluation is suppressed; `AliveAtClaim` DOES have the
kDelNow hatch, so the "earlier as delta" evaluation fires. One firing total. ✓

**OVERDELETE round R2.** Δ_D(R2) = claim {p(1,3), p(2,4)} (both C_nr=0,
C_r=0 now; TryClaimDel succeeds). D_s += {p(1,3), p(2,4)}. Fire:

- E_1 delta p(1,3)@pos1 (Y=1,To=3): p(From,1) SurvivesSoFar — none. No fire.
- E_1 delta p(2,4)@pos1 (Y=2,To=4): p(From,2) SurvivesSoFar — p(1,2). Instance
  p(1,2),p(2,4) → p(1,4). `SubDerivation(p(1,4), R)`: C_r 1→0, kInI → enqueue.
  **fires once (p(1,4)'s SECOND support, now removed).**
- E_0 delta p(1,3)@pos0 (From=1,Y=3): p(3,To) AliveAtClaim — p(3,4) has kDel
  set but kDelNow was RETIRED at end of R1 → AliveAtClaim = kInI &&(!kDel||kDelNow)
  = false. No fire. (Correct: p(1,3),p(3,4)→p(1,4) already counted in R1's E_1.)
- E_0 delta p(2,4)@pos0 (From=2,Y=4): p(4,To) — none. No fire.

delQ after R2 = {p(1,4)}.

**OVERDELETE round R3.** Δ_D(R3) = {p(1,4)} (C_r=0). No further instances
have a claimed row at a scanned position that fires (p(1,4) at pos0 needs
p(4,_): none; at pos1 needs p(_,1): none). delQ empty. **Round R4: Δ_D empty
→ break.**

D_s (overdeleted) = {p(2,3),p(3,4),p(1,3),p(2,4),p(1,4)}. Every C_r/C_nr for
these is exactly 0 (no counter went negative — every SubDerivation matched a
real support exactly once). **REDERIVE**: none have C_r>0, so none restored.
**INSERT seed**: edge net-additions empty this batch, nothing. Final batch-2
p = {p(1,2)} only. reachable after batch 2 = {(1,2)}.

Cross-check against oracle: the oracle ran this exact batch with 1410 total
assertions across 4 batches, all passing, including per-row
`C_nr>=0 && C_r>=0` at every commit and per-batch incremental==from-scratch.
No negative-counter SIGABRT ⇒ every decrement above matched a support exactly.
✓

#### Scheme B, round R1 (single emission, dynamic dispatch) — where it breaks

Scheme B fires a single section: "all sides pass a per-side predicate AND ≥1
side ∈ Δ_D(R1)", with the per-side predicate chosen dynamically. Consider the
double-claim instance p(2,3),p(3,4)→p(2,4), where BOTH sides ∈ Δ_D(R1).

The only per-side dynamic predicate that has no static notion of "the delta
position" must treat both sides symmetrically. Two sub-cases for how Scheme B
could define its per-side test:

- **B-lax** ("a side qualifies if it is InI-ish OR is itself in Δ", i.e. the
  literal generalization of (b)'s `removed_body` = "all sides InI AND ≥1 side
  net-deleted"). Here the section fires ONCE per combination where ≥1 side is
  claimed. But the enumeration must decide the delta binding: if it iterates
  "≥1 side ∈ Δ" as a driver, the instance p(2,3),p(3,4) qualifies under
  **both** drivers (p(2,3) claimed AND p(3,4) claimed). Without a
  position-relative asymmetry (SurvivesSoFar-fails-for-earlier), the natural
  set-at-a-time firing enumerates the instance once when driven by p(2,3) and
  again when driven by p(3,4) — **two firings** ⇒ `SubDerivation(p(2,4))`
  runs twice ⇒ C_r 1 → 0 → **−1**. Under the commit-time non-negativity
  assert (`Main.cpp:1497`, plan Commit assert) this **SIGABRTs**: "negative
  counter at commit." End-state-visible as a crash. ✗

- **B-guard** (add an ad-hoc "already-fired this instance" dedup, e.g. only
  fire when the delta side is the lexicographically-first claimed side).
  This is no longer "(b) increment-4 style" — it is a hand-rolled
  reconstruction of the position discipline, but done *dynamically per row*
  rather than *statically per emission*. It can be made to fire once, but
  now it must also correctly handle the R2 case where p(2,4) (from R1) joins
  with an already-final p(1,2): there is no "delta side" that is claimed
  *this* round for the instance p(1,2),p(2,4)→p(1,4) UNLESS p(2,4) is the
  driver — and p(2,4) is claimed in R2, p(1,2) never. B-guard's dynamic
  "first-changed" test must then read p(1,2) as "already present, standing"
  — which is exactly `SurvivesSoFar`/`AliveAtClaim`, i.e. B-guard is forced
  to reintroduce the very position-relative predicate table it claimed to
  avoid, just evaluated per-row instead of per-emission. It produces the
  right answer ONLY by re-deriving Scheme A's predicate matrix, at which
  point it is Scheme A with worse codegen (a per-row branch on
  before/after-delta status that is a compile-time constant). It gains
  nothing and risks getting the branch wrong.

**Conclusion from batch 2**: Scheme B in its honest "(b)-style, no delta
position" form (B-lax) DOUBLE-FIRES the same-round double-claim instance and
drives C_r negative — a commit SIGABRT, end-state-visible. To avoid that it
must smuggle back the position discipline (B-guard ≡ A). Scheme A fires the
instance exactly once by construction via the SurvivesSoFar/AliveAtClaim
asymmetry.

---

## 3. Discriminating batch trace — BATCH 3 (both-added crossing)

### 3.1 State at start of batch 3

After batch 2: p = {p(1,2)} (kInI). edges present: 1→2.

### 3.2 Batch 3 ops: `+ edge(4,5)`, `+ edge(5,6)`. INSERT.

**Seed phase** (`+ seeds`, lower position = edge, class NR): edge
net-additions = {(4,5),(5,6)}. Base rule `p:edge`:

```
  AddDerivation(p(4,5), NR): C_nr 0→1, before<=0 && after>0 → enqueue addQ
  AddDerivation(p(5,6), NR): C_nr 0→1, → enqueue addQ
```

addQ[p] = {p(4,5), p(5,6)}.

**INSERT fixpoint round R1.** Claim: Δ_A(R1) = {p(4,5), p(5,6)}
(TryClaimAdd sets kAdd|kAddNow on both). A_s = {p(4,5), p(5,6)}.

The target instance is `p(4,5),p(5,6) → p(4,6)`, derivable ONLY by combining
the two rows claimed in THIS SAME round — the canonical same-round
double-claim INSERT case.

#### Scheme A, round R1 (INSERT column)

Two emissions; delta over Δ_A(R1); predicates `InNewWithFrontier` (earlier
`j<i`) / `InNewSansFrontier` (later `j>i`).

**Emission E_0 (delta = position 0; position 1 read
`InNewSansFrontier = (kInI&&!kDel)||(kAdd&&!kAddNow)`).** delta ∈ {p(4,5),p(5,6)}:

- delta = p(4,5)@pos0 (From=4,Y=5): position1 p(5,To) with InNewSansFrontier.
  Candidate p(5,6): kInI=false, kAdd=true, but kAddNow=TRUE (claimed THIS
  round) → `kAdd && !kAddNow` = FALSE, and `kInI&&!kDel`=FALSE →
  InNewSansFrontier = FALSE. **p(5,6) EXCLUDED at the later position.** No fire.
- delta = p(5,6)@pos0 (From=5,Y=6): position1 p(6,To) — none. No fire.

**Emission E_1 (delta = position 1; position 0 read
`InNewWithFrontier = (kInI&&!kDel)||kAdd`).** delta ∈ {p(4,5),p(5,6)}:

- delta = p(5,6)@pos1 (Y=5,To=6): position0 p(From,5) with InNewWithFrontier.
  Candidate p(4,5): kAdd=true → InNewWithFrontier = TRUE (kAdd disjunct;
  kAddNow irrelevant to this predicate). Instance p(4,5),p(5,6) → head p(4,6).
  `AddDerivation(p(4,6), R)`: C_r 0→1, before<=0 && after>0 → enqueue addQ.
  **fires once.** ✓
- delta = p(4,5)@pos1 (Y=4,To=5): position0 p(From,4) — none. No fire.

**The same-round double-claim instance p(4,5),p(5,6)→p(4,6) fires EXACTLY
ONCE**: in E_1 (later position p(5,6) is delta, earlier position p(4,5) read
via `InNewWithFrontier` which passes on the kAdd disjunct), and NOT in E_0
(earlier position p(4,5) is delta, later position p(5,6) read via
`InNewSansFrontier` which FAILS because kAddNow is set this round). Mirror of
batch-2's OVERDELETE mechanism, mirrored sign. ✓

This also directly exercises **FLAG-F**: the earlier-position read
`InNewWithFrontier` MUST include the `kAdd` disjunct (=`InNew`) for E_1 to
fire p(4,6). If FLAG-F reading (ii) had narrowed `InNewWithFrontier` to
`(kInI&&!kDel)||(kAdd&&kAddNow)`, E_1 would STILL fire here (p(4,5).kAddNow is
set this round, so the narrowed form also passes) — so batch 3 alone does not
kill reading (ii). But note the exactly-once split still holds under both
FLAG-F readings, because the split is driven by `InNewSansFrontier`'s
`!kAddNow` (unchanged between readings), not by `InNewWithFrontier`. FLAG-H's
exactly-once conclusion is therefore INDEPENDENT of FLAG-F's resolution —
good, the two flags are orthogonal.

**INSERT round R2.** Δ_A(R2) = {p(4,6)} (claim). Fire: p(4,6) at pos0 needs
p(6,_): none; at pos1 needs p(_,4): none. addQ empty. **R3: break.**

Final batch-3 p = {p(1,2), p(4,5), p(5,6), p(4,6)}.
reachable after batch 3 = {(1,2),(4,5),(5,6),(4,6)}.

#### Scheme B, round R1 (INSERT)

Same structural failure as batch 2, mirrored: a single dynamic-dispatch
emission with no static delta position either (B-lax) double-fires p(4,6)
(driving C_r to 2 — over-derivation; here it does not go negative, but it
inflates the counter to a WRONG MULTIPLICITY: p(4,6) genuinely has exactly
ONE derivation, and a C_r of 2 means a later single retraction of one edge
leaves a phantom p(4,6) surviving — end-state-visible in a LATER batch that
removes one of edge(4,5)/edge(5,6)), or (B-guard) must reconstruct the
`InNewWithFrontier`/`InNewSansFrontier` asymmetry per-row ≡ Scheme A.

Concretely for a later-batch consequence: if C_r(p(4,6))=2 wrongly, then
removing edge(4,5) alone (SubDerivation only decrements p(4,6) once via the
seed→fixpoint cascade) leaves C_r(p(4,6))=1 > 0, so p(4,6) survives when it
must vanish — a spurious reachable(4,6). That is exactly the end-state
divergence the fixture is engineered to expose (DESIGN.md §3(i)). Under
Scheme A this cannot happen: p(4,6) reaches C_r exactly 1.

---

## 4. Same-round exactly-once mechanism (both schemes), stated precisely

The question "what stops an instance from firing under BOTH positions'
emissions in scheme A / both dispatch arms in scheme B":

**Scheme A (the specified scheme) — the mechanism is the SurvivesSoFar vs
AliveAtClaim (resp. InNewWithFrontier vs InNewSansFrontier) asymmetry across
the two emissions, keyed on the `kDelNow`/`kAddNow` frontier bit:**

For a 2-atom instance whose BOTH sides are claimed in the same round R:
- In emission E_earlier (earlier side is delta), the LATER side is read with
  the **permissive** predicate — `AliveAtClaim` (OVERDELETE) / `InNewWithFrontier`
  (INSERT) — which passes a same-round-claimed row (via `kDelNow` OR-clause for
  OVERDELETE; via the plain `kAdd` disjunct for INSERT). **→ fires.**
- In emission E_later (later side is delta), the EARLIER side is read with the
  **strict** predicate — `SurvivesSoFar` (OVERDELETE, `kInI && !kDel`, NO
  kDelNow hatch) / `InNewSansFrontier` (INSERT, excludes `kAddNow`) — which
  FAILS a same-round-claimed row. **→ does not fire.**

Net: exactly one firing. This is precisely MD:212 verbatim: "with the earlier
position as delta, the later one passes `AliveAtClaim`; with the later as
delta, the earlier fails `SurvivesSoFar`." Verified in the oracle's `EvalRead`
(`Main.cpp:254-259`): `kSurvivesSoFar = in_i && !del` (no del_now),
`kAliveAtClaim = in_i && (!del || del_now)` (del_now hatch),
`kInNewSansFrontier = (in_i&&!del)||(add && !add_now)` (excludes add_now),
`kInNewWithFrontier = (in_i&&!del)||add` (includes any add).

The asymmetry is STATIC — which side gets the permissive vs strict predicate
is fixed at compile time by position order (`FixReads` computes it from `p`
alone). No per-row dispatch, no runtime "which fired first" bookkeeping. Two
emissions, one fires, guaranteed by the predicate table.

**Scheme B has no clean mechanism.** With no static delta position, a single
emission that fires "≥1 side in Δ" over a symmetric per-side predicate either
double-counts (both sides in Δ ⇒ instance enumerated under both drivers) or
must reintroduce an explicit asymmetric per-side test — i.e. reconstruct the
Scheme A predicate table dynamically.

**The additional cross-round guard (Δ_D/Δ_A, not the raw queue).** Orthogonal
but co-required: firing ranges over the per-round CLAIMED frontier (Δ_D/Δ_A =
`TryClaim*` successes), never the raw drained queue. `TryClaimDel`/`TryClaimAdd`
(`Main.cpp:1279-1301`) are idempotent CAS on `kDel`/`kAdd`: a re-enqueued
already-claimed row returns false and is NOT re-added to the round. This stops
a diamond re-enqueue (a round-<k row re-appended by a round-k crossing) from
re-firing (the `diamond_reenqueue` fixture). It is a DIFFERENT exactly-once
axis (across rounds) from the SurvivesSoFar/AliveAtClaim axis (within a round);
both are needed. The plan (`plan.md:701-713`) and the checkpoint-(c) notes
corrected CF-IR (B3) both mandate the Δ_D discipline.

---

## 5. Which scheme the docs actually specify, and is it correct

**Specified scheme: A (k emissions per delta position).** Three independent
sources agree:

1. **MD §5.1 text**: the schema is a **sum over positions** Σᵢ (MD:192), the
   fixpoint table's rows are indexed **by the delta position i** and the
   per-position predicate is a function of the static `j<i` / `j>i` relation
   (MD:204-212). The pseudocode line "for each fixpoint-schema firing ...
   delta over Δ_D" (MD:240) reads as one firing per (position, delta-row).
2. **The oracle** (`bin/Oracle/Main.cpp`, an independent executable
   resolution): `Overdelete`/`InsertPhase` loop `for (unsigned p : r->same_pos)`
   — one `EnumerateRule(r, p, delta, ...)` per same-stratum position p, with
   `FixReads(r, p, deleting)` returning the per-position predicate vector as a
   pure function of p (`Main.cpp:1329-1400, 1454-1470`). This is Scheme A
   verbatim: k emissions, static per-position predicate assignment.
3. **plan.md** (owner-authored, amendment A3 + the "third join-section flavor"
   discussion) and **checkpoint-c-notes.md** treat the fixpoint firing as
   ranging over Δ_D with the position-relative predicate table — Scheme A.

**Is Scheme A correct?** Yes, verified end-to-end against the oracle ground
truth on the discriminating fixture:
- Oracle final state `reachable = {(4,5),(4,6),(4,7),(5,6),(5,7),(6,7)}`,
  1410 per-batch cross-assertions PASS (incremental==from-scratch, every
  counter == from-scratch instance count, non-negativity, frontier drainage),
  and `--project-monotone` (positive-only over net surviving inputs) agrees
  byte-for-byte (`INVARIANT: differential-final == monotone-projection`).
- My Scheme-A hand-trace of batches 2 and 3 reproduces exactly the counter
  multiplicities the oracle's non-negativity asserts require: p(1,4) decrements
  2→1→0 (never −1), p(4,6) increments 0→1 (never 2). The same-round
  double-claim instances (p(2,3),p(3,4)→p(2,4) in batch 2;
  p(4,5),p(5,6)→p(4,6) in batch 3) each fire exactly once under Scheme A.
- Scheme B (B-lax) double-fires those instances: batch 2 drives C_r(p(2,4)) to
  −1 (commit SIGABRT), batch 3 drives C_r(p(4,6)) to 2 (spurious survival on a
  later single-edge retraction). Both are end-state / assert visible. Scheme B
  is therefore INCORRECT unless it degenerates into Scheme A (B-guard).

**The precise emission rule to write into the plan:**

> The third (fixpoint) join-section flavor is built as **k separate emissions
> per recursive-stratum join, one per same-stratum delta position** p (k =
> number of same-stratum body atoms). Each emission pins the delta atom at
> position p to a row of the current per-round claimed frontier (Δ_D for the
> `overdelete_fixpoint_body`, Δ_A for the `insert_fixpoint_body`) and reads
> every OTHER same-stratum atom with a predicate fixed at compile time by its
> static position relative to p:
>   - same-stratum j < p : `SurvivesSoFar` (OVERDELETE) / `InNewWithFrontier` (INSERT)
>   - same-stratum j > p : `AliveAtClaim`  (OVERDELETE) / `InNewSansFrontier` (INSERT)
>   - lower-stratum j     : `InNew` (both phases; negated ⇒ absent-in-`InNew`)
> Every such emission's `UPDATECOUNT` is `kRecursive`. It is NOT a single
> emission with dynamic per-side dispatch; a `removed_body`/`added_body`-style
> per-side symmetric predicate would double-fire same-round double-claim
> instances (C_r → negative in OVERDELETE ⇒ commit SIGABRT, or an inflated
> multiplicity in INSERT ⇒ a later retraction leaves a phantom row).
>
> The same-round exactly-once guarantee is delivered structurally by the
> `SurvivesSoFar`/`AliveAtClaim` (resp. `InNewWithFrontier`/`InNewSansFrontier`)
> asymmetry across the k emissions, keyed on `kDelNow`/`kAddNow`: for an
> instance with two same-stratum atoms claimed in the same round, the
> earlier-as-delta emission fires (later atom passes the permissive predicate
> via its frontier bit) and the later-as-delta emission does not (earlier atom
> fails the strict predicate, which has no frontier-bit escape hatch).
> Orthogonally, all k emissions must range over the per-round CLAIMED frontier
> Δ_D/Δ_A (TryClaim* successes), never the raw drained queue, so a diamond
> re-enqueue of an already-claimed row cannot re-fire across rounds.

FLAG-H is thus settled: **k emissions, static per-position predicate**, with
the same-round mechanism being the strict/permissive predicate asymmetry (not
a dedup guard). This is fixture-confirmed against the positive-only oracle.
