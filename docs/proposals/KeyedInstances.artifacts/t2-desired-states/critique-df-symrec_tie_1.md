# Adversarial critique — t2-desired-df-symrec_tie_1.md

Critic pass at branch keyed-instances @ b577735e. Falsifying the desired-state
`-df-out` artifact against: the ground-truth `.dot`/`.ir`
(dump-inputs/symrec_tie_1.{dot,ir}), the code (ForEachView Query.h:1176-1214,
det_seq Query.h:465-472), and the spec (t2-dump-spec.md §1).

Method per the E-54 lesson: every citation re-read this session; every block/op
checked against the DOT node it claims; the det_seq RULE checked against the
code.

--------------------------------------------------------------------
## Verified-correct (the load-bearing spine holds)

- **ForEachView kind order** (artifact §2.1): SELECT·TUPLE·KVINDEX·JOIN·MAP·
  AGG·MERGE·NEGATE·COMPARE·INSERT — MATCHES Query.h:1178-1207 exactly. The
  det_seq BANDS fall out correctly: SELECT=0, TUPLE=1..7, JOIN=8..9, MERGE=10,
  INSERT=11. FIRM ids are right.
- **det_seq is the right key** (artifact §2.1, spec §1.2 decision (a)): det_seq
  Query.h:472 is the pointer-free dense stamp; the artifact does not use
  UniqueId. Conforms.
- **Census** (7 TUPLE, 2 JOIN, 1 SELECT, 1 MERGE, 1 INSERT = 12 views):
  matches the .dot node count (v-nodes: 4387028560 SELECT, 4387025776 JOIN,
  4387030656 JOIN, 4387036480 INSERT, 4387023856/4387024304/4387024752/
  46523432960/46523433408/46523433856/46523434304 = 7 TUPLE, 4387021296
  MERGE). Exactly 12. No invented or missing view.
- **The determinism WITNESS claim itself** (§2.2): the two JOINs are
  structurally symmetric and only det_seq separates them. Sound in spirit and
  correctly identified as the artifact's purpose. See F-A below for the one
  place the arm-labelling evidence needs a caveat.

**Arm identity re-verified from the DOT (independent of the artifact):**
- JOIN v4387025776 (^join.8, EQ9): pivot inputs p0→tuple6.c8(B), p1→
  tuple3.c11(A) (.dot:10-11). tuple6=tc(TABLE4), tuple3=edge(TABLE8). So
  tc.B==edge.A ⇒ `tc(A,X),edge(X,B)` = arm2 (tc-first). CORRECT.
- JOIN v4387030656 (^join.9, EQ10): pivot inputs p0→tuple2.c14(X), p1→
  tuple6.c7(A) (.dot:15-16). tuple2=edge, tuple6=tc. So edge.X==tc.A ⇒
  `edge(A,X),tc(X,B)` = arm3 (edge-first). CORRECT.
The .ir cross-check (§2.2, .ir:72-96) is also correct: join at .ir:74-75 reads
tc via %index:55[_,u64] (keyed col6=B) — the tc-first arm — matching ^join.8's
DOT. The second join .ir:89-90 reads tc via %index:47[u64,_] (keyed col5=A) =
edge-first = ^join.9. The arm→block-id assignment is SOUND.

--------------------------------------------------------------------
## FINDINGS

### F-A [HIGH → but self-disclosed as F1] — intra-TUPLE det_seq permutation is INVENTED, and it is not merely illustrative: the STRATUM labels bound to ^tuple.1..7 are a GUESS that may be internally inconsistent

The artifact assigns STRATUM values to each ^tuple.N (e.g. ^tuple.1 stratum=1,
^tuple.2 stratum=2, ^tuple.3 stratum=3, ^tuple.4/5 stratum=4, ^tuple.6 stratum=4,
^tuple.7 stratum=5). These STRATUM values are READ FROM THE DOT and are firm PER
DOT-NODE. But the det_seq→DOT-node binding for the seven TUPLEs is admittedly a
guess (F1). The DEFECT: the artifact pairs a guessed det_seq (1..7) with a firm
per-node STRATUM, producing rows like `^tuple.1 ... stratum=1`, `^tuple.2 ...
stratum=2`, `^tuple.3 ... stratum=3` that LOOK like a derived correspondence
(det_seq==stratum) but are NOT — det_seq is ForEachView/creation order, STRATUM
is a topological layer; there is no reason the two coincide. The three
SELECT-fed edge-guard tuples in the DOT carry STRATA {1,2,3} (v46523434304=1,
v46523433856=2, v46523433408=3); the artifact silently assumes creation order
tracks these strata 1→2→3. That is unsupported. When the emitter lands and the
real permutation is read, any of the 7! orderings of (det_seq ↔ DOT-node) is
possible, and the stratum column will then NOT ascend 1,2,3,4,4,4,5.

Evidence: DOT strata are v46523434304=STRATUM1, v46523433856=STRATUM2,
v46523433408=STRATUM3 (.dot:40,37,34); EQ2 v4387023856=STRATUM4 (.dot:22),
EQ3 v4387024304=STRATUM4 (.dot:25), v4387024752(TABLE4)=STRATUM4 (.dot:28),
v46523432960=STRATUM5 (.dot:31). det_seq Query.h:472 is ForEachView order,
NOT stratum. Severity HIGH because the artifact is destined to be a byte-compare
golden and the paired (id,stratum) rows will almost certainly be wrong at bless;
but the writer flagged this as F1 and correctly says the JOIN pair (not the
TUPLEs) is the witness. Recommendation: the ILLUSTRATIVE label must extend to
the stratum column of the tuple rows too, not just the id — the artifact
currently presents stratum as firm ("read directly from the .dot", §2.4) while
the id↔node binding that selects WHICH stratum is a guess. Amend §2.4 to say the
tuple STRATUM values are only firm once the id↔node binding is pinned at bless.

### F-B [MED] — the JOIN block grammar is INVENTED beyond the spec and one rendered token (`.rhs out B = tuple.3.X`) mislabels the column role

The JOIN grammar in §1 (the `pivot X <- A.col, B.col` line + per-side
`.lhs/.rhs <- view out Col = view.col` lines) is a genuine expansion beyond both
ir-dump-formats §1.2 (which sketches only `.lhs <- ^merge.4 (F, X=T)`) and
t2-dump-spec §1.3 (which does not give a JOIN grammar at all). The writer
discloses this as F4 and the motivation (keep the arm asymmetry visible) is
legitimate. But two problems:

(1) The invented grammar is NOT spec-conformant text — it cannot become a
byte-golden until the spec ratifies it. As written the artifact claims to be
"rendered per t2-dump-spec §1.2/§1.3" (§1 header) but the JOIN block is rendered
per a grammar the writer authored. This is a spec GAP (finding against the
spec), correctly surfaced, but the artifact overstates its conformance.

(2) Cross-check the specific out-column claims against the DOT. ^join.8 outputs
(.dot:12-13): p2→tuple6.c7(A) gives output A; p3→tuple3.c12(X) gives output B.
So ^join.8 output B is sourced from edge's c12=X. The artifact writes
`.rhs <- ^tuple.3  out B = tuple.3.X` — CORRECT. ^join.9 outputs (.dot:17-18):
p2→tuple2.c13(A) gives output A; p3→tuple6.c8(B) gives output B. The artifact
writes for join.9 `.lhs <- ^tuple.6 out B = tuple.6.B` and
`.rhs <- ^tuple.2 out A = tuple.2.A` — CORRECT. The column-role rendering is
accurate. So (2) is CLEAN; only (1) (unratified grammar) stands. Severity MED:
right data, unsanctioned form.

### F-C [HIGH] — the `; back-edge` markers are INCONSISTENTLY applied: two real back-edges (by the spec's own rule) are UNMARKED

Spec §1.3 defines `; back-edge` mechanically: mark a `=>` line when
user det_seq <= def det_seq. The artifact marks ONLY merge.10's two out-edges
(→tuple.6 id6, →tuple.7 id7; 6<=10, 7<=10). But applying the SAME rule to the
OTHER blocks, using the artifact's OWN guessed ids:

- ^tuple.6 (def det_seq 6) `=> ^join.8 .lhs` (user 8) and `=> ^join.9 .lhs`
  (user 9): 6<=8 and 6<=9 are FALSE (user > def) → correctly UNMARKED. OK.
- ^join.8 (def 8) `=> ^tuple.4` (user 4): 8<=4 FALSE → not a back-edge by the
  rule; but 4 < 8 so this edge points to a LOWER id — under the rule "user
  det_seq <= def det_seq" means user(4) <= def(8) which is TRUE → this IS a
  back-edge and is UNMARKED in §1 (line 118). **DEFECT.**
- ^join.9 (def 9) `=> ^tuple.5` (user 5): 5 <= 9 TRUE → back-edge, UNMARKED
  (line 126). **DEFECT.**
- ^tuple.4 (def 4) `=> ^merge.10` (user 10): 10 <= 4 FALSE → not marked. OK.
- ^tuple.5 (def 5) `=> ^merge.10` (user 10): FALSE → not marked. OK.

So by the spec's literal rule (user <= def ⇒ mark), join.8→tuple.4 and
join.9→tuple.5 MUST carry `; back-edge` and do not. The artifact applied the
rule as "def <= user" (or "target id lower than source") for the merge edges but
did not apply it uniformly. Note this is the OPPOSITE reading from what F8
discusses — F8 argues the marker UNDER-reports the cycle; in fact under a
consistent reading it should mark MORE edges (join→tuple downward edges), and
the artifact marks FEWER than its own stated rule requires. Severity HIGH: this
is a byte-golden and the marker set is internally inconsistent with the cited
rule. RESOLUTION NEEDED: the spec must pin the exact comparison direction
(user<=def vs def<=user) and the artifact must apply it uniformly; whichever is
chosen, the merge edges and the join→tuple edges are on the SAME side of the
comparison (both go to lower-id targets), so they must be marked identically.

### F-D [MED] — `class=` is inferred, not code-pinned; and the artifact hardcodes `class=monotone` for the edge-table tuples on an unverified deletion premise

The artifact (and its F2) infers class from {table present? deletions?}. The two
edge-backed tuples ^tuple.2/^tuple.3 are labelled `class=monotone`. This rests on
"edge is a #message ⇒ non-differential ⇒ monotone". That premise is plausible
but NOT verified against a QueryView accessor this session — the spec §1.3 names
the vocabulary {differential,monotone,table-less} without an accessor mapping.
The DOT gives NO class signal (it prints only TABLE ids). So `class=monotone` on
^tuple.2/3 and `class=differential` on ^tuple.6/^merge.10/^insert.11 are
UNVERIFIED against code. They are internally plausible (tc receives recursive
deletions ⇒ differential; edge does not) but the artifact should NOT present them
as firm ("read directly from the .dot ... class", §2.4) — the DOT carries no
class. Severity MED: likely-correct inference, but the emitter decision
(CanReceiveDeletions()/table-presence) is unmade and the §2.4 "firm" claim
overreaches. Same overreach applies to F2's edge case (i): whether a table-backed
monotone view prints BOTH `table=` and `class=monotone` is an unmade spec
decision the artifact silently resolves ("print both").

### F-E [LOW] — the `=>` port token for JOIN inputs uses `.lhs/.rhs` but the DOT/spec have no lhs/rhs naming for JOIN sides; side assignment is invented

The artifact writes `=> ^join.8 .lhs(...)` / `.rhs(...)` on producer blocks and
correspondingly `.lhs <-`/`.rhs <-` inside JOIN blocks. The DOT numbers JOIN
ports p0..p3 (pivot inputs then merged outputs) with NO lhs/rhs distinction; the
spec sketch (ir-dump-formats §1.2) uses `.lhs/.rhs` illustratively but does not
define which incoming view is lhs vs rhs. The artifact assigns tc-side=lhs,
edge-side=rhs consistently, which is a reasonable convention but is INVENTED and
must be pinned by the emitter (e.g. by JoinedView ordering). Because the two arms
swap which relation supplies the pivot, an arbitrary lhs/rhs convention could
render them confusingly; the artifact's choice (tc always lhs) keeps them
parallel, which is defensible. Severity LOW (convention, disclosed under F4), but
the lhs/rhs↔side binding is another unpinned emitter decision.

### F-F [LOW / confirm] — F7 (out shares tc's %table:4) is CORROBORATED by the .ir but the semantics deserve an owner confirm

Independently checked: the .ir declares ONLY %table:4 (tc, cols A/B .ir:9-12) and
%table:8 (edge, cols A/X .ir:17-20). There is NO table for `out`. The DOT
annotates the MATERIALIZE-out INSERT with TABLE 4 (.dot:19). So the out INSERT's
TableId()==4 is real (out is materialized through tc's backing store — out(A,B):
tc(A,B) is a pure forwarding, so the optimizer shares the store). The artifact's
`table=%table:4 class=differential` on ^insert.11 matches ground truth. This
confirms F7 is NOT a frozen-binary artifact; it is the real TableId() semantics.
No change needed to §1; the flag can be downgraded from "surprising, confirm" to
"confirmed intended".

### Edge/column-map spot checks (all PASS — no invented or missing edges)

- ^select.0 => three edge-guard tuples: DOT SELECT feeds v46523434304/
  v46523433856/v46523433408 (.dot:35-42, three tuples read c1/c2). 3 edges. OK.
- ^tuple.1 => ^merge.10 (A=A,B=X): base tuple c15=A,c16=X → merge c23/c24.
  Map B=X correct (edge's X becomes tc's B). OK.
- ^tuple.6 => ^join.8.lhs(A=A,X=B) & ^join.9.lhs(A=A,X=A): tuple6 c7=A,c8=B;
  join.8 reads c8(B) as pivot + c7(A) as out; join.9 reads c7(A) as pivot +
  c8(B) as out. The pivot maps X=B (join.8) and X=A (join.9) correctly capture
  the arm asymmetry. OK — this is the single best-rendered part of the artifact.
- ^merge.10 producers = tuple.1/4/5 (.dot:44-46); users = tuple.6/7
  (.dot:29,32). Callers line and => lines both correct. OK.
- ^insert.11 reads tuple.7 c9/c10 via input-position c0/c1 (.dot:20-21). OK.

No view is invented; no DOT view is dropped; every column edge in the DOT has a
corresponding `=>` line and vice versa. The STRUCTURE is faithful.

### Determinism-by-construction audit (the byte-golden requirement)

Every token in §1 is derived from det_seq (pointer-free, Query.h:472), FINALIZED
column ids (FinalizeColumnIDs, deterministic), TableId() (post-Program, stable),
STRATUM, and class (an accessor once pinned). NO pointer-derived value and NO
container-iteration-order value appears in the rendered text. The det_seq
ORDERING RULE is applied correctly (ForEachView kind bands verified against
Query.h:1178-1207). The ONE determinism-relevant defect is F-C (inconsistent
back-edge marking) — internally inconsistent, not nondeterministic. The intra-
TUPLE permutation (F-A/F1) is a bless-time pin, not a nondeterminism source (the
emitter WILL produce a stable permutation; the artifact just guessed it).

--------------------------------------------------------------------
## VERDICT: SOUND-WITH-AMENDMENTS

The artifact's LOAD-BEARING claim — that the two structurally symmetric recursive
arms are separated ONLY by det_seq, arm2(tc-first)→^join.8 (lower id),
arm3(edge-first)→^join.9 — is CORRECT and independently re-verified from both the
DOT wiring and the .ir fixpoint-join index keys. The ForEachView kind order, the
det_seq bands, the census, and every structural edge/column-map match ground
truth. As a determinism witness the artifact achieves its purpose.

Amendments required before this becomes a byte-golden:
1. [HIGH, F-C] Fix the inconsistent `; back-edge` markers: join.8→tuple.4 and
   join.9→tuple.5 satisfy the spec's user<=def rule and must be marked (or the
   rule/direction re-pinned and applied uniformly). This is an internal
   inconsistency in the artifact as written, independent of the id guesses.
2. [HIGH, F-A/F1] Extend the ILLUSTRATIVE caveat to the tuple STRATUM column,
   not just the ids: §2.4 currently calls stratum "firm/read from the .dot" while
   the id↔node binding that selects each tuple's stratum is a guess. The paired
   (id,stratum) rows will likely be wrong at bless.
3. [MED, F-B/F4] The JOIN block grammar is authored, not spec-sanctioned; the §1
   "rendered per t2-dump-spec" header overstates conformance. Spec must ratify a
   JOIN grammar (the data rendered is correct; the form is unpinned).
4. [MED, F-D/F2] class= is an unmade accessor decision; do not present it as
   "read from the .dot" (the DOT carries no class). Pin to a QueryView predicate.
5. [LOW, F-E] lhs/rhs↔side binding for JOIN is invented; pin it.
6. [LOW, F-F/F7] CONFIRMED: out shares tc's %table:4 (corroborated by .ir having
   no out table). Downgrade F7 from "surprising, confirm" to "confirmed".

None of these defeat the witness. The spec-side gaps (F-B, F-D, F-E, F8) are
legitimate findings AGAINST t2-dump-spec.md, correctly surfaced by the writer's
frictions; the one artifact-internal defect that a reviewer could miss is F-C.

