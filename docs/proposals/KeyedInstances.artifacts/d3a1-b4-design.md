# D3.a.1 STAGE-(b) DESIGN — LANE b4: witness + fixtures + DS-R4-10 fence + gates

Tip `95251825` (verified `git rev-parse HEAD`, tree clean). Binding context honored:
d3a1-substrate.md (§5/§6/§7 d2 ruling, XC-3), d3a-ruling-brief.md (OD-15),
d3a-substrate.md §7:888-1001 (d1-d7), d3a0-design.md §3. The slice lands as ONE
commit (XC-3: d1 not independently landable; this design assumes the co-landed
d1+d3+d4+d5 unit from lanes b1/b2/b3). Every claim below is file:line-anchored at
tip or EMPIRICAL (prototype run under the tip debug binary, artifacts in
`scratchpad/proto/`). Prediction tags: [BYTE] byte-identical, [STRUCT]
structured-change with exact predicted shape, [COMPUTED] exact bytes produced at
design time by running tip tooling.

---
## §0 HEADLINE DECISIONS + THE TWO EMPIRICAL DISCOVERIES

**D-b4-1 (the death observable).** A probe cannot witness death — the demanded
query entry `neighborhood_bf` INJECTS demand before reading (witness driver
comment, cases/demand_neighborhood_witness.main.cpp:5-8), so any post-retract
probe of the dead key is a REBIRTH. The only demand-blind observable is the
published-delta log. Therefore d6 adds a **published `@differential` output tap
over the query relation** (`nbhd_out`), and the driver observes retraction as
negative published deltas through its own log type (the `product_diff.main.cpp`
PrintLog idiom, :15-32). This simultaneously discharges G-15 (any published
output over the demanded closure must be `@differential`, Differential.cpp:174-179
— the faithfulness check makes the attribute mandatory once `-demand-retract`
flips the closure differential per d3a1-substrate §1.4/§1.5).

**EMPIRICAL-1 (tap viability, tip).** The tap parses and compiles TODAY under
plain, `-demand`, and `-demand -demand-instance` (proto/w1.dr, rc=0 all three;
my first rc=1 was a zsh non-word-splitting artifact, per the CLAUDE.md gotcha).
The demand pass does not reject it: stray-consumer accounting covers consumers
of `p_merge` (= `edge`) only (Demand.cpp:760-790); the tap consumes the QUERY
relation via the INSERT->SELECT seam, downstream of the materialization, and
`q_rel->inserts.Size()==1` still holds (Demand.cpp:476-480). The nested fences
(Build.cpp:1393-1447) key `jl[1].CanReceiveDeletions()` / `ViewSelfReachable` —
untouched by the tap.

**EMPIRICAL-2 (the nested publish gap, tip — NEW FINDING).** At tip the tap
program run through both lowerings (proto/w1.main.cpp) gives IDENTICAL cursor
answers but DIVERGENT published deltas: flat publishes `+(1,2) +(1,3)` etc. at
each demand/edge epoch; **the nested arm publishes NOTHING** (all flushes
empty). Band-(b)'s bare `pub_member.TryAdd` (Database.cpp:2442) feeds no
delta queue, so `PublishDifferentialMessageVectors`' transmit never sees the
rows. Consequence: (a) the d6 witness FORCES the d4 publish machinery — the
born scan must go through pub's differential interface
(AddDerivation + AddQueue append, the InstantiateEffects diff-arm shape
Rel.cpp:829-848) or the eqgate diverges on every non-empty flush; (b) the
eqgate's oracle power is upgraded from answer identity to **answer + published
delta-stream identity** (driver-sorted per epoch, so permutation-safe across
modes); (c) this is one more proof the slice is a single landable unit — d6
cannot land before d4, in addition to XC-3's d1-before-d3 abort chain.

**D-b4-2 (DS-R4-10 is a DEMONSTRATED mode-split hole, not just a missing
witness).** The `@never`-over-differential reject exists in-fixpoint
(Differential.cpp:92-106) but is visit-order fragile: it only runs while the
negate's OWN `can_produce_deletions` is still false (outer guard :81).
EMPIRICAL: proto/never2.dr (@never negate whose non-negated input is
differential 1-hop, negated chain 3-hop) and never3.dr (clause-reordered)
are **MODE-SPLIT at tip: rc=1 under opt/nocf, rc=0 (COMPILES) under
nodf/none** — the uncanonicalized graph's extra forwarding TUPLEs shift the
fixpoint interleaving past the check's window, and a latent
wrong-answer program (an `@never` gate over a retractable negated view,
the F18 once-absent-always-absent assumption violated) compiles under
`-disable-dataflow-opt`. never1.dr (1-hop chain) rejects in all 4 modes.
The fence therefore MOVES post-fixpoint (final closure bits are the unique
least fixpoint — order- and mode-independent) with byte-identical diagnostic
text, and the corpus witness uses the never2 shape so it doubles as a
regression fence against reintroducing the in-fixpoint fragility.

**D-b4-3 (netting is not witness-drivable — by design).** The forcing entry
appends only to add_vec and the retract entry only to del_vec (one call = one
signed epoch), so OD-15's same-batch death+re-demand annihilation cannot be
driven from the query surface — the impossibility is STRUCTURAL, exactly as
the ruling intends. NetBatch semantics are already unit-pinned
(tests/Runtime/RuntimeTest.cpp:116-156); the handler-side NETBATCH presence is
a [STRUCT] grep gate on the witness's generated header, not a runtime witness.

**D-b4-4 (oracle stays death-blind — CONFIRMED, goldens computed).** The
retract channel is a query-surface call, never a `.batches` message; the
fabricated `demand__` message does not exist in the oracle's plain
(undemanded) evaluation. EMPIRICAL: the tip oracle accepts the tap-extended
.dr and prints ONLY `neighborhood` rows (the `nbhd_out` message adds no
printed rows); the `INVARIANT:` line goes to stderr (present for the CURRENT
witness too, absent from its committed golden — runall splits stdout/stderr,
runall.sh:219-220). Both new sidecar goldens are [COMPUTED] exactly (§1.5).

---
## §1 SUB-DIFF (i) — THE d6 WITNESS GROWTH (G-14, G-15)

### 1.1 `cases/demand_neighborhood_witness.dr` — the tap [STRUCT]

Append (plus a header-comment paragraph recording the D3.a.1 retract phase and
the G-15 constraint; keep every existing comment block):

```
; D3.a.1 RETRACT (OD-15 SET-demand): the driver retracts standing demand via
; the generated retract entry (-demand-retract in .drflags); death is observed
; through the published @differential tap below -- probes CANNOT see death
; (a probe re-injects demand = rebirth). G-15: any published output over the
; demanded closure MUST be @differential (Differential.cpp faithfulness check)
; once the fabricated demand message is differential.

#message nbhd_out(u64 Start, u64 Node) @differential
    : neighborhood(Start, Node).
```

The tap reads the #query relation (query-in-body: proven parseable+acceptable,
EMPIRICAL-1). Everything else in the .dr is untouched.

### 1.2 `cases/demand_neighborhood_witness.drflags` [STRUCT]

`-demand` -> `-demand -demand-retract`. All four diffrun modes and (via
flags_of, runall.sh:156-159 + the eqgate's extra `-demand-instance`,
runall.sh:328) all four eqgate modes then run retract-enabled — ONE driver,
eight runs, the standing flat==nested==golden transitivity intact.

### 1.3 `cases/demand_neighborhood_witness.batches` [STRUCT]

Append ONE batch (and extend the header comment: the RETRACT phase adds one
while-dead edge; the oracle is demand-blind so it sees the edge, never the
retraction):

```
batch
+ add_edge 1 15
end
```

7 batches total. The retract calls appear NOWHERE in this file (D-b4-4).

### 1.4 `cases/demand_neighborhood_witness.main.cpp` — full driver spec [STRUCT]

Keep: the copyright line, the R-a2 LOAD-BEARING STRUCTURE comment (one edge
per epoch + immediate single probe, ADJ-R3(c)), the HP-5 exact-answer asserts,
the cursor contract (drain fully; sort before printing). Add: the PrintLog,
labeled flushes after EVERY entry-point call, and the retract/rebirth/second-
death phases. Complete shape:

```cpp
// (existing header comment + a D3.a.1 paragraph: death is log-observed;
//  probes rebirth; e7 is the dead-key-edge suppression discriminator.)
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>
#include "datalog.h"

// Published-delta observer (the product_diff PrintLog idiom). Sorted flush:
// published-delta ORDER within an epoch is unspecified and mode-varying
// (permcheck's order-free-per-epoch policy), so the driver sorts each
// epoch's tokens before printing — 4-mode byte-identity by construction.
struct PrintLog {
  std::vector<std::string> rows;
  void nbhd_out_2(uint64_t S, uint64_t N, bool added) {
    rows.push_back(std::string(added ? "+(" : "-(") + std::to_string(S) +
                   "," + std::to_string(N) + ")");
  }
  void flush(const char *label) {
    std::sort(rows.begin(), rows.end());
    std::cout << label << ':';
    for (const auto &r : rows) std::cout << ' ' << r;
    std::cout << '\n';
    rows.clear();
  }
};

int main() {
  const auto allocator = hyde::rt::MallocAllocator();
  DatabaseFunctors functors;
  PrintLog log;
  Database db(allocator);
  init(db, log, functors);

  auto probe = [&](uint64_t start, std::vector<uint64_t> expected,
                   const char *label) {
    std::vector<uint64_t> nodes;
    auto c = neighborhood_bf(db, log, functors, start);
    for (uint64_t n = 0; c.next(n);) nodes.push_back(n);
    std::sort(nodes.begin(), nodes.end());
    std::sort(expected.begin(), expected.end());
    assert(nodes == expected);          // HP-5: EXACTLY neighborhood(start).
    log.flush(label);                   // deltas of the injection epoch.
    std::cout << "nbhd " << start << ':';
    for (auto n : nodes) std::cout << ' ' << n;
    std::cout << '\n';
  };
  auto send = [&](std::vector<std::pair<uint64_t, uint64_t>> es,
                  const char *label) {
    hyde::rt::Vec<add_edge_input> edges(allocator);
    for (auto &[f, t] : es) edges.Add({f, t});
    add_edge_2(db, log, functors, std::move(edges));
    log.flush(label);
  };
  auto retract = [&](uint64_t start, const char *label) {
    neighborhood_bf_retract(db, log, functors, start);   // <- b1's surface.
    log.flush(label);
  };

  // ---- BIRTH (unchanged batches; labels + flushes added) ----
  send({{1,2},{1,3},{9,9},{7,8}}, "e1");
  send({{1,4},{3,5},{3,6}}, "e2");
  probe(1, {2,3,4}, "p1");
  probe(3, {5,6}, "p3");
  probe(9, {9}, "p9");
  probe(5, {}, "p5");

  // ---- R-a2 REBUILD (unchanged; one edge per epoch, immediate probe) ----
  send({{1,11}}, "e3");   probe(1, {2,3,4,11}, "p1b");
  send({{3,7}}, "e4");    probe(3, {5,6,7}, "p3b");
  send({{5,12}}, "e5");   probe(5, {12}, "p5b");
  send({{13,14}}, "e6");  probe(9, {9}, "p9b");   // undemanded key: no leak.

  // ---- D3.a.1 RETRACT: SET-demand death (OD-15) ----
  retract(1, "r1");              // full (T,F) retract: -(1,2..11) published.
  probe(3, {5,6,7}, "p3c");      // standing keys untouched by 1's death.
  probe(9, {9}, "p9c");
  // Edge for the DEAD key in a LATER epoch: must be a NO-OP (flat: no demand
  // row joins; nested: band-(a2) must gate on demand-liveness — FindInstance
  // still returns 1's iid, no tombstone, so liveness is the ONLY correct
  // gate). assert makes a leak a hard abort, not just a golden diverge.
  send({{1,15}}, "e7");
  assert(log.rows.empty() || (std::cerr << "dead-key edge published\n", false));
  probe(5, {12}, "p5c");         // no leak of 15 anywhere.
  retract(7, "r7");              // never-demanded key: retract is a no-op
                                 //   (remove of an absent row nets to
                                 //    nothing; was==now publishes nothing).

  // ---- REBIRTH: re-demand the SAME key (band-(a1); FindOrAdd rebinds the
  //      SAME iid — no tombstone; frozen is empty post-death so ALL rows are
  //      born, INCLUDING the while-dead (1,15) — rebirth is a fresh rescan,
  //      never a stale-store replay). ----
  probe(1, {2,3,4,11,15}, "p1c");

  // ---- SECOND DEATH: the rebuilt frozen set retracts in full (death ->
  //      rebirth -> death cycling; pins (T,F) against the REBUILT frozen). ----
  retract(1, "r1b");
  probe(9, {9}, "p9d");          // store sane after the second death.
  return 0;
}
```

NOTE on `assert(log.rows.empty() ...)` at e7: written so the message renders on
failure without a helper; adjust spelling at code, the OBLIGATION is a hard
abort on any dead-key-edge publish. (The flush after `send` clears rows, so the
assert sits BETWEEN the call and the flush — implementers: move the e7 send
inline rather than through the `send` lambda, or give `send` an
`expect_silent` flag. Cleanest: a `send_expect_silent` twin that asserts
`rows.empty()` before flushing. The golden below assumes label `e7:` prints
an empty flush line either way.)

### 1.5 The predicted goldens

**`goldens/demand_neighborhood_witness.stdout` — full replacement [STRUCT,
exact predicted bytes]** (sorting note: delta tokens sort as strings, so
`(1,11)` < `(1,15)` < `(1,2)` lexicographically; answer lines stay
numerically sorted as today):

```
e1:
e2:
p1: +(1,2) +(1,3) +(1,4)
nbhd 1: 2 3 4
p3: +(3,5) +(3,6)
nbhd 3: 5 6
p9: +(9,9)
nbhd 9: 9
p5:
nbhd 5:
e3: +(1,11)
p1b:
nbhd 1: 2 3 4 11
e4: +(3,7)
p3b:
nbhd 3: 5 6 7
e5: +(5,12)
p5b:
nbhd 5: 12
e6:
p9b:
nbhd 9: 9
r1: -(1,11) -(1,2) -(1,3) -(1,4)
p3c:
nbhd 3: 5 6 7
p9c:
nbhd 9: 9
e7:
p5c:
nbhd 5: 12
r7:
p1c: +(1,11) +(1,15) +(1,2) +(1,3) +(1,4)
nbhd 1: 2 3 4 11 15
r1b: -(1,11) -(1,15) -(1,2) -(1,3) -(1,4)
p9d:
nbhd 9: 9
```

Confidence per line class: birth/rebuild delta placement (deltas appear in
the epoch that DERIVES them: demand-injection epochs carry the probe-key's
initial rows; edge epochs for standing keys carry the new row) is EMPIRICALLY
proven on flat at tip (proto run: `p1: +(1,2) +(1,3)`, `edges2: +(1,4)`).
Retract-phase lines (r1/e7/r7/p1c/r1b) are derived from the ruled semantics —
if the landed behavior deviates, the bless ritual REJECTS (reds must match
this prediction exactly; no bless of an unexplained shape).

**`goldens/demand_neighborhood_witness.oracle.stdout` [COMPUTED — exact bytes,
produced by the TIP oracle on the final .dr + final .batches]:**

```
ORACLE: OK (7 batches, 2828 assertions)
neighborhood	1 2
neighborhood	1 3
neighborhood	1 4
neighborhood	1 11
neighborhood	1 15
neighborhood	3 5
neighborhood	3 6
neighborhood	3 7
neighborhood	5 12
neighborhood	7 8
neighborhood	9 9
neighborhood	13 14
```

**`goldens/demand_neighborhood_witness.monotone.stdout` [COMPUTED]:**
`MONOTONE-PROJECTION: 12 surviving facts` + the same 12 rows in the same
order. (Both computed from proto/final.dr + proto/final.batches with the tip
`drlojekyll-oracle`, stdout only; valid unless a sibling lane touches
`bin/Oracle` — none should. If the landing's bless output differs from these
bytes, STOP: something moved in the oracle.)

### 1.6 Eqgate + witness disposition summary

- The eqgate needs NO harness change (runall.sh:328 already composes
  `.drflags + -demand-instance`); no nested golden ever blessed (OD-10/OWN-5).
- Death oracle-blind: CONFIRMED (D-b4-4); oracle/monotone churn comes ONLY
  from the one new edge batch.
- The e7 step is the directed discriminator that FORCES b3's band-(a2)
  demand-liveness gate (see §6 I-3): without it the nested arm publishes
  five born rows at e7 and both the driver assert and the eqgate fire.
- The p1c step is the rebirth-freshness discriminator (FindOrAdd rebind +
  empty frozen ⇒ all five born, including the while-dead edge).
- The r1b step pins the (T,F) full retract against a REBUILT frozen set
  (death→rebirth→death cycling; the InstanceStore death-half unit
  InstanceStoreTest.cpp:317-349 pins the runtime half — r1b pins the
  emitted band + publish half).

---
## §2 SUB-DIFF (ii) — THE DS-R4-10 FENCE RIDER (G-17, OQ-NEVER)

### 2.1 The fence edit — `lib/DataFlow/Differential.cpp` [STRUCT on lib, BYTE on all pinned surfaces]

MOVE the `@never`-over-differential reject from in-fixpoint to post-fixpoint
(grounds: the demonstrated mode-split hole, D-b4-2; the final bits are the
unique least fixpoint, order/mode-independent — never1/2/3 must ALL become
all-4-modes rc=1):

1. DELETE the reject block inside the fixpoint `@never` arm
   (Differential.cpp:92-106 — the `negate->negated_view->can_produce_deletions`
   branch with its `reported` loop). KEEP the receive→produce lift (:86-90)
   and the whole surrounding arm structure.
2. INSERT after the fixpoint loop (after :142, before the
   `report_message_errors` gate — same unconditional both-runs firing profile
   as the check it replaces; the pre-demand run at lib/DataFlow/Build.cpp:2555
   fails the compile before the post-demand :2618 run re-reports):

```cpp
  // DS-R4-10 (OQ-NEVER, ruled REJECT): '@never' assumes once-absent-always-
  // absent, so a negated view that can produce differential updates
  // invalidates the gate (its rows can appear AND retract). Checked POST-
  // fixpoint on the final closure bits: the in-fixpoint check this replaces
  // was visit-order fragile — a negate whose own can_produce flipped first
  // (e.g. from a differential non-negated input) skipped the arm forever,
  // and the uncanonicalized -disable-dataflow-opt shape slipped through
  // (mode-split accept of a latent wrong-answer program).
  if (log.IsEmpty()) {
    for (const auto &negate : negations) {          // Query.h:1141
      if (!negate->is_never ||
          !negate->negated_view->can_produce_deletions) {
        continue;
      }
      auto reported = false;
      for (ParsedPredicate pred : negate->negations) {
        if (pred.IsNegatedWithNever()) {
          reported = true;
          log.Append(pred.SpellingRange(), pred.Negation().SpellingRange())
              << "'@never' cannot operate on a predicate that can "
              << "produce differential updates";
        }
      }
      assert(reported);
      (void) reported;
    }
  }
```

Diagnostic text BYTE-IDENTICAL to today's (:100-101). Iteration over the
`DefList<QueryNegateImpl> negations` (Query.h:1141) is id-ordered ⇒
deterministic multi-violation reporting. The `log.IsEmpty()` guard preserves
today's stop-on-first-error economy (bits may be incomplete if an earlier
error stopped the fixpoint — matching the current loop's `log.IsEmpty()`
condition). No demand-path interaction: NEGATE in a demanded body is
pre-rejected upstream (Demand.cpp:624-627), so this fence and the demand
machinery never see the same negate; the fence ALSO now correctly covers a
`@never` downstream of a `-demand-retract` differential closure in FLAT mode
(previously order-dependent).

Member-name verification at code: `negations` per Query.h:1141; the loop body
mirrors the deleted block verbatim (fields `is_never`, `negated_view`,
`negations` (the ParsedPredicate list, Query.h:969), `IsNegatedWithNever`).

**Acceptance-perimeter proof for [BYTE]:** the only corpus `@never` carriers
are negate_6 and map_5 (grep; negate_cobatch_mono / negate_downstream_diff
match only in comments) — both negated views monotone ⇒ bits unchanged ⇒
zero acceptance change, zero golden churn, zero .rel churn (negate_6.rel
pinned). Programs newly rejected: exactly the never2/never3 shapes under
nodf/none (previously silently accepted latent wrong-answer terrain — a
STRENGTHENING with no corpus footprint).

### 2.2 The directed corpus case — `negate_never_diff_1` (NEW, all-4-modes-diagnostic)

`cases/negate_never_diff_1.dr` (the ORDER-HOLE shape = proto/never2.dr, so the
case regression-fences the fragility, not just the reject):

```
; Copyright 2026, Peter Goodman. All rights reserved.
;
; negate_never_diff_1 -- the DS-R4-10 fence witness (OQ-NEVER, ruled REJECT):
; '@never' over a negated view that can produce differential updates is a
; clean diagnostic in ALL FOUR modes. The shape is the directed ORDER-HOLE
; probe: the @never negate's own can_produce flips first (its non-negated
; input is @differential at one hop) while the negated view's
; differentialness arrives through a THREE-hop chain -- the pre-D3.a.1
; in-fixpoint check MISSED this under -disable-dataflow-opt (mode-split
; accept: opt/nocf rejected, nodf/none compiled the latent wrong-answer
; program). The post-fixpoint fence rejects it in every mode.

#message add(i32 A) @differential.
#message nin(i32 A) @differential.

#local pos(i32 A).
pos(A) : add(A).

#local n0(i32 A).
n0(A) : nin(A).
#local n1(i32 A).
n1(A) : n0(A).
#local neg(i32 A).
neg(A) : n1(A).

#local x(i32 A).
x(A) : pos(A), @never neg(A).

#query qx(free i32 A) : x(A).
```

`cases/negate_never_diff_1.main.cpp`: the inert diagnostic stub (the
demand_cyclic_1.main.cpp idiom — comment + `int main() { return 0; }`).
NO .drflags (the fence is mode- and flag-independent), no golden, no
.batches.

Post-fix prediction: rc=1 + the rendered diagnostic in all 4 modes (today:
rc=1 opt/nocf, rc=0 nodf/none — EMPIRICAL). If stage-(c) finds the
post-fixpoint sweep somehow still mode-split on this shape, that is a
design-refuting red, not a bless candidate.

### 2.3 `tests/OptDiff/runall.sh` edits [STRUCT]

1. :358 case list gains `|negate_never_diff_1` (alphabetic placement near the
   other negate entries is fine; the list is a `case` pattern, order-free).
2. The header's expected-diagnostic inventory (runall.sh:20-45) gains one
   line: `negate_never_diff_1` (@never over a differential negated view —
   the DS-R4-10 post-fixpoint fence; the directed order-hole shape that was
   mode-split before D3.a.1).
3. Suite count 175 → **176**; CLAUDE.md's authoritative-list sentence updates
   at the landing commit (owner-visible doc edit, not a harness edit).

---
## §3 SUB-DIFF (iii) — THE `-demand-retract` FENCE / POSITIVE MATRIX

Expected b1 flag idiom (Main.cpp): `-demand-retract` sets
`gDemandRetract = true` AND `gDemand = true` — the `-demand-instance` implies
idiom verbatim (Main.cpp:476-479). Therefore:

| combination | disposition | grounds |
|---|---|---|
| `-demand-retract` alone | NO new reject — implies `-demand` (idiom parity) | Main.cpp:478 pattern |
| `-demand-retract` + no bound query | inert (demand pass no-ops; nothing fabricated ⇒ nothing differential) | Demand.cpp pass-head gating |
| `-demand-retract` + demand_multi_adorn_1 shape | existing reject unchanged — fires at the pass head BEFORE fabrication, never reads the retract toggle | Demand.cpp:443-460 |
| `-demand-retract -demand-instance` + demand_cyclic_1 shape | FENCE (i) unchanged — keys `ViewSelfReachable(jl[0])`, graph shape only | Build.cpp:1431-1437 |
| `-demand-retract -demand-instance` + demand_diff_input_1 shape | FENCE (iii) unchanged — keys `jl[1].CanReceiveDeletions()`; a differential DEMAND does not flip the summarized INPUT (forward-only closure, d3a1-substrate §1.5) | Build.cpp:1418-1421 |
| `-demand-retract` (flat) + recursive demand (tc shape) | ACCEPT — flat differential recursion is core machinery; LABELED RESIDUAL: unwitnessed combination (no corpus case adds it; candidate directed witness if stage-(c) wants one, NOT in this slice) | ruling-brief standing referee note |
| `-demand-instance` WITHOUT `-demand-retract` (tip witness config) | must stay EXACTLY tip-behaved: monotone demand, no death mint (Rel.cpp:1139 false), byte-identical generated text | §4 predictions |

**No existing fence-case .drflags changes. No new reject for the flag itself.**
The three demand fence cases stay byte-identical in verdict and diagnostic.
One NEW positive obligation on b1 (see §6 I-1): with `-demand-retract` OFF,
the fabrication path must be BIT-IDENTICAL to tip (the 175-case perimeter +
demand_tc_witness's four pinned dump goldens are the gate).

---
## §4 SUB-DIFF (iv) — THE GATE PLAN FOR THE WHOLE SLICE

### 4.1 Pre-registered per-surface predictions

| surface | prediction |
|---|---|
| 174 non-witness case stdouts × 4 modes | [BYTE] — no other case carries `-demand-retract`; the Differential.cpp fence move changes no accepted program's bits (§2.1 perimeter) |
| `demand_neighborhood_witness.stdout` | [STRUCT] — full replacement, exact §1.5 bytes |
| `demand_neighborhood_witness.oracle.stdout` / `.monotone.stdout` | [STRUCT] — exact §1.5 [COMPUTED] bytes |
| the 20 pinned `.golden` dump surfaces (incl. demand_tc_witness ×4, negate_6.rel) | [BYTE] — none carries the flag; PRECONDITION on b2: NO new DROp KIND (census lines render kind inventories; a new kind churns all 11 .rel pins — see §6 I-5). kInstanceDeath + the kNetRemoval vec ROLE already exist (Rel.h:54-69) |
| eleven `.rel` pins census `kInstanceDeath` count | [BYTE] 0 — death mints only under `-demand-instance` + differential demand; no pinned surface is nested |
| witness generated text, flat arm | [STRUCT] — handler gains `io_remove_vec` + `::hyde::rt::NetBatch` (Procedure.cpp:557-575 arm goes live); two-polarity demand ingest folds; the retract entry function; `nbhd_out` transmit both signs |
| witness generated text, nested arm | [STRUCT] — all of the above PLUS: store ctor `instance_<id>(allocator_, false)` (Database.cpp:1460-1467 arm live); death band before band-(a1) with `RecycleCurrent` (first codegen caller); (T,F) drop scan + RAT-7 partition-belt counters in band-(b); del/add queue appends replacing bare TryAdd; region diff bit accessor use |
| `.rel` dump of the witness nested arm (unpinned; eyeball + E-71) | [STRUCT] — kInstanceDeath=1 in census; the death op renders; ANY new dump-token spelling requires the E-71 grammar note in t2b-grammar.md (b2's duty, checked at review) |
| expected-diagnostic verdicts (existing 14 + kvindex_1 split) | [BYTE] verdict lines; negate_never_diff_1 ADDS `negate_never_diff_1 all-modes-diagnostic OK` |
| ctest | **6/6 binaries** (no new binary needed by b4; InstanceStore gains an in-binary arm, §4.3; if b2 adds a validators death-unit binary the count re-registers at adjudication) |
| data/ corpus (36 files × 4 modes) | [BYTE] — no `-demand-retract` anywhere in data/ |

### 4.2 Suite reds PRE-BLESS — the exact set (the bless ritual contract)

With all lanes' code landed, ONE full-suite run on the workroot must produce
EXACTLY these red verdict lines and no others:

```
demand_neighborhood_witness opt GOLDEN-DIVERGE
demand_neighborhood_witness nodf GOLDEN-DIVERGE
demand_neighborhood_witness nocf GOLDEN-DIVERGE
demand_neighborhood_witness none GOLDEN-DIVERGE
demand_neighborhood_witness oracle GOLDEN-DIVERGE
demand_neighborhood_witness monotone MONO-DIVERGE
demand_neighborhood_witness eqgate opt NESTED-GOLDEN-DIVERGE
demand_neighborhood_witness eqgate nodf NESTED-GOLDEN-DIVERGE
demand_neighborhood_witness eqgate nocf NESTED-GOLDEN-DIVERGE
demand_neighborhood_witness eqgate none NESTED-GOLDEN-DIVERGE
```

(10 lines: diffrun continues per-mode, runall --one continues past a failed
diffrun into oracle/eqgate — runall.sh:371-377, diffrun.sh:44-100.) Any
RUN-FAIL here is a DRIVER-ASSERT abort = implementation bug, never a bless
candidate; any red on ANY other case = design defect, stop.
negate_never_diff_1 must be GREEN on this same run (its runall.sh listing
lands with the code).

**The bless ritual (verbatim script):**
1. `DR=build/debug/bin/drlojekyll tests/OptDiff/runall.sh /tmp/wr 6` →
   confirm `SUITE: FAIL` with EXACTLY the 10 lines above.
2. Diff the produced outputs against the §1.5 predictions:
   `diff <workroot>/demand_neighborhood_witness.opt/stdout` vs the predicted
   golden (must be equal BYTES); same for the oracle/monotone stdouts vs the
   [COMPUTED] blocks; confirm all four modes' stdouts are IDENTICAL to each
   other; confirm all four eqgate stdouts equal the flat stdout (this is the
   flat==nested check passing EARLY, before the golden catches up).
3. ONLY then: `tests/OptDiff/runall.sh --bless /tmp/wr demand_neighborhood_witness`
   (blesses stdout + oracle + monotone; eqgate is never blessed —
   runall.sh:77-126).
4. Re-run the full suite → `SUITE: PASS` (176 cases), zero residual reds.
5. Deviation anywhere (an 11th red, a byte off prediction) = STOP, back to
   design; never bless-to-green (CLAUDE.md blessing rule).

### 4.3 ctest additions (the netting/death-coupling unit)

NetBatch SET-netting is already pinned (RuntimeTest.cpp:116-156) and the
death-half is pinned (InstanceStoreTest.cpp:317-349). The NEW arm covers the
three-way-coupling primitives the witness exercises only end-to-end
(tests/InstanceStore/InstanceStoreTest.cpp, same binary — ctest stays 6/6):

```
TEST(InstanceStore, DeathRebirthCycleRebindsIidAndSuppressesRescan)
  Store st(MallocAllocator(), /*monotone=*/false);
  g = FindOrAdd(K{1}); fill {2,3,4}; Seal();               // birth.
  st.RecycleCurrent(g);
  ASSERT_TRUE(st.TouchedFlag(g));       // THE a1/a2 suppression signal is UP
                                        //   post-Recycle (OQ-DEATH-VS-REBUILD:
                                        //   death's Touch is load-bearing).
  Seal();                               // death epoch: frozen empty,
  ASSERT_FALSE(st.SealedOccupied(g));   //   occupancy died,
  ASSERT_FALSE(st.TouchedFlag(g));      //   flag cleared by Seal (:205).
  ASSERT_EQ(st.FindInstance(K{1}), g);  // no tombstone: key still bound.
  ASSERT_EQ(st.FindOrAddInstance(K{1}), g);  // rebirth REBINDS the same iid.
  fill {2,3,4,15} via TouchCurrent; Seal();  // rebirth epoch (fresh rescan).
  ASSERT_EQ(Drain(st.Frozen(g)), {2,3,4,15});
  st.RecycleCurrent(g); Seal();              // SECOND death cycles clean.
  ASSERT_FALSE(st.SealedOccupied(g));
  st.DebugValidate();
```

(Existing arms already pin Recycle idempotence and the belt-fires negative;
this arm adds TouchedFlag observability across the death epoch + the
rebind-same-iid law + second-death cycling — the runtime mirror of witness
steps r1/p1c/r1b.) If b2 lands a death-band VALIDATOR unit (V-INST-DRAIN
removals clause etc.), that is b2's file; b4 registers no dependency.

### 4.4 ASAN (standing gate, both surfaces)

build/asan: full 176-case suite (compiler under ASAN) + all six ctest units +
the witness's eqgate (generated code under ASAN via the suite's driver
builds). Slice-specific attention: the death path (RecycleCurrent's
Reset/swap + the (T,F) scan reading frozen while pub retracts) is exactly
use-after-free terrain — an ASAN eqgate pass is a hard gate, 2 sweeps per the
standing cadence.

### 4.5 Config-invariance

3-run single-hash (identical bytes across three same-config compiles) +
debug==release dump identity on: demand_tc_witness (standing), PLUS the
moving carriers: demand_neighborhood_witness FLAT (`-demand -demand-retract`)
and NESTED (`+ -demand-instance`), both on `datalog.h` + the `.rel`/`.ir`
dumps. The death band + netting emission must be config-stable-silent.

### 4.6 Q5 — MUST RUN (generated + compiler bytes move)

`bench/runbench.sh` progsize@128, RELEASE binaries, SAME-SESSION INTERLEAVED
ABABAB, baseline A = tip 95251825 release snapshot, B = the slice. Grounds:
D3.a.0 waived Q5 by byte-identity; this slice moves Differential.cpp (a
per-compile fixpoint), Demand.cpp fabrication, and codegen — the waiver is
void. Expectation: noise-band (the fence sweep is O(#negates); the demand
machinery is flag-off in the progsize program). A >2% regression is a stop.

### 4.7 Liveness-by-perturbation checklist (d7, G-16 — run once, record in ledger)

| # | validator | procedure | expected |
|---|---|---|---|
| L1 | V-INST-DIFF-COHERENCE (Procedure.cpp:274-285) | scratch-worktree: negate `inst.differential` at the region-ctor call; rebuild; compile the witness NESTED | fprintf names the store + both bits; abort. Revert. (The TRUE-bit true==true path is now exercised by every witness run — vacuity lifted.) |
| L2 | OWN-3 fold abort | standing ctest death test (GuardAnnotationFoldTest, fork/waitpid) re-run on the slice tree | SIGABRT + record print (unchanged; multi-guard folds stay dormant until D3.a.3 — record the residual) |
| L3 | RAT-7 partition belt (b3's band-(b) counters) | compile witness nested; sed the GENERATED datalog.h to suppress one born-counter increment (or skip one drop); rebuild driver; run | the belt's fprintf+abort naming the store/iid; the un-perturbed suite run is the belt's green half |
| L4 | HP-7 belt OFF for the differential store | [STRUCT] grep: witness nested header contains `instance_<id>(allocator_, false)`; the DEBUG suite run executes r1/r1b shrinks without the monotone belt aborting | shrink survives debug = selector live (G-13 discharged observably) |
| L5 | V-INST-DRAIN removals clause (b2) | scratch: disable the demand kNetRemovals provisioning; compile witness nested | abort citing the removals frontier (b2 owns the exact text; b4 owns demanding the perturbation happen) |
| L6 | XC-3 abort-order confirmation | scratch: build d1-ONLY (toggle without d3/d4) once during integration; compile witness nested | FIRST abort is V-INST-DRAIN (Rel.cpp:4509-4512), then after hand-provisioning, V-INST-EMITTED — recording this validates the substrate's §2.3 chain empirically |
| L7 | dead-key a2 suppression | ALREADY a driver hard-assert (witness e7) — runs every suite pass | no perturbation needed; the assert IS the teeth |

### 4.8 Standing-gate roll-call (slice-specifics only)

- SUITE PASS(176) all 4 modes, debug tree; error-grep 0 across trees.
- eqgate LIVE ×4 — now refereeing answer + sorted delta-stream identity.
- 20/20 pinned dump regen [BYTE].
- ctest 6/6 debug + ASAN (with the new InstanceStore arm).
- E-62 clean (no `include/drlojekyll/DeltaRel/` surface).
- E-71: any new `.rel` token spelling (death render) gets its grammar note.
- data/ 36-file × 4-mode compile [BYTE].
- Nested-arm generated text: NOT byte-identical this slice (unlike D3.a.0) —
  the [STRUCT] inventory in §4.1 is the review checklist; there is no
  byte-neutrality claim to make.

---
## §5 SIBLING-INTERFACE EXPECTATIONS (for the adjudicator)

- **I-1 (b1, the retract surface).** A generated per-adornment retract entry,
  spec'd here as `neighborhood_bf_retract(db, log, functors, Start)` → void
  (name/shape b1's call — the driver in §1.4 tracks whatever b1 lands; it
  must exist under BOTH lowerings and all 4 modes, flag-gated on
  `-demand-retract`, populate ONLY the del_vec, and RUN THE FLOW in the same
  call so the negative deltas publish within the retract epoch). The
  fabricated message's own ABI stays suppressed (the registry at the
  kMessageHandler sites) — the retract entry is the sole remove-side writer.
  Flag idiom: `-demand-retract` implies `-demand` (§3). Flag-off:
  fabrication bit-identical to tip (the 175-case + tc-witness-goldens gate).
- **I-2 (b2, kinds + frontier).** NO new DROp kind (census-line churn on all
  11 `.rel` pins otherwise — if unavoidable, pre-register the churn and the
  permcheck disposition BEFORE the run). The demand kNetRemovals frontier +
  the kNetAdditions re-provisioning must both land (XC-3); L5/L6
  perturbations owed. E-71 note for any death-render spelling.
- **I-3 (b3, the a2 liveness gate — WITNESS-FORCED).** Band-(a2) must gate
  edge-triggered rebuild on DEMAND-LIVENESS (the demand table's membership
  for the key), NOT on FindInstance existence (no tombstone ⇒ dead keys
  still bind iids) and NOT on occupancy (alive-and-empty key 5 must keep
  rebuilding — already pinned by e5/p5b). Witness step e7 + its hard assert
  is the directed probe; a gate omission = driver abort + eqgate diverge +
  golden diverge, triply loud.
- **I-4 (b3, the publish path — WITNESS-FORCED, EMPIRICAL-2).** Band-(b)
  born/dropped rows must flow pub's differential machinery
  (AddDerivation/SubDerivation + Add/Del queue appends per the
  InstantiateEffects diff arm, Rel.cpp:829-848) so the commit sweep +
  transmit publish BOTH signs — at tip the nested arm publishes NOTHING
  downstream of pub (proven, §0). Drop-before-born per touched iid
  (OQ-PUBLISH-ORDER) is invisible to the sorted driver but belt-checked by
  RAT-7.
- **I-5 (all).** The witness + fence + harness edits land IN the single slice
  commit (XC-3 + EMPIRICAL-2 both forbid partial landings; a temporary fence
  is NOT wanted and no blocker requiring one was found).

## §6 EDIT-SPEC INVENTORY (b4-owned files)

| file | change |
|---|---|
| tests/OptDiff/cases/demand_neighborhood_witness.dr | +tap `#message nbhd_out ... @differential : neighborhood(...)` + header paragraph (§1.1) |
| tests/OptDiff/cases/demand_neighborhood_witness.drflags | `-demand -demand-retract` (§1.2) |
| tests/OptDiff/cases/demand_neighborhood_witness.batches | +1 batch `+ add_edge 1 15` + comment (§1.3) |
| tests/OptDiff/cases/demand_neighborhood_witness.main.cpp | full driver per §1.4 (PrintLog + labeled flushes + retract/rebirth/second-death phases; keeps ADJ-R3(c) structure + HP-5 asserts + e7 hard assert) |
| goldens/demand_neighborhood_witness.stdout | replaced via bless; predicted bytes §1.5 |
| goldens/demand_neighborhood_witness.oracle.stdout | replaced via bless; [COMPUTED] §1.5 |
| goldens/demand_neighborhood_witness.monotone.stdout | replaced via bless; [COMPUTED] §1.5 |
| lib/DataFlow/Differential.cpp | the DS-R4-10 post-fixpoint move (§2.1) |
| tests/OptDiff/cases/negate_never_diff_1.dr | NEW diagnostic case (§2.2) |
| tests/OptDiff/cases/negate_never_diff_1.main.cpp | NEW inert stub (§2.2) |
| tests/OptDiff/runall.sh | :358 list + header inventory (§2.3) |
| tests/InstanceStore/InstanceStoreTest.cpp | +TEST DeathRebirthCycleRebindsIidAndSuppressesRescan (§4.3) |
| CLAUDE.md | suite count 176 + diagnostic-list sentence (landing commit) |

Prototype artifacts backing the empirical claims: scratchpad/proto/{w1.dr,
w1.main.cpp, never1.dr, never2.dr, never3.dr, final.dr, final.batches,
final.oracle.stdout, final.monotone.stdout} — flat/nested runs + 4-mode rc
matrices reproduced in this doc.

