# Adversarial critique — t2-desired-df-demand_tc_witness.md

Critic pass at tip b577735e. Target artifact:
`scratchpad/desired-states/t2-desired-df-demand_tc_witness.md`.
Ground truth: `scratchpad/dump-inputs/demand_tc_witness.{dot,ir}` (post-optimize
demand-ON graph from the b577735e binary this session). Spec: t2-dump-spec.md §1;
form: ir-dump-formats.md §1; id rule: consolidated.md §1.A + code read this session.

Method: I reconstructed the FULL edge set from the DOT (38 `v -> v` edges, all
enumerated), reversed each (DOT is use→def; `=>` is def→use), and checked every
block, column, edge, port, and attribute line in §1 against it. I re-read
Query.h:1176-1214 (ForEachView kind order), Query.h:472 (det_seq), and the
consolidated stamp-site record. E-54 lesson applied: I read the cited DOT lines,
not the artifact's paraphrase of them.

--------------------------------------------------------------------
## VERDICT-RELEVANT SUMMARY

The block inventory (20 views, kinds, det_seq numbering) is CORRECT and FIRM.
The id rule matches the code. Almost all `=>` edges reverse correctly. But there
are REAL defects: one FABRICATED edge that does not exist in the DOT (CRITICAL for
a byte-compare golden), several port/pivot renderings that misread the DOT, and a
cluster of genuine spec-ambiguity findings the writer flagged that I confirm are
real (and one the writer got BACKWARDS). Details below, severity-ranked.

--------------------------------------------------------------------
## FINDINGS

### F0 [CRITICAL] — Q1 is BACKWARDS: the spec `; back-edge` rule fires on SEVEN edges, not zero; the artifact prints it on NONE, so it matches neither a literal spec impl nor an induction-membership impl

This is the sharpest defect and it inverts the writer's own headline finding.
The writer's Q1 (artifact §3 lines 248-260) asserts: "NO edge in the graph
satisfies user<=def ... a strict spec implementation emits ZERO back-edge
comments." I recomputed user_det_seq vs def_det_seq for all 24 `=>` edges
(def = the emitting block, user = the `=>` target). SEVEN edges satisfy
`user det_seq <= def det_seq` and would be flagged by a LITERAL spec §1.3
implementation:

  join.13(13) => tuple.3(3)      3 <= 13  FIRES
  join.14(14) => tuple.11(11)   11 <= 14  FIRES
  join.15(15) => tuple.7(7)      7 <= 15  FIRES
  join.16(16) => tuple.9(9)      9 <= 16  FIRES
  merge.17(17) => tuple.4(4)     4 <= 17  FIRES
  merge.17(17) => tuple.5(5)     5 <= 17  FIRES
  merge.18(18) => tuple.6(6)     6 <= 18  FIRES

EVIDENCE: DOT edge set (all 24 producer→user reversals enumerated this session);
det_seq numbering from artifact §2.1 (which I independently confirmed FIRM).

The artifact §1 prints `; back-edge` on NONE of these seven lines. It adds only a
prose note on `tuple.11 => merge.17` (line 102) — and that edge does NOT fire the
rule (user=17, def=11, 17<=11 false). So:

- Against a LITERAL spec implementation: the artifact is WRONG on 7 lines (missing
  7 `; back-edge` comments). A golden blessed from this artifact would DIVERGE
  from the actual emitter on those 7 lines.
- Against an INDUCTION-MEMBERSHIP implementation (mark edges into SET-0 members):
  the true back-edges are the edges that CLOSE the cycle into the SET-0 unions —
  `tuple.11 => merge.17` and `tuple.3 => merge.17` (both into path) and
  `tuple.5 => merge.18` and `tuple.12 => merge.18` (into d_path). The artifact
  flags only tuple.11=>merge.17 and MISSES the other three cycle-closing edges.

So the artifact matches NEITHER candidate rule. The writer correctly concluded
"the `<=` heuristic is unsound as a cycle marker" but reached it via a false
premise (zero fires) and then produced §1 text consistent with no implementable
rule at all.

WHAT IS ACTUALLY TRUE: the spec's `<=` rule is unsound because it OVER-fires
(every join/merge → lower-numbered-tuple edge trips it, 7 here), not because it
under-fires. The det_seq numbering is near-topological in the FORWARD direction
(producers mostly precede users), so the reverse edges from the high-numbered
joins/merges back to the tuples they feed all trip `user<=def` whether or not
they are real induction back-edges. The RIGHT marker is SET-membership: an edge
whose USER is a SET-0 union (merge.17/merge.18) and whose def is also in the SCC
is a genuine back-edge. RECOMMEND: (a) fix the spec rule to key on
InductionGroupId / SET membership; (b) re-derive the artifact's back-edge
comments from that rule (4 edges: the two into merge.17, the two into merge.18);
(c) drop the prose-only note idiom. Until (a), this surface cannot be blessed —
the artifact and any literal implementation disagree on 7 lines.

### F1 [HIGH] — JOIN output signatures omit the pivot column, but the `=>` arg maps include it: an internal inconsistency (arg count ≠ declared output arity) on joins 14/15/16

For a byte-compare golden, a self-inconsistent block is a latent bug: the
declared output tuple and the tail-call argument list must agree, or the emitter
cannot be written to produce both. Three joins push their PIVOT column to the
consumer in addition to the declared output, but the artifact's join HEADER lists
only the non-pivot output:

- `join ^join.15 ... -> (To:u64@c28)` (line 125) declares ONE output column. But
  DOT L46 `v34309506816:p0 -> v4315111424:c27` shows tuple.7 reads join.15's
  **c27 (the pivot From)** AND L47 reads c28 (To). The `=>` at line 127,
  `^tuple.7 (From=From, To=To)`, correctly maps TWO columns (From from c27, To
  from c28) — but c27 is not in the declared `-> (To:u64@c28)` signature.
- `join ^join.16 ... -> (To:u64@c30)` (line 132) vs DOT L51/L52: tuple.9 reads
  c29 (pivot From) and c30 (To). `=> ^tuple.9 (From=From, To=To)` (line 134) maps
  both; header declares one.
- `join ^join.14 ... -> (T:u64@c26)` (line 118) vs DOT L57/L58: tuple.11 reads
  c25 (pivot F) and c26 (T). `=> ^tuple.11 (F=F, T=T)` (line 120) maps both;
  header declares one.

EVIDENCE: DOT lines 46-47 (join.15/tuple.7), 51-52 (join.16/tuple.9), 57-58
(join.14/tuple.11); the `.ir` join at lines 121-130 confirms the pivot column is
a live projected value (`%col:16 as @From:92` flows to the output table). join.13
does NOT have this problem — its consumer tuple.3 reads only c23/c24, not the
pivot c22 (DOT L36-37), so its 2-col header matches its 2-col `=>`.

WHY IT MATTERS: the DataFlow JOIN view's output column list (`join.Columns()`)
almost certainly INCLUDES the pivot (the DOT shows c27/c29/c25 as readable ports
consumed downstream). If so, the CORRECT header for join.15 is
`-> (From:u64@c27, To:u64@c28)` and likewise for 14/16 — the artifact UNDER-
declares the join outputs. The spec must pin whether a JOIN block declares its
pivot in the output tuple (the DOT and the consumer edges say YES). As written,
the artifact is not implementable as a consistent golden.

### F1b [HIGH] — the JOIN `.in0/.in1`-by-producer-det_seq rendering diverges from the deterministic accessor order a natural emitter would use; and interleaved-producer joins (join.13) lose their column grouping

The writer's Q4 worries the join input order "may be a DefList whose order is NOT
guaranteed det_seq-sorted" and RE-KEYS the ports by producer det_seq to be safe.
I read the accessors: this remedy both misdiagnoses the mechanism and produces
output no straightforward emitter would emit.

MECHANISM (Query.cpp:822-844): `NthInputPivotSet(n)` = `out_to_in.find(columns[n])`
— a KEY LOOKUP into the pointer-keyed `unordered_map out_to_in` (Query.h:767)
returning an ORDERED `UseList` (insertion-ordered vector). `NthInputMergedColumn(n)`
indexes `columns[n+num_pivots]` likewise. So the DOT port order (Format.cpp:349-362:
ALL pivot-set inputs by output-column index + UseList order, THEN all merged
inputs by output-column index) is DETERMINISTIC BY CONSTRUCTION — the unordered_map
is only ever looked up by key, never iterated. Q4's "may not be sorted" is true
(it is NOT det_seq order — it is output-column-index + UseList order) but it IS
deterministic. So the (F)-determinism concern is unfounded here.

CONSEQUENCE: a `-df-out` emitter that reuses these accessors (the obvious
implementation) emits the DOT's port order, NOT the artifact's producer-det_seq
regrouping. For join.13 the DOT interleaves: p0=tuple.7.To, p1=tuple.10.M (the
pivot set for M), p2=tuple.7.From, p3=tuple.10.T (the merged outputs). The
artifact regroups into `.in0 <- tuple.7 (To,From)`, `.in1 <- tuple.10 (M,T)`
(lines 109-110) — a per-producer grouping that the accessor order does not
produce (it splits each producer's columns across the pivot/merged partition).
An implementer following the accessors will DIVERGE from this artifact on every
multi-input join. RECOMMEND: the spec should pin JOIN port rendering to the
accessor order (output-column index, pivot-set-then-merged, UseList within a
set) — deterministic and free — and the artifact should be re-derived to match,
OR the spec must mandate an explicit producer-grouping re-sort in the emitter
(more code, and it still needs a stable secondary key for two columns from the
same producer). As written, the join blocks will not byte-match a natural emitter.

RESIDUAL (genuine, shared with the DOT): the UseList order WITHIN a pivot set is
subject to the Query.h:772-773 caveat ("I don't think the ordering invariant is
maintained through canonicalization"). That is a real latent (F) hazard for BOTH
dumps — worth a validator — but it is not what Q4 diagnosed, and the artifact's
re-keying does not fix it (it would still need a deterministic tie-break for
same-producer columns).

### F2 [MED] — `producer=DEMAND-SEED` on ^select.1 is FABRICATED (no ground-truth source); it will not byte-match any real emitter output

Line 58 prints `producer=DEMAND-SEED` on ^select.1. The writer's own Q2 admits
this is speculative — there is NO producer field in the DOT and the string was
invented. For a byte-compare golden this is a guaranteed DIVERGE: the real
`producer=` tag (if the field is even emitted for a RECEIVE view) is whatever
Demand.cpp actually stores, which was not read. Worse, ^select.1 is the
FABRICATED demand receive — but a "seed" tag more plausibly belongs on the demand
SEED relation (tuple.8/%table:23, the `demand seed` per the writer's own §2.4
line 237), not on the receive. The tag is on the wrong view AND is invented.

This compounds with the writer's deeper Q2 point (the field is `#ifndef NDEBUG`,
so debug/release diverge). CONFIRMED as a real spec hole: t2-dump-spec.md §1.3
itself waffles ("...ONLY under #ifndef NDEBUG builds... NO — producer is
debug-only"). RECOMMEND the spec resolve producer BEFORE this golden is authored;
until then, no `producer=` line should appear in a desired-state artifact,
because none of its values can be ground-truth-sourced. This one speculative line
is the single most golden-destabilizing token in §1.


### F3 [MED] — INSERT redundancy + column-source spelling (Q3 CONFIRMED real); the `into %table:4` + `table=%table:4` double-print IS redundant, and insert has no output columns

Q3 is a genuine spec gap I confirm. Facts (Format.cpp:604-628, Columns.cpp:13-25):
- An INSERT view owns NO finalized output columns (it is terminal; FinalizeColumnIDs
  gives it none). Its DOT `c0/c1` (DOT L29-31) are INPUT-COLUMN POSITIONAL INDICES
  (`insert.UniqueId() << ":c" << i`, Format.cpp:620 — `i` is the loop index, NOT
  `col.Id()`), pointing at tuple.9's finalized cols c15/c16. So the artifact's
  `(From=From@c15, To=To@c16)` renders the SOURCE columns (tuple.9's), which is a
  reasonable choice, but note c15/c16 are tuple.9's ids, not insert's.
- The header `into %table:4` AND the attrs `table=%table:4` (lines 145-146) ARE
  redundant — same id, same view. Pick one. The spec ir-dump-formats §1 shows only
  `insert ^insert.12 (F,T) into %tc` (no attrs table= line), so the `into` form is
  the precedent; drop the attrs `table=` for INSERT.
- The trailing `; reads ^tuple.9; no tail calls (query result sink)` (line 147) is
  a prose comment with no spec basis — harmless but unspecified.

RECOMMEND spec pin: INSERT renders `insert ^insert.N (<dst-col>=<src>@<src-id> ...)
into %table:M`, no attrs line, no `=>`. Then the artifact drops line 146's
`table=%table:4 class=monotone stratum=9` OR the spec keeps the attrs line and
drops the `into` — but not both.

### F4 [LOW-MED] — spec deviations the writer ADDED without spec sanction: the `@cN` suffix on EVERY column, and `_MissingVar` verbatim (Q5 CONFIRMED)

Both are deterministic (no golden-instability), but both DEVIATE from the spec's
column rule `<var-or-cN>:<type>`:
- `@cN` suffix on every column (`name:type@cN`): the writer added this beyond spec.
  It IS deterministic (col.Id() is the finalized id, Columns.cpp:21). But the spec
  says the fallback is `cN` IN PLACE OF the name when the name is absent, not an
  always-appended suffix. As written the artifact prints BOTH (`M:u64@c1`), which
  the spec does not authorize. Spec must ratify or the artifact must drop `@cN`.
- `_MissingVar`: this is the literal output of `col.Variable()` (Format.cpp:74) for
  a column with no parse var — an impl placeholder, deterministic (constant string).
  The spec's `<var-or-cN>` implies the fallback should be `c<N>`, NOT `_MissingVar`.
  A natural emitter reusing `col.Variable()` WOULD print `_MissingVar` (so the
  artifact matches that emitter), but the spec text says otherwise. Resolve: either
  the emitter special-cases the no-var case to `cN` (then the artifact is WRONG on
  6 columns: c3,c10 wait—) — the affected columns are c3, c11, c14, c21, c33 and
  the c10-side (tuple.5 From is a real var). Precisely the DOT `_MissingVar` cells:
  select.1 c3, tuple.6 c11, tuple.8 c14, tuple.12 c21, merge.18 c33, and join
  pivot/merged demand cols. Spec must pick `_MissingVar` (impl-faithful) vs `cN`
  (spec-literal); they are DIFFERENT bytes.

### F5 [LOW] — Q6 (shared-model views) is real but the current symmetric rendering is DEFENSIBLE, not a defect

tuple.4 & merge.17 both `table=%table:8`; tuple.6 & merge.18 both `%table:12`;
tuple.2 & tuple.10 both `%table:19`. CONFIRMED against DOT (L4,32,38,43,48,53,61,64)
and .ir (six create %table blocks: 4,8,12,15,19,23). This is the group_ids/CSE-guard
invariant (distinct views sharing one DataTable model by identity). The artifact
prints `table=%table:N` on both views of each pair — FAITHFUL. Whether to add an
owner/reader distinction (`table=` vs `reads=`) is a spec ENHANCEMENT question, not
a correctness defect. The current rendering is deterministic and truthful. No change
required for a valid golden; flag for spec taste only.

### F6 [LOW] — Q7 (stratum source): CONFIRMED CORRECT, no offset

I verified all 20 strata against the DOT `STRATUM N` cells (select.0=0, select.1=1,
tuple.2=2, tuple.3-7=5, tuple.8=6, tuple.9=8, tuple.10=4, tuple.11=5, tuple.12=3,
join.13-15=5, join.16=7, merge.17=5, merge.18=5, insert.19=9). Every artifact
`stratum=N` matches its DOT cell exactly. The DOT annotates from Stratify (the same
pass `QueryView::Stratum()` reads), so the accessor returns the DOT number with no
offset. Q7's low-risk flag is resolved: CORRECT.

### F7 [LOW] — the multi-line rule-reproduction header (lines 42-45) is unspecified provenance the spec does not pin

Lines 43-45 reproduce the .dr clauses (`;; path(F,T) : edge_2(F,T).` etc.) — these
match cases/demand_tc_witness.dr lines 29-32 verbatim, so they are truthful. But the
spec/ir-dump-formats §1 only shows a single abridged `;; module: tc(...)` comment;
ir-dump-formats Q2 explicitly leaves clause provenance OPEN ("only where the parse
links survive optimization"). For a byte-compare golden, every header byte must be
emitter-produced. If the emitter does NOT reproduce clauses (the spec doesn't say it
does), these 4 lines DIVERGE. RECOMMEND: drop the clause reproduction or pin it in
spec; the `;; block id = det_seq...` explanatory comment (lines 47-50) has the same
issue (it is meta-commentary no emitter would print). A golden should contain ONLY
what the emitter emits.

--------------------------------------------------------------------
## CROSS-CHECKS THAT PASSED (confirming the artifact's spine is sound)

- BLOCK INVENTORY: exactly 20 live views, kinds and det_seq numbering all correct.
  ForEachView kind order (Query.h:1178-1207) matches the artifact's §2.1 sequence
  (selects, tuples, kv_indices, joins, maps, aggregates, merges, negations,
  compares, inserts). No invented or missing view.
- ID RULE FIRMNESS: FinalizeColumnIDs (Columns.cpp:13-25) iterates ForEachView and
  assigns col ids 1.., monotone per view — so col-id order == det_seq order. The
  artifact's within-kind ordering (tuples by c4<c6<c8<c10<c11<c12<c14<c15<c17<c19<c21;
  joins c22<c25<c27<c29; merges c31<c33) is FIRM, confirmed. (Minor: the insert's
  DOT c0/c1 are INPUT indices not finalized ids, so the §2.1 "ins c0,c1" evidence
  cell is slightly muddled — but insert.19 owns no output cols and is last in
  ForEachView, so det_seq=19 is correct regardless.)
- EDGE SET: I reversed all 24 producer→user edges. Every `=>` line, every `callers:`
  annotation, and every arg map (`dst=src`) traces to exactly one DOT edge with the
  correct direction — EXCEPT the join-pivot-projection under-declaration (F1) and
  the back-edge comments (F0). The tricky merge-input vs ported-edge direction (a
  MERGE edge `vMerge -> vView` is a use→def input edge per Format.cpp:704-706, while
  a `vX:pN -> vY:cM` port edge is also use→def) was handled CORRECTLY throughout:
  tuple.5 reads merge.17 (input) AND feeds merge.18 (output) — the artifact got both
  directions right (lines 75-77).
- STRATA: all 20 correct (F6).
- TABLE ASSIGNMENTS: all six models correct, table-less views correct (F5).
- CLASS=monotone: DEFENSIBLE — zero purple edges in DOT, zero overdelete/rederive,
  all 9 update-counts are `+recursive`/`+nonrecursive` in the .ir. Inference is
  correct for this program.
- MESSAGE NAMES: edge_2/2, demand__reachable_from_bf/1 both confirmed (DOT L6/L8,
  .ir L82/L88). ABI-suppression note truthful.
- dst=src direction: verified correct on sampled edges.

--------------------------------------------------------------------
## VERDICT

UNSOUND (as a byte-compare golden), but the STRUCTURAL SKELETON is sound and
salvageable. The block inventory, id rule, strata, tables, class, and ~all edges
are correct and well-evidenced. The artifact CANNOT be blessed as-is because:

  1. [CRITICAL F0] The back-edge comments match neither a literal spec rule (which
     fires on 7 edges) nor an induction-membership rule (4 edges) — and the
     writer's Q1 evidence for this is BACKWARDS (claimed 0 fires; actually 7). The
     spec §1.3 rule is unsound by OVER-firing and must be redefined before this
     surface is golden-able.
  2. [HIGH F1] JOIN output signatures under-declare (omit the pivot column that is
     provably projected to consumers on joins 14/15/16) — the header arity
     contradicts the `=>` arg count.
  3. [HIGH F1b] JOIN port rendering (producer-det_seq regrouping) diverges from the
     deterministic accessor order a natural emitter emits; the underlying order is
     already deterministic, so Q4's remedy is both unneeded and divergent.
  4. [MED F2] A fabricated `producer=DEMAND-SEED` token with no ground-truth source.

These are FIXABLE: redefine the back-edge rule (F0), include join pivots in output
signatures (F1), reuse accessor port order (F1b), drop the speculative producer line
(F2), and resolve the spec deviations (F3-F4, F7). The writer's LOUD frictions Q1,
Q3, Q4, Q5 are all real spec gaps (Q1's evidence inverted); Q6, Q7 are benign
(confirmed). The desired-state is a good-faith, mostly-accurate reconstruction whose
defects are concentrated in exactly the spec-underspecified regions the writer flagged
— the spec must be amended and the artifact re-derived before T3 blesses `df opt`.
