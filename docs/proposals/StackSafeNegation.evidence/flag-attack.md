# Adversarial attack on the FLAG-F and FLAG-H resolutions

Branch `derivation-counters`, HEAD 680916c. Snapshot binaries only:
`scratchpad/oracle-slice0` (drlojekyll-oracle). Attack fixtures under
`scratchpad/flag-fixtures/attack/`.

**Verdict: resolutions-survive.** No attack produced a wrong final IDB, a
counter-exactness violation, a double-fire, or a zero-fire that the oracle's
positive-only ground truth confirms. Every hand-traced attack under the
RESOLVED semantics agreed with the oracle (differential + `--project-monotone`).

---

## 0. Method and why the oracle is a fair adversary here

Both resolutions ground their claim in `bin/Oracle/Main.cpp`, and I read it to
confirm the oracle IS the resolved semantics, then to find an INDEPENDENT check
inside it that could catch a wrong resolution even when the differential path
"agrees with itself":

- `Pass()` (Main.cpp:252-263) implements the six read predicates. `kInNew` and
  `kInNewWithFrontier` are byte-identical `(in_i&&!del)||add` (line 255 vs 258)
  — this is FLAG-F reading R-i, hard-coded. `kInNewSansFrontier` excludes
  `add_now`; `kSurvivesSoFar` has no `del_now`; `kAliveAtClaim` has the
  `del_now` hatch.
- `FixReads(r,p,deleting)` (Main.cpp:1329-1343) returns the per-position read
  vector as a pure function of the static position `p` — `q<p` → SurvivesSoFar/
  InNewWithFrontier, `q>p` → AliveAtClaim/InNewSansFrontier, lower → InNew. This
  is FLAG-H Scheme A.
- `Overdelete`/`InsertPhase` (Main.cpp:1348-1475) loop `for (unsigned p :
  r->same_pos)` emitting one `EnumerateRule(r, p, delta, ...)` per same-stratum
  delta position over the per-round CLAIMED frontier `round` (= `TryClaim*`
  successes, line 1372-1380 / 1442-1450), never the raw drained queue. This is
  FLAG-H Scheme A + the Δ_D/Δ_A claim discipline, exactly the resolution.

So the differential path == resolved semantics. The FAIR, INDEPENDENT ground
truth inside the oracle is `CheckAgainstScratch` (Main.cpp:1701-1763), run
EVERY batch:

- **Presence** (line 1738-1743): incremental `in_i` must equal from-scratch
  semi-naive presence.
- **Per-class counters** (line 1744-1752): incremental `(c_nr, c_r)` must equal
  the from-scratch instance count PER CLASS, where the from-scratch count is
  computed by `EnumerateRule(r, -1, ...)` — **delta_pos = -1, i.e. the FULL
  join over all present rows, NOT the k-emission schema** (Main.cpp:1708-1718).
- **Non-negativity + drainage at commit** (Main.cpp:1495-1509).

The counter check is the load-bearing independent oracle: it compares Scheme
A's k-emission accumulated `c_r` against the honest full-join instance count. If
Scheme A over/under-counts (a double-fire or zero-fire), `s.c_r != er` fires
`FailMismatch` and the oracle aborts nonzero. `--project-monotone` is the
separate coarser final-state check. A kill therefore = oracle aborts, OR
differential final ≠ monotone final. Neither happened on any attack.

---

## 1. Attacks attempted (each: shape, oracle truth, outcome)

All fixtures live in `scratchpad/flag-fixtures/attack/`. "Oracle OK + monotone
agree" means: differential run exits 0 with the `INVARIANT: differential-final
== monotone-projection` line AND the `--project-monotone` final relation dump is
identical.

### A1 `k3_join` — three-way same-stratum join, k=3 (prioritized surface)

Rule `p(A,D):p(A,B),p(B,C),p(C,D)` (the ONLY recursive rule; k=3 same-stratum
positions). Batch1 adds a 3-hop chain 1→2→3→4 in one batch (the three middle
p-rows are all freshly claimed in the same INSERT round → a same-round TRIPLE
claim). Batch2 retracts edge(2,3).
- Oracle truth: `reachable={(1,2),(3,4)}` (no 3-hop head survives after the
  middle is cut). differential + monotone identical, 460 assertions, exit 0.
- **Survived.** Hand-trace under Scheme A: for the instance with all three
  positions in Δ_A, FixReads fires it exactly once — only when the LAST
  position (pos2) is the delta (pos0,pos1 read InNewWithFrontier=pass on kAdd;
  pos2 pinned), because delta=pos0 or pos1 has a `j>delta` position read via
  InNewSansFrontier which fails on kAddNow. Exactly-once holds for k=3.

### A2 `k3_mixed` — k=3 AND k=2 recursive rules together

`p(A,C):p(A,B),p(B,C)` plus `p(A,D):p(A,B),p(B,C),p(C,D)`. A head like p(1,4)
now has multiple derivations of different arity/structure (two 2-way + one
3-way). Batch1 builds chain 1→2→3→4; batch2 removes edge(2,3); batch3 restores.
- Oracle truth: full closure `{(1,2),(1,3),(1,4),(2,3),(2,4),(3,4)}` at end;
  differential + monotone identical, 1096 assertions, exit 0.
- **Survived.** The mixed-multiplicity head's `c_r` matches the full-join count
  every batch (per-class assert would have caught any Scheme-A miscount).

### A3 `selfloop` — diagonal binding (row is delta at two positions same round)

Nonlinear TC with a self-loop edge 2→2, so p(2,2) exists and can bind to BOTH
pos0 and pos1 of `p(2,Y),p(Y,2)` with Y=2. Batch1 adds 1→2, 2→2, 2→3 together;
batch2 removes the self-loop; batch3 removes 1→2.
- Oracle truth: `reachable={(2,3)}` at end; 682 assertions; monotone agrees.
- **Survived.** The physical row p(2,2) at both positions does not double-count:
  FixReads pins the delta at exactly one position per emission, and the
  same-round exactly-once asymmetry keeps the diagonal instance counted once.

### A4 `cycle2` — self-supporting 2-cycle with self-derived p(x,x)

Nonlinear TC over 1↔2. Add-only (batch `cycle2_add`) materializes the full
closure INCLUDING p(1,1),p(2,2) (self-derivations via p(1,2),p(2,1)). Then the
3-batch `cycle2` retracts both edges.
- Oracle truth (add-only): `{(1,1),(1,2),(2,1),(2,2)}`, monotone identical.
  Full 3-batch: drains to `{}`, monotone 0 facts, agree. 628 assertions.
- **Survived.** The non-well-founded self-supporting cycle drains to empty when
  its only base support is removed — REDERIVE correctly returns C_r=0 for the
  entire cycle (no phantom self-support survives).

### A5 `cycle3fw` — self-supporting 3-cycle WITH a firewall, mid-sequence

3-cycle 1→2→3→1 plus `pbase(1)` giving p(1,1) an independent C_nr firewall.
Batch1 seeds cycle+base; batch2 cuts edge(2,3); batch3 removes the base;
batch4 restores edge(2,3). Exercises REDERIVE + C_nr firewall refusal + cycle
re-formation.
- Oracle truth: full 3×3 closure at end; differential + monotone identical,
  **2014 assertions**, exit 0.
- **Survived.** The most stressful REDERIVE/firewall fixture built; every
  self-derivation counter tracks the full-join truth through cut/restore.

### A6 `reroute` — same-batch mixed add+remove that reroutes a path

Nonlinear TC. Batch2 simultaneously removes edge(2,3) AND adds a detour
2→5→3, so p(2,3)... (actually reroutes reachability through node 5) in one
batch — OVERDELETE and INSERT both act on overlapping heads in one epoch.
- Oracle truth: `{(1,2),(1,5),(2,5),(3,4)}`; differential + monotone identical,
  1265 assertions.
- **Survived.** kDelNow/kAddNow do not cross-contaminate across the two phases
  in one epoch.

### A7 `k3_mixedround` — k=3 with delta at pos2 fresh, pos0/pos1 prior (kInI)

Batch1 builds 1→2→3 (p(1,2),p(2,3) become kInI). Batch2 adds edge(3,4): the
3-way instance p(1,2),p(2,3),p(3,4) now has its EARLIER positions kInI (not
kAdd) and its LAST position fresh — directly exercises InNewWithFrontier's
`(kInI&&!kDel)` disjunct at pos0/pos1 (the R-iv discriminator, generalized to
k=3). Batches 3-4 retract.
- Oracle truth: `{(3,4)}` at end; differential + monotone identical, 1130
  assertions.
- **Survived.** Dropping the kInI disjunct (FLAG-F reading R-iv) would zero-fire
  this k=3 head; the oracle's count matches the full-join truth, confirming R-i.

### A8 `phantom` — REDERIVE at recursive support (firewall-free)

`p(1,3)` has both C_nr=1 (direct edge 1→3) AND C_r=1 (via p(1,2),p(2,3)).
Batch2 removes ONLY edge(1,3): the seed OVERDELETE drops C_nr to 0, but C_r=1
must keep p(1,3) alive via REDERIVE. Batch3 removes edge(2,3) to finally drain.
- Oracle truth: after batch2 p(1,3) survives on recursive support; after batch3
  only `{(1,2)}` remains; differential + monotone identical, 530 assertions.
- **Survived.** REDERIVE's `c_r>0` post-quiescence restore (Main.cpp:1407-1417)
  correctly keeps the recursively-supported row when its seed support vanishes.

### A9 `longchain` — many-round INSERT, k=3, prior-round siblings at earlier pos

6-node chain 1→2→…→6 added in ONE batch (forces many INSERT fixpoint rounds;
p-rows added in round r are read at earlier positions in round r+1 — the exact
R-ii separator territory, generalized to k=3). Batch2 cuts a middle edge,
batch3 restores.
- Oracle truth: differential + monotone final relation dumps IDENTICAL
  (verified by `diff`), **3027 assertions**, exit 0.
- **Survived.** A prior-round earlier-position sibling (kAdd, kAddNow=false
  after retire, kInI=false) is still read as "new" by InNewWithFrontier via the
  kAdd disjunct — restricting to kAddNow (R-ii) would under-count c_r here and
  trip the per-class assert; it did not, confirming R-i.

### A10 Fuzz stress (the decisive volume test)

`oracle --stress <seed> <rounds>` drives seeded random mixed add/remove batches
(8-node domain, 1/3 removes on differential messages). Any FAIL = the resolved
semantics diverged from from-scratch on SOME batch. Results:

| program | seeds | rounds | ran (non-timeout) | FAILs |
|---|---|---|---|---|
| `nonlin_tc_both_change` (k=2 nonlinear) | 1..200 | 25 | 200 | **0** |
| `diamond_reenqueue` | 1..200 | 25 | 200 | **0** |
| `firewall_cycle` | 1..200 | 25 | 200 | **0** |
| `cycle2` (self-deriving) | 1..200 | 25 | 200 | **0** |
| `selfloop` (diagonal) | 1..200 | 25 | 200 | **0** |
| `k3_mixed` (k=3) — spot 40 seeds | 1..40 | 30 | 40 | **0** |

Total ~1040 stress runs, **0 failures**. (k=3-over-8-nodes is dense; some seeds
hit the per-run wall-clock kill and are excluded from `ran`, not counted as
passes or fails. A deeper 300-seed×60-round run over the bounded programs was
launched as a bonus; the 25-round×200-seed sweep already exercises deep
multi-round fixpoints, diamond re-enqueues, same-round double/triple claims,
and phantom pairs at random.)

---

## 2. Why each prioritized attack surface did not yield a kill

- **k=3 three-way joins.** FixReads is a pure function of the static position
  `p` for any arity; the exactly-once telescoping fires a full-fresh k-tuple
  once (only the last-position-as-delta emission passes, all earlier positions
  pass InNewWithFrontier on kAdd and there is no `j>delta` position to fail on
  InNewSansFrontier). The per-class counter assert (full-join count, delta=-1)
  matched Scheme A's accumulated count on every k=3 attack (A1,A2,A7,A9).

- **A row in the delta frontier of two positions same round.** Even the
  diagonal p(2,2) binding at both positions (A3) fires once per emission with
  exactly one pinned delta; the strict/permissive asymmetry
  (SurvivesSoFar/AliveAtClaim, InNewWithFrontier/InNewSansFrontier) is keyed on
  static position, not on which physical row, so a self-binding row cannot
  double the instance.

- **Claim-round boundary (row claimed round k, read by a round-k emission at
  another position).** This is precisely the same-round double/triple-claim
  case (A1,A3,longchain A9). The permissive later-position predicate reads the
  same-round-claimed sibling via its frontier bit; the strict earlier-position
  predicate rejects it — net exactly one firing. Verified by non-negativity +
  per-class asserts holding.

- **Phantom pairs inside a fixpoint round / seed-round↔fixpoint-round
  boundary.** A8 (recursive support surviving a seed overdelete via REDERIVE)
  and A6 (same-epoch overdelete+insert reroute) both held; REDERIVE's
  post-quiescence `c_r>0` read restores exactly the self-consistent set.

- **Diamond re-enqueue.** The `diamond_reenqueue` fixture and its 200-seed
  stress held: firing over the CLAIMED Δ_D (TryClaimDel idempotent CAS,
  Main.cpp:1279-1292) absorbs re-enqueues so a round-<k row re-appended by a
  round-k crossing does not re-fire.

---

## 3. What a kill would have looked like (and did not appear)

- FLAG-F broken: a same-stratum earlier-position sibling that is kInI-from-a-
  prior-batch (kAdd=false) driving a zero-fire (R-iv) → the head would be
  MISSING from `reachable` vs monotone; OR a prior-round kAdd-not-kAddNow
  sibling driving an under-count (R-ii) → the per-class `c_r` assert would
  abort, or a later single-support retraction would drive `c_r` negative
  (commit SIGABRT) / leave a phantom row. A7 and A9 exercised exactly these and
  the oracle stayed OK with monotone agreement.
- FLAG-H broken: a same-round double/triple-claim instance firing under two
  drivers (Scheme-B-lax double-fire) → `c_r` negative in OVERDELETE (commit
  SIGABRT) or inflated in INSERT (phantom survival on later single retraction).
  A1,A3,A9 and all the same-round-claim stress batches exercised this; no abort,
  no phantom, monotone agreement everywhere.

None materialized. The resolutions' claims are consistent with the positive-only
oracle ground truth on every fixture I could construct, including all three
prioritized hard surfaces (k=3, dual-position same-round delta, claim-round
boundary) and the REDERIVE/firewall/phantom/diamond paths.

---

## 4. Honest caveats on the strength of this negative result

1. The oracle IS the resolved semantics for the differential path, so it cannot
   catch a bug that is IN BOTH the oracle and the resolution's shared reading.
   The independent leverage comes from (a) the from-scratch full-join counter
   check (delta=-1) and (b) `--project-monotone`, both of which are genuinely
   separate evaluators. Every attack was checked against BOTH. A resolution bug
   that is invisible to the full-join count AND to the positive-only projection
   would survive — but such a bug is by definition end-state-and-count
   invisible, which is outside the "wrong final IDB / counter-exactness
   violation / double-fire / zero-fire" kill criteria of this task.
2. I could not run the actual (c) CF-IR (not built; build dir off-limits). The
   hand-trace of the RESOLVED semantics is what I compared, and the resolved
   semantics is faithfully the oracle's `Overdelete`/`InsertPhase`/`FixReads`.
   A divergence between the eventual `lib/` lowering and this resolved
   semantics is a separate (implementation-fidelity) question this attack does
   not and cannot address.
