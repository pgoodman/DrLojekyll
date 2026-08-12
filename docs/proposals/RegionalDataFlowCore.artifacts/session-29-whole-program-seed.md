# Session-29 whole-program seed — REOPEN demand, grounded in code at tip

> Cold-start START-HERE for session 29. Branch `keyed-instances`, tip `05af6595`. Every anchor below was
> read at tip or recovered from git `48cd0a4f` (the pre-P1 tip, where the demand machinery still lived).
> **Re-verify every anchor before writing code — the pipeline drifts each session.** clangd in this repo
> is NOISE — trust the real build. Companion: `session-29-prompt.md` (the charter), `session-29-demand-
> reopen-seed.md` (the fork + roadmap), `session-28-triage-decision.md` (why the minimal-codegen cut is
> refuted). This seed is the STARTING pseudocode; the session-29 grounding loop BUILDS IT OUT.

---

## §0 STATUS + the load-bearing finding
- **LANDED:** P1–P7 + P7b + P7c (AccessPlan physical-access arc), P6.1–P6.3 + F16-close (compile-time
  analyses — trustworthy but consumer-less). OptDiff SUITE PASS (227), ctest 5/5.
- **Session-28 outcome:** the "make the model drive codegen" minimal cut was refuted unanimously — the
  runtime is already well-optimized for the corpus (P7 harvested the seeks; the rest is intrinsic). Owner
  redirected to **reopen demand** (the namesake, deleted at P1); **changing derivation cost (result-equal)
  is OK.**
- **THE LOAD-BEARING FINDING (recovered cost model, `recursive-demand-seed.md` §7 — READ THIS FIRST):**
  demand PAYS iff `pruned_eager_derivation_cost > demand_machinery_cost` (machinery = the demand table +
  its ingest + the guard join(s) + a forced intermediate materialization normal fusion would elide,
  ≥3 extra tables). **A message-rooted trivially-derived relation NEVER pays** (normal already indexes the
  bound column and answers with one probe). **Recursive demand is where pruning is ASYMPTOTIC** (a full
  closure vs one key's slice). The confirmed first slice (`r:-s,t`, a NON-recursive JOIN) sits BETWEEN:
  it prunes (bound A restricts s), but only in a **selective-query regime** (large relation, few distinct
  queried A). **=> The session-29 bench carrier MUST be engineered in the paying regime, and the grounding
  must HONESTLY confirm the non-recursive slice's pruning beats the machinery — or pull the recursive
  case (S4) forward. This is the first thing the spike must settle.** (§5 O1.)

---

## §1 Pipeline (`bin/drlojekyll/Main.cpp`) — unchanged
```
module  = ParseAndCombineModules(...)                          // lib/Lex + lib/Parse
query   = Query::Build(module, log, gPassPolicy)               // DataFlow IR (+Optimize+Stratify)
frozen  = FrozenRegionalProgram::Build(query, log)             // Regional model (compile-time)
program = Program::Build(frozen, log, gFirstId, gPassPolicy)   // ControlFlow(+Rel) over DataFlowGraph()
GenerateDatabaseCode(program, h_os, cc_os, ...)                // C++ codegen over lib/Runtime
```

---

## §A AS-IS: how a bound `#query` is served TODAY (a READ-time selector)
The AccessPlan machinery decides, AFTER materialization is complete, HOW to probe the settled table. It
never touches WHETHER rows are produced. (Anchors read at tip.)

```
// A.1 REGIONAL (lib/Regional/Planning.cpp, include/drlojekyll/Regional/RegionInstance.h)
SelectAccessPlan(req):                                         // RegionInstance.h:293-301
  if !req.has_free:                     return kFullKeyHashLookup    // all-bound -> Table::Find
  if req.available_bindings nonempty:   return kPartialKeyHashSeek   // bound+free -> Index::First/Next (P7)
  return kFullScanFilter                                             // no bound cols -> scan+filter
  // NO cost model; NO @key/declared_access_paths read (the P5<->P4 firewall is UP).
BuildRequestPorts(...):                                        // Planning.cpp:757-806
  per bound #query redecl -> RequestPortRecord{plan = ComputeQueryAccessPlan(decl,redecl)}   // :791
PlanFor(redecl) = request_ports.find((Id, BindingPattern)).plan   // :706-718 (nullopt if all-free)

// A.2 CONTROLFLOW (lib/ControlFlow/Build/Build.cpp:412-520) — the SINGLE consumption point
BuildQueryEntryPointImpl(decl, insert_view):
  col_indices    = bound-param indices
  plan           = context.frozen ? frozen->PlanFor(decl) : nullopt      // :446-447
  withhold_index = (plan == kFullScanFilter)                             // :448  THE DECISION
  scanned_index  = (!withhold_index && col_indices nonempty)
                   ? table.GetOrCreateIndex(col_indices) : nullopt       // index on BOUND subset
  assert V-PLAN-HONEST(plan, scanned_index)                             // per-kind implication belt
  impl->queries.push(ProgramQuery{decl, table, scanned_index, forcer, retract})
  // (BuildEmptyQueryEntryPointImpl :487 is the optimized-away-insert mirror, same shape.)

// A.3 CODEGEN (lib/CodeGen/CPlusPlus/Database.cpp:1527 EmitQueryFriends)
  via_index = spec.index.has_value()                                     // :1646 (plan-driven)
  bound+free cursor::next():
    if via_index:  id=pos; pos=index.Next(id); row=table.RowAt(id); ...  // :1668 SEEK, no re-check (P7c)
    else:          scan pos<table.NumRows(); filter row[i]!=bound -> skip // :1687 full-scan-filter
  cursor ctor: start = via_index ? index.First(key) : 0                  // :1729
```
**A in one line:** the bound value flows through ONE boolean (`withhold_index`) into the cursor and is
applied ONLY there — a post-hoc seek/filter over an already-fully-materialized relation.

---

## §B AS-IS: where the relation FULLY MATERIALIZES (plan-blind, the cost demand prunes)
Neither of these reads `context.frozen`/`PlanFor`/`AccessPlan` (grep-confirmed: those symbols appear ONLY
at Build.cpp:446-447/503-504/1381 — the §A sites). Every message materializes every reachable relation.

```
// B.1 EAGER DESCENT (acyclic) — lib/ControlFlow/Build/Procedure.cpp:14-124 -> Build.cpp:838/1138
ExtendEagerProcedure(io):                                    // once per received-message proc
  per receive: fold/loop the message rows, then
    BuildEagerInsertionRegions(receive, receive.Successors(), table)   // fills the EMPTY body
BuildEagerInsertionRegionsImpl(view, successors):            // Build.cpp:838
  InTryInsert(view)                                          // materialize this view's row
  for succ in successors: if !IsCutSuccessorDR(succ): BuildEagerRegion(view, succ, ...)
BuildEagerRegion(pred, view):                               // Build.cpp:1138 — dispatch by Kind
  JOIN->TABLEJOIN body | MERGE->union/inductive | MAP->generate | CMP->filter |
  SELECT/TUPLE-> ... ->BuildEagerInsertionRegions(successors) | INSERT->write TABLE (materialize!)
  // UNCONDITIONAL forward descent through the whole graph reachable from this message;
  // NO reference to any #query or AccessPlan.

// B.2 RECURSIVE FIXPOINT (cyclic) — lib/Rel/Stratum.cpp:1691 LowerRoundBody / :1799 LowerDRRounds
LowerRoundBody(round, scc_tables):                          // one INDUCTION per (SCC-group x phase)
  for t: VECTORCLEAR(Δ_t); for t: EmitClaimDrain(t); 
  for fire in kFixpointFire(group,sign): EmitJoinFire(fire)     // re-fire every recursive JOIN, Δ-over-Δ
  for fold in kChainFold(group,sign): EmitSeedLoop(fold)
  for t: EmitRetireFrontier(t)                                  // Δ-emptiness = break test
  // iterates each SCC to FULL Δ-quiescence — every derivable row computed, query-blind.
```
**B in one line:** production is driven purely by the graph + incoming rows; demand must inject the bound
constraint UPSTREAM of these two sites (fuse a guard into BuildEagerRegion's JOIN/INSERT body, or narrow
the LowerRoundBody frontier), instead of waiting for EmitQueryFriends' post-hoc cursor.

---

## §C RECOVERED: the deleted demand machinery (git `48cd0a4f`, deleted at P1 `dc965d3c`)
Two lowerings; the DataFlow graph was byte-identical between them (the instance lowering is a Rel/codegen
selector). Full recovered pseudocode: `session-28` recon transcript; distilled here.

```
// C.1 THE TRANSFORM — QueryImpl::ApplyDemandTransform (lib/DataFlow/Demand.cpp, 1464 lines)
//   gate: demand_mode OR any decl.HasInstanceKey(); else return (byte-identical off-mode).
1. collect the ONE bound #query q (>1 -> reject R-1BOUND); its relation p, adornment alpha.
2. TRACE q's projection chain to its full-width read of p's post-Connect MERGE -> p_bound positions.
3. SIP-LOCATE per MERGE member (= per rule body): find the guard site —
     recursive subgoal read at pos  -> kReadAtTuple | JOIN tracing to it -> kPushDown (JOIN-18) |
     base message atom -> kBaseAtom (JOIN-20). self-join / NEGATE / AGG on path / sideways -> REJECT.
4. FABRICATE demand__<q>_<alpha> message + a #local demand relation d_p (lib/Parse/Demand.cpp:
   real lex, kind-blind collision scan -> reject; differential attr if p is @differential).
5. MINT d_p = MERGE(root_member = TUPLE(receive of demand msg)  // the ROOT SEED
                    + one propagation member = TUPLE(project alpha off each demanding subgoal read));
   d_reader = TUPLE(d_p).
6. GUARD each site: JOIN(d_reader ⋈[pivot=pos] p_read)   // push-down guard, a REAL column edge (E-32)
   + RestoreOrder unless kReadAtTuple; stamp GuardAnnotation{kBody, forcing_index}.
7. QUERY-PROJECTION GUARD: JOIN(fresh raw_seed TUPLE(receive) ⋈[pivot=p_bound] q_read)  // answer filter
8. R-DUP grouped rewire (N adornments sharing (consumer,read) -> ONE MERGE-union rewire); TRIPWIRE:
   every d_p member traces to the receive, else abort. Register QueryDemandForcing{redecl, d_msg, alpha}.
// BEFORE: edge -> path(full closure) -> reachable_from(scan on From)
// AFTER:  demand__reachable_from_bf -> RECEIVE -> d_path MERGE -> d_reader; each path body GUARDED at its
//         SIP site; query projection guarded on the raw seed. Only rows a demanded answer needs materialize.

// C.2 THE INSTANCE LOWERING (-demand-instance): keyed InstanceStore<Key=alpha, Row=nested-cols>
//   Replaces the flat guard-join web with a per-key partition (Rel.cpp ResolveLiveRecognition:918 /
//   BuildSubgraphInstanceOps:1035; EmitSubgraphInstance in Database.cpp). Per epoch, band-ordered:
band-a1 BIRTH:   for k in demand_net_additions: iid=store.FindOrAddInstance(k);
                    if fresh: Touch; RESCAN(k)->current(iid)
band-a2 REBUILD: for r in input_net_additions: iid=store.FindInstance(r.key);   // FIND not add
                    if iid live && fresh: Touch; RESCAN(r.key)->current(iid)
band-b  PUBLISH: for iid in store.Touched(): publish (current(iid) \ frozen(iid)) -> pub; store.Seal()
RESCAN(k) = for row in input where row.key==k: current.TryAdd(row.value)   // ONE monotone pass (index.Find(k))
// SOUND only because non-recursive content reaches fixpoint in ONE body application (RESCAN not iterated).

// C.3 FENCES (clean diagnostics, single-adornment slice): >1 bound query; multi-clause query; all-free
//   sibling adornment; self-join; NEGATE/AGG on demand path; sideways 2nd adornment; stray consumer;
//   @key mismatches. -demand-instance ADDS: cyclic_demand (ViewSelfReachable) + recursive_content
//   (InductionGroupId) -> "recursive demand not yet supported" (the keyed lowering has NO fixpoint;
//   RESCAN is a single pass). This is the S4 gap (OQ-INDUCTION-UNION, DEFERRED).
```

---

## §D PATH FORWARD AS DIFFS (the first slice: `lookup(bound A,free C):-r(A,C); r(A,C):-s(A,B),t(B,C)`)
The spike (§5) decides Path R vs Path N by costing both to the first measurable pruning. Both must satisfy
the §E gate. Diffs are on the §A/§B/§C pseudocode.

### §D-R — Path R (resurrect + adapt the C.1 transform), sketch
```
+ // Restore lib/DataFlow/Demand.cpp (from git 48cd0a4f) + lib/Parse/Demand.cpp, adapted to tip:
+ //   - re-add the -demand flag (Main.cpp) OR the @key/cost activation gate; Query::Build regains the arg.
+ //   - the SIP transform mints d_r + guards r's ONE rule body at its kBaseAtom site (s(A,B) is the
+ //     bound-source atom) so only s(A,·) for demanded A joins t. NON-recursive: no InstanceStore needed
+ //     for slice 1 (the flat guard-join web already prunes — the store is S2+).
  // as-is (§B.1): BuildEagerRegion descends s->JOIN->t->INSERT r UNCONDITIONALLY for all A.
+ // after: the fabricated demand receive seeds d_r; the guard JOIN d_r ⋈ s filters s to demanded A before
+ //   the s⋈t join fires -> r materializes only demanded (A,C). EmitQueryFriends still seeks r (§A.3).
  // RISK: re-introduces the mode-gated DataFlow rewrite the greenfield cut removed; the transform mutates
  //   the IR that Optimize/CSE/dead-flow must stay blind to (the P1 motivation against it). PROVEN CODE.
```

### §D-N — Path N (native on the request-port/AccessPlan foundation), sketch
```
+ // Demand becomes a request-port ANNOTATION, no DataFlow rewrite, no fabricated shadow decls:
+ //   - the RequestPortRecord (already carries the bound query + AccessPlan) gains a DEMAND intent;
+ //     a new authority (a DemandPlan, peer of AccessPlan) records "materialize r keyed on alpha, seeded
+ //     by the request".
+ //   - the eager descent (§B.1 BuildEagerRegion) or the ingest, for a demand-planned relation, is
+ //     GATED on a demand-seed frontier: r's rule body fires only for rows whose alpha is demanded
+ //     (a guard fused at emission, driven by the DemandPlan — NOT a graph rewrite).
  // as-is (§B.1): the eager body is query-blind.
+ // after: BuildEagerRegion consults the DemandPlan for r; emits a demand-keyed guard/seed so only
+ //   demanded-A rows descend. The demand seed is a first-class AccessPlan input, not a smuggled #message.
  // RISK: larger design; the InstanceStore runtime + band-a1/a2/b lifecycle still need porting for S2+.
  //   ALIGNED with the greenfield goal (demand is a property of the typed request edge).
```

---

## §E EXIT GATE + IR desired-state sketch (predict-then-verify — session 29 builds this out)
Gate (all four; the FIRST answer-SET gate in this arc): **(1) result-equality** — the bound query's ANSWER
SET byte-identical ×4 modes (oracle/behavioral/I0-RefInterp); the derivation/fold count differs BY DESIGN.
**(2) codegen goldens MOVE** (`.rel`/`.ir`/`.h`). **(3) bench pruning** — a NEW selective carrier shows
fewer derived-rows/folds vs M3 (`bench/`), robust across the `.cost` scenario family, no non-selective
regression (or cost-gated). **(4) ctest 5/5, SUITE PASS** (non-demand corpus byte-identical).
```
// IR desired-state SKETCH (session 29 predicts exactly, then verifies):
//   .rel: a demand seed op (Path R: a fabricated-receive ingest + guard-JOIN dispatch arm;
//         Path N: a DemandPlan-driven keyed-materialization op) — NEW ops, census moves.
//   .ir:  r's eager insertion body gains a demand-guard gate (the s->JOIN body fires only for demanded A);
//         the query cursor (EmitQueryFriends) is UNCHANGED (still the P7 seek over the now-pruned r).
//   .h:   fewer rows inserted into table_r at runtime (a runtime effect, not a static byte — the static
//         change is the guard gate + the demand table/seed structure).
//   GOLDENS: the NEW carrier's .dr/.main.cpp(/.batches/.irgold); a .drflags/-demand or a cost-gate marks
//     it demand-ON. The 227 existing cases stay byte-identical (demand orthogonal to the 4 opt modes).
```

---

## §5 OPEN QUESTIONS the session-29 grounding must settle (in order)
- **O1 (FIRST, load-bearing):** does the NON-recursive slice's pruning actually beat the machinery cost in
  a realizable bench regime (§0 finding)? Engineer the carrier (large `s`/`t`, selective A, few queries)
  and MEASURE in the spike. If it can't clear the machinery, SAY SO and put "pull S4 (recursive demand,
  asymptotic pruning) forward vs re-scope the carrier" to the owner — do NOT land infra without a measured
  pruning.
- **O2 (the fork):** Path R vs Path N — the spike costs both to first pruning, commits with evidence (§D).
- **O3:** activation surface — a mode-gated `-demand` flag (Path R's natural form) vs a cost-model-driven
  automatic DemandPlan on the request edge (Path N's). Decide alongside O2.
- **O4:** the carrier's answer-net — `.batches` + oracle/monotone/behavioral so result-equality is refereed
  by four disjoint evaluators, and I0 RefInterp (demand-blind by construction) pins the definitional answer.
- **O5:** how much of the recovered C.1/C.2/C.3 (transform, InstanceStore, fences) is in the slice-1 blast
  radius vs deferred to S2–S5 (the roadmap in `session-29-demand-reopen-seed.md` §5). Keep slice 1 MINIMAL.
