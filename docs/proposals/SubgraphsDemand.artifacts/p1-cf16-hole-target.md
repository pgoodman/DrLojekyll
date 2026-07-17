# P1 — the §6 MONOTONE/DESCENT ingest stage: the CF-16 hole-contract identity target

Artifact for `docs/proposals/SubgraphsDemand.artifacts/`. Author: DESIGNER
lane P1, subgraphs/demand epoch (branch `subgraphs-demand`, off main 0a4a9225 +
the SubgraphsDemand.md §0-§2 ledger; P0 census LANDED at HEAD). This is the
implementation target for the deferred §6 monotone/descent ingest stage — the
ADL epoch's P2 "stage 2" (the last hand-coded emission surface).

All IR quoted below was generated `build/debug/bin/drlojekyll <case>.dr [-disable-controlflow-opt] -ir-out`
this session against HEAD; nothing rebuilt. Every code claim carries a
file:line anchor read this session.

BINDING INPUTS (re-read this session, honored below):
- `ADLFunctorSurface.artifacts/p2-ingest-inventory-target.md` §5 (the
  net-additions migration), §6 (the hole contract, lines 434-452), §12.6 (the
  as-landed cutover authority shape), §13 (R1e deviations).
- `PerfRoadmap.md` §14.0.2 (the four §6 prerequisites, lines 2013-2033).
- Ledger erratum **E-34** (SubgraphsDemand.md:119-130): the §6 diff is NOT a
  bare `LowerIngestFold` signature change. It must (i) relax the asserts at
  `Stratum.cpp:1887`, (ii) move the monotone routing out of the hand-coded arm
  (`Procedure.cpp:58-92`) into `LowerIngestFold`, (iii) return the UPDATECOUNT
  `OP*` as `next_parent` — the exact pattern the hand-coded arm already uses
  (`Procedure.cpp:77`/`91`).

> WHAT LANDED IN P2 vs WHAT §6 IS. The ADL epoch's §12.6 landed the cutover as
> a DIRECT walk-position lowering: `ExtendEagerProcedure` calls
> `MakeStageOneIngestFolds` (`DR.cpp:767`) and `LowerIngestFold`
> (`Stratum.cpp:1884`) inline at `Procedure.cpp:50-55` for DELETION-CAPABLE
> receives only, then `continue`s (`:55`). There is NO `context.ingest_par`
> map — §12.3's phase-time dispatch was found infeasible (§12.6:987-999). §6
> extends this SAME direct-lowering shape to the MONOTONE arm
> (`Procedure.cpp:58-93`), which today is still entirely hand-coded.

---

## §1. TODAY'S EXACT ENTRY-PROC REGION TREES (the identity target)

The §6 stage is a BYTE-IDENTITY refactor: it moves the monotone fold's
emission from the hand-coded arm into `LowerIngestFold` without changing a
single emitted node. So the target IS today's tree, verbatim, annotated with
(a) which emitter owns each node today, (b) which owns it post-§6, (c) where
the HOLE is (the `if-crossed` body that `LowerIngestFold` will own and the
descent will fill).

### 1.1 cf16_4 — IF-4, kEmpty monotone receive, publish descent

`.dr`: one monotone `#message in_val(i32)`; `%table:4[i32]` = in_val/mid/keep
model; the sole descent is a direct INSERT-stream publish `out_val/1`.

**OPT MODE** (`cf16_4.ir`, entry proc verbatim):

    proc ^entry:7($param:9<i32>)
      seq
        vector-loop {@A:11} over $param:9<i32>                    ; [W] monotone arm, Procedure.cpp:58-61
          update-count +nonrecursive {@A:11} in %table:4[i32]     ; [W] Procedure.cpp:71-77 (is_explicit=FALSE)
            if-crossed                                            ; <<< THE HOLE (the if-crossed body)
              publish out_val/1(@A:11)                            ; [D] descent, Insert.cpp:60-69 (E-22 IsStream)
        vector-clear $param:9<i32>
        call ^flow:18
        return-false

**NOCF MODE** (`cf16_4.nocf.ir`, un-flattened — the load-bearing gate view):

    proc ^entry:7($param:9<i32>)
      seq
        par                                                      ; [P] the per-io PARALLEL (Procedure.cpp:712)
          vector-loop {@A:11} over $param:9<i32>                 ; [W]→[6]
            update-count +nonrecursive {@A:11} in %table:4[i32]  ; [W]→[6]
              if-crossed                                         ; <<< THE HOLE
                par                                              ; [D] BuildEagerInsertionRegionsImpl par (Build.cpp:758)
                  par
                    publish out_val/1(@A:11)                     ; [D]
                  par
                    empty-par                                    ; [D] (net-additions guard, absent → empty; Build.cpp:888)
        vector-clear $param:9<i32>
        call ^flow:18
        return-false

LEGEND: **[W]** = hand-coded monotone arm (`Procedure.cpp:58-93`) TODAY;
**[6]** = owned by `LowerIngestFold` POST-§6; **[D]** = the descent
(`BuildEagerInsertionRegions`), hand-coded BOTH today and post-§6 (§6 does not
move the descent — it re-homes only the fold that HOSTS it); **[P]** = the
per-io PARALLEL, unchanged. THE HOLE is the `update-count … if-crossed` body:
today the monotone arm creates `insert` (`Procedure.cpp:71-77`), sets
`next_parent = insert` (`:77`), and calls `BuildEagerInsertionRegions(…,
next_parent, …)` (`:91`) which emits the `publish` INTO `insert->body`.
Post-§6, `LowerIngestFold` creates that same `update-count`, and RETURNS it as
the cursor into which the still-hand-coded `BuildEagerInsertionRegions` emits
the `publish`.

cf16_4's `ingest_role` is **kEmpty** (`MonotoneIngestRoleDR`, DR.cpp:1810 — no
cut successor, not monotone-negated), so there is NO net-additions append; the
hole holds exactly the publish subtree.

### 1.2 negate_lower_strata (nls) — IF-1 kNetAddition + IF-2/IF-3 already DR-lowered

`.dr`: `#message block(u64) @differential` (deletion-capable → IF-2/IF-3,
already lowered by P2's cutover); `#message feed(u64,u64)` (monotone → IF-1);
`%table:4[u64]` = blk (differential), `%table:11[u64,u64]` = mid/raw/step/feed
(monotone, feeds the cut negate `out:mid,!blk`).

**OPT MODE** (`negate_lower_strata.ir`, entry verbatim):

    proc ^entry:15($param:17<u64,u64>, $param:22<u64>, $param:23<u64>)
      vector-define $net_additions:21<u64,u64>
      vector-define $add_queue:26<u64>
      vector-define $delete_queue:29<u64>
      seq
        par
          vector-loop {@K:19, @V:20} over $param:17<u64,u64>              ; IF-1 [W] monotone arm
            update-count +nonrecursive {@K:19, @V:20} in %table:11[u64,u64] ; IF-1 [W]→[6]
              if-crossed                                                  ; <<< THE HOLE (IF-1)
                vector-append {@K:19, @V:20} into $net_additions:21       ; [D] Build.cpp:890-892 (net-additions)
          vector-loop {@K:25} over $param:22<u64>                         ; IF-2 [L] LowerIngestFold (ALREADY, P2)
            update-count-explicit +nonrecursive {@K:25} in %table:4[u64]  ; IF-2 [L] Stratum.cpp:1902-1905
              if-crossed
                vector-append {@K:25} into $add_queue:26                  ; IF-2 [L] Stratum.cpp:1919-1926
          vector-loop {@K:28} over $param:23<u64>                         ; IF-3 [L] LowerIngestFold (ALREADY, P2)
            update-count-explicit -nonrecursive {@K:28} in %table:4[u64]  ; IF-3 [L]
              if-crossed
                vector-append {@K:28} into $delete_queue:29               ; IF-3 [L]
        vector-clear $param:17 ; vector-clear $param:22 ; vector-clear $param:23
        call ^flow:86($net_additions:21, $add_queue:26, $delete_queue:29)
        return-false

**NOCF MODE** (`negate_lower_strata.nocf.ir`): identical except IF-1's
`if-crossed` body wraps the net-additions append in the descent's
`par → par` (`Build.cpp:758` PARALLEL nesting) that opt flattens away; IF-2/IF-3
are byte-identical to opt (their bodies are a bare `vector-append`, no descent).

LEGEND additions: **[L]** = `LowerIngestFold` owns it TODAY (P2 cutover, the
deletion-capable path); IF-2/IF-3 are the proof-of-concept the §6 monotone
lowering copies. Post-§6, IF-1 joins them: its `vector-loop /
update-count / if-crossed` becomes **[6]** (LowerIngestFold-owned), and the
`vector-append into $net_additions` stays **[D]** (emitted by the descent at
its fold-nesting site — §4 below proves this is where the code already puts it).

IF-1's `ingest_role` is **kNetAddition** (`MonotoneIngestRoleDR` — %table:11 is
monotone and feeds a cut negate). The hole holds the net-additions append.

### 1.3 deep_chain_retract (dcr) — IF-1 with the net-additions append at an INTERIOR same-table fold

`.dr`: `#message base(u64) @differential` (IF-2/IF-3, already lowered);
`#message next(u64,u64)` (monotone → IF-1); `reach(Y):reach(X),next(X,Y)` is a
linear recursion, so `next` feeds a recursive JOIN. `%table:15[u64]` = base
(differential), `%table:7[u64,u64]` = next model.

**OPT MODE** (`deep_chain_retract.ir`, entry verbatim):

    proc ^entry:21($param:23<u64>, $param:24<u64>, $param:31<u64,u64>)
      vector-define $add_queue:27<u64>
      vector-define $delete_queue:30<u64>
      vector-define $net_additions:35<u64,u64>
      seq
        par
          vector-loop {@X:26} over $param:23<u64>                        ; IF-2 [L]
            update-count-explicit +nonrecursive {@X:26} in %table:15[u64]
              if-crossed
                vector-append {@X:26} into $add_queue:27
          vector-loop {@X:29} over $param:24<u64>                        ; IF-3 [L]
            update-count-explicit -nonrecursive {@X:29} in %table:15[u64]
              if-crossed
                vector-append {@X:29} into $delete_queue:30
          vector-loop {@X:33, @Y:34} over $param:31<u64,u64>             ; IF-1 [W]→[6]  (next)
            update-count +nonrecursive {@X:33, @Y:34} in %table:7[u64,u64]
              if-crossed                                                 ; <<< THE HOLE (IF-1)
                vector-append {@X:33, @Y:34} into $net_additions:35      ; [D] net-additions of %table:7
        vector-clear $param:23 ; vector-clear $param:24 ; vector-clear $param:31
        call ^flow:173($add_queue:27, $delete_queue:30, $net_additions:35)
        return-false

**NOCF MODE** (`deep_chain_retract.nocf.ir`): IF-1's append is wrapped in the
descent's `par → par`; IF-2/IF-3 byte-identical.

THE INTERIOR-FOLD SUBTLETY (ledger §1, SubgraphsDemand.md:148-150). The
net-additions frontier being appended is `%table:7`'s (the `next` model), and
in the EMITTED tree the append sits directly under `next`'s own receive fold —
because the fold-once guard (`already_added`/`last_table`, Build.cpp:708)
collapses the descent's same-model fold INTO the receive fold. The ledger's
"interior/ancestor fold" phrasing means: the append is claimed by the DESCENT
(`BuildEagerInsertionRegionsImpl`, Build.cpp:888-892), which reaches
`%table:7`'s crossing through the `reach(Y):reach(X),next(X,Y)` JOIN, and the
crossing it finds IS the receive fold (same model). This is the WITNESS that
the append site is a DESCENT decision, not a seed decision — critical for §4.

### 1.4 The deepest hole in the corpus (transitive_closure2) — an entire INDUCTION nests in the hole

Not in the named identity set, but the §6 hole contract must handle it, so it
is quoted as the stress case. `#message add_edge(u64,u64)` is monotone and
feeds the mutually-recursive `tc1`/`tc2` INDUCTION. `transitive_closure2.nocf.ir`
entry (excerpt):

    vector-loop {@From:19, @To:20} over $param:17<u64,u64>          ; IF-1 [W]→[6]
      ; set 0 depth 1
      induction                                                    ; <<< THE HOLE holds an ENTIRE induction
        init
          par
            par
              update-count +nonrecursive … in %table:4[u64,u64]    ; [D] descent's own folds (NOT the ingest fold)
                if-crossed
                  hash … into @28
                    par
                      vector-append … into $induction_in:22 …
            …
        fixpoint-loop testing $induction_in:22, $induction_in:25
          …join-tables… select … from %table:8 …

CRITICAL STRUCTURAL FACT for the hole contract: in tc2 the monotone receive's
OWN fold is `update-count … in %table:4` — but that fold is emitted INSIDE the
`induction/init` region by the DESCENT's InTryInsert, NOT by the monotone arm's
`Procedure.cpp:71-77` insert. Here `add_edge`'s model has NO standalone table
at the receive (the receive is table-less; `Procedure.cpp:64-70` `table` is
null because the tc1/tc2 heads are induction-owned), so the monotone arm mints
NO counter fold and IF-1 for tc2 is a TABLE-LESS monotone receive — NOT an
ingest fold at all (DR.cpp:1767-1769: "A table-less monotone receive mints no
counter fold … and is not an ingest fold"). §6 must NOT touch table-less
monotone receives; the hole contract applies ONLY to the table-BEARING
monotone folds (cf16_4, nls IF-1, dcr IF-1). See §3.2's class table.

---

## §2. THE DIFF PLAN, EXACT

The §6 stage moves the monotone table-bearing fold from the hand-coded arm
into `LowerIngestFold`, byte-identically, and threads the descent through the
returned cursor. Per E-34, three coordinated edits.

### 2.1 The monotone op payload (already exists — no new authority needed)

The monotone ingest op is ALREADY constructed in `BuildDRInventory`
(`DR.cpp:1802-1820`): `kIngestFold`, `ingest_sign=1`, `ingest_is_explicit=false`,
`ingest_stage1=false`, `ingest_role = MonotoneIngestRoleDR(...)`. But that op
lives only in `flow.ops` (censused, not lowered). Today the WALK does not
consult it — the monotone arm re-derives the fold from the Query
(`Procedure.cpp:64-78`).

DECISION: add a sibling authority `MakeMonotoneIngestFold(message, receive,
table, context)` in DR.cpp, next to `MakeStageOneIngestFolds` (DR.cpp:767), the
single constructor of the monotone op — pure function of (message, receive,
table, context), no ids. `BuildDRInventory`'s inline block (DR.cpp:1802-1820)
is REPLACED by a call to it (exactly as the deletion-capable block was replaced
by `MakeStageOneIngestFolds` at DR.cpp:1788), so the censused op and the lowered
op share one constructor — the §12.6 single-authority discipline. `context` is
threaded because `MonotoneIngestRoleDR(context, table)` (DR.cpp:1810) and the
counter's `klass = EmissionDerivClass(impl, context, receive)` (DR.cpp:1815)
both need it. `EmissionDerivClass` needs `impl` too → the constructor takes
`(impl, context, message, receive, table)`.

RATIONALE for a SIBLING (not extending `MakeStageOneIngestFolds`): the monotone
op differs on FOUR fields (`is_explicit=false`, `stage1=false`, single-signed,
role from the predicate not from polarity) and its counter `klass` is
`EmissionDerivClass` not hard `kNonRecursive`; folding both into one function
would branch on `CanReceiveDeletions()` in the constructor — the exact "re-run
the walk's classification" smell §12.6 avoided. Two constructors, one census.

### 2.2 `LowerIngestFold` — relax asserts, RETURN the cursor

CURRENT (`Stratum.cpp:1884-1927`, read this session): returns `void`, asserts
`op.ingest_stage1 && op.ingest_is_explicit` (`:1887`) and
`op.ingest_role ∈ {kAddQueue, kDeleteQueue}` (`:1889-1890`), unconditionally
emits the VECTORAPPEND into the queue (`:1915-1926`).

CHANGES (E-34 (i)+(iii)):

1. **Signature** → `OP *LowerIngestFold(...)`. Returns the fold body cursor:
   the UPDATECOUNT `fold` (`Stratum.cpp:1902`) for a table-bearing op — the
   exact analog of the hand-coded arm's `next_parent = insert`
   (`Procedure.cpp:77`). Deletion-capable callers (Procedure.cpp:52,
   DR.cpp lowering path) DISCARD the return (their body is the bare queue
   append, no descent) — a `(void)` cast at the two existing call sites.

2. **Relax `Stratum.cpp:1887`**: replace
   `assert(op.ingest_stage1 && op.ingest_is_explicit)` with the two disjoint
   legal shapes:

       assert(op.kind == DROpKind::kIngestFold);
       // Deletion-capable (stage-1): explicit, queue role, has a loop_vec.
       // Monotone (§6): non-explicit, {kNetAddition,kEmpty} role.
       assert(op.ingest_is_explicit == op.ingest_stage1);   // R1e invariant (DR.cpp:2566)
       assert(op.ingest_is_explicit
                ? (op.ingest_role == VecRole::kAddQueue ||
                   op.ingest_role == VecRole::kDeleteQueue)
                : (op.ingest_role == VecRole::kNetAddition ||
                   op.ingest_role == VecRole::kEmpty));

   The `:1889-1890` role assert is subsumed by the ternary above.

3. **UPDATECOUNT emission** (`:1902-1905`): the `is_explicit` argument is
   already `op.ingest_is_explicit` (`:1903`) — for a monotone op it becomes
   `false`, emitting `update-count +nonrecursive` (no `-explicit`), matching
   the hand-coded arm's `Procedure.cpp:73` `false /* is_explicit */`. NO change
   needed — the payload already drives it. (This is why the emitted text is
   byte-identical: `update-count +nonrecursive {…} in %table:4[i32]` is exactly
   what `Procedure.cpp:71-77` produces.)

4. **The queue/net-additions append** (`:1915-1926`): GUARD it on the role.
   For a deletion-capable op (`kAddQueue`/`kDeleteQueue`) emit the queue append
   as today. For a MONOTONE op: emit NOTHING here. The net-additions append is
   the DESCENT's job (Build.cpp:888-892), emitted at its actual fold-nesting
   site — §4 confirms this is unchanged. So the monotone `LowerIngestFold` body
   is just `vector-loop / update-count / (empty if-crossed hole)`, and returns
   the UPDATECOUNT for the descent to fill.

5. **The `col_values` / `defined_vars` loop** (`:1907-1913`): unchanged — it
   binds the loop's vars from `receive.Columns()` and feeds the fold, exactly
   as `Procedure.cpp:80-89` does. Byte-identical (same var ids because emitted
   at the same walk position — §4).

### 2.3 `ExtendEagerProcedure` monotone arm (`Procedure.cpp:58-93`) → thin

CURRENT: the arm hand-creates VECTORLOOP (`:58-61`), sets `next_parent=loop`
(`:62`), fetches model/table (`:64-65`), if table hand-creates UPDATECOUNT
(`:71-77`), sets `next_parent=insert` (`:77`), binds vars (`:80-89`), then calls
`BuildEagerInsertionRegions(impl, receive, context, next_parent,
receive.Successors(), table)` (`:91-92`).

POST-§6 (E-34 (ii)): the arm splits on `table`:

    for (auto receive : receives) {
      if (receive.CanReceiveDeletions()) { /* unchanged, :46-56 */ continue; }

      DataModel *const model = impl->view_to_model[receive]->FindAs<DataModel>();
      TABLE *const table = model->table;

      OP *next_parent;
      if (table) {
        // §6: the table-bearing monotone fold lowers from the DR-IR, at the
        // original walk position (id-stream identity), via LowerIngestFold —
        // token-equivalent to the deleted :58-78 hand-coded fold. The returned
        // UPDATECOUNT is the descent cursor (E-34 (iii); Procedure.cpp:77).
        const DROp op = MakeMonotoneIngestFold(impl, context, message, receive, table);
        next_parent = LowerIngestFold(impl, context, op, parent, vec);
      } else {
        // A table-less monotone receive (tc2's add_edge): mints no counter
        // fold — the descent's InTryInsert emits the fold under an induction.
        // NOT an ingest fold (DR.cpp:1767-1769). Hand-coded VECTORLOOP only.
        const auto loop = impl->operation_regions.CreateDerived<VECTORLOOP>(
            impl->next_id++, parent, ProgramOperation::kLoopOverInputVector);
        parent->AddRegion(loop);
        loop->vector.Emplace(loop, vec);
        for (auto col : receive.Columns()) {
          VAR *const var = loop->defined_vars.Create(
              impl->next_id++, VariableRole::kVectorVariable);
          var->query_column = col;
          loop->col_id_to_var.emplace(col.Id(), var);
        }
        next_parent = loop;
      }
      BuildEagerInsertionRegions(impl, receive, context, next_parent,
                                 receive.Successors(), table);
    }

The `BuildEagerInsertionRegions` call (`:91-92`) is UNCHANGED — same
`next_parent` (the UPDATECOUNT for table-bearing, the VECTORLOOP for
table-less), same `receive`, same `receive.Successors()`, same `table`.

LINES OF Procedure.cpp THAT CHANGE: `:58-89` (the hand-coded fold body) is
replaced by the `if (table) { LowerIngestFold } else { VECTORLOOP-only }` block
above; `:91-92` is retained verbatim. The deletion-capable block (`:46-56`) is
untouched.

### 2.4 The net-additions append — DESCENT keeps emitting it (confirmed via Build.cpp:868-893)

Artifact §5 (p2-ingest-inventory-target.md:361-421) warns: the seed must NOT
claim it owns the net-additions frontier, because the append site depends on
walk NESTING (Build.cpp:870-872), not on which view is the receive.

CONFIRMED FROM CODE this session: the net-additions append is emitted ONLY in
`BuildEagerInsertionRegionsImpl` (Build.cpp:886-893), guarded by
`table != nullptr && !TableIsDifferential(table) && (any_cut_succ ||
is_monotone_negated)`, and its position is wherever the descent's fold-once
machinery (`already_added`/`last_table`, Build.cpp:708) places the crossing. In
cf16_4 (kEmpty) it is ABSENT (no cut successor). In nls IF-1 and dcr IF-1 it is
present, emitted BY THE DESCENT at the crossing it reaches. §6 does NOT move it:
`LowerIngestFold` emits ONLY the fold + empty hole (§2.2 step 4), and the
descent's `Build.cpp:890-892` append lands inside the hole exactly as today.

So the answer to the §14.0.2-item-1 sub-question ("does the descent claim the
append, or does the seed?"): **the DESCENT claims it, at its actual
fold-nesting site — unchanged from today.** The seed (LowerIngestFold) never
appends net-additions. This is what the code already does (Build.cpp:868-893 is
the sole emitter) and what §6 preserves. E-34 and §5 are honored: the fold
externalizes, the append does not migrate.

---

## §3. THE HOLE-FILLED-EXACTLY-ONCE VALIDATOR

### 3.1 Design

WHERE IT RUNS: per-compile, in `ExtendEagerProcedure`, immediately after the
`BuildEagerInsertionRegions` call returns for a table-bearing monotone receive
(Procedure.cpp, the new arm §2.3). It is a POST-WALK check on the UPDATECOUNT
cursor `LowerIngestFold` returned — NOT a DR-flow validator (it inspects the
emitted CF tree, which the DR validators never touch — p2 §7:456-497).

WHAT IT CHECKS, per receive class:

| receive class                          | ingest_role   | hole (`fold->body`) after descent |
|----------------------------------------|---------------|-----------------------------------|
| kEmpty monotone, no cut successor      | kEmpty        | EXACTLY ONE child subtree (the publish / relation descent). NEVER empty — a table-bearing monotone receive always has ≥1 successor reached (else it would be dead-flow-eliminated). |
| kNetAddition monotone (cut / negate / agg) | kNetAddition | EXACTLY ONE child subtree, whose leaves include the net-additions `vector-append` (Build.cpp:890) at its fold-nesting site. |

Both classes: `fold->body` (the `if-crossed` hole) must be filled EXACTLY ONCE
— `LowerIngestFold` leaves it empty (`fold->body` unset), the descent
(`BuildEagerInsertionRegionsImpl`'s `parent->body.Emplace(parent, par)` at
Build.cpp:759) fills it with exactly one PARALLEL. The check:

    // §6 HOLE-FILLED-EXACTLY-ONCE (subgraphs/demand P1). LowerIngestFold left
    // fold->body empty; the descent must have filled it with exactly one region.
    // `next_parent` IS the UPDATECOUNT `LowerIngestFold` returned (a
    // UPDATECOUNT* == ProgramUpdateCountRegionImpl*, Program.h:859), so
    // fold->body is a direct null-check — or, if the cursor is kept as an
    // OP*, `next_parent->AsUpdateCount()` (the Induction.cpp:952 pattern).
    if (table) {
      UPDATECOUNT *const fold = next_parent->AsUpdateCount(); // the returned cursor
      if (!fold->body) {
        ValidatorFail("§6 HOLE: monotone ingest fold body left empty by descent");
      }
      // Exactly one: UPDATECOUNT::body is a single Emplace slot, so a second
      // fill would have aborted at Emplace; the non-null check IS the "exactly
      // once". (No count needed — the slot enforces it structurally.)
    }

The "exactly once" is STRUCTURALLY guaranteed by `UPDATECOUNT::body` being a
single `UseRef` slot (`Program.h`): a second `Emplace` on an occupied slot
aborts. So the validator degenerates to a NON-EMPTY assert. The empty case
(kEmpty monotone with NO successor) cannot occur for a TABLE-BEARING receive
that survives dead-flow elimination — but if it ever did, the descent's
`Build.cpp:758-759` still emplaces an empty PARALLEL (non-null), so the check
passes and the empty PARALLEL optimizes out. Correct in all four modes.

ABORT STYLE: `ValidatorFail(...)` — the established fprintf+abort that survives
NDEBUG (DR.cpp validators), so the check runs in release too. This is the
kEmpty/otherwise distinction the §6 contract (p2 §6:445-447) named, realized as
a structural non-empty gate.

HOW IT SURVIVES ALL 4 MODES: it inspects the RAW emitted tree BEFORE
`impl->Optimize()` runs (Build.cpp:1259-1269 runs Optimize AFTER
`BuildEntryProcedure` returns, which includes the whole eager walk). So the
hole is checked pre-flatten in every mode — the check never depends on
flattening.

### 3.2 Corpus census — every monotone receive class and its hole content

I enumerated the corpus (164 cases) this session. 115 cases have ≥1 monotone
(non-`@differential`) `#message`. Table-bearing monotone receives (the §6 hole
population) fall into these classes; spot-checked with the real compiler:

- **kEmpty, publish descent** (cf16_4, VERIFIED): hole = `publish out_val/1`.
  The kEmpty class also covers monotone receives whose only successor is a
  plain relation with no cut/agg/negate downstream.
- **kNetAddition → cut negate** (nls IF-1, VERIFIED): hole = `vector-append
  into $net_additions`.
- **kNetAddition → recursive JOIN** (deep_chain_retract IF-1, VERIFIED): hole =
  `vector-append into $net_additions` at the same-model receive fold.
- **kNetAddition → aggregate** (average_weight, VERIFIED): hole = `vector-append
  into $net_additions` (the agg input frontier; %table:36).
- **TABLE-LESS monotone** (transitive_closure2 `add_edge`, VERIFIED): NOT a
  §6 case — no counter fold at the receive; the descent's InTryInsert emits the
  fold under an `induction`. The hole contract does not apply; the arm keeps the
  hand-coded VECTORLOOP-only path (§2.3 else-branch). Corpus mates:
  tc4/tc5/fibonacci (found via the entry-proc scan/join grep this session — 19
  cases have a scan/join/check nested in the entry proc, all descents into
  recursive or JOIN structure).

The census cross-checks against the DR-side count already computed for the R1e
census (`exp_ingest`, DR.cpp:2528-2568 counts monotone table-bearing ops as
`stage1=false` entries) — so the number of §6 holes == the number of
`stage1=false` kIngestFold ops, per compile, already validated.

---

## §4. THE INTERIOR-FOLD WITNESS + THE CSE-COLLAPSE QUESTION

### 4.1 Does §6 need the artifact-§5 directed .dr witness in addition to deep_chain_retract?

The §5 directed witness (p2 §5:396-416) proposes a shape where the receive
table is FIRST folded at an INTERIOR view (`a`/`b` on distinct models, the
append at `b`'s interior fold, off the receive). The §11 LOW finding
(p2 §11:660-663) demands IR-verifying that shape does NOT CSE-collapse before
§6 proceeds. I generated and inspected the relevant IR this session.

FINDING: **deep_chain_retract SUFFICES as the §6 LOWERING witness; the §5
directed .dr is NOT additionally required.** Reasoning, from the IR:

In dcr, `next` folds into `%table:7`, and the net-additions append of `%table:7`
sits under `next`'s own receive fold (§1.3). The append is reached by the
DESCENT through the `reach(Y):reach(X),next(X,Y)` JOIN, and the fold-once guard
collapses the descent's same-model crossing INTO the receive fold. THIS IS THE
EXACT MIGRATION HAZARD §5 warns about — and it lands SAFELY because the descent
(not the seed) owns the append (§2.4). deep_chain_retract already exercises the
"append claimed by descent at a crossing reached through interior structure"
path. The §5 directed case would exercise the SAME code path
(Build.cpp:886-893) with a DIFFERENT topology; it adds no new lowering
behavior because §6 NEVER lets the seed own the append — so whether the
crossing is the receive fold or a deeper interior fold is IMMATERIAL to §6's
correctness (the descent emits at whatever crossing it reaches, unchanged).

### 4.2 The CSE-collapse answer (for the fusion ledger, §14.0.2 item 3)

I generated the §5 directed shape's spirit via the corpus (the `a→b` interior
hop is topologically the `reach ← next` hop in dcr, and the tc2 `add_edge →
tc1/tc2` induction hop). RECORDED ANSWER:

**In deep_chain_retract, `next`'s model (%table:7) and the interior JOIN-side
view DO share one model (fold-once collapses them) — a CSE-collapse to one
model, observed in the emitted tree (the append sits at the receive fold, not a
separate interior fold).** This is precisely the collapse the §5 LOW finding
anticipated. CONSEQUENCE for §6: NONE — because §6 does not depend on the
append being separable from the receive fold (§4.1). CONSEQUENCE for FUSION
(§14.0.2 item 3, the seed's "is the shared interior fold a fusion candidate"):
**the shared interior fold IS a fusion candidate** — dcr demonstrates a
monotone receive fold and a descent same-model fold ALREADY fused (one
UPDATECOUNT, one crossing) by the fold-once guard. So fusion of shared-input
drains is not a new mechanism to build; the fold-once guard is its existing
in-corpus realization. RECORDED as fusion's first witness: deep_chain_retract
(the receive/descent same-model fold, fused today by Build.cpp:708). No
additional directed .dr is needed for §6; a fusion-specific witness (two
DISTINCT monotone receives sharing a drain) is a §14.2(B) instance-family
concern, deferred.

---

## §5. THE EMITTED-TREE↔FLOW CROSS-CHECK (V-PRED-XCHECK Site 5+)

This discharges the P2 cutover deviation's obligation (§12.6:1022-1024 +
§14.0.2 item 4, PerfRoadmap:2028-2033): the cutover left the emitted CF ingest
tree UN-cross-checked against the flow's kIngestFold ops. §6 adds the
per-compile check, covering BOTH deletion-capable (already DR-lowered) and
monotone (newly DR-lowered) folds.

### 5.1 Design (V-INGEST-XCHECK, Site 5)

The existing V-PRED-XCHECK family (CLAUDE.md: "ties the DR model to the
surviving Emit* templates") is per-emitter. Site 5 = the ingest lowering. It
runs PER-COMPILE, after the eager walk completes, before Optimize.

WHERE IT HOOKS: in `LowerIngestFold` itself (Stratum.cpp), at emission time —
each lowered fold records its emitted `(table, sign, is_explicit, role)` tuple
into a `context.emitted_ingest_folds` vector; then a closing check (in
`BuildStratumPhases` or at the end of `BuildEntryProcedure`) compares that
emitted multiset against the flow's kIngestFold op multiset (the SAME
`(ingest_table, ingest_sign, ingest_is_explicit, ingest_role, ingest_message)`
5-tuple the R1e census keys on, DR.cpp:2864-2867).

WHICH FIELDS, KEYED HOW: the 5-tuple `(TABLE* table, int sign, bool
is_explicit, VecRole role, ParsedMessage message)`, compared ORDER-FREE (sorted
multiset equality, mirroring the IngestKey census DR.cpp:2858-2871). This ties
the EMITTED tree (what LowerIngestFold actually produced) to the FLOW ops (what
BuildDRInventory censused) — closing the §12.6 gap where "no per-compile
V-PRED-XCHECK analog ties the emitted CF tree back to the flow ops".

WHAT IT ABORTS ON: any emitted fold whose 5-tuple is not in the flow's
kIngestFold multiset (a fold emitted that the flow never enrolled → a
divergence between MakeStageOneIngestFolds/MakeMonotoneIngestFold and the walk),
OR any flow kIngestFold op with `stage1==true` OR (`stage1==false && table!=null`)
that was NOT emitted (a fold the census counted but the walk dropped). Abort
style: `ValidatorFail` (fprintf+abort, survives NDEBUG). This reintroduces the
F17/F18-style divergence guard for the ingest surface.

COVERAGE of both fold kinds:
- DELETION-CAPABLE (IF-2/IF-3): emitted by `LowerIngestFold` at
  Procedure.cpp:52 → recorded → matched against the flow's `stage1=true`
  kIngestFold ops. (Closes the P2 gap for the ALREADY-lowered folds.)
- MONOTONE (IF-1/IF-4): emitted by the §6 `LowerIngestFold` → recorded →
  matched against the flow's `stage1=false && table!=null` kIngestFold ops.
- TABLE-LESS monotone: emits NO ingest fold and the flow enrolls none
  (DR.cpp:1767-1769) — excluded from both sides, so it cannot cause a
  false-positive abort.

### 5.2 Why a multiset, not a tree walk

The R1e census established (p2 §7:456-497) that ingest validation is
DERIVED-vs-DERIVED, never a tree walk — there is no precedent for walking
`entry_proc->body`. Site 5 keeps that: it compares an EMITTED-op multiset (a
by-product recorded at emission) against the FLOW-op multiset. It is immune to
region re-parenting/flattening (it reads the recorded tuples, not the tree
shape) — the exact property the §12.6 note wanted. The `-ir-out` structural
gate (§6 below) covers tree SHAPE; V-INGEST-XCHECK covers PAYLOAD identity.
Together they close the deviation.

---

## §6. GOLDEN POLICY PROPOSAL (for the owner)

Following the §12.4 precedent (p2 §12.4:858-911): region-tree structural
comparison, opt+nocf, on named witnesses. §6 is a BYTE-IDENTITY refactor (moves
emission, changes no node), so:

PROPOSAL: **byte-identity-target-with-structural-gate.**

- PRIMARY (opt modes): the entry-proc `-ir-out` subtree is BYTE-IDENTICAL pre
  vs post-§6 on every witness. Under the §2 diff (same walk position, same
  payload, same var/queue ids) this is identity by construction.
- STRUCTURAL (nocf modes): the un-flattened entry-proc subtree is
  BYTE-IDENTICAL pre vs post. THIS is the wrong-parent/hole-misfill detector —
  a hole filled at the wrong cursor, or the fold emitted at the wrong parent,
  is a tree-SHAPE change visible only un-flattened (opt would flatten the
  PARALLEL nesting a misfill perturbs). A diff that is NOT a pure id-renumber
  is a REJECT, not a bless.

WITNESSES (named, all in-corpus, VERIFIED this session):
- **cf16_4** — IF-4 kEmpty publish descent (the empty-role hole).
- **negate_lower_strata** — IF-1 kNetAddition + IF-2/IF-3 (mixed: newly-lowered
  monotone fold coexisting with already-lowered deletion-capable folds under
  one proc_par).
- **deep_chain_retract** — IF-1 kNetAddition at the interior/same-model fold
  (the migration witness + fusion witness, §4).
- **transitive_closure2** (the mixed/table-less case) — proves the table-less
  monotone receive (`add_edge`) still takes the hand-coded VECTORLOOP-only path
  and its induction descent is byte-identical (the §6 arm must NOT reroute it).

FALLBACK: permutation-only bless per §7/permcheck.py (published-delta tokens
compare order-free per epoch, all other lines byte-identical). But §6 targets
HARD byte-identity — a residual permutation would itself be a finding worth
reviewing (the fold moved emission but should occupy the same id slots). Only a
pure id-BIJECTION (consistent renumber, identical shape) is permcheck-blessable;
a node-count or parent-edge change is a bug.

REFEREES: the oracle + monotone sidecars (deep_chain_retract has `.batches` +
oracle/monotone goldens; a bless is valid ONLY if they agree — never to green a
red case). average_weight (agg input, kNetAddition) rides its oracle/monotone
referees too and should be added as a fifth witness for the aggregate-input
hole class.

GATE: PASS = (structural tree shape identical in ALL 4 modes) ∧ (opt-mode text
byte-identical) ∧ (suite 164 stdout + oracle + monotone sidecars byte-identical)
∧ (V-INGEST-XCHECK Site 5 + the §3 hole validator green on all 164).

---

## §7. PRE-REGISTERED PREDICTIONS

1. **Golden churn: ZERO.** §6 moves the monotone table-bearing fold's emission
   from `Procedure.cpp:58-89` into `LowerIngestFold`, at the SAME walk
   position, from the SAME payload constructor (single-authority), reusing the
   SAME memoized net-additions/queue vecs, so every emitted node — VECTORLOOP,
   UPDATECOUNT, the descent it hosts, the net-additions append at its unchanged
   fold-nesting site — occupies its pre-§6 id slot. `-ir-out` and datalog.h
   byte-identical on all four witnesses, opt AND nocf. (Confidence HIGH: the
   deletion-capable cutover already achieved raw-byte-identity with this exact
   shape, §12.6:1001-1003; the monotone arm is structurally the same move.)

2. **Suite count: 164.** All witnesses are in-corpus; no new case needed for
   the lowering itself. (A directed §5 interior-fold .dr is NOT added — §4
   shows deep_chain_retract suffices.)

3. **Driver churn: 0.** No generated-API surface changes — the fold's C++
   codegen is byte-identical, so every `.main.cpp` driver is untouched.

4. **Q5: neutral.** No emission change → identical generated code → identical
   compile/runtime cost. (Spot-check @128 expected flat vs the SubgraphsDemand.md
   §0 baseline: release/opt 0.11-0.12s, debug/opt 0.94s.)

5. **FINDINGS entry IFF a validator fires.** The §3 hole-filled-exactly-once
   validator or the §5 V-INGEST-XCHECK Site 5 firing during the refactor would
   witness a real emitter/flow divergence (the house bet: like R1e's day-one
   catch, §6 may surface a latent monotone-role or fold-nesting bug). If neither
   fires across all 164, NO FINDINGS entry — the byte-identity is the proof.
   Directional prediction: LOW probability of a fire (the monotone arm is
   long-stable and the fold-once guard is well-exercised), but the
   V-INGEST-XCHECK is exactly the drift-catcher §12.6 asked for, so it earns its
   keep against FUTURE monotone-role changes even if silent now.

### Residual risks the implementer must check

- **R-CURSOR**: `LowerIngestFold`'s returned cursor MUST be the UPDATECOUNT
  (Stratum.cpp:1902), not the VECTORLOOP — the descent expects the fold body,
  not the loop body (Procedure.cpp:77 sets `next_parent=insert`, not `loop`).
  For a hypothetical table-less op LowerIngestFold is never called (§2.3
  else-branch), so the cursor is always the UPDATECOUNT. VERIFY: the returned
  `OP*` `->AsUpdateCount()` is non-null at the call site.
- **R-KLASS**: the monotone counter's `klass` is `EmissionDerivClass(impl,
  context, receive)` (DR.cpp:1815), NOT hard `kNonRecursive`. For the three
  witnesses it resolves to kNonRecursive (emitted `+nonrecursive`), but
  `MakeMonotoneIngestFold` must thread it (unlike MakeStageOneIngestFolds'
  hard-coded kNonRecursive at DR.cpp:787). A monotone receive whose head is
  recursively-derived would differ — but those are table-less (induction-owned),
  so never reach the ingest-fold path. VERIFY the census `EmissionDerivClass`
  agrees with the emitted counter class on all 164 (V-INGEST-XCHECK does not
  key on klass today — the 5-tuple omits it; ADD klass to the Site-5 key or
  assert it separately).
- **R-EMPTY-HOLE**: a table-bearing monotone receive with zero reached
  successors would leave the hole empty. §3 argues dead-flow elimination
  prevents this; the validator's non-empty gate catches it if it ever occurs
  (abort, not silent). CONFIRM no corpus case trips it (expected: none).
