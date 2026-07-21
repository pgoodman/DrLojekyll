======================================================================
COMMITTED AT THE §20 EPOCH-CLOSE CHECKPOINT (2026-07-21). This is the
adjudicated BINDING implementation contract for its diff, verbatim from
the session scratchpad (d2c/d2c-design.md); its ADJUDICATION LEDGER records every
folded amendment, and the KeyedInstances.md §19(K)-(O) landing records
carry the owner rulings (RAT-1..RAT-10) that resolved its open items.
======================================================================

D2.c DESIGN — WITNESS + EQUIVALENCE GATE ENTER THE SUITE
========================================================
Designer: D2.c, 2026-07-21. Repo /Users/pag/Code/DrLojekyll, branch
keyed-instances, tip 53d95864 (verified: `git rev-parse HEAD` =
53d95864c203fda5f7538291c213ae0c5335596e; D1.a..D2.b all landed).

Binding inputs re-read at tip: d1-pinned.md HP-5 / HP-18 / §3 D2.c entry
(:328); d1-design-consolidated.md §A.6.2 (:495) / §A.6.4 (:519) / §B D2.c
(:652); KeyedInstances.md §19(N) (:2459) incl. RAT-6 / RAT-7 and the
recomposed fence set. All programs/drivers/diagnostics below were COMPILED
AND RUN at tip; the goldens are hand-captured from those green runs (never
blessed here — bless is an implementation-time act, from a green suite).

EVERYTHING BELOW IS EMPIRICALLY VERIFIED at tip unless marked "(bless-time)".

======================================================================
0. SUMMARY OF DECISIONS (the questions the brief poses)
======================================================================
- Witness .dr: COPIED into cases/ (not referenced) with its header comment
  AMENDED per RAT-6 (the flap/option-2 narrative retired → birth-only). The
  artifact copy at docs/proposals/KeyedInstances.artifacts/ stays as the
  historical record; cases/ owns the live copy.
- .eqgate sidecar format: a PRESENCE-GATED MARKER (comment-only). run_eqgate
  keys on `[ -f cases/<name>.eqgate ]`; the nested flags are fixed
  (`-demand-instance`), so no config lines are needed — mirroring how a bare
  `.batches`/`.drflags`/`.irgold` presence drives its arm. Content is a
  human-readable comment documenting the arm (never parsed).
- Oracle applies (§2): the witness carries .batches, so run_oracle runs; the
  oracle NEVER sees the drflags (runall.sh:49-52 / diffrun mold), computes the
  FULL closure with DisableDataFlowOpt (Oracle Main.cpp:740), and its golden
  certifies the closure ground truth that the demand-scoped .stdout is a
  per-key subset of. Three goldens: .stdout (demand-scoped driver),
  .oracle.stdout (full relation), .monotone.stdout (add-only → identical set).
- Fence set (RAT-6 / §19(N) recomposition — demand_midstream_edge_1 does NOT
  exist): {demand_cyclic_1, demand_recursive_content_1, demand_diff_input_1},
  each all-4-modes-diagnostic, rc=1, exact texts verified below.
- Suite 169 → 170 (witness) → 173 (three fences).
- NO nested golden is blessed (OD-10/OWN-5): the eqgate compares live stdouts.

======================================================================
1. THE WITNESS CASE FILES (exact contents)
======================================================================

### 1.1 tests/OptDiff/cases/demand_neighborhood_witness.dr
(copied from the artifact; header AMENDED per RAT-6 — the option-2 demand-flap
/ REBUILD / DEATH narrative is struck, replaced by the birth-only contract;
the program body is byte-identical to the artifact.)

----------------------------------------------------------------------
; Copyright 2026, Peter Goodman. All rights reserved.
;
; demand_neighborhood_witness -- the D2.c NESTED-lowering witness and the
; -demand-instance equivalence-gate carrier (KeyedInstances.md sec 19; the
; .eqgate sidecar re-drives it under `-demand -demand-instance`). The
; NON-RECURSIVE directed neighborhood shape (witness-deltarel-target.md sec 1):
; one bound query, one monotone hop, add-only input -- inside every stage-(b)
; R-MONO fence (A5 monotone input, acyclic demand, single adornment).
; demand_tc_witness stays the RECURSIVE flat witness; the nested lowering
; fences it out (its demanded body is recursive -> cyclic-demand reject).
;
; RAT-6 BIRTH-ONLY (KeyedInstances.md sec 19(N)): under R-MONO the "demand
; flap" rebuild is an if-crossed NO-OP (a re-asserted demand never re-seeds),
; so the witness is ENFORCED birth-only -- the .batches/.main.cpp land ALL
; edges (in two epochs) BEFORE any demand probe; there is NO edge-after-demand
; batch. Edge-after-demand is the LOUD labeled feature gap (plumbing arrives
; with the DeltaRel->Rel epoch), NOT exercised here.
;
; HP-5: the graph carries edges OUTSIDE each probed key's neighborhood, and
; the driver asserts each probe's answer is EXACTLY neighborhood(Start), so an
; over-materialized nested arm both aborts the driver AND diverges from the
; golden. The demand-ON answer for a key is a STRICT SUBSET of all edges;
; the closure oracle (never demand) referees that subset's ground truth.
;
; Under flat `-demand` this compiles and answers; the nested `-demand-instance`
; lowering must be ANSWER-IDENTICAL on the same batches -- the two-lowerings
; equivalence gate (run_eqgate), refereed live: flat == nested == golden.

#message add_edge(u64 From, u64 To).

#local edge(u64 From, u64 To).
edge(From, To) : add_edge(From, To).

#query neighborhood(bound u64 Start, free u64 Node) : edge(Start, Node).
----------------------------------------------------------------------

### 1.2 tests/OptDiff/cases/demand_neighborhood_witness.drflags
----------------------------------------------------------------------
-demand
----------------------------------------------------------------------
(the FLAT arm's flags; the eqgate nested arm appends `-demand-instance`. This
also flows to all four golden modes — demand stays orthogonal, never a 5th
mode, per the existing .drflags convention.)

### 1.3 tests/OptDiff/cases/demand_neighborhood_witness.batches
(add_edge ONLY; BIRTH coverage; multiple source keys; out-of-neighborhood
edges ALWAYS present per HP-5; TWO edge batches before any probe — multi-epoch
seals. This is the oracle's input; the driver sends the SAME edge stream.)

[ADJ:W2] Seal-surface attribution (crit-witness-2 UPHELD-MINOR; verified in the
nested datalog.h at tip — flow_46 lines ~256-274): the driver exercises BOTH
seal surfaces, but the mechanism was mis-stated as "multi-epoch seals" from the
two edge batches. Precise attribution: EVERY entry point (init, each edge batch,
each probe) runs proc_19 → flow_46 → instance_0.Seal() + table_8.Seal() +
table_11.Seal(). The TWO edge batches therefore drive TWO monotone edge-table
commit sweeps (table_8/table_11), each with an EMPTY touched-instance set (vec29
of demanded keys is empty on an edge batch — no FindOrAddInstance fires). Instance
CREATION (FindOrAddInstance) fires only on the FOUR distinct-key probes (1/3/9/5),
each a separate InstanceStore instance + its instance-Seal. Net: two monotone
edge-table seal epochs + four instance-creation/seal events — both surfaces
covered; only the attribution is tightened here.
----------------------------------------------------------------------
# demand_neighborhood_witness -- oracle input batches (the SAME add_edge stream
# the driver sends, BIRTH-ONLY: two edge epochs, NO post-demand edge). The
# oracle evaluates the plain (undemanded) program, so its neighborhood rows are
# the FULL single-hop relation -- the answer-identity referee for the demand-ON
# driver's per-key answers.
#
# Batch 1: 1's first two out-edges, the 9->9 self-loop, the detached 7->8.
batch
+ add_edge 1 2
+ add_edge 1 3
+ add_edge 9 9
+ add_edge 7 8
end
# Batch 2: 1's third out-edge and 3's out-edges (out-of-neighborhood for key 1).
batch
+ add_edge 1 4
+ add_edge 3 5
+ add_edge 3 6
end
----------------------------------------------------------------------
Graph: 1->{2,3,4}, 3->{5,6}, 9->9, 7->8. All targets = {2,3,4,5,6,8,9}.
HP-5 discrimination: neighborhood(1)={2,3,4} is a STRICT subset of all targets;
an over-materialized nested arm that ignored the key would emit non-neighbors.

### 1.4 tests/OptDiff/cases/demand_neighborhood_witness.main.cpp
(the demand_tc_witness.main.cpp mold; 4-arg demand query entry
`neighborhood_bf(db, log, functors, Start)` VERIFIED at tip — a demanded query
carries a forcing function so it takes (log, functors); message entry
`add_edge_2(db, log, functors, Vec<add_edge_input>)`, `add_edge_input =
Tup_u64_u64`. HP-5 asserts added: each probe asserts the drained answer ==
the exact neighborhood. RAT-6 birth-only: all edges before any probe.)
----------------------------------------------------------------------
// Driver for demand_neighborhood_witness (compiled with `-demand` via .drflags;
// the .eqgate sidecar additionally re-drives it under `-demand -demand-instance`
// and byte-compares the two stdouts). The demanded query entry
// neighborhood_bf(db, log, functors, Start) first INJECTS Start as demand
// (seeding the fabricated demand message through the synthesized injector, which
// runs the flow), then reads the answer.
//
// RAT-6 BIRTH-ONLY: every add_edge lands (in two epochs) BEFORE any demand
// probe -- there is NO edge-after-demand batch (that is the labeled feature
// gap; plumbing arrives with Rel). HP-5: the graph carries edges OUTSIDE each
// probed key's neighborhood, and each probe ASSERTS its drained answer is
// EXACTLY neighborhood(Start) -- an over-materialized nested arm aborts here
// (and also diverges from the golden). Cursor contract: drain fully before the
// next entry-point call; sort keyed drains before printing.

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

#include "datalog.h"

int main() {
  const auto allocator = hyde::rt::MallocAllocator();
  DatabaseFunctors functors;
  DatabaseLog log;
  Database db(allocator);
  init(db, log, functors);

  auto probe = [&](uint64_t start, std::vector<uint64_t> expected) {
    std::vector<uint64_t> nodes;
    auto c = neighborhood_bf(db, log, functors, start);
    for (uint64_t n = 0; c.next(n);) {
      nodes.push_back(n);
    }
    std::sort(nodes.begin(), nodes.end());
    // HP-5: the answer must be EXACTLY neighborhood(start) -- no over- or
    // under-materialization. A demand-scoping bug trips this before the golden.
    std::sort(expected.begin(), expected.end());
    assert(nodes == expected);
    std::cout << "nbhd " << start << ':';
    for (auto n : nodes) {
      std::cout << ' ' << n;
    }
    std::cout << '\n';
  };

  // Epoch 1: 1's first out-edges, the 9->9 self-loop, the detached 7->8.
  {
    hyde::rt::Vec<add_edge_input> edges(allocator);
    edges.Add({1, 2});
    edges.Add({1, 3});
    edges.Add({9, 9});
    edges.Add({7, 8});
    add_edge_2(db, log, functors, std::move(edges));
  }
  // Epoch 2: 1's third out-edge and 3's out-edges. BIRTH-ONLY: all edges are in
  // before any probe; the two epochs exercise multi-epoch seals.
  {
    hyde::rt::Vec<add_edge_input> edges(allocator);
    edges.Add({1, 4});
    edges.Add({3, 5});
    edges.Add({3, 6});
    add_edge_2(db, log, functors, std::move(edges));
  }

  probe(1, {2, 3, 4});  // multi-neighbor; a STRICT subset of all targets.
  probe(3, {5, 6});     // a neighbor-of-1 whose own edges are out-of-nbhd for 1.
  probe(9, {9});        // self-loop; over-materialization would add non-9 nodes.
  probe(5, {});         // 5 is a target but never a source -> empty answer.
  return 0;
}
----------------------------------------------------------------------
Probe design satisfies every brief item:
  - probe(1)={2,3,4}: multiple neighbors, strict subset (HP-5).
  - probe(3)={5,6}: 3 is a NEIGHBOR of key 1, yet 3's own edges {5,6} are
    OUT-OF-NEIGHBORHOOD for the probe-1 answer — "a key probed but whose edges
    are out-of-neighborhood for another probe".
  - probe(9)={9}: self-loop discrimination — over-materialization adds non-9.
  - probe(5)={}: an EMPTY answer (5 is a target, never a source).
VERIFIED: flat rc=0, nested rc=0, both stdouts byte-identical, all asserts pass.

### 1.5 tests/OptDiff/cases/demand_neighborhood_witness.eqgate
(PRESENCE-GATED MARKER; content is documentation only, never parsed.)
----------------------------------------------------------------------
# Equivalence-gate marker (KeyedInstances.md sec 19; d1-design-consolidated
# A.6.2). Presence of THIS FILE turns on run_eqgate in runall.sh --one:
# the nested arm (.drflags + -demand-instance) is built+run with the SAME driver
# in ALL FOUR optimization modes ([ADJ:H1]), and each mode's stdout is
# byte-compared against goldens/demand_neighborhood_witness.stdout. diffrun
# already proves flat == golden per mode, so nested == golden per mode gives
# flat == nested transitively across all four modes. No golden of its own -- the
# nested stdout is refereed LIVE, never blessed (OD-10/OWN-5). Tokens
# NESTED-GOLDEN-DIVERGE / EQGATE-*-FAIL / EQGATE-GOLDEN-MISSING surface at the
# summary grep.
----------------------------------------------------------------------

======================================================================
2. THE ORACLE / MONOTONE GOLDENS QUESTION (answered from source)
======================================================================
Q: Does run_oracle apply to a demand-ON case, and how do its full-closure
goldens reconcile with demand scoping?

A (verified in bin/Oracle/Main.cpp + runall.sh):
- run_oracle fires on ANY case with a .batches sidecar (runall.sh:196-201).
  The witness has .batches, so BOTH the oracle and the monotone projection run.
- The oracle NEVER receives the .drflags. runall.sh:49-52 states it, and the
  code proves it: `run_oracle` invokes `"$ORACLE" "$DRC" "$batches"` with no
  mode/demand flags (runall.sh:208, 228). Oracle Main.cpp:740 builds the query
  with `hyde::PassPolicy::DisableDataFlowOpt()` and NO demand argument — it
  interprets the PLAIN program. So the oracle computes the FULL relation, not a
  demand-scoped one. (This is exactly how demand_tc_witness reconciles: its
  .oracle.stdout is the full transitive closure; the driver's per-key demand-ON
  answer must equal the oracle's rows filtered to that key.)
- Reconciliation with HP-5: the oracle golden lists the FULL neighborhood
  relation (all 7 edges). The demand-ON .stdout answer for probe(1) is {2,3,4}
  — a STRICT SUBSET of the oracle's rows. That the driver answer for each key
  equals the oracle's rows for that key is the closure cross-check (byte-checked
  per-golden; the strict-subset relationship is the whole point of demand and
  is what HP-5 forces the graph to expose). The oracle CANNOT itself referee
  partial materialization (a full-closure oracle categorically over-approximates
  a demand answer) — hence HP-5 downgrades closure-scoping correctness to the
  blessed .stdout + the driver's own asserts, exactly as pinned.

Which goldens the witness carries and what each certifies:
  goldens/demand_neighborhood_witness.stdout      -- the demand-SCOPED driver
      output (per-probe answers). Certifies: demand scoping is EXACT (with the
      driver asserts) and flat==nested==this (eqgate). This is the one both
      eqgate arms and all four golden modes compare against.
  goldens/demand_neighborhood_witness.oracle.stdout  -- the FULL neighborhood
      relation over the surviving edges, no demand. Certifies: the closure
      ground truth each demand answer is a subset of.
  goldens/demand_neighborhood_witness.monotone.stdout -- the monotone
      projection. Certifies: differential-final == monotone-projection (add-only
      program -> the F16-class residue gate; identical row set to the oracle
      here because nothing is ever removed).
NO nested golden. The eqgate compares stdouts live (OD-10/OWN-5).

### 2.1 goldens/demand_neighborhood_witness.stdout  (VERIFIED capture)
----------------------------------------------------------------------
nbhd 1: 2 3 4
nbhd 3: 5 6
nbhd 9: 9
nbhd 5:
----------------------------------------------------------------------

### 2.2 goldens/demand_neighborhood_witness.oracle.stdout  (VERIFIED capture;
stdout only — the "INVARIANT: differential-final == monotone-projection (1
relations)" line goes to STDERR, Main.cpp:2676, and is NOT in the golden)
----------------------------------------------------------------------
ORACLE: OK (2 batches, 420 assertions)
neighborhood	1 2
neighborhood	1 3
neighborhood	1 4
neighborhood	3 5
neighborhood	3 6
neighborhood	7 8
neighborhood	9 9
----------------------------------------------------------------------
(tabs between name and columns, matching demand_tc_witness.oracle.stdout.)

### 2.3 goldens/demand_neighborhood_witness.monotone.stdout  (VERIFIED capture)
----------------------------------------------------------------------
MONOTONE-PROJECTION: 7 surviving facts
neighborhood	1 2
neighborhood	1 3
neighborhood	1 4
neighborhood	3 5
neighborhood	3 6
neighborhood	7 8
neighborhood	9 9
----------------------------------------------------------------------

======================================================================
3. THE THREE FENCE-WITNESS CASES (all VERIFIED reject at tip)
======================================================================
Each rejects rc=1 in ALL FOUR optimization modes under `-demand
-demand-instance` (its .drflags). Fence sites are in Program::Build's demand
pre-pass, lib/ControlFlow/Build/Build.cpp:1337-1347 (cyclic > recursive_content
> diff_input, an else-if chain). VERIFIED texts (grep -m1 "error:"):

[ADJ:H3] SPECIFICITY (crit-harness-3 UPHELD, LOW) — the three fences are NOT
uniformly "-demand-instance-specific"; ADJUDICATOR VERIFIED at tip by compiling
each under plain -demand AND under -demand-instance:
  - demand_cyclic_1:     plain -demand rc=0 (compiles) | -demand-instance rc=1.
  - demand_diff_input_1: plain -demand rc=0 (compiles) | -demand-instance rc=1.
  - demand_recursive_content_1: plain -demand rc=1 ALREADY ("Unsupported rule-body
    shape under -demand") | -demand-instance rc=1 (SAME upstream reject).
So cyclic + diff_input are genuinely nested-lowering-specific gates; recur is an
UPSTREAM plain-demand body-walk reject that -demand-instance does not change (its
-demand-instance .drflags token is INERT — the case would reject identically with
a bare -demand). It is a valid all-4-modes-diagnostic and a distinct PROGRAM, but
it pins the upstream body-walk guard (the shadowed Build.cpp:1340 belt), NOT a
-demand-instance-specific fence. §5.2's CLAUDE.md text is amended below to stop
calling all three "-demand-instance nested-lowering feature-gap fences".

  | case                         | exact diagnostic substring                                        | mechanism |
  |------------------------------|-------------------------------------------------------------------|-----------|
  | demand_cyclic_1              | Recursive demand relations are not yet supported under -demand-instance | Build.cpp:1338 (fence-i cyclic_demand: ViewSelfReachable(jl[0])) |
  | demand_recursive_content_1   | Unsupported rule-body shape under -demand; recompile without -demand    | UPSTREAM DataFlow demand body-walk (the Build.cpp:1340 recursive_content branch is SHADOWED — see note) |
  | demand_diff_input_1          | Demanded subgraphs over deletable (differential) inputs are not yet supported under -demand-instance | Build.cpp:1344 (fence-iii diff_input: CanReceiveDeletions()) |

NOTE on demand_recursive_content_1 (the brief's "if the two shapes reject via
the SAME path, say so and differentiate anyway"): empirically, a program with
induction-owned CONTENT in its demanded body is caught UPSTREAM of the
Build.cpp fences, in two ways, so the Build.cpp:1340 recursive_content branch
is currently unreachable/shadowed:
  (a) A single-relation direct recursion (bf-tc) trips fence-i CYCLIC first
      (the else-if precedence — demand_tc_witness itself yields "Recursive
      demand relations..." under -demand-instance, VERIFIED). This is exactly
      ADJ-C2 / OWN-4: "fence-i also rejects recursive content."
  (b) Reading the recursive relation through a NON-recursive wrapper (the shape
      below) trips the DataFlow demand transform's body-walk reject BEFORE
      control-flow build: "Unsupported rule-body shape under -demand".
This mirrors §19(N)'s finding that fence-iii's shape is "pre-empted upstream by
the demand body-walk rejects (the fence stands as the D3.a belt)" — the SAME is
true of recursive content. demand_recursive_content_1 therefore pins the guard
that ACTUALLY fires (the upstream body-walk reject), and is a GENUINELY DISTINCT
PROGRAM from demand_cyclic_1: its query relation (`demanded`->`wrap`) is
NON-recursive and reads a separate induction-owned relation (`conn`), whereas
demand_cyclic_1 queries the recursive relation directly. Both are all-4-modes
clean; the design records the shadowing explicitly rather than fabricating an
unreachable diagnostic. (demand_diff_input_1, by contrast, DOES reach its
Build.cpp fence at tip — its single-hop non-recursive body passes the demand
transform, then trips CanReceiveDeletions.)

### 3.1 tests/OptDiff/cases/demand_cyclic_1.dr
----------------------------------------------------------------------
; Copyright 2026, Peter Goodman. All rights reserved.
;
; demand_cyclic_1 -- fence-(i) witness: a bound #query whose demanded body is
; the recursive transitive-closure shape, so the fabricated demand relation is
; self-reachable (cyclic demand). Rejected under -demand-instance by the
; Program::Build pre-pass (Build.cpp:1338); ALL FOUR modes diagnostic. Under
; plain -demand this same program COMPILES (recursive demand is the flat
; demand_tc_witness feature) -- the fence is nested-lowering-specific.

#message link(u64 From, u64 To).

#local conn(u64 From, u64 To).
conn(F, T) : link(F, T).
conn(F, T) : conn(F, M), link(M, T).

#query connected(bound u64 From, free u64 To) : conn(From, To).
----------------------------------------------------------------------
### 3.1b tests/OptDiff/cases/demand_cyclic_1.drflags
----------------------------------------------------------------------
-demand -demand-instance
----------------------------------------------------------------------
### 3.1c tests/OptDiff/cases/demand_cyclic_1.main.cpp  (inert; never compiled)
----------------------------------------------------------------------
// demand_cyclic_1 is an expected-diagnostic case under -demand -demand-instance
// (see the .dr header): the compiler must exit 1 with a rendered diagnostic in
// all 4 modes, so this driver is inert and never compiled.
int main() {
  return 0;
}
----------------------------------------------------------------------

### 3.2 tests/OptDiff/cases/demand_recursive_content_1.dr
----------------------------------------------------------------------
; Copyright 2026, Peter Goodman. All rights reserved.
;
; demand_recursive_content_1 -- fence-(i)/recursive-CONTENT witness
; (OWN-4/ADJ-C2): a bound #query over a NON-recursive wrapper (`demanded` ->
; `wrap`) whose body reads a separate induction-owned (recursively derived)
; relation (`conn`). Distinct in shape from demand_cyclic_1 (which queries the
; recursive relation directly). Under -demand-instance it rejects in ALL FOUR
; modes; at tip the reject fires UPSTREAM in the DataFlow demand body-walk
; ("Unsupported rule-body shape under -demand") -- the Build.cpp:1340
; recursive-content branch is a shadowed belt (see D2.c design sec 3). Also
; diagnostic under plain -demand: multi-relation recursive-content demand
; pushdown is outside the single-adornment slice.

#message link(u64 From, u64 To).

#local conn(u64 From, u64 To).
conn(F, T) : link(F, T).
conn(F, T) : conn(F, M), link(M, T).

#local wrap(u64 From, u64 To).
wrap(F, T) : conn(F, T).

#query demanded(bound u64 From, free u64 To) : wrap(From, To).
----------------------------------------------------------------------
### 3.2b tests/OptDiff/cases/demand_recursive_content_1.drflags
----------------------------------------------------------------------
-demand -demand-instance
----------------------------------------------------------------------
### 3.2c tests/OptDiff/cases/demand_recursive_content_1.main.cpp (inert)
----------------------------------------------------------------------
// demand_recursive_content_1 is an expected-diagnostic case under
// -demand -demand-instance (see the .dr header): the compiler must exit 1 with
// a rendered diagnostic in all 4 modes, so this driver is inert / never compiled.
int main() {
  return 0;
}
----------------------------------------------------------------------

### 3.3 tests/OptDiff/cases/demand_diff_input_1.dr
----------------------------------------------------------------------
; Copyright 2026, Peter Goodman. All rights reserved.
;
; demand_diff_input_1 -- fence-(iii) witness: a bound #query whose demanded
; body summarizes a @differential (deletable) message input. Rejected under
; -demand-instance by the Program::Build pre-pass CanReceiveDeletions() check
; (Build.cpp:1344); ALL FOUR modes diagnostic. This single-hop non-recursive
; body passes the flat demand transform, so it REACHES the control-flow fence
; (unlike the recursive-content shape). Stands as the D3.a belt: when the
; retraction surface lands, deletable demanded inputs become supported.

#message pt(u64 Key, u64 Val) @differential.

#local ans(u64 Key, u64 Val).
ans(K, V) : pt(K, V).

#query getpt(bound u64 Key, free u64 Val) : ans(Key, Val).
----------------------------------------------------------------------
### 3.3b tests/OptDiff/cases/demand_diff_input_1.drflags
----------------------------------------------------------------------
-demand -demand-instance
----------------------------------------------------------------------
### 3.3c tests/OptDiff/cases/demand_diff_input_1.main.cpp  (inert)
----------------------------------------------------------------------
// demand_diff_input_1 is an expected-diagnostic case under
// -demand -demand-instance (see the .dr header): the compiler must exit 1 with
// a rendered diagnostic in all 4 modes, so this driver is inert / never compiled.
int main() {
  return 0;
}
----------------------------------------------------------------------

======================================================================
4. tests/OptDiff/runall.sh DIFF (run_eqgate + alternation + tokens)
======================================================================
Three mechanical edits; the summary grep at runall.sh:350
(`grep -qE 'FAIL|DIVERGE|EXPECT-ERROR|MISSING'`) ALREADY catches every eqgate
token (NESTED-GOLDEN-DIVERGE→DIVERGE, EQGATE-*-FAIL→FAIL,
EQGATE-GOLDEN-MISSING→MISSING) — NO grep change needed (verified against :350).
[ADJ:H1] The FLAT-NESTED-DIVERGE token is retired with the 4-mode rewrite (§4.1).

### 4.1 New function run_eqgate, defined in the --one worker (place it AFTER
run_irgold's definition, ~runall.sh:292, before `st=0` at :294).

[ADJ:H1] AMENDED — nested arm now loops ALL FOUR optimization modes (crit-
harness-1 UPHELD, MED). The original draft drove the nested lowering in OPT MODE
ONLY (`$(flags_of opt) -demand-instance`), leaving nested nodf/nocf/none
equivalence UNCHECKED — and `-demand-instance` drives control-flow lowering that
ProgramImpl::Optimize reshapes and `-disable-controlflow-opt` skips, so a
nocf/nodf-only nested divergence would escape both the eqgate (opt only) and the
flat 4-mode golden check (never carries `-demand-instance`). ADJUDICATOR
VERIFIED at tip: all four nested modes (opt/nodf/nocf/none) compile, run rc=0,
and are byte-identical to the golden — so there is NO current miscompile, but
the harness must exercise all four to catch a future non-opt-only nested
regression, per the brief's explicit "exercise the nested lowering" mandate.
[ADJ:H4] This ALSO absorbs crit-harness-4's redundancy point: the draft compared
nested==flat AND nested==golden, but diffrun already established flat==golden for
every mode, so we drop the redundant nested==flat compare and instead compare
each nested MODE's stdout to the golden directly (flat==nested falls out
transitively). Cost note (crit-harness-4): the eqgate now adds four
compile+build+run triples inside the witness's single --one worker slot, making
it the suite long-pole — acceptable for one tiny single-hop case; if it ever
bites, gate the extra three modes behind an env knob, but the correctness-
complete default is all four.

flags_of "<mode>" appends the .drflags (-demand) for that mode; the nested arm
adds -demand-instance (idempotent with the implied -demand). The per-mode nested
workdir is "$WORKROOT/$NAME/$NAME.eqgate.<mode>".

----------------------------------------------------------------------
  run_eqgate() {  # equivalence gate (D2.c): a case with a .eqgate sidecar is
                  # re-driven under the nested lowering (.drflags + the
                  # -demand-instance selector) with the SAME driver, in ALL FOUR
                  # optimization modes, and each mode's stdout is byte-compared
                  # against the case's committed golden. diffrun already proved
                  # flat==golden per mode, so nested==golden per mode gives
                  # flat==nested transitively across all four modes. No nested
                  # golden is ever blessed -- the two-lowerings answer-identity
                  # gate is refereed live (OD-10/OWN-5).
    sidecar="$HERE/cases/$NAME.eqgate"
    if [ ! -f "$sidecar" ]; then
      return 0
    fi
    golden="$HERE/goldens/$NAME.stdout"
    if [ ! -f "$golden" ]; then
      echo "$NAME eqgate EQGATE-GOLDEN-MISSING"
      return 1
    fi
    eqrc=0
    for mode in opt nodf nocf none; do
      out="$WORKROOT/$NAME/$NAME.eqgate.$mode"
      mkdir -p "$out"
      # shellcheck disable=SC2046  # flags_of emits zero or more separate words
      if ! timeout "$TIMEOUT" "$DR" "$DRC" $(flags_of "$mode") -demand-instance \
          -cpp-out "$out" >"$out/dr.log" 2>&1; then
        echo "$NAME eqgate $mode EQGATE-DR-FAIL"
        eqrc=1
        continue
      fi
      if ! "$CXX" -std=c++23 -g -I "$REPO_ROOT/include" -I "$out" \
          "$DRV" "$out/datalog.cpp" "$REPO_ROOT/lib/Runtime/Allocator.cpp" \
          -o "$out/case" >"$out/cxx.log" 2>&1; then
        echo "$NAME eqgate $mode EQGATE-CXX-FAIL"
        eqrc=1
        continue
      fi
      if ! timeout "$TIMEOUT" "$out/case" >"$out/stdout" 2>"$out/stderr"; then
        echo "$NAME eqgate $mode EQGATE-RUN-FAIL"
        eqrc=1
        continue
      fi
      if ! cmp -s "$golden" "$out/stdout"; then
        echo "$NAME eqgate $mode NESTED-GOLDEN-DIVERGE"
        eqrc=1
        continue
      fi
      echo "$NAME eqgate $mode OK"
    done
    return $eqrc
  }
----------------------------------------------------------------------
NOTE: the FLAT-NESTED-DIVERGE token is retired (subsumed by NESTED-GOLDEN-DIVERGE
+ the standing flat==golden diffrun check); the summary grep at runall.sh:350
still catches every surviving token (NESTED-GOLDEN-DIVERGE→DIVERGE,
EQGATE-*-FAIL→FAIL, EQGATE-GOLDEN-MISSING→MISSING). ADJUDICATOR VERIFIED: this
amended 4-mode loop is green at tip (opt/nodf/nocf/none each rc=0, byte-identical
to the sec 2.1 golden).

### 4.2 Wire it into the worker tail. At runall.sh:313-315, between run_oracle
and run_irgold:
  BEFORE:
      run_oracle || st=1
      run_irgold || st=1
      exit $st
  AFTER:
      run_oracle || st=1
      run_eqgate || st=1
      run_irgold || st=1
      exit $st

### 4.3 Extend the all-4-modes-diagnostic alternation at runall.sh:296 with the
three fence names (append before the closing paren):
  BEFORE:
    kvindex_2|kvindex_3|kvindex_4|agg_in_scc_1|kv_in_scc_1|algebra_dup_1|algebra_conflict_1|evm_func_parse|nonascii_1|truncated_decl_1|demand_multi_adorn_1)
  AFTER:
    kvindex_2|kvindex_3|kvindex_4|agg_in_scc_1|kv_in_scc_1|algebra_dup_1|algebra_conflict_1|evm_func_parse|nonascii_1|truncated_decl_1|demand_multi_adorn_1|demand_cyclic_1|demand_recursive_content_1|demand_diff_input_1)

### 4.4 --bless mirror: UNCHANGED. The witness is a normal diffrun+oracle case,
so the existing bless loop copies $NAME.opt/stdout, $NAME.oracle/stdout,
$NAME.monotone/stdout (runall.sh:76-92) — all three witness goldens covered.
The eqgate needs NO golden (its nested stdout is compared live), so --bless is
untouched. The fence cases produce no stdout (diagnostic arm), so bless skips
them via the `[ -f "$src" ]` guards. VERIFIED: bless loop reads
"$WORKROOT"/*/ then "$d$name.opt/stdout" etc., which matches diffrun's
"$WORKROOT/$NAME/$NAME.opt/stdout" layout.

### 4.5 Header comment (runall.sh:17-52): add a one-line note under the .batches
paragraph that a cases/<name>.eqgate sidecar additionally runs run_eqgate
(nested arm = .drflags + -demand-instance; flat==nested==golden, live, never
blessed), and add the three fence names to the diagnostic list at runall.sh:19.
(Doc-only; not load-bearing for behavior.)

======================================================================
5. CLAUDE.md EXACT EDITS (ride the same commit)
======================================================================

### 5.1 Case count 169 -> 173 (CLAUDE.md:51).
  BEFORE:  (`<name>.dr` + `<name>.main.cpp`, 169 corner-case programs as of the
           keyed-instances (F) landing — symrec_tie_1 is the standing determinism
  AFTER:   (`<name>.dr` + `<name>.main.cpp`, 173 corner-case programs as of the
           keyed-instances D2.c landing — symrec_tie_1 is the standing determinism

### 5.2 Diagnostic authoritative list (CLAUDE.md:87-89). Append the three fence
names + a clause, after `demand_multi_adorn_1 (...)`:
  BEFORE:  `nonascii_1`, `truncated_decl_1`, `demand_multi_adorn_1` (a `-demand` query
           name carrying >1 binding pattern — the demand pass's clean per-name reject,
           via its `.drflags` sidecar); `kvindex_1` is MODE-SPLIT (compiles
  AFTER:   `nonascii_1`, `truncated_decl_1`, `demand_multi_adorn_1` (a `-demand` query
           name carrying >1 binding pattern — the demand pass's clean per-name reject,
           via its `.drflags` sidecar), `demand_cyclic_1`/`demand_diff_input_1` (two
           `-demand-instance` nested-lowering feature-gap fences — recursive demand and
           a @differential summarized input; both COMPILE under plain `-demand` and
           reject only under `-demand-instance`) and `demand_recursive_content_1` (a
           recursive-content demanded body — rejected UPSTREAM by the plain-`-demand`
           body-walk, so its `-demand-instance` token is inert; it pins the shadowed
           Build.cpp recursive-content belt); each via its `-demand -demand-instance`
           `.drflags`; `kvindex_1` is MODE-SPLIT (compiles

### 5.3 New paragraph in the demand section (after CLAUDE.md:347, the
`## The demand transform` block) introducing the nested lowering + eqgate +
the birth-only feature gap. Insert a new subsection:
----------------------------------------------------------------------
## The keyed-instance nested lowering (`-demand-instance` — LANDED, birth-only)

`-demand-instance` (Main.cpp `gDemandInstance`; implies `-demand`; OFF the
PassPolicy registry — a lowering selector, not a pass) lowers a recognized
demanded subgraph to a keyed InstanceStore instead of the flat guard web (the
D2.b nested lowering). It is ANSWER-IDENTICAL to flat `-demand`: the
`demand_neighborhood_witness` case is the two-lowerings equivalence witness —
its `.eqgate` sidecar drives run_eqgate (runall.sh --one), which re-compiles the
nested arm (`.drflags` + `-demand-instance`) with the SAME driver and byte-
compares flat == nested == `goldens/demand_neighborhood_witness.stdout` LIVE (no
nested golden is blessed). The witness graph carries out-of-neighborhood edges
and the driver asserts each probe's answer is EXACTLY neighborhood(Start) — an
over-materialized nested arm both aborts and diverges (HP-5).

The witness is ENFORCED BIRTH-ONLY (RAT-6): all edges land before any demand
probe. EDGE-AFTER-DEMAND — adding a monotone input edge while a demand is
already standing and expecting the standing instance to rebuild — is a LABELED
FEATURE GAP, NOT a compile diagnostic (it is a batch-ordering property,
indistinguishable at compile time from a legal program; the demand-triggered
rebuild plumbing arrives with the DeltaRel->Rel epoch). The three nested-lowering
compile fences (Build.cpp:1337-1347, all-4-modes-diagnostic under
`-demand -demand-instance`): recursive demand (`demand_cyclic_1`),
recursive-content demanded body (`demand_recursive_content_1`, caught upstream
by the demand body-walk), and a @differential summarized input
(`demand_diff_input_1`).
----------------------------------------------------------------------

### 5.4 (optional, recommended) Known-gaps section (CLAUDE.md:349): the
edge-after-demand gap is already named in 5.3; no duplicate needed. If a
one-liner is wanted under `## Other known feature gaps`, add:
  "Adding a monotone input to an already-demanded keyed-instance subgraph
  (edge-after-demand) under `-demand-instance` is a labeled gap, NOT a
  diagnostic — the standing instance does not rebuild until the DeltaRel->Rel
  plumbing lands (RAT-6, birth-only)."

======================================================================
6. PREDICTIONS + GATES
======================================================================

### 6.1 Predictions P-D2c.1..3 (restated from §B, with verified status)
  P-D2c.1  Suite 169 -> 170 (witness) -> 173 (three fences); all four golden
    modes of demand_neighborhood_witness byte-agree (standing cross-mode law).
    STATUS: VERIFIED — flat arm builds+runs+matches golden in all 4 modes
    (opt/nodf/nocf/none all byte-identical to the sec 2.1 golden).
  P-D2c.2  run_eqgate green: flat == nested == .stdout on BIRTH (RAT-6, no
    flap-rebuild — the flap narrative is retired); oracle golden = full-closure
    dump, one .oracle.stdout for the flat lowering (the oracle never runs
    demand). STATUS: VERIFIED — flat==nested byte-equal (both rc=0, HP-5 asserts
    pass both arms); oracle+monotone captured (sec 2.2/2.3).
  P-D2c.3  [G2] 676-row corpus A/B byte-identical vs frozen e6264b54 knob-off
    (new cases only — nothing existing changes); [G3] irgold zero-churn (no
    .irgold added to the witness at D2.c per OD-10/OWN-5). STATUS: expected —
    the diff adds only new cases + one runall.sh function + doc; no compiler or
    existing-golden bytes change.

### 6.2 Derived expected .stdout (hand-derived from batches+probes, then
VERIFIED by running): see sec 2.1. Derivation:
  edges = 1->2,1->3,1->4, 3->5,3->6, 9->9, 7->8.
  neighborhood(1) = {2,3,4}; neighborhood(3) = {5,6}; neighborhood(9) = {9};
  neighborhood(5) = {} (5 is never a source). Sorted keyed drains ->
  "nbhd 1: 2 3 4" / "nbhd 3: 5 6" / "nbhd 9: 9" / "nbhd 5:". MATCHES the run.

[ADJ:H2] NEW-CASE BLESS BOOTSTRAP (crit-harness-2 UPHELD, MED). The checklist
below reads "bless from GREEN; NEVER on red", but a brand-new case's FIRST run is
necessarily red — diffrun.sh:90-91 emits GOLDEN-MISSING for all four witness
modes and run_oracle (runall.sh:212/232) emits GOLDEN/MONO-MISSING until the
goldens exist, and runall.sh:350 makes any MISSING a SUITE FAIL. CLAUDE.md's
actual rule is narrower: "never bless a red case green" = never bless to MASK A
REGRESSION (a case that ran and DIVERGED). The new-case bootstrap is the sanctioned
exception: (1) run the suite — the witness reports GOLDEN-MISSING (expected, not a
divergence); (2) REVIEW the produced .opt/stdout, .oracle/stdout, .monotone/stdout
against the hand-derived sec 2.1-2.3 texts (this human review IS the gate that
substitutes for a golden compare); (3) `runall.sh --bless <wr>
demand_neighborhood_witness` to seed the three goldens FROM that reviewed run;
(4) re-run — now SUITE PASS (173) with the eqgate live. The prohibition is on
blessing a case that DIVERGED from an existing golden, never on seeding a
first-ever golden from a reviewed run. (Fence goldens: none — diagnostic arm.)

### 6.3 Implementation gates checklist (run at landing; bless ONLY from green):
  [ ] SUITE PASS (173) — `DR=build/debug/bin/drlojekyll runall.sh <wr>` ends
      "SUITE: PASS (173 cases)"; the witness diffrun (4 modes) + run_oracle
      (oracle+monotone) + run_eqgate all green; the 3 fences all-4-modes OK.
  [ ] Cross-mode law: all four golden modes of EACH new case byte-agree
      (witness -> one golden; fences -> all-4 diagnostic). VERIFIED pre-land.
  [ ] run_eqgate: nested == golden in ALL FOUR modes ([ADJ:H1]; each
      "<name> eqgate <mode> OK"; NESTED-GOLDEN-DIVERGE / EQGATE-*-FAIL absent
      from verdicts). VERIFIED pre-land (all 4 nested modes byte-identical).
  [ ] HP-5: driver asserts pass (no abort) on BOTH arms; over-materialization
      would trip the assert. VERIFIED pre-land.
  [ ] [G2] 676-row corpus + data/ A/B byte-identical vs frozen e6264b54
      knob-off (new cases don't touch existing output).
  [ ] [G3] irgold zero-churn (no new .irgold; existing goldens untouched).
  [ ] ctest 5/5 (debug) — no runtime/InstanceStore change here, expected green.
  [ ] ASAN: build/asan suite PASS (173) + ctest 5/5 under ASAN; the nested
      witness compiled+run under the ASAN binary zero reports (per §19(F) +
      the D2.b precedent).
  [ ] Q5 bench neutral (ABABAB, no compiler change -> expected 0% noise).
  [ ] Bless ritual: `runall.sh --bless <wr> demand_neighborhood_witness` copies
      .stdout/.oracle.stdout/.monotone.stdout from the reviewed workroot (the
      [ADJ:H2] new-case bootstrap: the first run is GOLDEN-MISSING, NOT a
      divergence — review the produced texts against sec 2.1-2.3, bless, re-run
      to SUITE PASS); NEVER bless a case that DIVERGED from an existing golden;
      NO nested golden blessed (the eqgate compares live). CLAUDE.md +
      runall.sh edits ride the SAME commit.
  [ ] cursor-contract review on the new driver: sorted keyed drains (present),
      full drain before next entry point (present).

### 6.4 Files touched by the D2.c landing (for the commit):
  NEW: tests/OptDiff/cases/demand_neighborhood_witness.{dr,drflags,batches,
       main.cpp,eqgate}
  NEW: tests/OptDiff/goldens/demand_neighborhood_witness.{stdout,oracle.stdout,
       monotone.stdout}  (blessed from green)
  NEW: tests/OptDiff/cases/demand_cyclic_1.{dr,drflags,main.cpp}
  NEW: tests/OptDiff/cases/demand_recursive_content_1.{dr,drflags,main.cpp}
  NEW: tests/OptDiff/cases/demand_diff_input_1.{dr,drflags,main.cpp}
  EDIT: tests/OptDiff/runall.sh (run_eqgate + wire + :296 alternation + header)
  EDIT: CLAUDE.md (count, diagnostic list, nested-lowering subsection)
No fence goldens (diagnostic cases); no nested golden (live eqgate).

======================================================================
7. TIP-VERIFICATION LOG (what was actually compiled/run)
======================================================================
- tip: git rev-parse HEAD = 53d95864c203... (matches brief).
- witness flat (-demand): compiles; driver (assert form) rc=0; stdout = sec 2.1.
- witness nested (-demand -demand-instance): compiles; driver rc=0; stdout
  BYTE-IDENTICAL to flat (cmp clean).
- witness all 4 flat modes (opt/nodf/nocf/none): each compiles+runs, stdout
  byte-identical to the golden (none-mode explicitly re-checked, cmp clean).
- oracle (no flags): "ORACLE: OK (2 batches, 420 assertions)" + 7 rows (sec 2.2);
  monotone: "MONOTONE-PROJECTION: 7 surviving facts" + 7 rows (sec 2.3).
- demand_cyclic_1: rc=1 all 4 modes, "Recursive demand relations are not yet
  supported under -demand-instance".
- demand_recursive_content_1: rc=1 all 4 modes, "Unsupported rule-body shape
  under -demand; recompile without -demand".
- demand_diff_input_1: rc=1 all 4 modes, "Demanded subgraphs over deletable
  (differential) inputs are not yet supported under -demand-instance".
- demand_tc_witness under -demand-instance: rc=1, "Recursive demand relations
  ..." (confirms it is the fence-i shape; the neighborhood witness, not tc, is
  the nested carrier — as designed).
- Query ABI (both arms): neighborhood_bf(Database&, Log&, Functors&, uint64_t
  Start) -> cursor.next(uint64_t &Node); add_edge_2(db, log, functors,
  Vec<add_edge_input>); add_edge_input = Tup_u64_u64.

======================================================================
8. ADJUDICATION LEDGER (D2.c design round; 6 findings, all ruled AT CODE)
======================================================================
Adjudicator re-verified every disputed fact at tip 53d95864 by compiling and
running the exact shapes (probe workdir under scratchpad/d2c/probe). Rulings:

crit-harness-1 [MED] run_eqgate opt-mode-only — UPHELD → AMEND (§4.1).
  Real coverage hole: nested nodf/nocf/none equivalence was unchecked, and
  -demand-instance drives control-flow lowering reshaped by ProgramImpl::Optimize
  (skipped by -disable-controlflow-opt). VERIFIED all 4 nested modes green +
  byte-identical to golden at tip (no current miscompile), so the fix is
  prophylactic. run_eqgate rewritten to loop all 4 modes, nested==golden per
  mode; FLAT-NESTED-DIVERGE token retired (subsumed). The brief's explicit
  "exercise the nested lowering" mandate is now honored across the full mode grid.

crit-harness-2 [MED] bless bootstrap chicken-and-egg — UPHELD → AMEND (§6.3).
  A new case's first run is necessarily GOLDEN-MISSING (SUITE FAIL until seeded).
  The real rule is "never bless a DIVERGED case" (mask a regression), not "never
  bless unless SUITE PASS". Added the sanctioned new-case bootstrap: run →
  REVIEW produced texts vs sec 2.1-2.3 → bless → re-run to SUITE PASS(173).
  Verified against diffrun.sh:90-91 + runall.sh:212/232/350.

crit-harness-3 [LOW] recur not -demand-instance-specific — UPHELD → AMEND
  (§3, §5.2). VERIFIED: demand_recursive_content_1 rejects under PLAIN -demand
  (rc=1, "Unsupported rule-body shape under -demand"); its -demand-instance token
  is INERT. cyclic + diff_input compile under plain -demand (rc=0), reject only
  under -demand-instance. CLAUDE.md §5.2 text amended to stop calling all three
  "-demand-instance nested-lowering feature-gap fences"; recur re-homed as the
  upstream plain-demand body-walk reject / shadowed belt.

crit-harness-4 [LOW] eqgate cost + nested==flat redundancy — PARTIALLY UPHELD →
  ABSORBED (§4.1). The nested==flat compare was redundant with diffrun's
  flat==golden; the [ADJ:H1] rewrite drops it (compares nested-per-mode==golden
  instead). Cost acknowledged: the eqgate is now a 4x compile long-pole in one
  worker slot — accepted for one tiny case, with an env-knob escape hatch noted.

crit-witness-1 [LOW] HP-5 answer-level discrimination only — REJECTED as defect
  (non-defect; the critic concurs). HP-5's answer-level assert is the pinned
  contract; benign internal over-materialization re-filtered at the bound cursor
  is unobservable at stdout and is a known boundary the design §2 already states.
  No change. (The [ADJ:H1] 4-mode nested loop does NOT close this — it is a
  different, and by design out-of-scope, coverage axis.)

crit-witness-2 [LOW] seal-attribution loose — UPHELD-MINOR → AMEND (§1.3).
  VERIFIED in the nested datalog.h (flow_46): every entry point seals
  instance_0 + table_8 + table_11; the two edge batches drive two monotone
  edge-table seal epochs (empty touched set), and FindOrAddInstance fires only on
  the four distinct-key probes. Attribution tightened; substantive both-surfaces-
  covered claim holds.

======================================================================
9. FINAL VERDICT: GO-WITH-AMENDMENTS
======================================================================
The D2.c design is architecturally sound and empirically verified at tip: the
witness compiles/runs/answers identically flat vs nested across ALL FOUR modes,
the three fences reject as claimed, and the goldens/oracle/monotone captures are
byte-exact. Two MED findings required real amendments — the opt-only eqgate
coverage hole (now a 4-mode loop, re-verified green here) and the bless-bootstrap
overclaim (now a stated new-case exception) — plus three LOW doc-precision
amendments (recur specificity, seal attribution, cost/redundancy) and one LOW
rejected-as-non-defect (HP-5 answer-level boundary, already acknowledged). None
touch the compiler or the witness program bytes; all amendments are to the
harness function, the CLAUDE.md text, and the design prose. Implementation may
proceed under the [ADJ:*] amendments, with the per-diff design ritual and the
§6.3 gate checklist (incl. the ASAN sweep) still gating the landing.
