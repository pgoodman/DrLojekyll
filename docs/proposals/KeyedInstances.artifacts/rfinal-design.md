# R-FINAL — BINDING DESIGN DRAFT (stages (b)/(c) adjudicated)

> **House banner (r*-design idiom).** Tip **e30632be** (repo + worktree
> `wt-rfinal-probe` both `git rev-parse HEAD` = e30632be, verified this session).
> Bindings: owner-ruling-brief.md R1–R6 RATIFIED AS RECOMMENDED (ratification
> record at its tail: `.deltarel`→`.rel` lockstep; emission op SURFACES
> `order=`/`seq=`; Fold-B witness picked empirically = cf16_2). Diff order:
> **Fold A → Fold B → emission op → flip (SD-1..SD-4) → rename.** Doc mold:
> rel-arch-pseudocode.md §4–§5 (M1–M17 + R-FINAL block, errata E-130..E-137);
> ledger KeyedInstances.md §20(V). This draft = the adjudicated design; the
> per-lane b1..b5 designs are the substrate, amended in place by the
> ADJUDICATION RECORD (§7). NEVER edit repo; dumps only under rfinal-bc/<lane>/.

The adjudicator re-verified every HIGH/MED finding AT THE CODE (§7 cites). All
critique findings ACCEPTED (design amended); **zero REFUTED, zero ESCALATE** — no
finding conflicts with a ratified ruling. The one **DEFECT** verdict (flip) is
lifted to SOUND-WITH-FIXES by the HIGH-1 amendment (constant-fact SET root).

---

## 1. FOLD A — pivot-belt TUPLECMP retirement (lands FIRST)

**What.** Stop emitting the redundant per-join-row equality re-check; delete the
eager-only TUPLECMP frame `BuildJoin` mints, re-home its variable-shadow one frame
up onto the TABLEJOIN. Emission-SHAPE change (not byte-identity). Premise
(RE-VERIFIED at code): `Index::First` does full-KEY equality — `slot.used &&
slot.hash==hash && slot.key==key` (Table.h:801), a real aggregate `operator==`;
`Next` = `next[id]` (Table.h:807-810) walks the per-key `Add`-chain
(Table.h:765-773). The index is built over the side's full pivot set
(`GetOrCreateIndex(pivot_col_indices)`, Join.cpp:392-393), so the belt re-checks
columns the index already matched exactly → redundant. The no-index path is DEAD
(Data.cpp:352-355 null-return commented out; Join.cpp:526-528 `assert(false)`).

**DIFF (control-flow build, Join.cpp).**
1. Delete the eager cmp mint + body-Emplace, Join.cpp:317-321 (`cmp` decl stays as
   the delta null-return, or is dropped under Option B).
2. Delete the belt fill / dead-`assert(false)` arm, Join.cpp:513-528 (the
   `index_of_index` if-branch AddUses + the else `assert(false)`); the
   `if(for_delta){}` / else-if / else collapses.
3. **Re-home the hider**, Join.cpp:532-534: `cmp->col_id_to_var[out_col->Id()]` →
   `join->col_id_to_var[out_col->Id()]` (keep the `!for_delta &&
   pred_view_idx == most_represented_pred_view_idx` guard).
4. Caller Join.cpp:601-603: `auto [join, cmp] = BuildJoin(...)` /
   `OP *parent = cmp` → `parent = join` (TABLEJOIN IS an OP, Program.h).
5. **Signature — Option B (recommended):** `BuildJoin` returns a bare
   `TABLEJOIN*` (touches the Build.h decl + Stratum.cpp:1532-1535, dropping the now
   -vacuous `assert(cmp == nullptr)`). Option A (keep the `pair`, return
   `{join,nullptr}`, Join.cpp-only) is the strict-minimal fallback if the
   Stratum.cpp caller edit is judged out-of-scope for the first slice.

**Body-anchor re-home.** The descent (checkmembers / inductive-PAR / InTryInsert /
fixpoint-LET / BuildEagerInsertionRegions) hangs under `join->body` instead of
`cmp->body`. Position-agnostic PROVEN: `VariableFor`/`VariableForRec`
(Region.cpp:231-275) resolve up the ancestor chain, so removing the cmp frame
(whose sole pre-descent entry is the re-homed :533) changes only WHERE a var
caches, never its VALUE. `EmitJoin` (Database.cpp:2766) emits `region.Body()`
generically → only the `if`-compare wrapper vanishes + a dedent.

**AMENDED — the `col_id_to_var` re-home is an INTENTIONAL same-key last-write-wins
overwrite (F1, ACCEPTED).** The b1 §1 "disjoint / single-write / no collision"
table was FACTUALLY WRONG. Code truth: the pivot-arm `out_col` (role `kJoinPivot`,
Join.cpp:505) IS a `join_view.PivotColumns()` entry — the SAME key set the pivot
loop writes at Join.cpp:344 (`join->col_id_to_var[pivot_col.Id()] = pivot-vector
var`). Re-homing collapses BOTH writes into `join`'s map at the same key: :344
writes the pivot-vector var, the later `ForEachUse` (:532-534, runs after the
:336-345 loop) OVERWRITES it with the scanned var. Byte-correct because (a) write
ORDER is guaranteed — the :336-345 pivot loop is strictly before the :474
`ForEachUse`, so the scanned var wins; (b) pivot-loop rendering reads
`join->pivot_vars` (a SEPARATE DefList, Database.cpp:2776), NEVER `col_id_to_var`,
so the overwrite is invisible to the loop; (c) today the descent hangs under cmp,
so `join->col_id_to_var[pivot]`'s pivot-vector var is already never read by any
descent (always cmp-shadowed) — post-fold the descent reads the scanned var
directly from `join`, identical to today's shadowed result.
**IMPLEMENTER WARNING (pin this):** do NOT convert the re-home to a conditional /
insert-if-absent / reorder — the overwrite is deliberate; guarding it silently
regresses pivot-output provenance and diverges the `.h`. Invariants to state in
the M-mold: (i) the :336-345 pivot loop MUST precede the :474 `ForEachUse`;
(ii) pivot-loop var authority is `join->pivot_vars`, never `col_id_to_var`.

**AMENDED — EmitJoin empty-body guarantee (F2, ACCEPTED).** `EmitJoin` early-
returns and emits NOTHING (drops the whole join + pivot loop) when
`!body && !added_body && !removed_body` (Database.cpp:2770). Today the deleted
:321 Emplace makes `join->body` non-empty UNCONDITIONALLY; post-fold it is
non-empty only if the descent emplaces. **Guaranteed safe:** every eager-join
descent emplaces ≥1 region under `parent(=join)` — the backstop
`BuildEagerInsertionRegionsImpl` UNCONDITIONALLY does `parent->body.Emplace` at
Build.cpp:861-862, plus CHECKMEMBER :657 / InTryInsert fold Build.cpp:830 /
fixpoint LET :689. State this guarantee explicitly (cheap insurance against a
future descent refactor).

**Table.h Runtime-contract pin (shared, PLACED HERE).** One NOTE at Table.h:789
above `First`: full-key exactness (`slot.key == key` at :801; `Next` walks the
same-key chain) + the codegen-no-re-verify contract. Retire the 4 stale
"approximate"/"index scans are approximate" eager sites: Build.h:504, Join.cpp:300,
Join.cpp:516, Join.cpp:521-525, and the eager clause of Program.h:1510. (The 5th
`approximate` hit, Database.cpp:2788, is Fold B's — delta side.)

**Delta path UNTOUCHED.** All removed code is eager-only (mint was `if(!for_delta)`;
belt was the `else-if` arm; delta took the empty `if(for_delta){}` at :511; the
hider keeps its `!for_delta` guard). Option B touches only the delta caller's
binding (drops a null-assert), zero emitted-region change.

**Gate family (R4, emission-SHAPE — NEVER permcheck):**
- irgold re-bless = EXACTLY `{demand_tc_witness (h,ir), symrec_tie_1 (ir)}` — the
  only h/ir pins (all other carriers pin df/deltarel only, untouched). Verified
  by grep of all 14 `.irgold`.
- 58/175 cases churn `.ir`+`.h` (matches a4; b1's independent classifier over 175
  regenerated IRs: 85 join-carrying → 58 EAGER-belt). All 58 churn (empirical: 0
  of 260 equality if-compares trivially-equal → belt always emits a live `if`).
- `.df`/`.deltarel`/census byte-identical; permcheck N/A.
- A/B stdout 0-DIVERGED across 4 modes + data/ sweep (a divergence ⇒ premise false
  ⇒ HARD STOP — latent Runtime bug the belt masked, not a Fold-A defect).
- ctest 5/5 debug + 5/5 ASAN; ASAN-compiler SUITE zero reports.
- Q5 progsize shrink (perf, never gates).

**AMENDED pre-registered predictions:**
- **[BYTE — nocf/none only] (F3, ACCEPTED):** under `-disable-controlflow-opt`, the
  folded `.h` differs from baseline ONLY by the removed `if(…){}` wrapper + dedent,
  no reordered emission (a VAR-id shift falsifies). Under opt-ON the body regions,
  now direct TABLEJOIN children, become newly visible to Program::Optimize
  flattening/no-op-removal/LET-elision the cmp frame shielded — benign extra
  structural churn is ALLOWED; the opt-mode `.h` expectation is
  **"re-bless + cross-mode agreement preserved,"** NOT "only the if-wrapper."
  `demand_tc_witness` (h opt) is the one golden-testable opt-mode header.
- **[STRUCT]:** exactly 2 irgold carriers re-bless; census/.deltarel byte-identical;
  A/B 0-diverged.
- **Delta count (F4, ACCEPTED wording):** 27 delta-ONLY cases do not churn; the 1
  both-carrying case churns ONLY its eager region (28 delta-carrying = 27 + 1).

**Cmp mints no next_id** (DefUse.h:896 `CreateDerived<TUPLECMP>` does not touch
`impl->next_id`; only the VAR mints :338/:506 and TABLEJOIN :314 do) → removing it
CANNOT renumber VARs. The belt is NOT the liveness guard (the CHECKMEMBER path
Join.cpp:621-660, gated on `CanReceiveDeletions() || has_unit_pred`, is the sole
liveness authority and is untouched).

---

## 2. FOLD B — side_key_eqs delta re-check retirement (lands SECOND)

**What.** The codegen-only delta-side twin of Fold A. Same premise
(`IndexedColumns(i) == pivot_cols[i] == index key column`, full-key-exact probe).
Delta join has NO TUPLECMP (`for_delta` gate, Join.cpp:317-318), so NO IR/build/
re-home change — a pure codegen string edit.

**DIFF (Database.cpp only).**
1. Delete the stale "approximate" comment, Database.cpp:2787-2790.
2. Delete the `side_key_eqs` decl, Database.cpp:2791.
3. Delete the key_eq build loop, Database.cpp:2855-2862.
4. Edit Database.cpp:2896-2897: drop the `side_key_eqs[i] << " && "` prefix
   (`indexed_cols`/`side_reads`/scan arms STAY). After-shape (cf16_2 `.h:235`):
   `if (table_4.InNew(j53_0) && table_4.InNew(j53_1) &&
   (table_4.NetAdded(j53_0)||table_4.NetAdded(j53_1)))`.

**Redundancy — closed chain at code.** `indexed_cols == pivot_cols[i]`
(IndexedColumns, Program.cpp:1143-1146) built from `pred_index->columns`
(Join.cpp:409-429); `pivot_for_col` (Database.cpp:2808-2817) `Unsupported`s any
`index.KeyColumns()` col not in `indexed_cols`; `GetOrCreateIndex` keys on the
sorted-unique pivot set and reuses only on identical col-spec (Data.cpp:350-367);
the all-columns null-return is dead (Data.cpp:352-355). So the pushed key-eq pairs
ARE the index key columns → redundant in every reachable arm.

**AMENDED — duplicate-column corner (F3, ACCEPTED qualification).** The §1.4 "no
counterexample exists" proof establishes `{indexed_cols} == {index.KeyColumns()}`
as SETS. It does NOT close the case where two distinct pivots map to the SAME
table column with DIFFERENT pivot vars (`GetOrCreateIndex` SortAndUnique-dedups,
Data.cpp:350; the key_eq loop domain does not) — there one key_eq conjunct would be
non-redundant. Downgrade "proven" → **"proven modulo the distinct-pivot-per-column
invariant"**; that invariant is EMPIRICALLY true across all 28 delta cases (every
key-eq column maps to a unique pivot var), and the 28×4 A/B stdout sweep is the
backstop.
[CORRECTED at the Fold B landing (the Fable review's finding [1]): the
sentence "there one key_eq conjunct would be non-redundant" MIS-MODELED the
pre-fold code — the deleted loop resolved each column via FIRST-MATCH
pivot_for_col, so in the duplicate-column corner it emitted the FIRST
pivot's equality TWICE and never the second pivot's constraint: the old
belt NEVER protected that corner, and reverting Fold B would NOT restore
protection there. The corner is now guarded STRUCTURALLY instead: EmitJoin
carries an always-on JOIN-KEY-DUP abort (indexed key columns pairwise
distinct per side), converting the empirical qualifier into a gate.]

**WITNESS PICK — cf16_2 (ratified R5-empirical, owner-ruling-brief.md:274).**
Enumerated all 28 delta cases; cf16_2 is the ONLY case exercising the MULTI-COLUMN
`side_key_eq` inner conjunction (`r53_0.f==v54 && r53_0.c1==v55` — the fold's
actual loop body), plus a self-join, SMALLEST gen `.h` (301 lines / 13-line `.dr`),
already has a `.batches` oracle, non-diagnostic, all-4-modes-clean (diffrun
verified). Sidecar: `cf16_2.irgold = "h opt"` ONLY (the `.ir` is Fold-B-invariant —
delta join renders `added:`/`removed:` with no belt, cf16_2 ir:94/98/102).
`linear_rec_downstream` (6 sections / 3 joins / 1049 lines) is the OPTIONAL second
breadth carrier if a wider net is wanted.

**Gate family (R5, emission-SHAPE):**
- RAT-8 land order: land fold + `cf16_2.irgold` sidecar → pre-registered red →
  bless → byte-verify folded → PASS.
- **AMENDED pre-registered red (F1, ACCEPTED):** the exact suite token is
  **`cf16_2 irgold h.opt IRGOLD-MISSING`** (runall.sh:293) — NOT "GOLDEN-MISSING"
  (that token is the stdout/oracle path, runall.sh:197/:224). `run_irgold` still
  produces `h.opt.out` (runall.sh:283), which `--bless` (runall.sh:107-118) copies
  to `goldens/cf16_2.h.opt.golden`.
- **AMENDED byte prediction (F2, ACCEPTED):** **133 of 161 COMPILED cases**
  `datalog.h` byte-identical; 28 churn `.h` only; 14 diagnostics emit no header
  (175 total). `.ir`/`.df`/`.deltarel` untouched.
- stdout 0-DIVERGED (divergence = premise false = HARD STOP); exactly one new
  golden (`cf16_2.h.opt.golden`).

**Table.h pin:** referenced (placed in Fold A at Table.h:789); Fold B deletes only
the stale Database.cpp:2787-2790 "approximate" comment.

---

## 3. THE PER-JOIN EMISSION OP — kJoinEmit / kProductEmit (lands THIRD)

**What (R3).** Model the once-per-join TABLEJOIN / TABLEPRODUCT EMISSION as two new
DR-IR ops on the M1–M17 mold. Referent: one op per (emitting proc/section,
join_view, form ∈ {eager,delta}) = 1:1 with a BuildJoin call. BYTE-MOVE slice:
emission byte-identical (no irgold re-bless); the `.deltarel` grows (11-pin
structural census-growth re-bless). TABLEJOIN mints once per BuildJoin
(Join.cpp:313), SHARED eager (Join.cpp:601) + delta (Stratum.cpp:1532) — the
2-caller M13 discharge; TABLEPRODUCT mints at sole Product.cpp:148 (eager-only, no
M13).

**ENUM.** `kJoinEmit(27)` / `kProductEmit(28)` at the tail after `kIngestLoop(26)`.
**Census 27 → 29.** Enum-edit list (M-mold): DeltaRel.h enumerators; Format.cpp
kAllKinds (append both, currently ends at kIngestLoop, Format.cpp:1085-1086); the
totality-guard message `"a 28th DROpKind"` → `"a 30th"` (Format.cpp ~1099);
**AND — NOTE-1, ACCEPTED — the census banner comment `// ---- census (27 DROpKind
counts …` at Format.cpp:1066 → "29"** (un-listed in b3, part of the same edit).

**PAYLOAD (new arm).** `emit_join_view`, `emit_form (JoinEmitForm{kEager,kDelta})`,
`emit_order_key = ContinueJoinOrder(view)`, **`emit_walk_seq`** (NEW — the
emplace_back counter, a drain-order tie-break `ContinueJoinOrder` lacks because it
COLLIDES on equal-depth joins), `emit_stratum` (delta). NO stored table (render
`table=` re-derives via `ModelTableOrNull`, E-107). Two id-neutral effect-free
ctors `MakeJoinEmitOp`/`MakeProductEmitOp` + `JoinEmitKeyOf`. Effect-FREE at flow
layer, NOT id-neutral at lower (M17).

**CAPTURE.** `emit_walk_seq` plumbs as ONE counter in the WorkItem BASE ctor
(Build.cpp:1378 `WorkItem(Context&, unsigned)` already takes Context;
`context.work_item_seq++` stored as `WorkItem::seq`) — covers all 4 join/product
creation sites (Join.cpp:764, Product.cpp:380, Induction.cpp:728/775) + the 2
induction emplaces; monotone tie-break = emplace/drain order. Eager =
mint-and-replay: mint+record at the work-item ctor into a NEW
`context.emitted_join_events` stream (per-(proc,view) cardinality — can't derive
from view-keyed marker records); replay TAIL-APPENDED so every `op.N` byte-stable
(ADJ-S2 / M4). Delta = flow-derived from `dr_flow.joins`.

**AMENDED — delta enrollment placement (HIGH-1, ACCEPTED — was a build-order
DEFECT).** b3 §2.2c enrolled the delta ops in the `BuildDRInventory` tail
(DeltaRel.cpp:2576) reading `flow.join_stratum`. CODE TRUTH: `flow.join_stratum` is
populated by `DeriveDRStrata` (DeltaRel.cpp:2803/2805, Kahn lift :2882/:2893/:2898),
called AFTER `BuildDRInventory` — the call order in `BuildStratumPhases`
(Stratum.cpp) is `BuildDRInventory` (:2125) → `DeriveDRStrata` (:2132) →
`ValidateDROps` (:2140). At the b3 site `join_stratum` is EMPTY ⇒ `js == end()` for
every join ⇒ ZERO delta `kJoinEmit` enrolled ⇒ `ValidateDROps` (:2140, which DOES
see the lifted map) computes `exp=1` but flow has 0 for d5_recursive_negate ⇒ a
FALSE abort on the sole delta carrier. **FIX (binding):** the EAGER replay stays in
the BuildDRInventory:2576 tail (`emitted_join_events` fully populated by the eager
walk at CompleteProcedure Procedure.cpp:869, before BuildStratumPhases). The DELTA
enrollment moves to a NEW step **AFTER DeriveDRStrata** — in `BuildStratumPhases`
between Stratum.cpp:2132 and :2140 — appending to `dr_flow.ops` with
`emit_stratum = the LIFTED join_stratum[jv]`. Still tail-appended after all
eager/ingest ops → `op.N` stability ([BYTE-3]) preserved.

**AMENDED — join/product Site-5 placement (HIGH-2, ACCEPTED — was a build-order
DEFECT).** b3 §4.2 placed the Site-5 multiset check as an "ingest sibling" at
~Stratum.cpp:2143 (beside the `emitted_ingest_folds`/`emitted_ingest_loops` checks,
:2161/:2213). CODE TRUTH: ingest folds/loops lower during the EAGER WALK so their
emitted-record is complete by :2143; but the DELTA `LowerJoinEmit` push is at
Stratum.cpp:1532 INSIDE `LowerDRFlow`, called at Stratum.cpp:2398 — AFTER :2143 and
:2140. A Site-5 at :2143 sees eager pushes only, while the enrolled side (post
HIGH-1 fix) has eager+delta → multiset MISMATCH → abort on d5. **FIX (binding):**
the kJoinEmit/kProductEmit Site-5 is a NEW CLOSING BLOCK at the END of
`BuildStratumPhases`, after the stratum loop's `LowerDRFlow` (:2398) /
`LowerDRRounds` (:2410) — past :2410, where both eager and delta
`emitted_join_emits` are complete. NOT beside the :2143 ingest checks, NOT inside
ValidateDROps. One block covers both forms. Key (join_table_id, form, order_key,
walk_seq) — a SET for eager (walk_seq unique); delta uses stratum in the seq slot.

**AMENDED — AllSidesSameScc is a lambda, extract it (MED-1, ACCEPTED).** The b3
call `AllSidesSameScc(flow, jv)` is NOT a callable — it is the local lambda
`all_sides_same_scc` at Stratum.cpp:1451 (captures `impl` + `recursive_sccs`), used
at the lowering skip Stratum.cpp:1486/:1520. **FIX:** extract to a file-scope
`bool AllSidesSameScc(ProgramImpl*, const RecursiveSccMap&, QueryView)` and share it
across ALL THREE sites (lowering skip :1520, delta enrollment, delta expect()) to
keep ONE authority (the kIngestFold single-authority discipline). Both new sites
already receive `recursive_sccs` (BuildStratumPhases Stratum.cpp:2125/:2132/:2140
all take it).

**LOWER.** `LowerJoinEmit` wraps the UNTOUCHED `BuildJoin` at drain; both callers
route through it (Join.cpp:601 eager + Stratum.cpp:1532 delta = M13 discharge).
`LowerProductEmit` wraps the sole Product.cpp:148 site (eager-only, no M13). Work
item carries `emit_op` (1:1, no lookup). CARVE-3: op contract = {TABLEJOIN,
pivot_vars, out_vars}; TABLEINDEX EXCLUDED (shared CSE via GetOrCreateIndex,
byte-identical by construction; the per-join index budget is global-order-dependent
so a constant budget would be false). Id-stream proof = M17 hole-contract
drain-anchored byte-move.

**COUNT.** Both forms carry a scalar `expect()` (delta from `dr_flow.joins`; eager
walk-authoritative, ADJ-S12 — closes part of the marker count gap at the emission
layer since UNLIKE the 8 markers this is flow-derivable).

**E-71 render (RATIFIED SURFACE `order=`/`seq=`).** Dedicated render case: header
`form=<eager|delta>`, args `table=` [E-107 omit-null] `order=` [`seq=` eager-only];
`JoinEmitFormName` loud-abort table (M7'); no `cmp=`/`functor=`/sublines. `key_of`:
shared lead-0 arm both kinds (dump-only, groups marker+emission by table_id); NOT
`IsEagerMarkerKind`; no V-READY (effect-free).

**AMENDED — MED-2/NOTE-2/NOTE-3/NOTE-4 (all ACCEPTED, framing/pin):**
- **[STRUCT-8] seq tripwire is TAUTOLOGICAL (MED-2).** `emit_op` stores `this->seq`
  at ctor and Run passes `*this->emit_op`, so `emit_walk_seq == this->seq` is
  `x==x` and can never fail. **DROP the seq assertion pre-flip** (state seq becomes
  load-bearing only AT the flip, where an independent drain-order replay recomputes
  it). KEEP the companion `emit_order_key == ContinueJoinOrder(view)` — that IS a
  real re-derivation check. Do NOT advertise the seq guard as a passing safety net
  on 175 cases.
- **Render stratum (NOTE-2).** §5.4 does NOT wire `emit_stratum` into
  `DROpStratum`, so every join-emit block (incl. the d5 delta) would render
  `stratum=0`. **PIN: render `stratum=0` for kJoinEmit/kProductEmit, OR wire
  `emit_stratum` into DROpStratum before blessing — recommend WIRING it for the
  delta form** so the d5 block renders its real stratum (matches [STRUCT-3/4]); pick
  one and pin it in the golden.
- **Count causal story (NOTE-3).** The "marker double-counts (2 views × 2
  in-edges)" gloss does not generalize (demand_tc is kEagerJoin=8 over 5
  join-tables). The count PREDICTION (emit == distinct join view == TABLEJOIN region
  count) is empirically correct on all 7 carriers; reword the causal story only.
- **"N procs = N events" (NOTE-4).** BuildEntryProcedure is called once
  (Build.cpp:1622); the cardinality unit is the distinct work item in the single
  CompleteProcedure walk. Reword; counts unaffected.

**Gate family (byte-move):**
- Emission byte-identical (no irgold re-bless — `.h`/`.cpp` unchanged, A/B
  0-diverged).
- `.deltarel` grows: 11-pin STRUCTURAL census-growth re-bless (census 27→29 line +
  new kJoinEmit/kProductEmit blocks) — NOT permcheck.
- Site-5 (post-fix placement) + both scalar `expect()` live.
- **Empirical carrier ledger (b3-emit, re-verified 7/7 by the critic lane):**
  SEVEN carriers gain blocks — join_1 (2 eager), demand_tc (5), symrec (2),
  optimize_2 (1 product), elim-cond-cycle-simple (1), booleans (2), d5 (1 DELTA);
  FOUR census-line-only — map_3, merge_2, negate_1, negate_6. d5_recursive_negate
  WITNESSES the delta form FREE (kEagerJoin=0 + TABLEJOIN + added/removed bodies) —
  no new delta carrier needed. Emission-op count == TABLEJOIN/TABLEPRODUCT region
  count on all 7; the once-per-emission < per-visit-marker claim holds (2<4, 1<2,
  5<8).

---

## 4. THE DIRECTION FLIP — SD-1..SD-4 (lands FOURTH)

**What (R2).** Make the S2-replica reachability (the DR side) the ONE authority for
the eager marker SET + canonical order, and retire the walk-side dispatch
record/replay's ORDER role. Derivation-order enrollment (option-2, NOT
scheduler-replay) — emission-safe because `key_of` is DUMP-AND-VALIDATOR ONLY
("emission never reads this key," DeltaRel.cpp:4672/4689). Within-band order key =
`Depth` (tiebroken by `DeterministicOrder`). The emission op's `emit_walk_seq`
(§3) is a SEPARATE surviving signal the flip does NOT touch (R1).

> **VERDICT CORRECTION: the flip lane's DEFECT is LIFTED to SOUND-WITH-FIXES by the
> HIGH-1 amendment below.** The design is landable once the constant-fact SET root
> is added. No ESCALATE — the fix completes the SET derivation strictly within R2's
> ratified frame.

**AMENDED — SET-roots MUST include all-constant-TUPLE roots (HIGH-1, ACCEPTED — the
DEFECT).** b4 SD-3.2/3.3 derived the eager SET by receive-rooted reachability only
(`worklist := ⋃ io.Receives()`). CODE TRUTH: the eager walk has TWO marker-minting
roots — (1) receives (`BuildEagerInsertionRegions(receive, …)`, Procedure.cpp:121);
(2) **all-constant facts** (Procedure.cpp:801-826: for every `impl->query.Tuples()`
with all-constant InputColumns, `BuildEagerRegion(impl, view, view, context, let,
nullptr)` at Procedure.cpp:826 — the b4 "813" cite is stale, :813 is the const-check
body). Constant-fact source views are NOT downstream of any receive, so
receive-rooted `Successors()` reachability can NEVER reach them. **Empirically
proven at tip:** `elim-cycle-simple.dr` has ZERO `#message` (`kIngestLoop=0`) yet the
walk mints `kEagerForward=2 kEagerInsert=1` from `one(1).`; **`elim-cond-cycle-simple`
is a SD-3.6 BLOCK-REORDER PIN** whose `one(1).` mints `kEagerInsert/kEagerSelect/
kEagerForward` on `%table:5`, NOT receive-reachable. Affects ≥20 corpus cases with
constant facts (cf15_1/2, conflicting_constants, deadflowelimination_2/5, elim-*,
fibonacci, insert_1/5, merge_1/4, negate_2, optimize_6, recursion, tuple_6, view_3, …).
As written this REDS: (a) the always-on SD-4 SET oracle (`derived_keys != walk_keys`
→ abort on a COMPILING corpus case → SUITE FAIL); (b) the census-byte-equal STRUCT
gate — the census line is COUNTED FROM `flow.ops` (Format.cpp:1068-1074, with an
always-on `census_total != flow.ops.size()` abort at :1099), and dropped markers
never enter `flow.ops`; (c) the elim-cond-cycle-simple pin re-bless (blocks missing,
not reordered). **FIX (binding):** add all-constant `query.Tuples()` as additional
SET roots, mirroring Procedure.cpp:801-808 verbatim —
```
worklist := ⋃ io.Receives()
for tuple in impl->query.Tuples():
    if all tuple.InputColumns() IsConstant():
        worklist.push(QueryView(tuple)); reached.insert(...)
```
Then reachability propagates from constant facts identically to the walk; the SET
becomes complete; SD-4 stays an independent SET check; census + enrollment reproduce.

**AMENDED — census line is a flow.ops readout (MED-1, ACCEPTED framing).** State
explicitly that the census LINE (Format.cpp:1066-1099) is computed by `count_kind`
iterating `flow.ops`, NOT the walk census map; the walk census reaches the dump only
via SD-3.2(b) enrolling `n` copies into `flow.ops`. So census-byte-equality depends
on BOTH a COMPLETE derived SET (HIGH-1) AND exact per-view multiplicity `n`. Same
root cause makes HIGH-1 red the census gate, not only SD-4.

**Corrections 1 & 2 (b4, CONFIRMED sound by the critic; retained).** (1) inductive
merges are reachability-TRANSPARENT but marker-SILENT — the induction Runs RESUME
the walk past the merge (Induction.cpp:558 InductiveSuccessors, :617
NonInductiveSuccessors). (2) multiplicity (per-view count) is a scheduler artifact
NOT graph-derivable without re-importing the deleted coupling — so MULTIPLICITY
comes from the WALK via a lightweight `eager_marker_census (kind,view)->count`; the
flip derives the SET + payloads + canonical Depth order graph-side, and the walk
census supplies COUNT. **ADJUDICATOR NOTE (no ESCALATE):** keeping a slim walk-side
COUNT census is CONSISTENT with R2(d) "counted-consume (i)" — R2(d) ratified that
"the emission is edge-multiset-valued but the dump is already view-valued-
with-repeats; (i) preserves that exactly … the count oracle supplies the only
cross-check (i) lacks." The ORDER still becomes a pure graph function
(Depth+DeterministicOrder), which is what R2(a) retires from the observable surface.
This is a design refinement within the ratified frame, not a ruling reversal.

**SD-1 (BYTE).** Extract `IsCutSuccessorDR` (4 clauses, demand-guard last,
byte-faithful to Build.cpp:967-971) to the DR side as sole authority; Build.cpp:970 +
`AnyCutSuccessorDR` (DeltaRel.cpp:130) call it. Fix the stale `Build.cpp:857-858`
comments at DeltaRel.cpp:127/:2383 → the real live test is Build.cpp:970 (correct
`:970` already appears at DeltaRel.cpp:3171/:3174). CONFIRMED at code.

**SD-2 (BYTE).** Relocate `ClassifyEagerSink` (Build.cpp:1095-1110) +
`MessageOfInsertOrNull` (:1113-1118) to `*_DR`; they read only Context maps
populated pre-inventory → value-identical (preserve the ADJ-S13 `find` discipline).

**SD-3 (STRUCT — the flip core).** Census increment on the walk;
`BuildDREagerInventory` derives the SET (post HIGH-1 fix: receives ∪ constant-fact
TUPLEs) + Depth+DeterministicOrder order; retire the `EmittedEagerOp` payload stream
ORDER role / `RecordEagerDispatch` / EAGER_WEB switch (the walk-side COUNT census
survives per Correction 2 / R2(d)). `kJoinEmit` + `emit_walk_seq` SURVIVE (separate
§3 slice, not a marker). 11-pin re-bless: 4 pure-relabel (negate_1/6, d5, map_3) under
a label-remap check; 7 block-reorder under the per-lead-0-band order-free multiset
(permcheck-style, scoped to the eager sign-0 band; ingest folds keep op.0/op.1,
INGEST_LOOP untouched).

**SD-4 (VALIDATOR).** SET-agreement oracle (derived keys == walk-census keys,
order-free) + 3-way perturbation liveness. **After the HIGH-1 fix the SET oracle is
green/silent on all 11 pins × 4 modes and on the ≥20 constant-fact corpus cases** (it
was the un-perturbed abort in the defect). §7d KEPT — cross-checks
MonotoneIngestRoleDR/IsCutSuccessorDR vs walk provisioning, unchanged.

**Gate family (R2):** SD-1 + SD-2 = **BYTE** (emission + dump both unchanged). SD-3
= **STRUCT** = emission `.h`/`.cpp` byte-identity (the 844-row A/B) + census-byte-
equal on all 11 pins (post-fix) + per-lead-0-band order-free multiset re-bless. SD-4
= **VALIDATOR-ONLY** (prove live by perturbation). The count oracle is SD-3's
compensating control and MUST land with it.

**Pre-registered carrier census values (re-verified EXACT at tip, both lanes):**
symrec `F7 I1 J4`; join_1 `F6 I2 C6 J4`; merge_2 `F10 I5 U5`; demand_tc `F12 I2 J8`;
d5 `F2`. (Plus the constant-fact witnesses: elim-cond-cycle-simple `kEagerSelect=1
kEagerInsert=2`, elim-cycle-simple `kEagerForward=2 kEagerInsert=1` — the HIGH-1
regression witnesses.)

---

## 5. THE RENAME — DeltaRel→Rel NARROW(b) + .deltarel→.rel lockstep (lands LAST)

**What (R6).** NARROW(b): dir + library target + file basenames + flag +
dump-surface, in lockstep, ONE commit. NOT FULL (the ~972 DR-* identifier sweep —
deferred optional cosmetic; `LowerRelStep_*` already carries bare "Rel" so NARROW
harmonizes). EXCLUDE by name the collision family: `gDRStream`, `dr_out`, `-dr-out`
(Main.cpp 58/286/318/148-151/330 — the "Dr. Lojekyll" amalgamation flag),
`dr_define_static_library` (fn name stays; target arg → Rel), `RelOut` (Oracle =
Relation), the DeltaRelationalIR.md epoch name + journal prose.

**Half A (build/symbol).** `git mv lib/DeltaRel → lib/Rel`; basenames
`DeltaRel.{h,cpp}` → `Rel.{h,cpp}` (Format.cpp stays); 6 `#include "DeltaRel.h"` →
`"Rel.h"` (Format.cpp:16, DeltaRel.cpp:14, Stratum.cpp:12, Procedure.cpp:8,
Build.cpp:5, InstanceOrderTest.cpp:21); lib/Rel/CMakeLists.txt vars + target;
lib/CMakeLists.txt:6, lib/ControlFlow/CMakeLists.txt:47/67;
tests/DeltaRelValidators → tests/RelValidators (exe + ctest NAME); symbol
`SetDeltaRelDumpStream` → `SetRelDumpStream` + `gDeltaRelStream` → `gRelStream`
(Format.h:17, Format.cpp:1113, DeltaRel.h:1131, Main.cpp:57/78/379).

**AMENDED — lib/DeltaRel/CMakeLists.txt line 36 is LIVE CODE (HIGH-1, ACCEPTED).**
b5 A4 mislabeled "lines 28-38 = doc comment." Code truth (cat -n at tip): lines
28-35 ARE the doc comment; **line 36 is
`target_include_directories(DeltaRel PRIVATE` — a second command referencing the
target BY NAME** (:36-39). Renaming the `dr_define` target to `Rel` while leaving
:36 reds CMake configure (`Cannot specify include directories for target "DeltaRel"
which is not built by this project`). **FIX:** explicit step — the `DeltaRel` target
arg at line 36 → `Rel`; scope the doc-comment claim to lines 28-35 only.

**AMENDED — tests/DeltaRelValidators/CMakeLists.txt exe rename = FIVE sites (MED-2,
ACCEPTED).** `deltarel_validators_test` → `rel_validators_test` occurs at lines
**8, 14, 19, 34, 40** (verified by grep at tip): 8 add_executable, **14
target_include_directories (target-name ref — same class as HIGH-1, omitted by
b5)**, 19 target_link_libraries, 34 sanitizer-arm target_link_libraries, **40
add_test COMMAND (prose-only in b5)**. b5 listed only {8,19,34}. **FIX:** enumerate
all five; flag :14 as a target-name reference.

**Half B (flag + dump, INDIVISIBLE lockstep).** Main.cpp `-deltarel-out` → `-rel-out`
CLEAN BREAK (no alias; sole consumer is runall.sh:275, internal debug sink, no
public ABI; flag anchors Main.cpp 210/284/365-367/372-379); 11 `.irgold` tokens
`deltarel opt` → `rel opt`; runall.sh:275 flag + filename both → `rel`; 11 goldens
`git mv *.deltarel.opt.golden → *.rel.opt.golden`. LOCKSTEP: {B1 flag, B3
runall:275, B2 sidecar, B4 golden} form a closed chain read at compile+compare — any
partial split reds ≥1 of the 11 pins (IRGOLD-FAIL runall.sh:279/289 or IRGOLD-MISSING
:293). The harness runs `-deltarel-out` UNCONDITIONALLY for every irgold case
(runall.sh:271-274, incl. aggregate_1/barrier_neck_1), so a B1/B3 flag split reds ALL
irgold cases at :279, not just the 11 — the lockstep is TIGHTER than the §2 table
shows.

**Half C (docs).** CLAUDE.md forward prose only. Leave DeltaRelationalIR.md + the 24
KeyedInstances.artifacts/ journal docs UNTOUCHED (journal prose, not renamed
retroactively).

**AMENDED — retirement-grep pre-classify (LOW-4, ACCEPTED).** Gate 6
(`grep -rn "deltarel\|-deltarel-out\|SetDeltaRelDumpStream\|DeltaRelValidators\|
deltarel_validators"`) surfaces two LIVE comment tokens NOT in b5's keep-list:
(1) `tests/InstanceStore/InstanceStoreTest.cpp:356` references the RENAMED path
`tests/DeltaRelValidators/InstanceOrderTest.cpp` → update to
`tests/RelValidators/...`; (2) `lib/ControlFlow/Build/Induction.cpp:1002` mentions
`.deltarel` → update to `.rel`. Pre-classify both (comment-only; neither breaks a
gate) so gate 6 does not flag them as strays.

**AMENDED — surface count is 17 (11-case) / 19 (suite), NOT 18 (MED-3, ACCEPTED).**
The 11 DR `.irgold` sidecars pin 17 surfaces (8 deltarel-only + demand_tc 4 + negate_1
2 + symrec 3 = 17, matching 17 goldens); whole-suite irgold = 19 (+ aggregate_1 df,
barrier_neck_1 df). State **17** (the 11 DR cases) or **19** (suite-wide) — "18" is
stale carry-over.

**NOTE-5 (ACCEPTED).** A7 Format.h prose second line is 14 (not 15); decl is 17.
Off-by-one, not load-bearing.

**Gate family (R6):** build+link green; ctest `RelValidators` 5/5 debug + ASAN;
SUITE:PASS(174); **17-surface** irgold regen all OK; A/B 0-DIVERGED (zero-semantic
proof); post-rename grep retires every live `deltarel` token (minus the pre-classified
comments); the exclusion family byte-unchanged.

---

## 6. CROSS-SLICE CONSISTENCY

**Shared Table.h pin.** ONE Runtime-contract NOTE at Table.h:789 (full-key
exactness + codegen-no-re-verify) placed by **Fold A** (lands first), referenced by
**Fold B**. The 5 stale "approximate" sites split: 4 eager (Build.h:504, Join.cpp:300,
Join.cpp:516, Join.cpp:521-525, Program.h:1510 eager clause) retired by Fold A;
1 delta (Database.cpp:2788) retired by Fold B.

**walk_seq survives the flip.** `emit_walk_seq` (captured in the WorkItem base ctor,
Build.cpp:1378) is the emission op's drain-order key — byte-LOAD-BEARING (drain order
→ next_id → generated bytes; a3 proved this class of order is NOT flow-reproducible,
0/11). It is a SEPARATE surviving signal from the eager MARKER order the flip retires:
R1 ordered emission-op BEFORE flip precisely so `emit_walk_seq` is built + proven
byte-identical while the walk-side capture machinery exists, and the flip then retires
only the marker record/replay ORDER role. The flip does NOT touch `emit_walk_seq`.

**Census arithmetic ledger.** Pre-R-final: 27 kinds (`kIngestLoop(26)`, banner "27").
The emission op (§3) adds `kJoinEmit(27)`/`kProductEmit(28)` → **29 kinds** (banner
Format.cpp:1066 "29"; totality guard "30th"; kAllKinds appends both). Folds A/B do NOT
touch the census (emission-shape, no new op). The flip does NOT touch the census count
(re-derives the SAME markers; census-byte-equal is a GATE). Final state: 29 kinds.

**Pin-churn ledger (per slice, cumulative on the 11 .deltarel + h/ir/df carriers):**
| Slice | irgold re-bless | other churn | gate class |
|---|---|---|---|
| Fold A | demand_tc (h,ir), symrec (ir) — 2 | 58/175 .ir+.h; A/B 0-div | emission-SHAPE (structural) |
| Fold B | cf16_2 (h.opt) — 1 NEW golden | 28/161 compiled .h; A/B 0-div | emission-SHAPE |
| Emission op | NONE (byte-move) | 11-pin .deltarel census-growth (27→29 + blocks) | STRUCT (census-growth) |
| Flip | 11-pin (4 relabel + 7 reorder) | emission byte-identical (844-row A/B); census-byte-EQUAL | STRUCT + VALIDATOR |
| Rename | 11 .irgold token + 11 golden `git mv` | runall.sh:275; emission byte-identical | BYTE (zero-semantic) |

The Fold-A carriers (join carriers) re-bless conceptually TWICE across the sequence
(emission-op census-growth, then flip marker order) — accepted per R1.

**Build-order corrections (the two emission-op DEFECTS + the flip DEFECT) share a
root:** the DR-IR phase pipeline in `BuildStratumPhases` (Stratum.cpp) is
`BuildDRInventory(2125) → DeriveDRStrata(2132) → ValidateDROps(2140) → [ingest
Site-5 ~2143-2214] → LowerDRFlow(2398) → LowerDRRounds(2410)`. Any new
delta-dependent enrollment reads `join_stratum` only AFTER :2132; any new Site-5 over
a DELTA-lowered emitted-record must sit AFTER :2410. Any new SET derivation that must
match the walk must root at BOTH walk roots (receives AND constant-fact TUPLEs,
Procedure.cpp:121 + :826).

---

## 7. ADJUDICATION RECORD (finding → disposition; verified at code, tip e30632be)

All ACCEPTED (design amended per §1–§5). Zero REFUTED. **Zero ESCALATE** — no
finding conflicts with a ratified R1–R6 ruling (the flip's walk-side count census
is consistent with R2(d) counted-consume, §4 note).

**Fold A (c-foldA-critique):**
- F1 MED — col_id_to_var re-home is a same-key double-write, not "no collision".
  **ACCEPTED.** Verified: Join.cpp:344 (`join->col_id_to_var[pivot_col.Id()]`) vs
  re-homed :533 (`[out_col->Id()]`, out_col ∈ PivotColumns, role kJoinPivot :505) —
  same key. §1 amended to intentional last-write-wins + 2 invariants + implementer
  warning.
- F2 LOW — descent-fills-body asserted not proven; EmitJoin drops empty-body join.
  **ACCEPTED.** Verified Database.cpp:2770 early-return; Build.cpp:861-862 backstop.
  §1 states the guarantee.
- F3 LOW — [BYTE] "only if-wrapper" holds only nocf/none. **ACCEPTED.** §1 scopes
  the prediction; opt-mode = re-bless + cross-mode agreement.
- F4 LOW — delta count phrasing. **ACCEPTED.** §1: 27 delta-only + 1 both.
- NOTE — no A/B yet. **ACCEPTED** as stage-(c/d) obligation (§8).

**Fold B (c-foldB-critique):**
- F1 MED — RAT-8 wrong suite token. **ACCEPTED.** Verified runall.sh:293
  `IRGOLD-MISSING` (vs :197/:224 GOLDEN-MISSING). §2 uses exact token.
- F2 LOW — 147/175 over-counts diagnostics. **ACCEPTED.** §2: 133/161 compiled.
- F3 LOW — duplicate-column corner not closed. **ACCEPTED.** §2 qualifies "proven
  modulo distinct-pivot-per-column," empirical backstop.
- (witness pick cf16_2) — within ratified mandate (owner-ruling-brief.md:274);
  CONFIRMED sole multi-column carrier.

**Emission op (c-emit-critique):**
- HIGH-1 — delta enrollment reads join_stratum before it exists. **ACCEPTED.**
  Verified Stratum.cpp:2125/2132/2140 order + DeriveDRStrata populates join_stratum
  DeltaRel.cpp:2803/2805. §3: move delta enrollment after DeriveDRStrata (2132–2140),
  emit_stratum = lifted value.
- HIGH-2 — join/product Site-5 reads emitted before delta lowering. **ACCEPTED.**
  Verified delta LowerJoinEmit at Stratum.cpp:1532 inside LowerDRFlow(:2398). §3:
  Site-5 = closing block after :2410.
- MED-1 — AllSidesSameScc is a lambda. **ACCEPTED.** Verified Stratum.cpp:1451
  lambda used :1486/:1520. §3: extract to file-scope helper, 3-site authority.
- MED-2 — seq tripwire tautological. **ACCEPTED.** §3: drop seq assertion pre-flip;
  keep emit_order_key check.
- NOTE-1 — census banner Format.cpp:1066. **ACCEPTED.** §3 enum-edit list.
- NOTE-2 — render stratum=0. **ACCEPTED.** §3: pin/wire (recommend wire delta).
- NOTE-3/NOTE-4 — count causal-story / per-proc framing. **ACCEPTED** (reword).

**Flip (c-flip-critique) — DEFECT lifted to SOUND-WITH-FIXES:**
- HIGH-1 — SET-roots miss the all-constant-TUPLE root. **ACCEPTED (the DEFECT).**
  Verified Procedure.cpp:826 constant-fact BuildEagerRegion + Format.cpp:1066-1099
  census counts flow.ops + census abort :1099. §4: add constant-fact TUPLE SET
  roots (Procedure.cpp:801-808 mirror). Empirical witnesses: elim-cycle-simple,
  elim-cond-cycle-simple (a pin).
- MED-1 — census line is a flow.ops readout (framing). **ACCEPTED.** §4 wording.
- NOTE-1 — 826-not-813 cite drift; SD-1 stale-comment claims correct. **ACCEPTED.**
- (Corrections 1 & 2) — CONFIRMED sound; walk-side count census consistent with
  R2(d), no ESCALATE (§4 note).

**Rename (c-rename-critique):**
- HIGH-1 — A4 mislabels CMakeLists.txt:36 target_include_directories as doc comment.
  **ACCEPTED.** Verified line 36 live code. §5: explicit target rename, doc comment
  = 28-35.
- MED-2 — A6 exe-rename line-list incomplete. **ACCEPTED.** Verified 5 sites
  (8,14,19,34,40). §5 enumerates all five.
- MED-3 — "18-surface" miscount. **ACCEPTED.** §5: 17 (11 DR cases) / 19 suite.
- LOW-4 — retirement-grep two live comment tokens. **ACCEPTED.** §5 pre-classifies
  InstanceStoreTest.cpp:356 + Induction.cpp:1002.
- NOTE-5 — Format.h prose line 14 not 15. **ACCEPTED.**

---

## 8. FOLD A READINESS (the FIRST implementable slice)

Fold A is design-COMPLETE and code-anchored at tip; the remaining obligations are
stage-(d) EMPIRICAL, not design:

1. **Prototype build + 4-mode A/B on a CLEAN worktree (BLOCKING).** b1 correctly
   DEFERRED the prototype build — `wt-rfinal-probe` was DIRTY at critique time
   (`M lib/ControlFlow/Build/Build.cpp`, a concurrent lane's edit). Fold A's entire
   safety case is a paper argument until the 58-case × 4-mode A/B stdout sweep runs
   green (0-DIVERGED); a divergence = premise-false = HARD STOP (a latent Runtime
   bug the belt masked, NOT a Fold-A defect, but it blocks the landing). Provision a
   clean worktree at e30632be, build under `build/<yourname>`, run the sweep + data/
   40-file compile sweep.
2. **desired-states target.** Add the Fold-A desired state to the rel-arch-pseudocode
   §5 R-FINAL block / re*-desired-states sidecar on the M-mold (the TUPLECMP-retired
   join body; the Table.h:789 pin; the F1 last-write-wins invariants).
3. **Blind-prototype scope (stage-(d) three-way convergence precedent).**
   dump-blind author == blind worktree prototype == pristine implementation,
   BYTE-IDENTICAL on the 2 irgold carriers + census/.deltarel; Option B signature
   (bare TABLEJOIN* + Stratum.cpp:1532-1535 null-assert drop) is the recommended
   scope, Option A (Join.cpp-only pair) the fallback.
4. **Pre-registered reds:** exactly `{demand_tc_witness h+ir, symrec_tie_1 ir}`
   IRGOLD-DIVERGE → bless → PASS(174); A/B 0-diverged; ctest 5/5 debug + ASAN;
   Q5 progsize shrink.

No design obligation remains for Fold A. The F1/F2/F3/F4 amendments are text/pin
corrections already folded into §1. Ready to enter stage-(d) prototype.
