# D3.a.1 — BINDING DESIGN (stages (b)/(c) adjudicated) — differential demand: retract channel, death, (T,F) publish, witness

> **House banner.** COMMITTED at the stage-(c) close (ledger §20(AJ)) after
> ORCHESTRATOR re-verification of all ten load-bearing anchor families at code
> (Database.cpp:2394-2398 gate-sans-liveness; Table.h:421-424 Present +
> :355-378 Add/SubExplicit; Rel.cpp:800-803 kInstanceDemand; Rel.cpp:1139
> P-DEATH; Procedure.cpp:196-205 ClassifyVector arm; Procedure.cpp:274-345
> coherence/ctor/enrollment; Rel.cpp:4496-4524 V-INST-DRAIN; View.cpp:513-514 +
> Query.h:1254-1258; Differential.cpp:81-106 in-fixpoint @never arm;
> Database.cpp:2439-2452/:1460-1466 + Program.h:1188-1190). Produced by the
> 2026-07-28 stage-(b)/(c) fleet: 4 xhigh design lanes + 4 fresh critics +
> 1 xhigh adjudicator (~1.59M tokens; 20 findings CONFIRMED and folded, 0
> refuted, 0 escalations). The ANNEXES live beside this file as
> d3a1-b{1,2,3,4}-design.md (referenced below as b1/b2/b3/b4-design.md).
> Binding for the D3.a.1 stage-(d) prototype + pristine implementation.

> XHIGH ADJUDICATION at tip **95251825** (verified `git rev-parse HEAD`, tree
> clean). Inputs: b1/b2/b3/b4-design.md + the four critiques (read in full);
> every load-bearing critique finding RE-VERIFIED AT CODE by this adjudicator
> (grounds in §7). Binding context honored, not re-litigated: d3a1-substrate.md
> §5/§6/§7 (d2 ruling: CO-ACTIVATION; P-STORE stays `TableIsDifferential(pub)`
> at Rel.cpp:1055/:1059; P-DEATH stays demand-keyed at Rel.cpp:1139; predicates
> NEVER folded; XC-3 co-landability), d3a-ruling-brief.md (OD-15: SET-demand +
> batch SET netting; death = full (T,F) retract + RecycleCurrent under the
> PINNED three-way coupling; drop-before-born per iid; RAT-7 belt; no iid
> tombstone), d3a-substrate.md §7:888-1001 (d1-d7), d3a0-design.md §3.
> THE WHOLE SLICE LANDS AS ONE COMMIT (XC-3 + EMPIRICAL-2: d1 needs d3/d4, and
> d6 needs d4 — two independent proofs; no temporary fence, none needed).
> House rules: always-on validators = fprintf+abort surviving NDEBUG; new dump
> tokens get E-71 grammar notes; the eqgate flat==nested answer identity (now
> UPGRADED to answer + sorted published-delta identity) is the standing oracle;
> per-surface [BYTE]/[STRUCT] pre-registered in §6.
>
> The four lane designs travel with this draft as ANNEXES. This draft BINDS:
> where it amends a lane, the amendment wins; where silent, the lane's edit
> spec is adopted verbatim (its critique's cosmetic anchor fixes applied).

---

## §0 HEADLINE SHAPE + ADJUDICATOR RATIFICATIONS

**The merged slice** = ONE commit, four sub-diffs:

- (i) **b1** — `-demand-retract` toggle (implies `-demand`, orthogonal to
  `-demand-instance` + the 4 golden modes, OFF PassPolicy) → fabricated demand
  message gains a synthetic `@differential` token → the demand closure flips
  differential via the ordinary machinery (ZERO new differential code flat);
  a SECOND `kQueryMessageInjector` proc + generated hidden friend
  `<name>_<pattern>_retract(db, log, functors, bound...)` (void) writes the
  del_vec; SET netting = the landed NETBATCH arm engaging automatically.
- (ii) **b2** — death lowering: band-(a0) INSIDE the SUBGRAPHINSTANCE region
  (seal-precedent head-mirror, ratified R-1), `removal_frontier` UseRef +
  `RemovalFrontier()` accessor, `{sid,kInstanceDeath}` enrollment closing
  V-INST-EMITTED, FindInstance+RecycleCurrent-only emitter (kNoInstance ⇒
  silent skip, ratified R-6), V-INST-DRAIN regime split + pure
  `CheckInstanceDeathFrontier` + G-8 source check + V-INST-DEATH-COHERENCE;
  ZERO ControlFlow pre-provisioning (the differential machinery provisions the
  demand sextet + ± frontier filters before ValidateDROps).
- (iii) **b3** — band-(b) (T,F) drop scan (OVERDELETE-first per iid), delete =
  SubDerivation + DelQueue append (GROUP_UPDATE fold mold), born arm flips to
  AddDerivation/`added_row`/AddQueue (DiffTable has no TryAdd), always-on
  generated V-INST-PARTITION belt, `IsDifferential()`/`DelQueue()`/`AddQueue()`
  accessors + queue UseRefs, d5 `, false` ctor by co-activation, **PLUS the
  adjudicator-folded band-(a2) DEMAND-LIVENESS GATE (R-3, the b3-critique
  HIGH-1 / b4 I-3 reconciliation — §3.2)**.
- (iv) **b4** — witness growth (the `@differential` `nbhd_out` tap, retract/
  dead-key-edge/rebirth/second-death phases, PrintLog sorted flushes), the
  DS-R4-10 post-fixpoint fence move + `negate_never_diff_1` (with the is_dead
  filter, §4.1), the InstanceStore death-rebirth unit, the consolidated gate
  plan (10-red pre-bless set), FINDINGS.md F25.

### Ratifications (adjudicator rulings within the granted frame; none escalate)

- **R-1 (b2 F-4): band-(a0)-INSIDE-the-region is RATIFIED** over the literal
  d3a1-substrate.md:387-389 phrase "its own pre-instantiate emission". The
  operative constraint is the EPOCH POSITION (§4.4: death before a1/a2 for the
  store), which textual band order inside ONE region realizes and no
  region-flattening pass can reorder; kInstanceSeal is the standing precedent
  for a region-less self-lowered instance op (enrollment Procedure.cpp:341-345,
  emission Database.cpp:2461). b3's design already assumed exactly this
  interface ("death emission runs BEFORE band-(a1) for its store and calls
  RecycleCurrent" — b3 §7). The substrate line is re-read as an as-landed gap
  observation; this ratification is the one line b2's critique asked for.
- **R-2 (XC-3 chain reorder — stated LOUDLY):** the merged design keeps XC-3's
  VERDICT (d1 is not independently landable; the slice is one commit) but
  REORDERS its counterfactual abort chain. Under b2's D-3 regime-split
  V-INST-DRAIN, a d1-only build no longer aborts at V-INST-DRAIN (the
  differential arm's DR-side producers exist by BuildDRInventory time); its
  FIRST abort is **V-INST-EMITTED** (death minted at Rel.cpp:1139-1149, never
  lowered; Procedure.cpp:435-464 prints the enrolled-vs-emitted sizes).
  d3a1-substrate.md:729-733's "V-INST-DRAIN first" is superseded as a
  counterfactual description; the stage-(d) L2 checkpoint OBSERVES the
  reordered chain (b4's L6 procedure is amended accordingly, §4.5).
- **R-3 (b3 HIGH-1 = b4 I-3): the band-(a2) demand-liveness gate is FOLDED
  as a binding b3 edit** (§3.2). Verified at code: the landed a2 gate
  (Database.cpp:2394-2398) is `iid != kNoInstance && !TouchedFlag(iid)`;
  iids are append-only with no tombstone (OD-15) and TouchedFlag resets at
  Seal (InstanceStore.h:205), so after a death a later input edge for the
  dead key would re-materialize its whole neighborhood into pub — divergent
  from flat (which joins against the absent demand row and produces nothing)
  and triply red on b4's witness (e7 hard abort + eqgate + golden). Under
  R-MONO `FindInstance != kNoInstance` WAS the liveness gate (demand never
  retracts); d1 breaks that equivalence — the gate must probe the demand
  table's `Present` membership (Table.h:421-424). NOT an OD-15 conflict: the
  ruling forbids iid tombstones, and the gate tombstones nothing — it keys
  rebuild on demand presence, exactly what the flat==nested oracle requires.
- **R-4 (b1 MED-1): the retract driver contract is TOTAL and IDEMPOTENT** —
  strict SET semantics. Differential-message ingest folds are EXPLICIT folds
  (`AddExplicit`/`SubExplicit`, Database.cpp:2046-2053); `SubExplicit` of an
  absent or non-explicit row is a structural no-op and `AddExplicit` is
  idempotent (Table.h:355-378, verified). Retract-of-never-demanded and
  double-retract are SILENT NO-OPS (no counter goes negative, no [DBG] assert,
  no NDEBUG poisoning); b1 §0.6/§2.5/§8.2's contrary claims are struck, the
  docs rider documents the total surface, and no CHECKMEMBER pre-filter belt
  is owed. The `AddExplicit` kExplicit-bit idempotence is ALSO the stated
  second leg of SET-demand (b1 LOW-1): N probes of a standing key leave the
  explicit contribution at exactly 1, so ONE retract crosses 1→0 — b4's
  witness r1/r7 lines depend on precisely this.
- **R-5 (test packaging): `DeathFrontierTest.cpp` joins the EXISTING
  `rel_validators_test` binary** (tests/RelValidators/CMakeLists.txt:8 — one
  executable; verified). ctest stays **6/6 binaries** (b4's table stands; b2's
  "+1" is a new TEST, not a new binary). The fork/waitpid mold is the
  EINTR-SAFE GuardAnnotationFoldTest (tests/DataFlowValidators/…:73-84), NOT
  InstanceOrderTest's bare `(void) waitpid` (:72); ride-along: harden
  InstanceOrderTest.cpp:72 to the same loop (3-line hygiene fix, same file
  family, b2 owns it).
- **R-6 (b2 D-5): kNoInstance ⇒ silent SKIP in the death band, RATIFIED.**
  Unreachable today (netting + append-only iids); idempotence-preserving under
  D3.a.2 interleavings; divergence is loud via belt + eqgate. No always-on
  abort in that arm.

---

## §1 SUB-DIFF (i) — b1 ADOPTED, WITH AMENDMENTS

b1-design.md §1-§5 edit specs E1-E15 are ADOPTED (anchors corrected per
b1-critique COSM-1: E2's call is Main.cpp:68-69; the handler PARAMETER-order
exhibit is Procedure.cpp:552-561; the `_input`-alias suppression site is
Database.cpp:3367; the handler detail spelling is `<name>_<arity>_detail`).
Flag idiom verified at Main.cpp:467-480 (the `-demand-instance` implies-block
is the copy source). The retract entry's emission seam (before the `!has_free`
early return, Database.cpp:~1667) verified; note EmitQueryFriends is ALREADY
differential-aware for the query table (Find+Present existence arm,
:1673-1678) — the flat retract arm needs no query-surface work.

**A1.1 (R-4): §0.6/§2.5/§8.2 driver-contract text REPLACED** — the surface is
total + idempotent; retract of a non-standing key is a silent no-op
(SubExplicit structural no-op); the docs rider (CLAUDE.md demand section)
says so. b4's witness keeps `r7` (no-op retract) as the positive witness of
totality; no contract-violation probe exists to design.

**A1.2 (b1 LOW-1): the SET-semantics evidence chain in §3/§4 gains the
`AddExplicit` idempotence leg** — stated as load-bearing (the witness's common
path: every probe re-forces `+k` as its own epoch; N probes ⇒ explicit
contribution stays 1; one retract crosses 1→0 and mints the kNetRemovals
frontier row that is b2's death trigger).

**A1.3 (b1 LOW-2): the §6 flag-ON [STRUCT] header prediction gains two
program-wide shapes** (folded into §6 of this draft): (a) EVERY other message
handler's primary CALL doubles the empty vector for the now
deletion-capable demand IO (Procedure.cpp:600-607); (b) the primary
`flow_<id>` proc's parameter list gains the demand removal vector
(Procedure.cpp:28-37), changing its signature and every caller.

**A1.4 (R-2): b1 §6's "expected reds" restatement of the d1-only abort chain
is corrected** — first abort V-INST-EMITTED, not V-INST-DRAIN (the regime
split discharges the drain leg by construction).

Interface notes locked: `-demand-instance` does NOT imply `-demand-retract`;
the toggle reaches ONLY the witness via `.drflags` (all lanes' [BYTE] rows are
conditioned on this — LOCKED); the fabricated message's ABI suppression is
untouched (keys `IsDemandMessage`, never differentialness); no new dump-token
spelling from b1 (no E-71 note owed by this sub-diff).

---

## §2 SUB-DIFF (ii) — b2 ADOPTED, WITH AMENDMENTS

b2-design.md §0-§6 (D-1..D-7, E1-E13) ADOPTED under R-1/R-2/R-6, with
b2-critique F-7/F-8 cosmetics applied (E6's doc comment is Database.cpp:
2282-2289; E2's noexcept precedent is DemandFrontier/InputFrontier at include
Program.h:858/:861; §7.2's effect count reads "12 on the `effects:` line, 13
model effects incl. the kFlagRead that renders on `reads:`").

**A2.1 (b2-critique F-1, VERIFIED + EXTENDED): ClassifyVector's
kSubgraphInstance arm (lib/ControlFlow/Build/Procedure.cpp:196-205) is
extended for EVERY new region vector member** — the arm today classifies only
`demand_frontier`/`input_frontier` (verified); the new members would fall
through UNCLASSIFIED and survive only via the frontier-filter-append
written-by-primary coupling (producer relocation would silently repoint the
death drain to a fresh-empty local). New lines in the arm:

```cpp
        if (vec == si->removal_frontier.get()) {
          read.insert(vec);   // D3.a.1 band-(a0) death drain (read-only)
        }
        if (vec == si->del_queue.get() || vec == si->add_queue.get()) {
          written.insert(vec);  // D3.a.1 band-(b) signed publish appends
        }
```

(the written-set insert mirrors the kAppendToInductionVector arm's idiom; the
del/add-queue half is the adjudicator's EXTENSION of F-1 to b3's members —
same hazard, same fix, listed in b3's inventory as E4b but placed here with
b2's arm edit so the arm is touched once). The a2 demand-liveness gate reads a
TABLE, not a vector — no classification needed.

**A2.2 (b2-critique F-2): the L3 liveness checkpoint is RE-SCOPED** — the
"comment out one mint_filter call" perturbation aborts at the frontier-filter
census (`exp_filter`, Rel.cpp:3798-3810, expect :3959) long before the death
clause; observing it proves the census, not the wiring. L3 (stage-(d), scratch
worktree) becomes: temporarily make `CheckInstanceDeathFrontier` test a role
the flow never mints (e.g. `VecRole::kDeleteQueue` in place of `kNetRemoval`)
and compile the witness nested — the §3.2 abort must fire, proving the clause
RUNS inside ValidateDROps on the real flow; predicate correctness stays L1's
(the landed DeathFrontierTest unit). Revert.

**A2.3 (R-5 / b2-critique F-3): E12's mold is the EINTR-safe
GuardAnnotationFoldTest** (retry loop + `waited != pid` loud); DeathFrontierTest
lands as a TEST in the existing `rel_validators_test` binary; ride-along
hardening of InstanceOrderTest.cpp:72.

**A2.4 (b2-critique F-5): V-INST-DEATH-COHERENCE gains a liveness row** — d7
L5 (§6.4): scratch-perturb the mint (Rel.cpp:1144, set `death.demand_table =
input_table` or equivalent), compile the witness nested, observe the §1.2
abort, revert.

**A2.5 (b2-critique F-6): the stale-comment sweep ALSO rewrites the
function-head comment Procedure.cpp:260-264** ("Under R-MONO no kInstanceDeath
is minted (HP-17)" + the band list) to name the three-op protocol and the
death band, alongside the :341 line b2 already updates.

**A2.6 (orphan-mint fence, symmetric with b3 LOW-1):** the death wiring's
memoized fetch is preceded by an always-on existence check — the
`(demand, kNetRemovals)` entry must ALREADY be in `context.table_delta_vecs`
(minted by the step-1 EmitFrontierFilter lowering); on a miss, fprintf+abort
("orphan-mint fence: death drain vector not pre-minted") instead of letting
`TableDeltaVector` silently mint a drainless orphan (Build.cpp:747-770 mints
on miss). Same idiom at b3's queue fetches (§3.3).

Explicit non-edits re-affirmed: Rel.cpp:1139 death-mint gate; DeathEffects
(Rel.cpp:861-884 — the zero-counter signature is exactly what the band
emits); OD-2 sort / V-INST-ORDER / V-INST-PAIR / census `exp_death`;
Operation.cpp Hash/Equals (removal_frontier and the demand-table member are
pure functions of the Equals key via V-INST-SOLE + V-INST-PAIR);
Build.cpp:999's monotone-append gate.

E-71 notes owed by b2 (2): the `.rel` kInstanceDeath block's FIRST LIVE
PRODUCTION (no new spelling — every terminal already renders elsewhere;
verified against Format.cpp:883-893 + the live census `kInstanceDeath=0`
tokens), and the `.ir` `death <vec>` only-when-present token.

---

## §3 SUB-DIFF (iii) — b3 ADOPTED, WITH AMENDMENTS (incl. THE GATE)

b3-design.md §0-§5 (E1-E7) ADOPTED (COSM-1 applied: `Commit` is Table.h:546).
The `const bool diff = region.IsDifferential();` local is HOISTED to the top
of `EmitSubgraphInstance` (it now gates band-(a2) too, §3.2). Merge-order
note (from b3-critique): b2's band-(a0) inserts into the same function between
:2350 and :2352, so b3's ":2428-2433 verbatim" anchors shift textually at
merge — content unaffected.

### 3.1 Belt/drop-scan/selector — as designed

E5's drop scan (frz loop, `cur.Find(drow) == kNoRow` gate, SubDerivation +
DelQueue append, `++dropped`/`++carried`), the born-arm AddDerivation fork,
the V-INST-PARTITION always-on generated belt, E6's `diff-publish -> <delq> /
<addq>` render (E-71 note owed), and the zero-edit d5 flip are adopted
verbatim. The §0.8 inert-append residual (pub's claim drains lower in step 1,
before the band; the appends die at proc return; publish rides the commit
sweep, which reads the DiffTable's OWN touched set — SubDerivation/
AddDerivation Touch rows regardless) is RECORDED as the named F17-family
re-derivation obligation for any future in-flow consumer of pub; verified
correct by the b3 critic, and it is exactly why b4's transmit tap still
publishes both signs (EMPIRICAL-2's fix lands through the counters + commit
sweep, not through the inert queue appends).

### 3.2 THE BAND-(a2) DEMAND-LIVENESS GATE (E8, NEW — the R-3 fold)

**Region plumbing (E8a-E8d):**
- lib/ControlFlow/Program.h (impl, after `pub_table` :1177 area, beside the
  E3 queue members): `UseRef<TABLE> demand_table;  // D3.a.1 differential
  regime only: probed by the band-(a2) demand-liveness gate. NULL monotone.`
- include/drlojekyll/ControlFlow/Program.h (after `PubTable()` :865):
  `// Valid ONLY when IsDifferential(): the demand relation's table — the
  band-(a2) rebuild gate probes its Present membership (a dead key still
  binds an iid; demand presence is the ONLY correct liveness signal).`
  `DataTable DemandTable(void) const;`
- lib/ControlFlow/Program.cpp: impl beside InputTable/PubTable.
- lib/ControlFlow/Build/Procedure.cpp (inside b3's E4 `if (inst.differential)`
  block): `si->demand_table.Emplace(si, op->demand_table);`
  Hash/Equals untouched (same pure-function-of-store argument, recorded in
  the commit message).

**Emission (E8e, Database.cpp band-(a2), replacing the :2394-2398 gate in the
differential arm ONLY; monotone arm byte-identical):**

```cpp
  cc << cc.Indent() << "const auto iid = " << sname << ".FindInstance(Key_"
     << id << "{" << JoinExprs(ekeyexprs, ", ") << "});\n";
  if (diff) {
    // D3.a.1 DEMAND-LIVENESS gate: iid existence stopped implying liveness
    // when demand became retractable (append-only iids, no tombstone —
    // OD-15; TouchedFlag resets at Seal). Gate the rebuild on the demand
    // table's POST-COMMIT presence for the key (Present, Table.h:421-424).
    // Sound because the demand table is QUIESCENT in an input-edge epoch
    // (one entry call = one epoch; the edge handler never writes demand) —
    // Present == committed presence. Flat agrees by construction (the edge
    // joins against the absent demand row). D3.a.2 RIDER: a differential
    // input interleaving re-derives this quiescence argument.
    const auto demand_member = table_member[region.DemandTable().Id()];
    cc << cc.Indent() << "const auto dq = " << demand_member << ".Find({"
       << JoinExprs(ekeyexprs, ", ") << "});\n";
    cc << cc.Indent() << "if (iid != ::hyde::rt::kNoInstance && dq != "
       << "::hyde::rt::kNoRow && " << demand_member << ".Present(dq) && !"
       << sname << ".TouchedFlag(iid)) {\n";
  } else {
    // tip text verbatim (live-demanded gate + first-touch dedup).
    cc << cc.Indent() << "if (iid != ::hyde::rt::kNoInstance && !" << sname
       << ".TouchedFlag(iid)) {\n";
  }
```

**Model coverage (adjudicated — NO Rel.cpp effect change):** the instantiate
op's diff-independent effect set ALREADY declares the demand-table read —
`kInstanceDemand` "frozen read of the demand key, no hazard (HP-8)"
(Rel.cpp:800-803, verified). The model's granularity is per-(op, table,
kind), not per-site; the a2 probe is a second realization of the declared
read. InstantiateEffects, V-INST-EFFECT expectations, census, and DeathEffects
are all UNTOUCHED by the gate. Band-(a1) needs NO gate: its drain is the
demand kNetAdditions frontier, whose rows are presence GAINS by construction
(NetAdded, Table.h:457-461).

**Ordering/coupling facts:** the demand-column order == the instance-key
order (the a1 drain constructs `Key_<id>` from the demand table's own delta
tuples), so `Find({ekeyexprs...})` is the demand row in column order; the
brace-literal member-call idiom is the landed band-(b) precedent
(`neighborhood_4.TryAdd({key.c0, row.c0})`). In a RETRACT epoch the input
frontier is empty (per-epoch-fresh primary-proc locals), so a2 never runs for
the dying key in its death epoch — TouchedFlag suppression (the OD-15
coupling) remains the same-epoch belt; the gate is the CROSS-epoch belt.

### 3.3 Queue orphan-mint fence (b3 LOW-1 fold)

In E4, before each queue Emplace: always-on existence check that
`(pub, kDeleteQueue)` / `(pub, kAddQueue)` are ALREADY memoized in
`context.table_delta_vecs` (fprintf+abort "orphan-mint fence" on miss — a
differential pub skipped by the claim-drain mint would otherwise get
drainless orphan vectors + id churn, silently). Same idiom as A2.6.

### 3.4 Predictions re-baselined (b3-critique MED-1 fold)

b3 §3's witness generated-text predictions were derived from the TAP-LESS
nested regeneration; the merged witness carries b4's `nbhd_out` tap. BINDING
disposition: the b3 §3 BAND-BODY prediction (drop scan + born arm + belt
lines) and the one-line ctor prediction stand MODULO ids; every absolute id
(`table_8`, `vec25/29`, `flow_46`, `gd<N>`, census numerals) is re-anchored at
the stage-(d) **(d0) baseline step**: apply the tap edit ALONE on tip, compile
the witness `-demand -demand-instance` (+`-rel-out`/`-ir-out`), and record the
tap-ful pre-d1 baseline (header + `.rel` census + table ids). b2's §5.1 death
block and §7.2 census predictions are conditional on the SAME (d0) baseline
(their deltas — kClaimDrain +2/table, kFrontierFilter +2/table, kCommitSweep
+1, kIngestFold +1 net, kSubgraphInstantiate=1, kInstanceDeath=1,
kInstanceSeal=1 — are binding arithmetic; the absolutes re-anchor). The [BYTE]
non-witness row is 174 cases (b3 LOW-2 fixed; suite total 175 incl. the
witness, becoming 176 with negate_never_diff_1).

---

## §4 SUB-DIFF (iv) — b4 ADOPTED, WITH AMENDMENTS

b4-design.md §0-§6 ADOPTED (witness .dr tap, .drflags, .batches `+ add_edge
1 15`, driver phases, [COMPUTED] oracle/monotone goldens, DS-R4-10 move,
negate_never_diff_1, runall.sh edits, CLAUDE.md 176, InstanceStore unit,
gate plan), with:

**A4.1 (b4-critique MED-1, VERIFIED): the post-fixpoint DS-R4-10 loop gains
the dead-view filter.** `PrepareToDelete` on a negate CLEARS `negated_view`
(View.cpp:513-514, verified) and the check it replaces ran under ForEachView's
`is_dead` filter (Query.h:1254-1258, verified); the raw `negations` DefList
retains dead defs (RemoveUnused purges only zero-use defs; direct
PrepareToDelete calls exist, e.g. Compare.cpp lifted negates). The loop's
guard becomes:

```cpp
      if (negate->is_dead || !negate->negated_view || !negate->is_never ||
          !negate->negated_view->can_produce_deletions) {
        continue;
      }
```

**A4.2 (b4-critique LOW-2): the firing-profile claim is re-grounded** —
`TrackDifferentialUpdates` has FOUR call sites (Build.cpp:2555/:2618,
Connect.cpp:287, Optimize.cpp:797 per-CSE-round); the post-fixpoint block
placed before the `report_message_errors` gate runs at all four. With A4.1's
filter every extra run is a no-op on the log for accepted programs (the bits
are the same least fixpoint; `log.IsEmpty()` preserves stop-on-first-error).
This is the pre-registered execution profile stage-(c) verifies.

**A4.3 (b4-critique LOW-1): `send_expect_silent` is NORMATIVE for e7** — the
§1.4 listing's post-flush `assert` is dead code (flush clears rows). The
driver gets a `send_expect_silent` twin that, BEFORE flushing, does
`if (!log.rows.empty()) { std::cerr << "dead-key edge published\n";
std::abort(); }` — abort, not assert, so the teeth survive any NDEBUG driver
build. The golden's empty `e7:` line is unchanged.

**A4.4 (b4-critique MED-2): FINDINGS.md gains F25** — the demonstrated
DS-R4-10 mode-split latent accept (@never over a differential negated view
compiles under nodf/none at tip; repro = the never2 shape; fix = the
post-fixpoint move; witness = negate_never_diff_1). Added to the b4 edit
inventory; the F22 caught-pre-code precedent.

**A4.5 (R-2): liveness row L6 is amended** — the d1-only integration
checkpoint expects FIRST abort **V-INST-EMITTED** ("3 enrolled vs 2 emitted"),
not V-INST-DRAIN; there is no hand-provisioning second step (the drain legs
pass by regime split). Recording that abort IS the V-INST-EMITTED death-leg
liveness proof (b2's L2 — the two rows merge).

**A4.6 (b4-critique LOW-3 + COSM-1/2):** bless-ritual step 2 diffs
`<workroot>/demand_neighborhood_witness/demand_neighborhood_witness.<mode>/stdout`
(runall.sh:372 + diffrun.sh:61 nesting); the Rel.h anchor for kInstanceDeath
is split (VecRole kNetRemoval at Rel.h:54-69; DROpKind::kInstanceDeath
separately, live use Rel.cpp:1138-1146); the InstanceStore test is renamed
`DeathRebirthCycleRebindsIidAndTogglesTouchedFlag` (it pins TouchedFlag
observability + same-iid rebind + second-death cycling, not rescan
suppression).

The witness golden prediction (§1.5, 35 lines) STANDS under the merged design
— its e7/p1c/r1b lines are exactly what the R-3 gate + rebirth-fresh-rescan
+ (T,F)-against-rebuilt-frozen semantics produce, and its r1/r7 lines are what
R-4's total-idempotent surface produces. The retract entry spelling
`neighborhood_bf_retract(db, log, functors, start)` matches b1's generated
surface exactly (interface CLOSED).

---

## §5 CROSS-SLICE RECONCILIATION (every declared sibling expectation, resolved)

| # | expectation | holder → provider | resolution |
|---|---|---|---|
| X1 | toggle spelling/scope: per-case `-demand-retract`, implies `-demand`, NOT implied by `-demand-instance` | b2/b3/b4 → b1 | MATCHED — all four lanes agree; every [BYTE] row conditioned on it, LOCKED |
| X2 | retract entry `<name>_<pattern>_retract(db, log, functors, bound...)` void, both lowerings, runs the flow in-call, del_vec-only | b4 → b1 | MATCHED — b1 §2.2/§2.4 == b4 I-1 verbatim |
| X3 | demand kNetAdditions re-provisioning + kNetRemovals provision by ValidateDROps time | b1 → b2 | RESOLVED via b2 D-3: NO ControlFlow pre-provisioning; DR-side sextet + ± filters exist pre-validator; V-INST-DRAIN regime-split checks the real feeder per regime (b1 §7's phrasing updated, no code implication) |
| X4 | death before band-(a1), Recycle-only, NO pub retract by the death band | b3 → b2 | MATCHED — R-1 band-(a0) inside the region; the drop scan is the sole −1 publisher (DeathEffects zero-counter signature enforces it) |
| X5 | drop scan handles empty-current dead key as full drop; belt accepts (0, 0, frz) | b2 → b3 | MATCHED — b3 §0.4/§3 death row |
| X6 | pub Del/AddQueue fetched memoized post-validator | b2 → b3 | MATCHED + fenced (A2.6/§3.3 orphan-mint checks) |
| X7 | G-12 `IsDifferential()` accessor ownership | b2 ↔ b3 | b3 owns it (E1/E2); b2 keys the death band on OP PRESENCE (D-2), never the bit — d2 ruling honored on both sides |
| X8 | band-(a2) demand-liveness gate | b4 → b3 (b3 lacked it) | **FOLDED as §3.2 (R-3)** — the one genuine interface MISMATCH found; resolved at code |
| X9 | band-(b) must feed pub's differential machinery so the transmit publishes both signs | b4 → b3 | MATCHED — b3 E5 is exactly the EMPIRICAL-2 fix (AddDerivation/SubDerivation + counters + commit sweep) |
| X10 | witness gets the `@differential` tap; G-15 discharged; predictions re-baselined | b3 → b4 / b4 → b3 | RESOLVED — tap is IN (load-bearing observable); b3/b2 id-level predictions re-anchor at the (d0) tap-ful baseline (§3.4) |
| X11 | NO new DROp kind (11 `.rel` census pins) | b4 → b2 | MATCHED — b2 mints no kind; kInstanceDeath/kNetRemoval exist; pins keep `kInstanceDeath=0` |
| X12 | demand_tc_witness stays bare `-demand`, fully [BYTE] | b2/b3 → b4 | MATCHED — b4 touches only the neighborhood witness's sidecars |
| X13 | d1-only abort chain wording | b1/b4 (V-INST-DRAIN first) vs b2 (V-INST-EMITTED first) | RESOLVED per R-2: V-INST-EMITTED first under the merged design; L2/L6 merged |
| X14 | ctest count | b2 (+1) vs b4 (6/6) | RESOLVED per R-5: 6/6 binaries, +2 TESTs (DeathFrontierTest in rel_validators_test; the renamed InstanceStore arm) |
| X15 | retract-only-standing contract | b1 → b4 | SUPERSEDED by R-4 (total + idempotent); witness r7 is the totality witness |

The d2 ruling is honored slice-wide: Rel.cpp:1055/:1059 (P-STORE) and :1139
(P-DEATH) are untouched, no shared helper is minted, the emitter keys the
death band on op presence and the drop scan/belt on the region bit —
co-activation makes them agree THIS slice; D3.a.2 splits them by design.

**Stage-(d) protocol additions (binding):** (d0) the tap-ful pre-d1 baseline
regeneration (§3.4) BEFORE any code lands; (d1..) the lane prototypes under
the blind-worktree convergence ritual as in D3.a.0; the L2/L3/L5-L8
perturbations run in the prototype worktree and their observed abort texts are
recorded in the ledger entry; the un-perturbed suite run is every belt's green
half.

---

## §6 GATE FAMILY + PRE-REGISTERED PREDICTIONS

### 6.1 Per-surface predictions

| surface | prediction |
|---|---|
| 174 non-witness case stdouts × 4 modes | **[BYTE]** — no other case carries the flag; the fence move changes no accepted program (perimeter: only negate_6/map_5 carry `@never`, both monotone; data/ has none) |
| 20 pinned dump surfaces (demand_tc_witness ×4 dumps, 11 `.rel` pins, 14 `.irgold`, `.df`/`.h` regen set) | **[BYTE]** — flag-off no id mints, no op mints, census `kInstanceDeath=0` everywhere pinned; V-INST-DRAIN monotone legs keep their exact abort strings |
| diagnostic verdict lines (existing 14 + kvindex_1 split + 3 demand fences) | **[BYTE]**; `negate_never_diff_1` ADDS one all-4-modes-diagnostic green line |
| data/ corpus (36 × 4) | **[BYTE]** |
| witness generated header, FLAT arm | **[STRUCT]** — demand handler gains 2nd Vec param + one `::hyde::rt::NetBatch` line; demand-closure tables flip `Table<>`→`DiffTable<>`; two-polarity demand ingest folds + claim/commit tails; ONE new `inject_<id>` + ONE new hidden friend `<name>_bf_retract` (void); EVERY other handler's primary CALL doubles the empty vec for the demand IO; `flow_<id>` signature gains the removal vec; `nbhd_out` transmit both signs |
| witness generated header, NESTED arm | **[STRUCT]** — all of the above PLUS `instance_0(allocator_, false)` ctor; band-(a0) death drain (FindInstance + RecycleCurrent, kNoInstance skip); the a2 gate's `Find`/`Present` conjuncts; the (T,F) drop scan + AddDerivation born arm + V-INST-PARTITION belt; del/add queue appends |
| witness `.rel` (unpinned, eyeball + E-71) | **[STRUCT]** — census gains `kInstanceDeath=1`; the death block renders (sign=- ctx=seed, 4 effects, demand/pub/store args) BEFORE its instantiate; instantiate effects flip to the 13-effect diff arm (12 on `effects:`); absolutes re-anchored at (d0) |
| witness `.ir` (unpinned) | **[STRUCT]** — `death <vec>` token + `diff-publish -> <delq> / <addq>` before `seal` |
| goldens churn | **EXACTLY 3 files**, all witness-owned (stdout replacement predicted verbatim in b4 §1.5; oracle/monotone [COMPUTED] byte-exact), via `--bless` after the ritual |
| ctest | **6/6 binaries** (2 new TESTs in existing binaries), debug + ASAN |
| E-71 grammar notes | **3**: `.rel` kInstanceDeath first-production (b2), `.ir` `death` token (b2), `.ir` `diff-publish` token (b3) |

### 6.2 Expected pre-bless red set (EXACT — any deviation = stop, never bless)

The b4 §4.2 10-line set verbatim: `demand_neighborhood_witness` ×
{opt,nodf,nocf,none} GOLDEN-DIVERGE, oracle GOLDEN-DIVERGE, monotone
MONO-DIVERGE, eqgate ×4 NESTED-GOLDEN-DIVERGE. All four mode stdouts must be
byte-identical to EACH OTHER and to the b4 §1.5 prediction; all four eqgate
stdouts must equal the flat stdout (flat==nested passing EARLY).
`negate_never_diff_1` green on the same run. Post-bless: SUITE PASS(176),
zero residual reds.

### 6.3 Gate roll-call

SUITE PASS(176) ×4 modes debug (+ release), error-grep 0 across trees; ASAN:
full suite + ctest + eqgate (death path = use-after-free terrain, hard gate,
2 sweeps); eqgate LIVE ×4 refereeing answer + sorted delta-stream identity;
20/20 pinned regen [BYTE]; config-invariance 3-run single-hash on
demand_tc_witness + BOTH witness arms (header + `.rel`/`.ir`); Q5 MUST RUN
(ABABAB progsize@128, release, baseline = tip snapshot; >2% = stop — the
D3.a.0 byte-identity waiver is void); E-62 clean; permcheck N/A (driver-sorted
flushes; no published-delta order change on any pinned case).

### 6.4 d7 liveness-by-perturbation table (consolidated; run at stage (d), abort texts recorded, all reverted)

| # | belt | procedure | expected |
|---|---|---|---|
| L1 | V-INST-DRAIN death clause | LANDED unit: DeathFrontierTest (EINTR-safe fork/waitpid, both arms) | SIGABRT on the missing-vec/missing-producer flow; exit 0 positive |
| L2 | V-INST-EMITTED death leg + XC-3 chain | d1-only build (toggle without d3/d4) in the prototype worktree; compile witness nested | FIRST abort V-INST-EMITTED "3 vs 2" (R-2) — no V-INST-DRAIN abort |
| L3 | death-clause wiring | scratch: make CheckInstanceDeathFrontier test a never-minted role; compile witness nested | the clause's abort fires from ValidateDROps (A2.2) |
| L4 | end-to-end kill/rebirth | witness r1/e7/p1c/r1b phases — LANDED, every suite run | golden + eqgate + e7 abort teeth |
| L5 | V-INST-DEATH-COHERENCE | scratch: mint the death with the wrong demand table | the §1.2(b2) abort (A2.4) |
| L6 | V-INST-DIFF-COHERENCE both polarities | b3 §6.1 stamp perturbations, TRUE-bit carrier now exists | abort both directions |
| L7 | V-INST-PARTITION belt | b3 §6.2: drop `++dropped;` then `++born;` in the emitter; run witness retract batches | generated abort first death epoch / first birth epoch |
| L8 | a2 demand-liveness gate | scratch: drop the `Present(dq)` conjunct from E8e; rebuild; run witness | e7 goes triply red (driver abort + eqgate + golden) — then the landed assert stays the permanent teeth |
| L9 | HP-7 disarm sanity (negative) | debug witness run post-`, false` | death-epoch shrink does NOT trip the monotone Seal belt; DebugValidate green |
| L10 | OWN-3 fold abort | re-run GuardAnnotationFoldTest ctest on the slice tree (both flavors) | unchanged SIGABRT + record print |

---

## §7 ADJUDICATION RECORD (finding → severity → disposition → verified-at)

| finding | sev | disposition | verified at |
|---|---|---|---|
| b3 HIGH-1 zombie rebirth (a2 gate) | HIGH | CONFIRMED → FOLDED §3.2 (R-3) | Database.cpp:2394-2398 (no liveness conjunct); InstanceStore.h append-only iids + flag reset at Seal; Table.h:421-424 Present; d3a-substrate.md:734-740 G-REBIRTH; b4-design:665-671 I-3 |
| b4 MED-1 is_dead null-deref | MED | CONFIRMED → FOLDED A4.1 | View.cpp:513-514 `negated_view.Clear()`; Query.h:1254-1258 ForEachView filter |
| b1 MED-1 wrong failure-mode contract | MED | CONFIRMED → FOLDED R-4/A1.1 | Table.h:355-378 Add/SubExplicit; Database.cpp:2046-2053 explicit-fold emission |
| b2 F-1 ClassifyVector arm | MED | CONFIRMED → FOLDED A2.1 + EXTENDED to del/add queues | Procedure.cpp:196-205 (only demand/input frontiers classified) |
| b2 F-2 L3 unobservable | MED | CONFIRMED → RESCOPED A2.2 | Rel.cpp:3798-3810/:3959 census precedes :4496+ |
| b3 MED-1 stale prediction baseline (tap) | MED | CONFIRMED → RE-BASELINED §3.4 (d0) | b4 §1.1 tap is load-bearing; b3 §1/§3 derived tap-less |
| b4 MED-2 missing FINDINGS.md F25 | MED | CONFIRMED → FOLDED A4.4 | FINDINGS.md F22 precedent |
| b2 F-3 EINTR mold | LOW-MED | CONFIRMED → FOLDED R-5/A2.3 (+ InstanceOrderTest hardening) | InstanceOrderTest.cpp:72 bare waitpid vs GuardAnnotationFoldTest.cpp:73-84 loop (both re-read) |
| b1 LOW-1 AddExplicit leg unstated | LOW | CONFIRMED → FOLDED A1.2 | Table.h:356-368 |
| b1 LOW-2 [STRUCT] gaps | LOW | CONFIRMED → FOLDED A1.3/§6.1 | Procedure.cpp:600-607/:28-37 |
| b2 F-4 substrate re-reading | LOW | CONFIRMED-as-ambiguity → RATIFIED R-1 | d3a1-substrate.md:387-389 wording; seal precedent Procedure.cpp:341-345/Database.cpp:2461 |
| b2 F-5 coherence-validator vacuity | LOW | CONFIRMED → FOLDED A2.4 (L5) | no perturbation existed in any lane's d7 plan |
| b2 F-6 stale head comment | LOW | CONFIRMED → FOLDED A2.5 | Procedure.cpp:260-264 read |
| b3 LOW-1 orphan-mint memoization | LOW | CONFIRMED → FOLDED §3.3 + A2.6 | Build.cpp:747-770 mints on miss; Rel.cpp:2541 induction-skip example |
| b3 LOW-2 174 vs 175 | LOW | CONFIRMED → FOLDED §3.4 | `ls cases/*.dr | wc -l` = 175 incl. witness |
| b4 LOW-1 e7 assert dead code | LOW | CONFIRMED → FOLDED A4.3 (abort, NDEBUG-surviving) | flush clears rows in the §1.4 listing |
| b4 LOW-2 four call sites | LOW | CONFIRMED → RE-GROUNDED A4.2 | Build.cpp:2555/:2618, Connect.cpp:287, Optimize.cpp:797 |
| b4 LOW-3 bless path | LOW | CONFIRMED → FOLDED A4.6 | runall.sh:372 + diffrun.sh:61 nesting |
| b1 COSM-1 / b2 F-7,F-8 / b3 COSM-1 / b4 COSM-1,COSM-2 | COSM | CONFIRMED → anchor/text fixes applied in §1/§2/§3/§4 | spot-checked; consistent with adjudicator reads |

Tally: **20 lane findings — 20 CONFIRMED (0 refuted), all folded/ratified/
rescoped; 1 adjudicator extension (A2.1's queue classification); 0 escalated.**

---

## §8 OWNER-ESCALATION

**NONE.** No OD-15 or d2-ruling conflict survived adjudication:
- The a2 demand-liveness gate (R-3) operates WITHIN OD-15 (no iid tombstone
  is introduced; demand presence is probed, not stored).
- The XC-3 chain reorder (R-2) preserves the ruling's verdict (one-commit
  co-landability) and only corrects a counterfactual's abort order.
- Band-(a0)-inside-region (R-1) satisfies the substrate's epoch-position
  requirement by construction.
- The predicates stay unfolded; P-STORE/P-DEATH sites untouched; the
  three-way OD-15 coupling is stated and pinned in every affected section.

