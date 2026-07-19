# Adversarial critique — t2-desired-df-transitive_closure.md

Critic pass at branch keyed-instances @ b577735e. Falsifying the desired-state
`-df-out` golden against ground truth:
- DOT: scratchpad/dump-inputs/transitive_closure.dot (read this session)
- IR:  scratchpad/dump-inputs/transitive_closure.ir
- SRC: tests/OptDiff/cases/transitive_closure.dr
- CODE re-read: lib/DataFlow/Query.h:1176-1214 (ForEachView kind order),
  lib/DataFlow/Columns.cpp:13-25 (FinalizeColumnIDs).
- SPEC: KeyedInstances.artifacts/t2-dump-spec.md §1; ir-dump-formats.md §1.

Ground-truth column names as printed in the DOT ports (the SAME id space the
spec §1.3 mandates the dump use):
  select.0 c1=From  c2=To
  tuple.1  c3=From  c4=To
  tuple.2  c5=AutoVar_2  c6=Node
  tuple.3  c7=From  c8=X
  tuple.4  c9=Node
  tuple.5  c10=Node
  tuple.6  c11=From c12=To
  tuple.7  c13=From c14=To
  tuple.8  c15=Node
  tuple.9  c16=From c17=To
  join.10  c18=X(pivot) c19=From c20=To ; ports p0=X p1=AutoVar_2 p2=From p3=Node
  merge.11 c21=From c22=To
  merge.12 c23=Node

---

## FINDING 1 — CRITICAL. tuple.2 and tuple.3 block headers print the WRONG column names (From,To instead of the finalized names) — the golden would fail byte-compare against a spec-conformant emitter.

DEFECT. §1 lines 65 and 69:
  `tuple ^tuple.2 (From:u64, To:u64)`
  `tuple ^tuple.3 (From:u64, To:u64)`

But the finalized columns of these two views are:
  tuple.2 = (AutoVar_2:u64, Node:u64)   [DOT line 29: c5=AutoVar_2, c6=Node]
  tuple.3 = (From:u64, X:u64)           [DOT line 32: c7=From, c8=X]

Spec §1.3 (t2-dump-spec.md:65-67): "Column naming: `<var-or-cN>:<type>` where N
is the FINALIZED column id (FinalizeColumnIDs ... the same id space the DOT
ports print, deterministic)." The DOT ports print AutoVar_2 / Node / From / X.
A spec-conformant emitter reads `col->Variable()` / the finalized id from the
QueryColumnImpl DefList in ForEachView order (Columns.cpp:13-25) and will emit
those exact strings — never `To` for c6/c8. So the emitter's output diverges
from this golden on lines 65 and 69; the strict `cmp -s` gate (spec §3.2) fails
at first bless.

EVIDENCE OF SELF-CONTRADICTION: the artifact's OWN derivation §2.1 (lines
160-161) lists `^tuple.2 [c5=AutoVar_2, c6=Node]` and `^tuple.3 [c7=From, c8=X]`,
and §2.2 lines 203/211 quote the DOT verbatim. The §1 golden text contradicts
the artifact's own evidence table. This is not an open question — it is an
error in the deliverable (the golden is the contract; §3 F4 mischaracterizes
this as a mere "convention" choice, see Finding 3).

Even under the artifact's OWN stated F4 rule ("params = OUTPUT columns"), the
output columns are AutoVar_2/Node and From/X — NOT From/To — so the rendering is
wrong on its own terms, not just against the spec.

FIX: header lines must read
  `tuple ^tuple.2 (AutoVar_2:u64, Node:u64)`
  `tuple ^tuple.3 (From:u64, X:u64)`
(or the `c5`/`c6`/`c7`/`c8` fallback spelling). NB tuple.4/tuple.5/tuple.6/
tuple.7/tuple.8/tuple.9/tuple.1 headers are all CORRECT (their finalized names
happen to be From/To/Node) — only tuple.2 and tuple.3 carry non-From/To
finalized names and only those two are wrong.

---

## FINDING 2 — CRITICAL (cascades from F1). The `=>` column-maps on tuple.2 and tuple.3 name a nonexistent `From`/`To` source, inverting/inventing the dst=src map.

DEFECT. §1 lines 67 and 71:
  tuple.2:  `=> ^join.10 .rhs(AutoVar_2=From, Node=To)`
  tuple.3:  `=> ^join.10 .lhs(From, X=To)`

Spec §1.3 (t2-dump-spec.md:80): "`=>` lines ... arguments as `dst=src` column
maps". The SRC of an `=>` edge is a column of the PRODUCING view (tuple.2 /
tuple.3). tuple.2's columns are {AutoVar_2, Node}; tuple.3's are {From, X}.

- `AutoVar_2=From`: src `From` is not a column of tuple.2. Invalid.
- `Node=To`:        src `To` is not a column of tuple.2. Invalid.
- `X=To`:           src `To` is not a column of tuple.3. Invalid (tuple.3 has X, not To).

The map appears to be written against the fictitious (From,To) header from F1
rather than the real producer columns. The producer→port edge from the DOT is:
  tuple.2: c5(AutoVar_2)→join p1 ; c6(Node)→join p3
  tuple.3: c8(X)→join p0(pivot)  ; c7(From)→join p2
A correct `dst=src` map (dst=join port name, src=producer col) is e.g.
  tuple.2: `.rhs(AutoVar_2=AutoVar_2, Node=Node)`  (or the port-index form)
  tuple.3: `.lhs(X=X, From=From)`
depending on the F3 port-labeling rule — but in NO reading does `From`/`To`
appear as a tuple.2/tuple.3 source. This line diverges byte-for-byte from any
correct emitter output.

NOTE the JOIN BLOCK BODY is internally correct and inconsistent with these two
lines: §1 lines 98-99 render
  `.lhs <- ^tuple.3 (From, X)`      (correct: tuple.3's real cols)
  `.rhs <- ^tuple.2 (X=AutoVar_2, To=Node)`  (correct: pivot AutoVar_2, To←Node)
So the artifact renders the SAME edge two ways — correctly in the join block,
incorrectly in the tuple blocks. Whichever the emitter picks, the two cannot
both be goldens. (Also a redundancy question for the spec: the join↔producer
edge is printed twice, once per endpoint; F3 should also settle whether the
producer-side `=>` even renders for JOIN inputs or only the `.lhs/.rhs <-`
lines do.)

---

## FINDING 3 — MED (spec + artifact). The rename convention (F4) is genuinely unratified AND the artifact applies it inconsistently — this is what produced F1/F2, so it must be settled before first bless.

DEFECT (spec gap, correctly raised as F4 but under-weighted). The dump must
choose ONE column-naming rule for a renaming TUPLE:
  (A) params = the view's OWN finalized columns (what the DOT ports print,
      what FinalizeColumnIDs assigns) — deterministic, mechanical. Under (A),
      tuple.2=(AutoVar_2,Node), tuple.3=(From,X). This is the ONLY rule
      consistent with spec §1.3's stated "same id space the DOT ports print".
  (B) params = the columns the block RECEIVES (ir-dump-formats §1.1's prose:
      "block parameters = the columns the block RECEIVES"). Under (B) a tuple
      renaming tc.From→Node would show its INPUT names.

The artifact claims to pick (A) ("params = OUTPUT columns", F4 line 355) but
then renders tuple.2/tuple.3 as (From,To) which is NEITHER the output names
(AutoVar_2/Node, From/X) NOR a coherent input-name reading — it looks like the
UNION's column names (From/To) leaked down onto the tuples that read it. That
leak is the root cause of F1 and F2.

CONSEQUENCE: this is not merely "needs a one-line spec ruling" (F4's framing).
Until the rule is fixed, THREE of the sixteen blocks (tuple.2, tuple.3, and the
tuple.4/5 rename hint) are unverifiable, and two of them (tuple.2, tuple.3) are
demonstrably wrong. The spec §1.3 as written already implies (A); the artifact
should be corrected to (A) rather than waiting on ratification.

Note ir-dump-formats §1.1 ("parameters = columns the block RECEIVES") and
t2-dump-spec §1.3 ("finalized column id ... DOT ports print") are in TENSION —
a receiving-tuple's received columns differ from its output columns under
rename. The spec should explicitly resolve this; recommend (A) since it is the
only mechanically-deterministic, DOT-cross-checkable choice.

---

## FINDING 4 — LOW/spec (F3 confirmed real; structure is right). JOIN lhs/rhs role labels and the port rename direction are not graph-carried; the desired-state's binding is defensible but the spec must pin it.

CONFIRMED as a real spec gap, not an artifact error. The DOT carries only
ordered ports p0..p3 with producers (lines 14-17):
  p0=X(pivot)←tuple.3.c8 ; p1=AutoVar_2(pivot)←tuple.2.c5 ;
  p2=From←tuple.3.c7      ; p3=Node←tuple.2.c6
The artifact binds lhs=tuple.3 (p0/p2), rhs=tuple.2 (p1/p3). Nothing in the
graph fixes which producer is "lhs" vs "rhs"; the emitter could key off
joined_views order and flip them. The STRUCTURE the artifact asserts (pivot X;
output From←p2, To←p3; two producers tuple.3/tuple.2) is FIRM and matches the
DOT exactly. Only the lhs/rhs LABELS are a naming choice. Spec must pick
p-index rendering (faithful) vs role labels (needs a defined lhs/rhs rule).
Non-blocking for structure; blocking for byte-golden (a flip changes bytes).

---

## FINDING 5 — LOW (F5 confirmed; class=monotone is CORRECT). Recursion does not imply differential — verified against the .ir.

The artifact's class assignment is CORRECT and its F5 friction is a valid spec
clarification, not a defect. Verified: transitive_closure.ir shows only
`update-count +nonrecursive` / `+recursive` (lines 32, 71, 85-86) and NO
overdelete/rederive; add_edge is a #message with no deletion (.dr:1); the DOT
colors no node purple. So every table-backed view (tuple.2, tuple.3, merge.11,
insert.13/14/15) is correctly class=monotone even though tc is recursive
(SET 0 DEPTH 1, the induction). The spec's three-class vocabulary should state
that recursion alone does not imply differential (class keys off
deletion-capability). If an emitter instead keyed class off "has an induction",
every %table:4 label would wrongly flip to differential and this golden would
break — the spec must nail the definition. Agree with the artifact.

---

## FINDING 6 — LOW (F1 restated as I confirm it). INSERT det_seq ids 13/14/15 are genuinely unwitnessed; the artifact's own hedge is correct. Relative order is plausible but MUST be checked at first bless.

CONFIRMED. INSERTs carry no output columns (DOT MATERIALIZE nodes use c0/c1
LOCAL port indices, not finalized col ids — lines 18/21/24), so FinalizeColumnIDs
assigns them no finalized id and the col-id monotonicity proof that firms
det_seq 0..12 does NOT reach 13/14/15. The kind ordering (ForEachView:
inserts LAST, Query.h:1205) firms that all three come after both merges. The
INTERNAL order 13<14<15 is predicted from DefList creation order; the artifact
correctly flags it ILLUSTRATIVE-PREDICTED and correctly notes the strata
(9<10<11) are consistent with it. Non-blocking IF the first bless is reviewed
against a det_seq print (as the artifact recommends). Recording as a real
residual: if the inserts' DefList order differs, insert.13/14/15 ids AND the
`=> ^insert.N` targets on tuple.6/tuple.7/tuple.8 move together.

DETERMINISM CHECK (dimension 3): the insert ordering is DefList-insertion order,
which is pointer-free (a std::vector), so whatever the order is, it is stable —
no pointer-derived token here. The uncertainty is only about WHAT the order is,
not whether it is deterministic. Clean on determinism.

---

## FINDING 7 — LOW. det_seq firmness argument re-audited against pass order — SOUND.

Verified the artifact's firmness proof holds. det_seq's LAST stamp is
IdentifyInductions head (Build.cpp:2597 per consolidated §4.b); FinalizeColumnIDs
runs at 2603; between them FinalizeDepths (2602) mints/kills no views and does
not reorder DefLists. So ForEachView yields the identical sequence at 2597 and
2603 → the finalized col-id order (readable from the DOT) IS the block-id
det_seq order. The mapping det_seq 0..12 ↔ c1..c23 is therefore FIRM as the
artifact claims. CSE's earlier re-stamp (Optimize.cpp:287, run at 2585) is
superseded by the 2597 stamp, so it does not perturb the drained ids. No defect;
recording that I confirmed the load-bearing pass-order claim rather than trusting
the citation (E-54 discipline).

---

## FINDING 8 — LOW (additions beyond spec letter: F6/F7/F8/F9). Enumerated; none break determinism, each needs a keep/drop ruling.

- F6 `; callers:` on every non-source block: an addition (ir-dump-formats §1
  showed callers only on MERGE fan-in). Deterministic (callers listed in a
  fixed order — verify the emitter sorts them by det_seq; the artifact lists
  merge.11 callers as tuple.1,tuple.9 and merge.12 as tuple.4,tuple.5, both
  ascending det_seq — GOOD, but the spec must MANDATE the sort key or the
  caller list order is emitter-defined and could be pointer/use-list order).
  >> SUB-RISK (MED-adjacent): if the emitter builds the caller list by walking
  the use-list (QueryColumnImpl uses), that iteration order must be proven
  det_seq-sorted; otherwise `; callers:` is a determinism HOLE. The artifact
  does not cite where the caller list comes from. Flag for the emitter spec.
- F7 `set=0 depth=1` on merge.11: faithfully surfaces DOT `SET 0 DEPTH 1`
  (line 50). Deterministic (small ints). Keep/drop + pin spelling.
- F8 `;; module:` clause header: acknowledged largely invented; post-optimize
  clause strings are not reliably reconstructable (spec Q2). The clause LIST
  also reorders vs source (.dr lists is_node query at line 12, after the tc
  local; the header groups all three queries first) — harmless only because
  the header is illustrative. If any part is emitted it should be `;; module:
  transitive_closure` alone. Do not let the clause list become golden.
- F9 back-edge over-report: the `<=` rule flags 8 edges incl. merge.11=>
  tuple.6/tuple.7 which exit the cycle. This is spec-intended (ids-not-layout)
  and arithmetically correct given the det_seq order; verified each mark
  (join.10=>tuple.1, merge.11=>tuple.2..7, merge.12=>tuple.8) is exactly the
  set with user<=def. Cosmetic-noise concern only; consider `; id<=` renaming.

---

## FINDING 9 — LOW (F2 confirmed). No producer line is correct for this case.

CONFIRMED. transitive_closure compiles WITHOUT -demand (no .drflags sidecar;
.dr has no forcing), so no view carries a producer tag and the artifact
correctly emits no `producer=` anywhere. Spec §1.3 says producer prints only
when non-empty (debug builds, pass-minted views like DEMAND-*). Agree — a
reviewer must not expect a producer line here. Recording that the spec's "T3
goldens blessed from DEBUG preset, producer IS golden-visible" line
(t2-dump-spec.md:72-79) is per-CASE conditional: for THIS case debug and
release agree (empty), so the golden is preset-agnostic. No defect.

---

## POSITIVE VERIFICATION (what I checked and found SOUND)

- EDGE COMPLETENESS (dimension 1): mechanically reconciled all 18 `=>` push
  edges in §1 against the DOT dependency arrows (converting union-member arrows
  and read arrows to canonical producer→user). RESULT: exact match — zero
  missing, zero invented edges. No fabricated or dropped view/op. The 16 blocks
  (SELECT×1, TUPLE×9, JOIN×1, MERGE×2, INSERT×3) exactly match the DOT's 16
  view nodes.
- STRATA: all 16 strata match the DOT STRATUM values node-by-node (select 0,
  tuple.9 1, tuple.1/2/3 2, tuple.4 3, tuple.5 4, tuple.6 5, tuple.7 6,
  merge.12 7, tuple.8 8, join.10 2, merge.11 2, insert.13/14/15 9/10/11).
- TABLE annotations: table=%table:4 on tuple.2, tuple.3, merge.11, insert.13,
  insert.14; table=%table:8 on insert.15 — matches DOT TABLE 4 / TABLE 8 exactly
  (and cross-references the .ir %table:4[u64,u64] / %table:8[u64], lines 9/17).
- class= assignments (monotone on all 6 table-backed views, table-less on the
  rest) verified against the .ir (no overdelete/rederive) — CORRECT (Finding 5).
- ID-RULE (dimension 2): ForEachView kind order in the artifact
  (selects→tuples→kv→joins→maps→aggs→merges→negations→compares→inserts) matches
  Query.h:1178-1207 VERBATIM. The det_seq↔col-id firmness proof holds under the
  real pass order (Finding 7).
- back-edge marks: all 8 arithmetically correct under the `<=` rule.
- JOIN block body structure (pivot X, output From←p2 To←p3, producers
  tuple.3/tuple.2) matches DOT ports p0..p3 exactly.
- DETERMINISM (dimension 3): every id/name token is det_seq- or
  finalized-col-id- or fixed-enum- derived; no pointer-derived value appears.
  EXCEPTION FLAGGED: the `; callers:` list order is not proven det_seq-sorted by
  the artifact (Finding 8 sub-risk) — the emitter must sort it or it is a hole.
  The artifact's listed caller orders happen to be ascending det_seq.

---

## VERDICT: UNSOUND (as a byte-compare golden), structurally SOUND.

The desired-state has EXACTLY the right graph: every view, every edge, every
stratum, every table, every class, and the id-ordering RULE are all verified
correct against ground truth. But the deliverable is a strict byte-compare
golden (spec §3.2, `cmp -s`), and it contains two CRITICAL column-naming errors
that a spec-conformant emitter would NOT reproduce:

  F1 (CRITICAL): tuple.2 header must be (AutoVar_2:u64, Node:u64), tuple.3 must
     be (From:u64, X:u64) — the artifact prints (From,To) for both, contradicting
     the DOT ports AND its own §2 evidence table.
  F2 (CRITICAL): the `=>` column-maps on tuple.2/tuple.3 name nonexistent From/To
     sources, cascading from F1.
  F3 (MED, spec): the rename-tuple naming rule (F4) is unratified and applied
     inconsistently; spec §1.3 and ir-dump-formats §1.1 are in tension. Resolve
     to rule (A) = view's own finalized columns before first bless.

Blocking-for-bless items: F1, F2 (fix the two block headers + their edge maps),
F3 (pin the naming rule), F4 (pin JOIN lhs/rhs label rule — bytes depend on it).
Non-blocking, spec-clarification: F5 (class def), F6 caller-order MUST be pinned
to avoid a determinism hole, F7/F8/F9 keep-or-drop.

The artifact's own frictions list is high quality: F1/F3/F5 correctly
self-identify the real risks. The gap is that F4 UNDERSELLS its severity — it is
not a cosmetic convention choice, it is the source of two concrete wrong lines
in the shipped golden text. Correct the two headers + two edge lines, pin the
three naming rules, and this becomes a correct golden.
