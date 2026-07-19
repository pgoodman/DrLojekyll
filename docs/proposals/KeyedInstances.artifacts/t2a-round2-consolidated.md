# T2a round-2 fleet — XHIGH CONSOLIDATOR adjudication

Tip 369c6a63 (code == frozen 35b89aab). All load-bearing cites re-checked at
code by the consolidator; the rest trusted from the critics where they agree and
I spot-checked the anchor.

## GO/NO-GO: **GO for T2a with 5 mandatory emitter amendments + 2 owner pins.**

The pseudocode spine is CODE-CORRECT: L1 (private selects-first stamp order,
Query.h:1176-1214 vs public joins-first), last det_seq stamp = IdentifyInductions
(mode-robust), flag-off byte-identity (emitter gated on gDFStream), overload
disjointness (QueryDF tag struct), T2b.0-landed / drain window
(Stratum.cpp:2166-2167 verified). No NO-GO defect. Two real emitter-body bugs
(A2 AutoVar_N, A3 reachability edge-set) MUST be fixed before code — both would
produce a WRONG dump; one fails loudly at gate 8, the other silently over-marks.

Ordering constraint: T2a step 8 byte-diff runs against **(p1)-re-rendered +
residual-amended** copies of the three .df contracts, never the committed text.
The deltarel contract amendment (9 `reads: InI` deletions) is a T2b/T2b-bless
item, not T2a, but is logged here as a committed-contract ledger event.

--------------------------------------------------------------------
## 1. EMITTER PSEUDOCODE — AMENDMENT LIST (each = tag + exact change)

- **A1 [B1/F1, CONFIRMED at Query.h:1217-1266 + DefUse.h:1055-1058]** — the §2.1
  `ForEachDFView` comment "each Query::<Kind>() DefList range already skips
  is_dead views" is FALSE (the const stamping walk skips via `if(!view->is_dead)`
  per-kind; the public `DefinedNodeIterator::operator++` does NO skip). CHANGE:
  add `if (v.impl->is_dead) return;` at the head of the `ForEachDFView` callback
  wrapper (so Pass 1, Pass 2, SCC, and the kind-classifier map all share the
  dead-skip), AND replace the false comment with: "the public DefLists are
  physically dead-view-free at the post-Build drain (RemoveUnusedViews,
  Build.cpp:2596); the callback replicates the const ForEachView is_dead skip so
  the traversal set is UNCONDITIONALLY identical to the stamp set — Pass 1's s>=N
  is the always-on tripwire if that ever regresses." (`v.impl->is_dead` is
  reachable via the already-added lib-private "Query.h" include, L3.)

- **A2 [F12, CONFIRMED at Parse/Format.cpp:14-22 + Format.cpp:74]** — `col_tok`
  renders the named branch as `os << var->Name()`, which streams the raw token
  and DROPS the `AutoVar_<IdInClause()>` mapping the contracts require
  (tc `^tuple.2` = `(AutoVar_2:u64,...)`). CHANGE: render the variable branch as
  `os << *c.Variable() << ':' << c.Type();` (invokes
  `operator<<(OutputStream&,ParsedVariable)`, which emits AutoVar_N for
  kIdentifierUnnamedVariable) — NOT `var->Name()`. Keep the `c<id>` fallback ONLY
  for the no-variable case; NEVER fall through to the `std::optional` operator
  (it prints `_MissingVar`, §1.3-forbidden).

- **A3 [R2 reachability, CONFIRMED at Link.cpp:343-346]** — §4 runs Tarjan over
  `QueryView::Successors()`, which is a SUPERSET of the `=>` edge set:
  Link.cpp:343-346 wires INSERT→SELECT materialization into `successors`, an edge
  the `=>` grammar never renders (INSERT terminal). A materialization-closed
  recursive relation would then be one SCC over Successors but broken at the
  INSERT over `=>` edges → design (B) OVER-MARKS `; cycle` vs both blessed
  contracts (which computed over the `=>` graph). CHANGE: run Tarjan over the
  emitter's OWN `=>` edge map (§3.2's inverse-of-DOT producer→user set), NOT
  `Successors()`. Delete the §4 "Equivalence caveat" paragraph's false claim
  ("equivalence is total"); replace with "SCC is computed over the `=>` edge set
  exactly, so same-SCC ⟺ reachable-over-`=>` by construction." Then design (B)'s
  O(V+E) cost survives and it matches the contracts.

- **A4 [R4 reachability, SPEC-GAP → PIN]** — §4 mandates "ITERATIVE Tarjan" but
  gives no body; iterative Tarjan is the most error-prone SCC transcription
  (child-lowlink propagation via a resume frame). CHANGE: the T2a diff must carry
  a concrete iterative-Tarjan body (index/lowlink arrays, work-stack frames with
  a child-iterator cursor, on-stack set). Seed the outer loop over `ForEachDFView`
  (det_seq order) so internal SCC numbering is deterministic (R5 guard). NOTE
  (fold into the comment): the corpus graphs are tiny (tc ~13, demand_tc ~20) so a
  correct RECURSIVE Tarjan would also pass every gate — iterative is
  defense-in-depth, correctness > stack-safety-theater at this scale; do not let
  "iterative" introduce an SCC bug.

- **A5 [F14, CONFIRMED]** — two wrong line cites: §3.3(8) `neg.HasNeverHint()`
  is Query.h:800 (not "negate:11"); §3.3(7) `MergedViews()` is Query.h:736 (not
  ":820 area", which is NegatedView). CHANGE: correct both anchors. (Accessors
  exist; trivial.)

### Non-amendments folded into comments (no code change, but state them)
- **F4 → gate-6 broadening**: extend the %table eyeball (T2a step 6) from tc-only
  to ≥1 real `%table:` in EACH of the three step-4 carriers (tc/symrec/demand) —
  a table-backed view reading nullopt would silently bless an empty attribute.
  CHANGE step 6 wording. (No emitter code change.)
- **B8 → framing**: reword the spec's "the ONLY always-on authority in release"
  to "always-on WITHIN THE DUMP PATH (fires iff -df-out requested; NOT a guard on
  ordinary compiles)". Dump-path gating is the right scope (the stamp is a clean
  re-stamp by construction; the witness catches a LATER view-killing pass, which
  only matters for the dump). No every-compile assert needed.

--------------------------------------------------------------------
## 2. CONTRACT AMENDMENT LISTS (byte-level, LOUD — ledger events)

### 2.1 t2-desired-df-transitive_closure.md — AMEND (1 byte-error)
- **C-TC-1 [(p1) pin, CONFIRMED]**: §1 line 84 renders
  `recv ^select.0 () -> (From:u64, To:u64)`. Pin (p1) mandates
  `select ^select.0 (From:u64, To:u64)   ; <recv provenance>` — leading keyword
  = id kind `select` (not `recv`), NO `() -> (...)` arrow, provenance in trailing
  `;`. select.0's own cols are c1=From,c2=To (firm). CHANGE: re-render line 84 to
  the (p1) form. The contract's STATUS header claims v3 compliance but §1
  predates the v3.1 (p1) pin; its own R3 mislabels this "OPEN" — (p1) already
  CLOSED it as a hard pin. (The provenance spelling `add_edge` is illustrative,
  not graph-firm; `select` + no-arrow are firm.)

### 2.2 t2-desired-df-symrec_tie_1.md — HOLDS (no amendment)
- The symrec contract §1 does NOT show a `recv`/arrow header (its SELECT renders
  the (p1) form already) — CONFIRMED by the re-verifier. det_seq mechanism,
  class map, arm wiring, .in<K>, strata-per-role, cycle-6-set all byte-exact.
  The seven TUPLE ids 1..7 stay [RESIDUAL]-illustrative (no creation-order signal
  in .dot/.ir) but their (role,stratum,table) triples are firm — bless-time pin,
  not a defect.

### 2.3 t2-desired-df-demand_tc_witness.md — HOLDS at graph facts, PENDING (p1)
- The demand contract §1 renders SELECT as `select` (the re-verifier CONFIRMED
  20 blocks 1:1, all 24 `=>` lines, 13 cycle marks byte-exact). No `recv`/arrow
  header error found. BUT L2 flagged the trailing comment lead-in `; recv
  #message ...` as a rendering-surface pin (§3 residual 4) — the NAME/arity are
  graph-verified; only the fixed lead-in spelling is unpinned. NOT a byte-error;
  a bless-time surface pin. NO amendment required.

### 2.4 t2-desired-deltarel-average_weight.md — AMEND (9 byte-deletions + 1 minor)
- **C-AW-1 [p4 pin, CONFIRMED at DeltaRel.cpp:730/464 (kInIReadFrozen) +
  1690-1705 (sweep clean)]**: DELETE the nine `reads: InI(%table:N)` lines — the
  InI read is the kInIReadFrozen EffKind (already present in each op's `effects:`
  block), and p4 forbids kInIReadFrozen on `reads:` (reads: = Pred spellings from
  kFlagRead only). Lines: 203 (op.4 KV GU), 233/239 (op.6/7 SEED_FOLD), 271
  (op.0 sum GU), 306 (op.2 count GU), 370/376 (op.8/9 SEED_FOLD), 403/409
  (op.10/11 SEED_FOLD). After deletion each op has NO `reads:` line (its only
  membership content was the InI crossing, which stays under effects:). The
  FRONTIER_FILTER `reads: NetDeleted/NetAdded` lines (kFlagRead → Pred spelling)
  are p4-COMPLIANT — KEEP. The contract itself flags this at its F-5 as a KNOWN
  unfixed defect; unblessable as-is.
- **C-AW-2 [MINOR, bundle with C-AW-1]**: normalize the `reads: —` placeholder —
  op.52 (line 190) and op.24 (line 214) render `reads: —` while the other 13
  claim drains omit the line entirely. OMIT the line uniformly (delete the two
  `reads: —`), matching the 13 claim drains and the p4-stripped ops. Keep op.24's
  `; TryClaimDel...` as a trailing comment.

--------------------------------------------------------------------
## 3. PER-RESIDUAL FINAL RULINGS (CONFIRMED / AMENDED + evidence)

### transitive_closure
- **R1 (INSERT det_seq ids 13<14<15)** — **CONFIRMED-AS-PREDICTED.** Inserts own
  no output columns (Columns.cpp assigns none), so det_seq is unwitnessed by any
  col-id; DefList order = query->inserts creation order (Build.cpp clause order).
  Strata 9<10<11 firm but strata≠det_seq. Stays PREDICTED; reconcile at first
  bless (adjust the contract if DefList order differs).
- **R2 (.in0=tuple.3/.in1=tuple.2 JOIN port assignment)** — **CONFIRMED-AS-
  PREDICTED (pin p3).** DOT port order (Format.cpp:328-362) is pivot-set use-list
  then merged-output, NOT joined_views iteration — the true `.in<K>` source
  (joined_views index) is unwitnessed by any dump. Consistent, unfalsified,
  UNPROVEN. If joined_views swaps, producer `.in<K>` lines and join-body
  `.in0/.in1` move together. **Carries F3's falsifiability caveat**: gate 8 is
  edited-to-match for this field, so first bless must cross-check `.in<K>`
  ordering is a deterministic FUNCTION of det_seq / joined_views UseList order,
  not "whatever the emitter printed twice" (Q5 proves stability, not correctness).
- **R3 (recv keyword/arrow)** — **AMENDED** → C-TC-1 above.
- **R4 (table-id eyeball)** — **CONFIRMED.** %table:4 (shared tc/reachable_from/
  reaching_to) + %table:8 (is_node) live in tc.ir:9,17.

### symrec_tie_1
- **RESIDUAL 1 (.in<K>: arm2 .in0=tc/.in1=edge; arm3 .in0=edge/.in1=tc)** —
  **CONFIRMED.** joined_views = pivot_groups next_views order (Build.cpp:1246-49);
  DOT p0/p1 = that order, pointer-free. Byte-exact to §1.
- **RESIDUAL 2 (det_seq tie-break: arm2=^join.8, arm3=^join.9)** — **CONFIRMED.**
  block id = det_seq = joins-DefList creation order = source-clause order
  (Build.cpp:2530); arm2 (line 18) before arm3 (line 19). OrderViewsDeterministically
  Sort/first-col-id levels tie but do NOT set block id.

### demand_tc_witness
- **R1 (.in<K>)** — **CONFIRMED.** demand-side always .in0, read side .in1
  (Demand.cpp construction order); all §1 tags match DOT pivot-ports.
- **R2 (13 `; cycle` markers, reachability-exact)** — **CONFIRMED.** Independent
  Tarjan over the 24-edge `=>` graph (reachability critic AND the demand
  re-verifier both recomputed) yields EXACTLY the 13 marked edges + the correct
  bare set; demand-seed answer arm sits outside the SCC. NOTE: this confirmation
  is valid ONLY because the recompute used the `=>` edge set — which is precisely
  why emitter amendment A3 (Tarjan over `=>`, not Successors) is mandatory.
- **R3 (demand__reachable_from_bf/1 numbering + c3)** — **CONFIRMED.** select.1 =
  fabricated receive, `demand__` prefix (Demand.cpp:177) + `reachable_from` + `_bf`
  adornment; single nameless col → c3. Config-invariant, stays visible (a3).

### average_weight (deltarel)
- **Residual 1 (within-band order under +1 key `t?uintptr_t(t->id)+1u:0u`)** —
  **CONFIRMED.** +1 is a monotone shift (a<b ⟹ a+1<b+1), so band-9 sweeps
  4,8,12,17,23,28,32,36 and band-10 seals sc#0/sc#1/sc#2 order are IDENTICAL to
  plain-id-ascending; the +1 only makes null=0 disjoint. A1 stratum re-render
  (owner_stratum GU-below-drain split) orthogonal, intact. Verified op_table_id
  form at DeltaRel.cpp:3394-3402.
- **Residual 2 (op.4 never-minted-vec note)** — **CONFIRMED.** %table:36
  (monotone) mints no phase vec (mint loop DeltaRel.cpp:878-880 skips
  non-differential); op.4 band-(a) drains + op.52 append reference t36 vecs by
  ROLE NAME, never a `$role.idx` — the LOUD flag that `flow.TableVec(t36,...)`
  asserts (find+assert) is accurate; render by role only. 43 vecs total (7 diff ×
  6 + 1 join-pivots), no t36 vec line.
- **Residual 3 (kInIReadFrozen / reads: under p4)** — **AMENDED** → C-AW-1 (9
  line deletions). op.44 sweep already p4-clean (DeltaRel.cpp:1690-1705 mints
  {kInIReadFrozen, kFlagWrite}, no Pred read → no reads: line). FRONTIER_FILTER
  NetDeleted/NetAdded reads are kFlagRead → KEEP.
- **deps section** — **PREDICTED/floor (no amendment).** The contract's own
  F-9/F-10 declare it non-byte-exact: dep_edges built by iterating two
  std::unordered_maps (hash-order, not stable) and the flag-class WAR block is a
  "confirmed floor, not certified-complete". Emitter-time work: canonically sort
  dep_edges by (from,to,kind) + full flag enrollment. Consistent with §7 A.3.

--------------------------------------------------------------------
## 4. REJECTED FINDINGS (loud)

- **REJECT F5 (predictions critic) + the T3-composition critic's :264 anchor** —
  BOTH claim `run_oracle || st=1` is at runall.sh:264 and the pseudocode's :248
  is "drift". FALSE at tip 369c6a63: `run_oracle || st=1` is at
  **runall.sh:248** (verified), and the summary grep is at :284 (verified). The
  pseudocode's :248/:284 anchors are CORRECT. The two critics were reading a
  different tip's line numbers. The pseudocode needs NO re-anchor for T3 step 1.
  (The `run_irgold || st=1` sibling placement beside :248 is correct as written.)

- **REJECT (partial) the reachability critic's R1 "SCC direction-agnostic ⟹ (B)
  robust to the ForEachUser trap" as a reason to keep Successors** — the note is
  mathematically true (SCC(G)==SCC(reverse G)) but IRRELEVANT to A3: the defect is
  the EDGE SET (Successors ⊋ `=>`), not the direction. Robustness to direction
  does not rescue running over the wrong edge set. (The critic itself reaches the
  same conclusion; flagging so the "robust" language is not misread as
  "Successors is fine".)

- **DOWNGRADE B1 severity dispute** — B1's own re-assessment (no is_dead view
  reaches the drain on the current pipeline; DOWNGRADE to LATENT MEDIUM) is
  ACCEPTED, but the FIX is still mandatory (A1): the spec §1.2 directive is
  "SHARE the traversal, never re-derive it", and the false justification comment
  is a self-inconsistency in the witness's charter. So B1-as-latent is accepted;
  the amendment is NOT waived.

--------------------------------------------------------------------
## 5. OWNER-BEFORE-CODE PINS (2)

- **PIN-1 (constant-column token, F13)** — genuinely under-determined and
  un-cross-checked by any contract with a real constant column. The DOT precedent
  (Format.cpp:60-71 `do_const`) renders a constant column as its literal VALUE /
  TAG / TRUE, NOT `c<id>`. The pseudocode's `col_tok` renders `c<Id()>:<type>` for
  `IsConstantOrConstantRef()`. OWNER MUST RULE: (a) render the DOT `do_const`
  value (provenance-faithful, matches the existing dump), OR (b) pin
  `c<id>:<type>` and DOCUMENT the deliberate DOT divergence — and either way add a
  contract with a REAL constant column (symrec/demand have none) so gate 8 is
  falsifiable for this path. Blocks blessing any .df golden that contains a
  constant column; tc/symrec/demand do NOT, so T2a wiring is NOT blocked, only the
  constant-column bless.

- **PIN-2 (T3 `h`-surface plumbing, F9)** — CONFIRMED at Main.cpp:79-121: `h`
  is emitted only by `-cpp-out <DIR>` as `<gDatabaseName>.h` inside a dir, and
  `#database` renames it (not always `datalog`). The flat
  `$NAME.irgold/<surface>.<mode>.out` layout is unimplementable for `h` without a
  post-copy. OWNER/impl pin: run_irgold special-cases h — `-cpp-out
  $NAME.irgold/cpp.<mode>/` then `cp $NAME.irgold/cpp.<mode>/<dbname>.h
  $NAME.irgold/h.<mode>.out`, resolving `<dbname>` by globbing the single `*.h`.
  df/deltarel/ir stream to `<surface>.<mode>.out` directly. One-compile-all-
  surfaces survives (one compile still emits all four; only the h read is a copy).
  This is a T3 item, not T2a — logged so T3 does not stall.

--------------------------------------------------------------------
## 6. ACCEPTED-SOUND (no action) — spot-confirmed
- L1/L3 wiring, QueryDF overload disjointness, Pass-1 bijection math (~0u
  subsumed by s>=N; N==0-safe; seen-bitset pigeonhole; sum/XOR rejected), det_seq
  last-stamp mode-robustness, flag-off byte-identity, T2b.0 landed
  (op_table_id :3401), E-62 tripwire clean, T2b drain window
  (Stratum.cpp:2166-2167), T3 one-compile-all-surfaces + strict-cmp + IRGOLD-*
  token grep-visibility, P1 four-mode aliasing, SCC cycle-safety (Tarjan
  on-stack), determinism (only equality of scc_id observed).
- SPEC-GAPs accepted as pins-not-blockers: B7 (empty-program .df = bare header;
  add an empty witness at T3 or scope out), F10 (Q5 runspec=progsize chain, opt
  mode, ±0.5%=noise; T2a's Q5 is a wiring-regression tripwire, near-vacuous;
  T2b's rests on the pre-guard `if(stream){...}`), F5-composition (bless MUST
  iterate the cases/<name>.irgold sidecar and hard-error on a missing produced
  file, never the `[ -f ] && cp` skip-idiom — prevents silent under-bless).
