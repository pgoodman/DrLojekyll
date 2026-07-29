# D3.a.1 — STAGE (a) WHOLE-PROGRAM SUBSTRATE (COMMITTED; ledger §20(AI))

> **House banner.** Produced by the 2026-07-28 stage-(a) fleet: 3 seed-UNREAD
> opus derivation lanes (retract channel / instance runtime+band /
> frontiers+netting) + fleet1's verified death-substrate map as a fourth lane +
> 1 xhigh seed-READ consolidator (~513k tokens, zero deaths). The ORCHESTRATOR
> personally re-verified all ten load-bearing anchor families at code before
> commit (incl. both halves of XC-3 and the §6 co-activation chain). The d2/L3
> ritual-head adjudication is §7 at the tail. AS-LANDED authority for D3.a.1
> stages (b)-(f); pseudocode idiom per d3a-substrate.md §7.

Tip `a87aad5f` (verified `git rev-parse HEAD`, tree clean). AS-LANDED ONLY — no design.
Legend: **[ALWAYS-ON]** = fprintf(+record)+abort, survives NDEBUG. **[DBG]** = `assert` /
`#ifndef NDEBUG`, compiled out. **DORMANT** = present but unreachable on tip behavior
(grounds stated); **LIVE** = exercised on the `-demand-instance` corpus today.

Inputs consolidated: laneA-retract.md, laneB-runtime.md, laneC-netting.md (all tip
a87aad5f), fleet1/lane3-death.md (parent tip 0d33bdca — every load-bearing anchor
re-checked at a87aad5f; the docs-only errata commit between was line-count-preserving in
Rel.cpp per §20(AH), and no drift was found). Binding context honored, not re-litigated:
d3a-ruling-brief.md (OD-15), d3a-substrate.md §7:888-1001 (d1-d7), d3a0-design.md §3
(incl. the L3 rider), KeyedInstances.md §20(AH). Orchestrator-re-verified anchors +
XC adjudications at the tail. Idiom: §7-style compact pseudocode, anchors inline.

===============================================================================
## §1 THE RETRACT CHANNEL AS LANDED
===============================================================================

### 1.1 Fabrication (lib/Parse/Demand.cpp) — the d1 toggle field

```
FabricateDemandMessage(name, param_types):                 # Demand.cpp:163-204
  LexInternedAtom -> real interned kIdentifierAtom          # :51-100 via :170-172
  collision scan (reserved demand__ prefix; user collision
    -> nullopt, clean-diagnostic reject)                    # :179-185
  message = declarations.CreateDerived<ParsedMessageImpl>(
      kMessage); messages.AddUse(message)                   # :189-192
  message->name / name_view / directive_pos / rparen        # :194-197
  FabricateParams(module, message, param_types)             # :199 -> :106-126
  # NEVER SET: message->differential_attribute (Parse.h:388)
  #   -> ParsedMessage::IsDifferential() == differential_attribute.IsValid()
  #      (Parse.cpp:1336-1338) is FALSE for every fabricated demand message.
FabricateDemandLocal: same shape, ParsedLocalImpl           # :206-241
```
`differential_attribute` is set ONLY at parse time on `@differential`
(lib/Parse/Message.cpp:176-189; redecl consistency Parser.cpp:1394-1410 [laneA]).
Construction-time SELECT `can_*` seeds (Select.cpp:26-30/39-43) are irrelevant —
`TrackDifferentialUpdates` resets + re-derives every run (§1.4). **The ONE d1 toggle is
this field**; everything in §1.2-§1.5 and §2 keys off it mechanically.

### 1.2 The demand seed injector (lib/ControlFlow/Build/Build.cpp) — the del_vec arm

Two builders, identical vector shape: `BuildQueryForceProcedureImpl` (`@first`,
:249-371) and `BuildQueryForceProcedureFromRegistry` (demand, :385-447); dispatch by
per-adornment registry identity in `BuildQueryForceProcedure` (:452-483 [laneA]).

```
BuildQueryForceProcedureFromRegistry(impl, ctx, query, entry):   # :385
  proc = Create(kQueryMessageInjector); proc->has_raw_use        # :396-398
  one kParameter input var per entry.bound_params                # :401-405
  col_types from the fabricated message's own params             # :408-411
  add_vec = vectors.Create(kParameter, col_types, 0)             # :413-415
  del_vec = nullptr                                              # :416
  if message.IsDifferential():                                   # :418  <- FALSE today; arm DORMANT
    del_vec = vectors.Create(kEmpty, col_types, 0)               # :419-420
  VECTORAPPEND(add_vec <- proc->input_vars)                      # :426-432  THE ONLY VECTORAPPEND
  CALL messsage_handler[message](add_vec [, del_vec /*Empty.*/]) # :434-440
  RETURN kReturnTrueFromProcedure                                # :442-444
```
EXACTLY what the del_vec arm does / does not wire (verified at code):
- Created ONLY under `message.IsDifferential()` (:418), kind `kEmpty` (:419-420);
  passed as CALL `arg_vecs[1]` with the literal `// Empty.` comment (:438-440).
- **NEVER a VECTORAPPEND target** — the proc's sole append targets `add_vec` (:429).
  Same in the `@first` builder (:340-345 create, :362-364 pass empty [laneA]). So even
  with the message differential, the forcer passes an EMPTY remove vector: forcing is
  add-only by construction. **Retracting a standing demand has NO writer at tip** — the
  retract DATA channel is plumbed end-to-end (injector -> handler param -> NETBATCH ->
  two-polarity ingest) but structurally empty from the forcer side. (OD-15
  OQ-RETRACT-POLICY names this channel; stage (b) owns giving it a writer.)
- Registry: `QueryDemandForcing` built at Demand.cpp:1127-1128; handler map
  `messsage_handler.emplace` Procedure.cpp:550; forcer stored on the query entry point
  (Build.cpp:504-514) and invoked by the generated `q_bf(db, bound...)` [laneA].

### 1.3 The handler + batch SET netting

```
BuildIOProcedure(io):                                       # Procedure.cpp:534-609
  io_vec        = param vec (adds)                          # :552-554
  io_remove_vec = param vec IFF message.IsDifferential()    # :557-561
  if io_remove_vec != nullptr:                              # :570 (comment :566-569)
    NETBATCH(add=io_vec, remove=io_remove_vec)              # :571-575
  CALL primary(io_vec [, io_remove_vec], empties for other
    receives, doubled iff CanReceiveDeletions)              # :577-608 [laneA]

hyde::rt::NetBatch(adds, removes):                          # Runtime/Vec.h:176-218
  distinct-scan: flags bit0=in adds, bit1=in removes        # :181-198
  rewrite: flags==1 -> adds; ==2 -> removes;
           ==3 -> ANNIHILATED (dropped from both)           # :206-217
  # SET-with-annihilation, first-appearance order: {+x,-x,-x} is a NO-OP
  # (the OQ3 semantics OD-15 OQ-RETRACT-POLICY adopts verbatim)
EmitNetBatch -> "::hyde::rt::NetBatch(...)"                 # Database.cpp:2764-2767 [laneA]
```
Netting is a REGION in the message handler, once per received batch, BEFORE the flow —
never a counter/commit-sweep step. Today NO netting engages on the demand path (message
monotone => no io_remove_vec, no NETBATCH). If differential: the injector's vectors flow
through the SAME handler NETBATCH — add_vec deduped against an always-empty del_vec,
annihilating nothing. The OD-15 same-batch death+re-demand impossibility rests on THIS
netting once a real remove-side writer exists.

### 1.4 The flat-lowering differential-propagation derivation (the L3 oracle side)

```
QueryImpl::TrackDifferentialUpdates(log, report):           # Differential.cpp:40-181
  RESET: every view can_receive = can_produce = false       # :50-54
  SEED1: SELECT over an IO whose ParsedMessage
         .IsDifferential() -> can_receive=can_produce=true  # :56-65 (the demand toggle's entry)
  SEED2: every AGG/KV can_produce = true                    # :71-76 [laneA]
  FIXPOINT until no change:                                 # :78-142 [laneA]
    NEGATE arm (@never lifts only if receiving; ordinary
      negate always produces; @never-over-differential
      negated view -> ERROR)                                # :81-111
    can_receive -> can_produce lift                         # :114-117
    INSERT->SELECT seam: producing INSERT marks its
      seam SELECTs can_receive                              # :119-127
    COLUMN-EDGE forward: producing view marks every
      column USER can_receive                               # :133-140
  authoritative post-demand run: lib/DataFlow/Build.cpp:2618
  (report_message_errors=true; early pre-demand run :2555)  # verified
```
Closure walk with ONLY the demand message differential (flat guard web; laneA's
exhaustive walk, spot-verified): `recv` SELECT seeds -> col-edge+lift chain through
`root_head`/`raw_seed` -> `root_member`/`prop_member` -> `d_merge` -> `d_reader` ->
every guard JOIN (a column-user of the demand side, MintGuardJoin Demand.cpp:162-208)
-> the guarded body views (guard outputs REPLACE the p-read, RewireConsumer) -> `p`'s
MERGE + INSERT -> the INSERT->SELECT seam + col-edges -> **everything transitively
downstream of `p`**. Blockers checked, none apply on the slice: NEGATE/AGG in a demanded
body are pre-rejected by the demand pass (Demand.cpp:624-627, verified); pure MAPs
forward fine (Map.cpp:18 [laneA]).

**Consequence:** `TableIsDifferential(pub)` (Build.cpp:705-721, verified: any member
view `CanProduceDeletions()` :706-709, or agg/KV :715-718) becomes TRUE for the demanded
pub — the flip is the WHOLE demanded closure, NOT localizable to the demand relation.
Machinery that then engages per flipped table (the switch is `TableIsDifferential`; no
NEW machinery): phase-table census `any_phase_table` (Stratum.cpp:2345-2351, verified)
+ drain strata (:2427-2431, verified); DiffTable flavor + membership predicates + claim
gates + commit sweep (§4.2); two-polarity ingest folds — `receive.CanReceiveDeletions()`
selects `MakeStageOneIngestFolds` over the monotone fold (Procedure.cpp:50-59,
verified); negate/source flavor flips (Negate.cpp:58, Stratum.cpp:1492 [laneA]).

### 1.5 No plain-`-demand` fence blocks it; one incidental reject

- The demand pass rejects (multi-bound/multi-adorn/multi-clause/self-join/NEGATE-AGG/
  stray consumers, Demand.cpp:434-789 [laneA]) NEVER test message differentialness.
- The three nested fences are gated `if (demand_instance)` (Build.cpp:1393, verified);
  FENCE (iii) diff-INPUT keys `in.CanReceiveDeletions()` on the BODY input jl[1]
  (:1418-1421, verified) — a differential DEMAND does not make the summarized input
  deletable (propagation is forward along column edges; the input's views stay
  monotone), so it does NOT fire. DORMANT under plain `-demand` entirely.
- The one incidental surface: message-faithfulness — "can produce deletions but is not
  marked @differential" HARD ERROR for a PUBLISHED message reached by the flipped
  closure (Differential.cpp:174-179, verified; the reverse @differential-but-cannot
  branch is commented out :164-172, verified). The demand message itself is
  receive-only, never the subject. Constraint on d6 witness design: any published
  output over the demanded closure must be `@differential`.

===============================================================================
## §2 THE DEATH PATH AS LANDED (lane3 folded, re-anchored at a87aad5f)
===============================================================================

### 2.1 The mint gate + the death op

```
BuildSubgraphInstanceOps (per recognized+live forcing):     # Rel.cpp:1019ff
  diff = TableIsDifferential(pub_table)                     # :1055  (verified)
  inst_desc.differential = diff   # ONE spelling, shared
                                  # with InstantiateEffects # :1059  (verified)
  ... push kSubgraphInstantiate (table_op_sign=+1 [laneC :1091])
  if demand_table && TableIsDifferential(demand_table):     # :1139  DORMANT (verified;
    death = DROp(kInstanceDeath)                            #   demand msg monotone §1.1)
      .ctx=kSeed; .table_op_table=pub_table; .sign=-1       # :1141-1143 (OD-2 comment)
      .demand_table; .demanded_view; .instance_store_id=sid
      .forcing_index                                        # :1144-1147
      .effects = DeathEffects(pub_table, demand_table)      # :1148
    flow.ops.push_back(death)                               # :1149
  ... kInstanceSeal always (self-lowered)                   # :1152ff
```
TWO INDEPENDENT DIFFERENTIAL AXES at the mint (lane3's headline, re-verified): the
death gate keys **demand_table** (:1139); the store/region/descriptor bit + the
InstantiateEffects fork key **pub_table** (:1055/:1059). Their reconciliation is d2 —
the fact base is §6.

```
DeathEffects(pub, demand):                                  # Rel.cpp:861-884 (verified)
  kVecDrain   {value_table=demand, vec_role=kNetRemoval}    # :865-869  <- ASSUMED frontier,
  kInstanceDemand {read_table=demand}                       # :870-873     nothing provisions (§4.1)
  kStateOld   {read_table=pub}                              # :874-877
  kInstanceRebuild {value_table=pub, sign=-1}               # :878-882
  # zero-counter death signature (comment :862-863): EXACTLY ZERO
  # {kStateEmit, kCounter, kInIReadFrozen, kVecAppend}
```

### 2.2 Validator obligations on a minted death (all [ALWAYS-ON] ValidatorFail/abort)

- **V-INST-EFFECT death arm** (Rel.cpp:4346-4368 [laneC]): drains==1 && demands==1 &&
  olds==1 && rebuilds==1 && rebuild_sign==-1; ANY emit/counter/crossing/append =>
  abort. The death drain is counted but NOT source-checked for
  {demand, kNetRemoval} (the instantiate arm IS source-aware, :4287-4300 [laneC]) —
  a d3 strengthening opportunity.
- **V-INST-SOLE** (:4335-4340, verified): fires on a differential/aliasing INPUT, never
  on a differential demand; sole-instantiate-deriver-per-pub (:4384-4388 [laneC]).
- **V-INST-PAIR** (:4390-4414 [laneC]): per store {inst,seal} or {death,inst,seal},
  n_death<=1.
- **OD-2 sign sort** (verified): kInstanceDeath/kSubgraphInstantiate fall through to
  the default phase key (Rel.cpp:5104-5110 comment + return), share
  `table_op_table=pub_table` => equal table_id => `key_less` reaches the sign compare
  "− before +" (:5111-5118, the sign line :5117) => death sorts strictly BEFORE its
  instantiate. Guarantor: **CheckInstanceOrder / V-INST-ORDER** (Rel.cpp:4754-4785,
  verified; grouped by `instance_store_id`, NEVER table_id/forcing_index — HP-3;
  pinned_order positions with op-index fallback; runs unconditionally on every flow,
  call at Rel.cpp:5652 [laneC]). DROpStratum loud-fails on a missing
  `instance_stratum` entry (Rel.cpp:4738-4752 [laneC]).
- **V-INST-DRAIN** (Rel.cpp:4494-4524, verified): per INSTANTIATE only, checks
  `context.table_delta_vecs[demand][kNetAdditions]` and `[input][kNetAdditions]`
  non-null. NO removals clause. Runs inside `ValidateDROps` (fn head Rel.cpp:3334)
  called at **Stratum.cpp:2186** — see the XC-3 timing consequence below.
- **V-INST-EMITTED** (verified): `LowerSubgraphInstances` iterates
  `dr_flow.SubgraphInstances()` — kSubgraphInstantiate ONLY (Rel.cpp:585-593) — and
  enrolls exactly `{sid, kSubgraphInstantiate}` + `{sid, kInstanceSeal}`
  (Procedure.cpp:341-345; comment "Death is not minted under R-MONO"). The check
  (Procedure.cpp:435-464) builds `enrolled` over flow ops of kinds
  {kSubgraphInstantiate, **kInstanceDeath**, kInstanceSeal} (:446-450) and
  multiset-compares sorted emitted vs enrolled => `std::abort()` (:455-463).

### 2.3 The abort chain under a naive d1 flip — [XC-3], corrected at code

Lane3's counterfactual derivation (steps 1-8) holds EXCEPT step 6's "V-INST-DRAIN =>
PASS". At tip, the ONLY pre-validator provisioner of a demand-table kNetAdditions
VECTOR is the eager boundary append (Build.cpp:997-1004, verified), and it is gated
`!TableIsDifferential(table)` (:999). ControlFlow kNetAdditions provisioner census
(verified grep, lib/ControlFlow/Build): Build.cpp:1003 (eager, monotone-only);
Induction.cpp:606 (induction vectors); Stratum.cpp:750/1150/1271/1385/1499 — all
inside LowerDRFlow/LowerDRRounds, which run at Stratum.cpp:2477/:2489, AFTER
ValidateDROps at :2186; Procedure.cpp:289/294 (LowerSubgraphInstances, later still).
So with the demand table differential:

1. the monotone boundary append STOPS firing for the demand table (:999 gate), and
2. no differential-machinery TableDeltaVector call has run yet at ValidateDROps time,

=> **the FIRST abort is V-INST-DRAIN** ("demand net-additions frontier was never
provisioned", Rel.cpp:4509-4512), BEFORE the V-INST-EMITTED abort lane3 identified
(which becomes the SECOND abort once provisioning is fixed and the death is still
unlowered). Consequence for stage (b): d1 and d3 are NOT independently landable —
flipping the message differential without re-provisioning the demand kNetAdditions
frontier (route: the differential frontier machinery, §4.1) aborts every
`-demand-instance` compile at V-INST-DRAIN. Everything else in lane3's derivation
re-verified: the death passes V-INST-EFFECT/SOLE/PAIR/ORDER and the census
(exp_death gate Rel.cpp:3989-3991 mirrors the mint gate [lane3]); the death's
kNetRemoval drain resolves `~0u` => silently NO dep edge (ResolveVecIdx, Rel.cpp:
4638-4655 [laneC]; the :794-mold fallback) — the missing frontier is a silent
no-edge at the linearizer, never a graph-validator catch.

### 2.4 RecycleCurrent — the runtime death primitive

```
void RecycleCurrent(uint32_t iid):                          # InstanceStore.h:216-219 (verified)
  Touch(iid)          # append-once => Seal visits; TouchedFlag set (suppression, §4.3)
  current[iid]->Reset()
# UNCONDITIONAL + IDEMPOTENT (comment :210-215): twice in one epoch == once.
```
ZERO codegen callers (DORMANT; grep — def + tests/InstanceStore only). The (T,F) drop
scan is ABSENT from EmitSubgraphInstance (XC-1 standing; §3.2). The death-half runtime
contract is unit-pinned: `DeathHalfRecycleThenPartialReaddDropsRows`
(InstanceStoreTest.cpp:317-349, monotone=false [laneB]) — RecycleCurrent + partial
re-add => genuine (T,F) drop set; comment :334 states the band-(b) publish contract
`dropped = frozen∖current, born = {}`; epoch-3 full death => SealedOccupied false.

### 2.5 The lane3 M1-M6 checklist, re-affirmed at tip (discharge mapping in §5)

M1 death lowering + `{sid, kInstanceDeath}` enrollment (closes V-INST-EMITTED).
M2 EmitInstanceDeath: drain demand kNetRemovals -> FindInstance -> RecycleCurrent
   (its FIRST codegen caller); plus the band-(b) (T,F) drop scan retracting into pub.
M3 provision the demand kNetRemovals frontier + extend V-INST-DRAIN — WIDENED by
   XC-3: the demand kNetAdditions provisioning route must ALSO be re-derived for a
   differential demand table (the monotone append switches off).
M4 store monotone=false (plumbing landed, D3.a.0; auto under co-activation §6);
   N-1 CLOSED by OQ-N1 (full-rescan model keeps NumRows-occupancy exact).
M5 injector del_vec downstream completeness — ADJUDICATED at code: plumbing is
   complete end-to-end but the forcer never populates it (§1.2); a retract WRITER is
   a stage-(b) design item, not a landed seam.
M6 V-INST-FRESH death-before-band ordering — the OD-2 sort + Touch suppression
   deliver it at the DR layer; NO codegen counterpart exists (§4.3/§4.4).

===============================================================================
## §3 THE INSTANCE RUNTIME + BAND AS LANDED
===============================================================================

### 3.1 InstanceStore<Key, RowT> lifecycle (include/drlojekyll/Runtime/InstanceStore.h)

Nested buffers are compile-time MONOTONE `Table<RowT>` (`using Table = ...Table<RowT>`
:61, verified) — never DiffTable, regardless of the ctor flag; drops are expressed by
RecycleCurrent + rescan-refill, never per-row counters (the OQ-MODEL frame at code).

```
members:                                                    # :320-330 [laneB]
  monotone; keys; hashes; frozen[]; current[];              # NO signed count member
  sealed_occupied[]; touched[]; touched_flag[]; slots
ctor: InstanceStore(allocator, bool monotone_ = true)       # :66 (verified; comment
  # `monotone` gates ONLY the HP-7 seal belt (:185); "Nothing else reads it" (:64-65)
FindInstance(key) -> iid | kNoInstance(~0u, :52), NO mint   # :99-101
FindOrAddInstance(key):                                     # :105-124
  probe; else mint dense iid (id-space-exhausted
    fprintf+abort :112-115 [ALWAYS-ON]); frozen.Add(empty);
  current.Add(empty); sealed_occupied.Add(0); flag.Add(0)
Touch(iid) [private]: append-once flag+touched              # :298-305 (fn head :298, verified)
TouchCurrent(iid): Touch; return *current[iid]              # :130-133 (verified)
Touched(): touched.SortAndUnique(); return                  # :144-147 (verified)
TouchedFlag(iid)                                            # :149-151 (verified)
Seal():                                                     # :174-208 (verified)
  for iid in touched:
    #ifndef NDEBUG  if (monotone):  HP-7 BELT [DBG]         # :177-194: frozen ⊆ current
      for r in frozen[iid]: assert(current.Find != kNoRow)  #   asserted per row (:189-191)
    #endif
    swap frozen[iid]<->current[iid] via .Set                # :199-201
    current[iid]->Reset()                                   # :202
    sealed_occupied = frozen[iid]->NumRows()>0              # :203-204
    touched_flag.Set(iid, 0)                                # :205
  touched.Clear()                                           # :207
RecycleCurrent(iid): Touch + current Reset                  # :216-219 (§2.4)
DebugValidate() [DBG]: post-Seal coherence                  # :222-235 [laneB]
WorkingOccupied = current NumRows>0; SealedOccupied         # :161-169 [laneB]
```
The belt is **[DBG]-only** AND `monotone`-gated — D3.a.1's `monotone=false` stores skip
it even in debug (legitimate shrink); the [ALWAYS-ON] family here is only the id-space
abort + the CODEGEN-side V-INST-FRESH (§3.2). Occupancy = NumRows()>0 stays EXACT under
the ruled full-rescan death model (OQ-N1 closed; laneB's derivation: Seal reads the
FINAL NumRows faithfully; a flap Recycle+refill re-fills before Seal).

### 3.2 EmitSubgraphInstance (lib/CodeGen/CPlusPlus/Database.cpp:2290-2465, verified)

```
locals: id, sname="instance_"+id, demand=DemandFrontier(),
  input_front=InputFrontier(), input, pub, input_member,
  input_fields, pub_member, key_pos, row_pos, in_key, in_row # :2292-2304
emit_instance_rescan(keyexprs):  # shared a1/a2 mold         # :2312-2350
  emit: if (sname.WorkingOccupied(iid)) { fprintf(
        "V-INST-FRESH: instance %u current non-empty at
         band-(a) entry (store <id>)"); abort(); }           # :2314-2320 [ALWAYS-ON in
  emit: auto &cur = sname.TouchCurrent(iid);                 # :2321       generated code]
  emit: full input_member scan + key-equality filter
        + cur.TryAdd(Row_<id>{...})   # MONOTONE add         # :2322-2345
band-(a1) demand drain:                                      # :2352-2371
  for [k0..] in VecName(demand):
    iid = FindOrAddInstance(Key{k...})   # MINTS             # :2362
    if (!TouchedFlag(iid)) rescan(kbinds)  # first-touch     # :2365-2368
band-(a2) edge drain [R-REBUILD-a2]:                         # :2373-2407
  for [e0..] in VecName(input_front):
    iid = FindInstance(Key{e_key...})    # NON-adding, HP-5  # :2394
    if (iid != kNoInstance && !TouchedFlag(iid)) rescan      # :2397-2402
band-(b) publish:                                            # :2409-2458
  pub_exprs[npub]: key slots "key.c<r>", row slots "row.c<r>"# :2417-2426  <- row-var
  for iid in sname.Touched():                                # :2428          HARDCODED
    cur=Current(iid); frz=Frozen(iid); key=KeyAt(iid)        # :2430-2433
    for r in cur rows:                                       # :2434  SCAN CURRENT ONLY
      if (frz.Find(row) == kNoRow):        # (F,T) BORN gate # :2439-2440 (HP-6 comment)
        pub_member.TryAdd(RowExpr(pub_exprs))                # :2442 (index arm :2445-2452:
        # + EmitIndexAdds when pub_has_indexes                 TryAdd .added -> index adds)
sname.Seal();                                                # :2461 (kInstanceSeal self-lowered)
#ifndef NDEBUG sname.DebugValidate(); #endif                 # :2462-2464
```
MISSING at tip (XC-1 standing, re-confirmed by full read): NO (T,F) drop scan (frozen is
read ONLY as the born-dedup filter :2439); NO retract/SubDerivation/queue append —
publish is monotone TryAdd only; NO RecycleCurrent call.

D3.a.1 insertion points + in-scope locals (laneB, verified against the emit):
1. **(T,F) drop scan**: a second loop inside `for iid : Touched()` over `frz`, gate
   `cur.Find(drow)==kNoRow` (dropped = frozen∖current, drop-before-born per
   OQ-PUBLISH-ORDER). In scope: `iid`, `cur`, `frz`, `key` + outer `pub_member`,
   `pub_exprs`, `pub_has_indexes`, `npub`, `key_pos`, `row_pos`. `pub_exprs` hardcodes
   the row var name `row.c<r>` (:2420-2426) — NOT reusable for a `drow`; needs a
   row-var-parameterized pub-expr builder (mirror of the keyexprs lambda).
2. **Delete-side emit**: the SUBGRAPHINSTANCE region owns NO del/add queue vectors
   (contrast GROUP_UPDATE's `region.DelQueue()/AddQueue()`, Database.cpp:2253-2274
   [laneB]) — new UseRef<VECTOR> members + Emplace in LowerSubgraphInstances.
3. **Partition belt** (RAT-7, d4): born+carried==cur.NumRows AND
   dropped+carried==frz.NumRows, [ALWAYS-ON] in generated code — no counter scaffold
   exists at tip.
4. **Death band**: LowerSubgraphInstances iterates instantiate ops only
   (Procedure.cpp:267); a death band must be its own pre-instantiate emission for the
   store (epoch position §4.4).

### 3.3 Region + descriptor lowering (lib/ControlFlow/Build/Procedure.cpp:265-347, verified)

```
LowerSubgraphInstances(impl, context, dr_flow, seq):
  for op in dr_flow.SubgraphInstances():        # instantiate ops ONLY (Rel.cpp:585-593)
    inst = dr_flow.instances[sid]
    V-INST-DIFF-COHERENCE [ALWAYS-ON]:                       # :274-285 (verified)
      inst.differential != TableIsDifferential(op->table_op_table) => fprintf+abort
    demand_front = TableDeltaVector(demand, kNetAdditions)   # :287-289 (memoizing mint)
    input_front  = TableDeltaVector(input,  kNetAdditions)   # :292-294
    si = CreateDerived<SUBGRAPHINSTANCE>(seq, sid,
                                         inst.differential)  # :296-298 (3-arg, D3.a.0)
    ... key/row positions, input_key_cols (+ [ALWAYS-ON]
        bound-cols-vs-key-arity check :319-325 [laneB]) ...
    enroll {sid,kSubgraphInstantiate},{sid,kInstanceSeal}    # :341-345
```
The diff bit's three sinks (all landed D3.a.0, all FALSE today): region impl
`const bool differential` (lib/ControlFlow/Program.h:1188-1190, comment "read by
EmitSubgraphInstance in D3.a.1. FALSE today.", verified) — **NO public accessor**
(include/drlojekyll/ControlFlow/Program.h:848-880 verified: StoreId/frontiers/tables/
positions only); descriptor `ProgramInstanceStore.differential` (lib Program.h:
1950-1953, verified) set at Stratum.cpp:2338 (verified), public
`ProgramInstanceStoreInfo::IsDifferential()` consumed ONLY at the header store-ctor
emitter — Database.cpp:1460-1467 (verified): `instance_<id>(allocator_` + `, false`
IFF differential. The emitter's only reachable live signal today is
`pub.IsDifferential()` (DataTable::IsDifferential, Program.cpp:963, verified); both
routes agree by V-INST-DIFF-COHERENCE.

===============================================================================
## §4 FRONTIERS + PUB-SIDE DIFFERENTIAL INTERFACE + THE COUPLING TIMELINE
===============================================================================

### 4.1 The provisioning chain (two vec layers, laneC, spot-verified)

DR layer: `DRVec`/`VecRole` (Rel.h:54-69), minted `MintTableVec` (Rel.cpp:616-628)
into `flow.table_vecs[table][role]`; `ResolveVecIdx` returns `~0u` on a missing
(table, role) => silently NO dep edge (Rel.cpp:4638-4655; the :794-comment mold).
ControlFlow layer: `VECTOR*`/`VectorKind` (Program.h:254-297), lazily minted+memoized
by `TableDeltaVector` (Build.cpp:745-777) into `context.table_delta_vecs` — what
codegen emits.

The demand/input kNetAdditions chain as landed (LIVE on the witness):
```
1 APPEND  eager boundary append, MONOTONE tables only        # Build.cpp:997-1004
          gate: table && !TableIsDifferential(table) &&      # :999 (verified)
                (any_cut_succ || monotone_negated)
2 DRAIN   (DR model) InstantiateEffects: drain{demand,       # Rel.cpp:784-788
          kNetAddition} + drain{input, kNetAddition}         # :793-800 [laneC]
3 DRAIN   (codegen) LowerSubgraphInstances fetches the SAME
          memoized VECTORs -> region frontiers               # Procedure.cpp:287-294
4 band-(a1)/(a2) drain them                                  # Database.cpp:2359/:2391
```
Differential tables instead get the full frontier sextet minted DR-side
(Rel.cpp:1818-1830, verified: DeleteQueue/AddQueue/OverdeleteSet/AdditionSet/
kNetRemoval/kNetAddition; + claimed pair for recursive SCC tables :1837-1843) and the
net frontiers PRODUCED by the commit-band frontier filter `mint_filter`
(Rel.cpp:2511-2538, verified: drain overdelete|addition set -> flagread
NetDeleted|NetAdded -> append kNetRemoval|kNetAddition), per differential
non-induction-owned table (:2540-2543). **NO demand kNetRemovals vec is minted,
appended, or drained anywhere at tip** (laneC grep, re-affirmed); the only would-be
consumer is DeathEffects' drain (§2.1). Under d1 the demand table's frontier
provisioning necessarily SWITCHES routes (the :999 gate excludes it from route 1) to
the differential machinery — with the XC-3 validator-timing consequence (§2.3). Note
the semantic upgrade riding along: the differential route's net frontier is the
POST-NETTING commit-band product (SET-netted adds/removes), exactly the OD-15
death/birth trigger shape.

### 4.2 The pub-side signed-delta interface (what the (T,F) scan must feed)

Runtime (include/drlojekyll/Runtime/Table.h, DiffTable): `AddDerivation`/
`SubDerivation(row, DerivClass)` -> `Delta{crossed, added_row, id}` (:344/:351/:331-335
[laneB]); claim gates at dequeue `TryClaimDel` (stale gate C_nr>0 => drop, :465-495)
/ `TryClaimAdd` (Total<=0 => drop, :497-527) — the F17 re-tests [laneC]; `Commit(sink)`
publishes ONLY `was!=now` per touched row, per-class >=0 asserts **[DBG]-only**
(:551-552), `kInI := now`, compaction floor 4096 (:592-601 — suite-sized never fires)
[laneC].

Producer patterns, mined from the ONLY landed differential-fold emitter (GROUP_UPDATE
`emit_touched` Database.cpp:2221-2279 + `emit_add_deriv` :2204-2219 [laneB]):
```
delete side: pub_member.SubDerivation(<row>, DerivClass::kNonRecursive);
             VecName(DelQueue).Add(<row>);                   # agg model :2269-2275
add side:    gd = pub_member.AddDerivation(<row>, kNonRecursive);
             if (gd.added_row) EmitIndexAdds(...);           # :2210-2218
             VecName(AddQueue).Add(<row>);
```
The DR model ALREADY declares exactly this shape for a differential pub: the
InstantiateEffects diff arm (Rel.cpp:829-848, verified) emits per sign {+1,-1} a
kCounter{pub,sign,kNonRecursive} + kInIReadFrozen crossing + kVecAppend{pub,
kDeleteQueue|kAddQueue}; the R-MONO else-arm is one +1 counter, zero appends
(:849-856, verified). DORMANT (no differential pub on any accepted program). The
V-INST-EFFECT instantiate regime split expects exactly this per `diff`
(counters==2/crossings==2/appends==2 vs 1/0/0, Rel.cpp:4301-4335 area [laneC]) — mint
and validator read the SAME `diff`, so co-activation keeps them consistent by
construction. A (T,F) scan is the missing PRODUCER of the -1 rows this arm models;
downstream claim/commit machinery consumes them structurally (LowerCommitSweeps).

### 4.3 TouchedFlag as the coupling primitive (the OD-15 three-way coupling at code)

Writers: `Touch` (append-once, :298-305) via TouchCurrent (:130-133) and
RecycleCurrent (:217); reset by Seal (:205). Codegen readers: band-(a1) `!TouchedFlag`
(:2365), band-(a2) `iid!=kNoInstance && !TouchedFlag` (:2397). V-INST-FRESH is the
[ALWAYS-ON] generated-code guard at band-(a) entry (:2314-2320); its [DBG] dual is the
Seal HP-7 belt (monotone stores only, :177-194). Interleaving table (laneC, logic
re-checked):

| same-epoch order                | outcome |
|---|---|
| a1 then a2 (same iid), either order, or twice | first-touch rescans + sets flag; later sight SKIPS. V-INST-FRESH OK |
| death(Recycle) BEFORE a1/a2     | Touch sets flag => a1/a2 SKIP => current stays empty => dead key publishes (T,F) full retract at band-(b). THE suppression the death needs |
| a1/a2 fill BEFORE death         | death empties a filled current, flag already set — a co-demanded key's birth defeated (WRONG order) |
| death WITHOUT Touch (counterfactual) | later rescan would re-fill => dead key reborn (WRONG) — RecycleCurrent's Touch is load-bearing |

=> the death band MUST run before band-(a1)/(a2) for its store AND must Touch (it
does). Matches OD-2 (death −1 sorts before instantiate +1) and OD-15
OQ-DEATH-VS-REBUILD verbatim. Same-batch death+re-demand is killed upstream by the
handler NETBATCH (§1.3) once the retract writer exists.

### 4.4 The epoch timeline as landed (verified skeleton)

```
BuildEagerProcedure tail (Procedure.cpp:878-890):
 1 proc->body: injector appends -> ingest folds/loops + eager web
   (CompleteProcedure :882) -> BuildStratumPhases(:886):
     BuildDRInventory/mints -> DeriveDRStrata (:2156)
     -> delta JoinEmit enroll -> ValidateDRInventory
     -> ValidateDROps (:2186)  <- V-INST-DRAIN here [XC-3]
     -> LinearizeAndValidateDRFlow (:2187) -> Site-5 blocks
     -> stash context.dr_flow (:2291) -> DumpRelIfEnabled
     -> per-stratum LowerDRFlow (:2477) / LowerDRRounds (:2489)
        <- differential frontier VECTORs first minted HERE
 2 PublishDifferentialMessageVectors (:889 -> :349-471):
     transmit publishes -> LowerSubgraphInstances (:432)
       [bands a1/a2/b + Seal, one region per instantiate]
     -> LowerCommitSweeps (:433) -> V-INST-EMITTED (:435-464)
     -> RETURN
runtime per epoch: netting (handler) -> folds park queue rows ->
  stratum phases fill net frontiers -> instance bands drain them ->
  band-(b) publish -> Seal -> commit sweeps publish was!=now
```
A death band sits at the HEAD of the instance step for its store (before its a1);
the DR op order guarantees it graph-side; the codegen emission slot does not exist
yet (d3). Runtime data-timing is sound for a death drain: the demand net-removals
frontier (differential route) is filled by the stratum phases in step 1, before the
instance bands in step 2 — same relation the a1 additions drain relies on.

===============================================================================
## §5 THE SLICE GAP LEDGER (gap -> anchor -> which dN/M discharges it)
===============================================================================

| # | gap (as landed) | anchor at tip | discharged by |
|---|---|---|---|
| G-1 | fabricated demand message monotone (`differential_attribute` never set) | Demand.cpp:163-204; Parse.h:388 | **d1** |
| G-2 | forcer/injector del_vec never populated — retracting a standing demand has NO writer (channel plumbed, empty) | Build.cpp:416-421,426-440 | **d1** (M5) — stage (b) designs the writer |
| G-3 | demand kNetAdditions provisioning DISAPPEARS for a differential demand table (monotone-append gate) + V-INST-DRAIN runs pre-lowering | Build.cpp:999; Stratum.cpp:2186 vs :2477/:2489; Rel.cpp:4509-4512 | **d1+d3 co-land** [XC-3] (M3 widened) |
| G-4 | demand kNetRemovals frontier never minted/appended/drained; DeathEffects' drain dangles to `~0u` (silent no-edge) | Rel.cpp:865-869; :4638-4655 | **d3** (G-DEMAND-NEG, M3) |
| G-5 | V-INST-DRAIN checks kNetAdditions only, no removals clause | Rel.cpp:4494-4524 | **d3** (M3) |
| G-6 | kInstanceDeath has NO lowering/enrollment — a minted death aborts at V-INST-EMITTED (second abort after G-3) | Procedure.cpp:267,341-345,435-464 | **d3** (G-DEATH-LOWER, M1) |
| G-7 | RecycleCurrent has ZERO codegen callers; no EmitInstanceDeath | InstanceStore.h:216-219 | **d3** (M2) |
| G-8 | death drain not source-checked {demand, kNetRemoval} in V-INST-EFFECT | Rel.cpp:4346-4368 | **d3** (strengthening, optional per stage (b)) |
| G-9 | no (T,F) drop scan in band-(b); frozen read only as born-dedup; pub_exprs row-var hardcoded | Database.cpp:2434-2456, :2420-2426 | **d4** (G-TF-PUBLISH, M2) |
| G-10 | SUBGRAPHINSTANCE region owns no del/add queue vectors; monotone TryAdd is the only publish | Program.h impl (contrast :2253-2274 GROUP_UPDATE); Database.cpp:2442 | **d4** |
| G-11 | RAT-7 partition belt absent (born+carried==cur, dropped+carried==frz) | Database.cpp:2409-2458 (no counters) | **d4** |
| G-12 | emitter cannot read the region diff bit — impl member has no public accessor; only pub.IsDifferential() reachable | Program.h:1188-1190; include Program.h:848-880; Program.cpp:963 | **d4/d5** (accessor or pub-branch; coherence ties them) |
| G-13 | `monotone=false` never selected — every emitted store belt-armed (bit false program-wide) | Database.cpp:1460-1467; Stratum.cpp:2338 | **d5** (automatic under co-activation, §6) |
| G-14 | witness has no retract batches; flat-side retraction unexercised | tests/OptDiff demand_neighborhood_witness | **d6** |
| G-15 | any published output over the demanded closure must be @differential or the faithfulness check hard-errors | Differential.cpp:174-179 | **d6** (witness-design constraint) |
| G-16 | V-INST-DIFF-COHERENCE / OWN-3 fold abort / partition belt never perturbed with a TRUE bit | Procedure.cpp:274-285; View.cpp:609ff | **d7** (M4's liveness half) |
| G-17 | DS-R4-10 @never-over-deletable reject fence + witness unassigned | Differential.cpp:81-111 (@never arm) | **rider** (per §7 d-list) |
| G-18 | N-1 signed-count contingency | InstanceStore.h:161-173 | CLOSED (OQ-N1, full-rescan; record-only) |

(M1->G-6, M2->G-7+G-9, M3->G-3+G-4+G-5, M4->G-13+G-16+G-18, M5->G-2, M6 delivered by
OD-2+Touch at the DR layer — its codegen slot is part of G-6/G-7.)

===============================================================================
## §6 THE L3 FACT BASE (facts ONLY — the d2 ruling is the orchestrator's)
===============================================================================

**The two predicates at code.**
- P-STORE: `DRInstance.differential = TableIsDifferential(pub_table)` — Rel.cpp:1055
  (`diff` local) / :1059 (stamp; same local feeds the InstantiateEffects fork
  :829-856). Live re-check: V-INST-DIFF-COHERENCE, Procedure.cpp:278-285 [ALWAYS-ON].
  Sinks: region 3-arg ctor Procedure.cpp:296-298 -> Program.h:1188; descriptor
  Stratum.cpp:2338 -> store-ctor `, false` Database.cpp:1460-1467.
- P-DEATH: `demand_table && TableIsDifferential(demand_table)` — Rel.cpp:1139.
- `TableIsDifferential(t)` itself: any member view `CanProduceDeletions()` OR
  IsAggregate/IsKVIndex — Build.cpp:705-721 (verified).

**Does a @differential demand message make TableIsDifferential(pub) TRUE?**
- FLAT WEB (laneA's exhaustive walk, §1.4, spot-verified at the seed/lift/seam/
  col-edge sites): **YES.** The Differential.cpp fixpoint propagates the seeded
  demand receive through root/merge/reader -> every guard JOIN -> the guarded body ->
  p's MERGE/INSERT -> the seam -> all of p's downstream. Non-localizable; no
  interceptor on the slice (NEGATE/AGG pre-rejected on the demand path,
  Demand.cpp:624-627).
- NESTED LOWERING (adjudicated at code — laneA walked only the flat web): **YES,
  IDENTICALLY.** `-demand-instance` is a ControlFlow-only selector: Main.cpp passes
  `gDemand` to `Query::Build` (:69) and `gDemandInstance` ONLY to `Program::Build`
  (:82, param Build.cpp:1276, registered :1468). The Query graph — and therefore the
  can_produce bits `TableIsDifferential` reads — is IDENTICAL flat vs nested; the
  nested pub_table's member views (p's MERGE/INSERT/readers) carry the same flipped
  bits. The lanes do not disagree; laneA + lane3 both assert the flip, and the
  nested-side extension follows from the shared Query graph.
- Therefore at code: **P-DEATH true => P-STORE true (CO-ACTIVATION HOLDS)**, under
  both lowerings, with zero code change beyond the d1 toggle.

**The converse, on ACCEPTED `-demand-instance` programs at D3.a.1:** P-STORE without
P-DEATH is unreachable — the pub's views are demand-minted guards/body/merge whose
only other differential sources are (i) a differential summarized input — FENCE (iii)
rejects (Build.cpp:1419-1421, :1442-1444, verified), (ii) agg/KV or negation on the
demand path — pre-rejected (Demand.cpp:624-627), (iii) impure MAP — rejected at
ControlFlow (Build.cpp:1360-1364 [laneA, lane-cited]). So at D3.a.1 the two predicates
are **extensionally EQUAL on accepted programs**; the explicit disjunction
`TableIsDifferential(demand) || <input differential>` is also extensionally equal to
P-STORE given the closure propagation (any differential demand/input flips pub).

**Where the predicates DIVERGE (the D3.a.2 horizon):** with FENCE (iii) lifted, a
differential INPUT flips pub (same closure) while the demand stays monotone —
P-STORE true, P-DEATH false: the store is droppable-content (belt must be off,
diff-pub effects live) with NO demand death minted. The pub-keyed P-STORE covers this
automatically; a demand-keyed store predicate would leave the HP-7 belt armed against
a legitimately shrinking store. Facts only; the naming/spelling choice is d2's.

**Observable consequences of co-activation at d1 (leaving P-STORE as-is):**
1. Store ctor emits `, false` (belt OFF), region bit true, descriptor true — all
   three sinks flip together off the one :1059 stamp; V-INST-DIFF-COHERENCE cannot
   desync (same predicate, same table, both sides).
2. InstantiateEffects diff arm goes live (2 counters/2 crossings/2 queue appends)
   and V-INST-EFFECT's regime split expects exactly that — consistent by shared
   `diff` (Rel.cpp:1055).
3. The demand table itself becomes a DiffTable (two-polarity ingest folds, claim
   gates, commit sweep, net frontiers) — the death trigger arrives as the NETTED
   kNetRemovals frontier, the OD-15 shape.
4. The abort chain of §2.3 engages until d3/d4 land (V-INST-DRAIN first, then
   V-INST-EMITTED): d1 is NOT independently landable.
5. The flat `-demand` lowering of the same program becomes fully differential with
   no new machinery (§1.4) — the eqgate flat==nested oracle stays live for retract
   batches (d6), per the ruling brief's standing referee note.

**If instead the store predicate were switched to the explicit disjunction:** at
D3.a.1 no observable difference exists on accepted programs (extensional equality
above); the difference is only in which future graph shapes keep the bit true, plus
the loss of the "one authority, one spelling" property §7 records for :1059 — the
coherence validator would then compare two DIFFERENT predicates and become a real
desync detector rather than a tautology guard. Facts; no recommendation.

===============================================================================
## XC ADJUDICATIONS (code is authority; numbering continues d3a-substrate XC-1/2)
===============================================================================

- **[XC-3]** Lane3's counterfactual step 6 ("V-INST-DRAIN => PASS" under a
  differential demand) is WRONG at tip: the eager boundary append is gated
  `!TableIsDifferential(table)` (Build.cpp:999) and ValidateDROps (containing
  V-INST-DRAIN, fn Rel.cpp:3334) runs at Stratum.cpp:2186, BEFORE the differential
  frontier VECTORs are first minted in LowerDRFlow/LowerDRRounds (Stratum.cpp:2477/
  :2489; ControlFlow kNetAdditions provisioner census verified by grep). So the FIRST
  abort under a naive d1 flip is V-INST-DRAIN (Rel.cpp:4509-4512), with lane3's
  V-INST-EMITTED as the SECOND. LaneC listed provisioning routes (A)/(B) as a free
  stage-(b) choice — also corrected: d1 (the OD-15-ruled channel) FORCES route (A)
  and simultaneously disables route (B)'s monotone append. Consequence: d1 and d3
  must co-land (or interpose a temporary fence).
- **[XC-4]** LaneA's Differential.cpp seed anchor ":59-62" — the seed loop is
  :56-65 (assignments :60-61). Cosmetic drift; the walk is unaffected.
- **[XC-5]** Lane anchors for `Touch`: laneB :298-305 is correct at tip (fn head
  :298, verified); laneC's ":295-300" includes the comment block. Cosmetic.
- **[XC-6]** Lane3 was derived at parent tip 0d33bdca; §20(AH) records the errata
  commit as line-count-preserving in Rel.cpp, and every lane3 anchor I re-touched
  (:1139, :861-884, :4346ff, :4754ff, :5104ff, Procedure.cpp:265-347/:435-464,
  InstanceStore.h:216-219) verifies unchanged at a87aad5f. Lane3 is treated as a
  current-tip source with the single XC-3 correction.

===============================================================================
## THE TEN MOST LOAD-BEARING ANCHORS (for the orchestrator to re-verify personally)
===============================================================================

 1. Demand.cpp:163-204 — fabrication sets name/params only; `differential_attribute`
    (Parse.h:388) untouched; IsDifferential read Parse.cpp:1336-1338.
 2. Build.cpp:416-421 + :426-440 — del_vec created iff IsDifferential, kEmpty,
    passed `// Empty.`, NEVER a VECTORAPPEND target.
 3. lib/DataFlow/Differential.cpp:56-65 (seed) + :114-140 (lift/seam/col-edge) +
    lib/DataFlow/Build.cpp:2618 (authoritative run) + lib/ControlFlow/Build/
    Build.cpp:705-721 (TableIsDifferential) — the co-activation chain.
 4. Rel.cpp:1055/:1059 (P-STORE stamp) vs :1139 (P-DEATH gate) — the two axes;
    DeathEffects :861-884.
 5. Build.cpp:999 gate + Stratum.cpp:2186 (ValidateDROps call) vs :2477/:2489
    (LowerDRFlow/LowerDRRounds) + Rel.cpp:4494-4524 (V-INST-DRAIN) — the XC-3
    abort-order claim.
 6. Procedure.cpp:267 (instantiate-only iteration), :341-345 (enrollment),
    :435-464 (V-INST-EMITTED multiset incl. kInstanceDeath at :447-449).
 7. Database.cpp:2290-2465 — bands a1/a2/b + Seal; V-INST-FRESH :2314-2320
    [ALWAYS-ON]; born-only gate :2439; pub_exprs row-var hardcode :2420-2426.
 8. InstanceStore.h:174-208 (Seal + [DBG] monotone-gated HP-7 belt :177-194) +
    :216-219 (RecycleCurrent, zero codegen callers) + :66 (ctor flag).
 9. Rel.cpp:829-856 — InstantiateEffects diff/mono fork (the pub signed-delta
    effect model the (T,F) scan realizes); regime-split V-INST-EFFECT counterpart.
10. Main.cpp:69/:82 + Build.cpp:1276/:1393/:1468 — `-demand-instance` is
    ControlFlow-only (the nested-lowering half of the §6 co-activation answer);
    FENCE (iii) at :1419-1421/:1442-1444.

TRUNCATION NOTES: none — all three stage-a lane files and fleet1/lane3-death.md are
complete on disk (coherent tails, statuses/final summaries present); the lane FINAL
MESSAGES in the orchestrator prompt match the files' content.

===============================================================================
## §7 THE d2/L3 ADJUDICATION (orchestrator, AT CODE — the first ritual-head
##    question of D3.a.1, ruled BEFORE stage-(b) lanes; §20(AF) §3.3 rider)
===============================================================================

RULED: **CO-ACTIVATION — the store predicate stays `TableIsDifferential(pub)`
as landed (P-STORE unchanged; zero code change beyond the d1 toggle).** The
explicit disjunction is REJECTED. Adjudicated at code by the orchestrator
without an owner brief, per the charter's escalation rule: the §6 fact base
proves the two spellings are extensionally EQUAL on every accepted D3.a.1
program (P-DEATH ⇒ P-STORE via the lib/DataFlow/Differential.cpp closure,
under BOTH lowerings; the converse blocked by FENCE (iii) + the Demand.cpp
:624-627 rejects + the impure-MAP reject), so the choice changes NO observable
behavior at this slice. Grounds beyond equality:
  (1) D3.a.2 CORRECTNESS: the predicates first diverge when FENCE (iii) lifts
      (differential INPUT: P-STORE true, P-DEATH false). The pub-keyed spelling
      handles that divergence CORRECTLY for the store (belt off for a
      legitimately shrinking store; diff-pub effects live) with no edit; a
      demand-keyed or disjunction spelling would need the input term added at
      D3.a.2 anyway — the closure already makes pub-diff their extensional
      union.
  (2) ONE AUTHORITY: keeps the §7/d3a0-design one-spelling property of the
      Rel.cpp:1059 stamp; all three sinks (region ctor, descriptor, store-ctor
      emit) continue reading the single `diff` local (Rel.cpp:1055).
  (3) VALIDATOR SEMANTICS: V-INST-DIFF-COHERENCE remains what it was designed
      to be — a stamp-vs-live-predicate drift guard (same predicate both
      sides). Under the disjunction it would silently become a cross-predicate
      comparator, changing its meaning without a ruling.
CONSEQUENCE FOR d3 (recorded, binding for stage (b)): the death mint gate
KEEPS its own predicate `TableIsDifferential(demand_table)` (Rel.cpp:1139) —
the two axes stay separately spelled at their two sites, and co-activation
(not predicate unification) is the reconciliation. A stage-(b) design MUST NOT
fold the two predicates into one shared helper: their extensional equality is
a THEOREM of the current fence set, not an invariant — D3.a.2 breaks it by
design, in the direction the two sites already handle correctly.
XC-3 RIDER (re-stated as a d2 consequence): d1 (the @differential toggle) is
NOT independently landable — the abort chain (V-INST-DRAIN first per XC-3,
V-INST-EMITTED second) engages until d3/d4 land. Stage (b) must either
co-design d1+d3(+d4) as one landable unit or interpose a temporary
pre-pass fence for the intermediate commits.


===============================================================================
## §8 THE POST-D3.a.1 STATE (2026-07-28, tip 33cabcf1; orchestrator-read
##    anchors — the epoch's whole-program view AFTER slice 1, §20(AK).
##    SINGLE-PASS: the next session's fleet re-verifies THIS section +
##    §20(AH)-(AK) before D3.a.2 code. §1-§6 above are the PRE-slice-1 map
##    (stamped at 95251825; the slice's ~1069 inserted lines drifted its
##    anchors); §7 is the d2 ruling, still binding.
===============================================================================

    THE PIPELINE AS IT STANDS (whole-program; only slice-1 deltas spelled
    out — everything else per §1-§6 modulo drift):

    flags (bin/drlojekyll/Main.cpp):
      gDemandRetract :50; `-demand-retract` implies -demand :488-491;
      threads ONLY into Query::Build (:70, fifth param) — DataFlow-side
      like -demand; -demand-instance stays ControlFlow-only. OFF
      PassPolicy; orthogonal to the 4 golden modes.

    fabrication (lib/Parse/Demand.cpp):
      FabricateDemandMessage(name, types, differential) — when the flag
      is on, stamps message->differential_attribute =
      Token::Synthetic(kPragmaDifferential, DisplayRange()) :209 ->
      IsDifferential() holds -> TrackDifferentialUpdates seeds the
      receive (lib/DataFlow/Differential.cpp seed arm) -> the WHOLE
      demanded closure flips differential via the ORDINARY machinery
      (P-STORE co-activates per the §7 d2 ruling; zero bespoke flat
      code — the standing eqgate-oracle premise).

    the retract entry (lib/ControlFlow/Build/Build.cpp + codegen):
      BuildQueryRetractProcedureFromRegistry :494 (a second
      kQueryMessageInjector; params = the bound query params; appends to
      del_vec; add_vec rides empty; the handler-presence lookup is an
      ALWAYS-ON fence — review-[E]; the forcer twin at :385ff keeps its
      pre-existing assert, unified at the f2 dedup), dispatched :575;
      ProgramQuery.retract_function; the generated hidden friend
      `<name>_<pattern>_retract(db, log, functors, bound...)` (void,
      both lowerings). SET netting = the handler NETBATCH arm
      (Procedure.cpp; NetBatch in Vec.h) — adds∩removes annihilate;
      AddExplicit idempotence + SubExplicit structural no-op make the
      surface TOTAL+IDEMPOTENT (design R-4).

    the mint (lib/Rel/Rel.cpp):
      kInstanceDeath gate :1140 (`demand_table &&
      TableIsDifferential(demand_table)` — P-DEATH, demand-keyed, LIVE
      under -demand-retract; comment :1138-1139); DeathEffects
      unchanged (4-effect zero-counter signature); OD-2 sign sort +
      V-INST-ORDER death-before-instantiate unchanged.

    validators (lib/Rel/Rel.cpp):
      V-INST-DRAIN REGIME-SPLIT ~:4500-4550: monotone arms check the CF
      vector (cf_ok -> HasTableDeltaVector, Build.h:327/Build.cpp:849);
      the DIFFERENTIAL demand arm checks the DR-side vec + the `+`
      kFrontierFilter producer (dr_ok) — the CF vecs mint later, in the
      stratum lowering (XC-3); CheckInstanceDeathFrontier :4789 (pure;
      called :4550; death-tested in rel_validators_test via the shared
      tests/DrTest/DeathHarness.h) requires the demand kNetRemoval vec +
      the `-` kFrontierFilter producer; the V-INST-EFFECT death arm is
      G-8 source-aware (drain must be kNetRemoval of op.demand_table).

    the lowering (lib/ControlFlow/Build/Procedure.cpp):
      death_by_sid :281-283; per-instantiate wiring :360-410:
      V-INST-DEATH-COHERENCE :379 [ALWAYS-ON] (death names the SAME
      demand/pub as its instantiate), orphan-mint fences [ALWAYS-ON] at
      EVERY memoized fetch (band-(a1) demand frontier :318 — review-[B];
      death drain :399; pub del/add queues), removal_frontier.Emplace
      :404, demand_table.Emplace :368 (the a2 gate's read;
      model-covered by the declared kInstanceDemand effect),
      {sid,kInstanceDeath} enrollment (V-INST-EMITTED balances at
      3 ops/differential store, 2/monotone); ClassifyVector's
      kSubgraphInstance arm classifies removal_frontier(read) +
      del/add queues(written).

    the band (lib/CodeGen/CPlusPlus/Database.cpp, EmitSubgraphInstance):
      band-(a0) DEATH :2406-2439 (drain the NETTED kNetRemovals
      frontier -> FindInstance -> kNoInstance silent-skip (R-6) ->
      RecycleCurrent :2435 — Touch + current.Reset, its FIRST codegen
      caller); band-(a1) unchanged + fence; band-(a2) DEMAND-LIVENESS
      gate :2487-2519 (diff arm ONLY: `if (iid != kNoInstance) { dq =
      demand.Find(key); if (dq != kNoRow && Present(dq) &&
      !TouchedFlag) rescan }` — the review-[I] nest; monotone arm
      tip-verbatim); band-(b) two-regime :2525ff: the (T,F) DROP SCAN
      (frz rows absent from cur -> SubDerivation + DelQueue append,
      OVERDELETE-first per iid) BEFORE the born scan (diff:
      AddDerivation + AddQueue + EmitIndexAdds gated on added_row;
      monotone: TryAdd verbatim); the generated V-INST-PARTITION belt
      :2557-2650 [ALWAYS-ON in generated code]: born+carried==cur &&
      dropped+carried==frz per touched iid; CollectEffects registers
      DemandTable under the diff regime (review-[D]); Seal unchanged;
      store ctor `, false` live for differential stores (HP-7 belt
      OFF for them — [DBG] elsewhere).

    the DS-R4-10 fence (lib/DataFlow/Differential.cpp):
      POST-FIXPOINT sweep (before report_message_errors, under
      log.IsEmpty()), guard `is_dead || !negated_view || !is_never ||
      !can_produce_deletions`, per-predicate diagnostics + an
      ALWAYS-ON no-predicate fallback (review-[C]);
      negate_never_diff_1 = the all-4-modes witness.

    witnesses (tests/OptDiff):
      demand_neighborhood_witness — DIFFERENTIAL regime: `-demand
      -demand-retract` (+ eqgate nested arm), the @differential
      nbhd_out tap (eqgate = answer + SORTED PUBLISHED-DELTA identity),
      phases birth/rebuild/RETRACT/dead-key-edge(e7 abort teeth)/
      rebirth/second-death; demand_neighborhood_mono_witness — the
      R-MONO twin (pre-slice bytes, bare `-demand`, second eqgate
      case; keeps the monotone nested lowering end-to-end covered).
      Suite 177; ctest 6 binaries (rel_validators_test carries
      DeathFrontierTest; the fork/waitpid harness is shared
      tests/DrTest/DeathHarness.h).

    THE TWO PREDICATES (the §7 ruling, as landed): P-STORE =
    TableIsDifferential(pub) (Rel.cpp mint stamp -> region ctor ->
    descriptor -> `, false`), P-DEATH = TableIsDifferential(demand)
    (:1140). CO-ACTIVE on every accepted D3.a.1 program; they FIRST
    DIVERGE at D3.a.2 by design (diff input: P-STORE true, P-DEATH
    false — belt off, no death minted; the pub-keyed spelling already
    handles it). NEVER fold them.

    THE PATH FORWARD AS DIFFS ON THIS STATE (ruled order, OD-15):

    D3.a.2 — DIFFERENTIAL INPUT (NEXT; OQ-INPUT ruled YES; opens at
      stage (a) — re-derive the input-side substrate from code, THEN
      diffs):
      e1 LIFT FENCE (iii): Build.cpp:1516-1560's diff_input arm (the
         `in.CanReceiveDeletions()` reject) — demand_diff_input_1
         FLIPS from all-4-modes-diagnostic to a compiling case
         (runall.sh diagnostic-list + CLAUDE.md edits ride).
      e2 LIFT V-INST-SOLE's input arm (Rel.cpp ~:4339 — the
         `TableIsDifferential(op.input_table)` forbiddance; keep the
         pub-alias half).
      e3 THE INPUT REMOVAL TRIGGER: the ruled semantics — ANY input
         change (either sign) for a live-demanded key fires
         RecycleCurrent + full rescan; band-(b) diff-at-publish emits
         the net retractions. Mechanism to design: the input table's
         kNetRemovals frontier as a SECOND a2 drain (or one combined
         ± drain) -> a Recycle-then-rescan arm (the a2 gate's
         demand-liveness conjuncts unchanged); InstantiateEffects'
         input drain arm grows the removal leg (+ V-INST-EFFECT/
         V-INST-DRAIN input-arm regime split mirroring the demand
         arm); TouchedFlag stays the same-epoch dedup.
      e4 THE QUIESCENCE RE-DERIVATION (the §3.2 rider, FIRST-CLASS):
         the a2 gate's `Present == committed presence` argument
         assumed the demand table is quiescent in an input epoch;
         with differential INPUT the epoch shapes multiply — re-derive
         the interleavings (input-removal epoch vs demand-retract
         epoch vs mixed batches) and pin the coupling like OD-15 did
         for death (netting/TouchedFlag/V-INST-FRESH).
      e5 THE DIVERGENCE GOES LIVE: first program with P-STORE true &&
         P-DEATH false (diff input, monotone demand). V-INST-DIFF-
         COHERENCE unaffected (same predicate both sides); belts/
         effects on the P-STORE side must not assume a death exists
         (V-INST-PAIR already allows n_death==0).
      e6 WITNESS: a differential-INPUT eqgate witness (edge retract
         batches; flat -demand already handles them — the oracle
         stays live); decide demand_diff_input_1's disposition
         (promote vs keep as a lifted-fence compile witness) at
         stage (b).
      e7 G-INPUT-NEG / G-STALE discharge (the §5 gap ledger rows).
      e8 LIVENESS: perturbation rows for the new arm (the L-table
         idiom; NESTED-arm vehicles; never-minted roles chosen per
         the L3 amendment).
    D3.a.3 — MULTI-ADORNMENT: loop the pass per adornment;
      (query, BindingPattern) keying sweep; N disjoint stores
      (OQ-ADORN-KEY). PRECONDITIONS (both BINDING): f1 the
      View.cpp:571-583 fold-predicate re-derivation with directed
      witnesses BOTH directions (survivorship role policy;
      proxy-TUPLE invariance); f2 the review-[F] retract/forcer
      builder dedup (one parameterized builder + one dispatcher;
      harden the forcer's assert-only handler guard to the always-on
      fence while there).
    DEFERRED all-epoch: recursive demand (FENCE (i); the §20(AB)
      NeedsInductionCycleVector precondition binds any toucher).

    RITUAL AMENDMENTS BANKED THIS SLICE (bind future stage-(d) runs):
      WIP-COMMIT the prototype worktree BEFORE perturbation cycles;
      perturbation roles must be GENUINELY never-minted for the flow
      (kProductInput, not kDeleteQueue — differential tables own
      queues); runtime-perturbation vehicles are the NESTED arm
      (diffrun's four modes are the flat arm); background shells use
      ABSOLUTE paths (cwd resets silently).
