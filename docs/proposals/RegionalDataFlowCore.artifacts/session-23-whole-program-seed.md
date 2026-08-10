# Keyed-instance rewrite — POST-P6.2 whole-program pseudocode + path forward as diffs (session-23 seed)

Session 22 close (2026-08-10). Branch `keyed-instances`. **P1 + P2 + P3 + P4 + P5 + P6.1 + P6.2 ARE
LANDED** (compile-clean, OptDiff **SUITE: PASS (226)**, ctest **5/5**). **The P6 COMPILE-TIME first cut
is COMPLETE.** This seed supersedes `session-22-whole-program-seed.md` §1 for the Regional layer — P6.2
added the `rules`/`inherited_symbolic_fields`/`SymbolicFieldId` routing authority + its `-region-out`
render. It is the START-HERE backbone for **session 23**, whose headline is a FORK — RANKED in §3 as **P7
(physical cut) #1, the runtime cut (P6.3–P6.6) #2** — to confirm with the owner at the [OWNER STOP].

**GREENFIELD RULING (owner), still governing THROUGH P6.2.** Every gate so far is STRUCTURAL, never
answer-equality (the M3 backend answers correctly; the regional model is a compile-time observer).
**The RUNTIME cut (P6.3+) is where this ends** — real evaluation makes answers observable and
answer-equality gates (cross-checked vs M3) legitimate. Memory `greenfield-rewrite-motivation`.

## §0. Status — what P6.2 changed, what is now populated vs dormant

**Landed at P6.2 (full record `p6.2-grounding.md`):**
- `struct RuleRoutingProjection { RuleId id; RelationId head;
  std::vector<std::pair<SymbolicFieldId,SymbolicFieldId>> body_to_head; }` (Regional.h — replaced the
  empty stub). `RegionTemplate::rules` + `inherited_symbolic_fields` POPULATED. NEW interner
  `RegionInstanceRelations::symbolic_field_table` (+ `next_symbolic_field`, `InternSymbolicField`,
  `SymbolicFieldOf`) — a region-global `SymbolicFieldId` per `(relation, ordinal)`, peer of the P5
  `schema_table`, DISTINCT domain.
- Three anon statics in `Planning.cpp`, wired into `Build` after the P6.1 populate, before census:
  `AssignSymbolicFields` → `R.rules = BuildRuleRoutingProjections` → `PromoteSharedSymbolicField`.
  CLAUSE-SOURCE (the headline — DataFlow loses per-relation field identity via CSE; §1.1 of the
  grounding doc). Promotion is a `while(changed)` union-find FIXPOINT (F28), directional per-head-field,
  F16 co-occurrence trap closed. Negated/aggregate/@product body atoms excluded (Q5, sound
  under-approximation).
- `Format.cpp` renders gated own-width `rule`/`shared-field` blocks after the recursive-component
  block, NO census count. Codegen BYTE-UNCHANGED (nobody outside `lib/Regional` reads the routing
  fields; `Program::Build` reads `DataFlowGraph()`).
- Carriers: NEW `key_corecursion_1` (@key co-recursion, F16 arm) + `fixpoint_force` (F28 referee); 30
  region goldens gained a pure suffix; 2 new cases. SUITE 224→226.

**Post-P6.2 honest baseline for session 23 (what is now populated vs still dormant):**
- POPULATED compile-time authorities: RegionalFactId (P3), BindingState-empty (P3),
  AccessPlan (P4, the ONLY codegen-driving one), DeclaredAccessPath + per-relation binding-schema DAG
  (P5), recursive_components (P6.1), rules + inherited_symbolic_fields + SymbolicFieldId (P6.2).
- STILL DORMANT (ctest-only synthetic rows — the P6.4+ RUNTIME half): `activation_edges` (RESERVED-
  EMPTY), `AddDerivation`/`RouteResults` (zero real-compile callers), `RootedReachability` (single
  topo sweep, no propagation). `DeriveActivationEdge` still does NOT exist (a prose comment).
- The actual recursive EVALUATION is STILL the retained M3 backend (ControlFlow/Build/Stratum.cpp
  OVERDELETE→REDERIVE→INSERT over split `C_nr`/`C_r`). Nothing in codegen reads the regional routing.
- `BindingStateId.vals` stays `{}` (value-free); only the empty state `{0}` is interned.

## §1. The whole program TODAY (POST-P6.2) — pseudocode

### §1.1 Pipeline (bin/drlojekyll/Main.cpp) — anchors current at tip
```
compile(module, flags):
    query   = Query::Build(module, log, gPassPolicy)              # DataFlow IR (3-arg since P1)
    frozen  = FrozenRegionalProgram::Build(query, log)            # Regional model: P2 typed owner + P3
                                                                   #   request/derivation + P4 AccessPlan +
                                                                   #   P5 paths/DAG + P6.1 recursive_components
                                                                   #   + P6.2 rules/SymbolicFieldId
    SetRelDumpStream(gRelStream)
    program = Program::Build(frozen, log, gFirstId, gPassPolicy)  # ControlFlow IR (reads frozen: P4 PlanFor)
    GenerateDatabaseCode(program, h, cc)                          # C++ codegen (M3 backend)
```

### §1.2 Regional — FrozenRegionalProgram::Build (P2..P6.2) [POST-P6.2]
```
Build(query, log):                                               # lib/Regional/Planning.cpp
    out.dataflow_graph = query; R = out.region; RR = out.instances
    input/result ports + ABIs (declaration order)                # P2
    BuildRequestPorts: bound #query -> RootLease request-port + AccessPlan; all-free -> PermanentRoot  # P3+P4
    R.relation_schemas += BuildRelationSchemaFromInsert/Origin (+ P5 declared_access_paths)            # P2+P5
    for schema: for p in declared_access_paths: RR.MaterializePrefixChain(p)                           # P5 DAG
    R.recursive_components = ComputeRecursiveComponents(query, R.relation_schemas)                     # P6.1
    AssignSymbolicFields(R, RR)                                                                        # P6.2
    R.rules = BuildRuleRoutingProjections(R, RR)                 # clause-source, frozen-relation filter# P6.2
    PromoteSharedSymbolicField(R, RR)                            # while(changed) union-find fixpoint   # P6.2
    out.census = DeriveRegionalCensus(query); RECOUNT belt (no P6.1/P6.2 census token)
    RunFreezeValidators: V-FROZEN-NO-OPEN-PORT, V-OWNERSHIP-ACYCLIC, V-PLAN-HONEST(P4), V-PREFIX-CHAIN(P5)
    return out
# rules is the SOLE routing authority; PromoteSharedSymbolicField is the future P6.3 fusion input.
```

### §1.3 ControlFlow + codegen — unchanged since P4
```
Program::Build(frozen,…): query=frozen.DataFlowGraph(); context.frozen=&frozen (P4 PlanFor read)
    …build DR flow, induction fixpoint (Stratum.cpp), commit sweeps…   # the retained M3 backend
    BuildQueryEntryPointImpl: plan=frozen.PlanFor(redecl); withhold index iff kFullScanFilter (P4)
# P5/P6.1/P6.2 added NOTHING here. Codegen is the M3 backend; the regional model is a compile-time observer.
```

## §2. The target — authorities + the two edge kinds (carried)
```
Logical fact          RegionalFactRelation : RegionalFactId -> RegionalFact  (SINGLE fact authority)
Residual              BindingState (schema + typed values + FactDerivation ids)
Logical access path   DeclaredAccessPath (ORDERED; [A,B] != [B,A])           (P5 LIVE)
Order-free schema     per-relation binding-schema DAG                        (P5 LIVE; P8 rebuilds region-global)
Physical structure    AccessPlan (FullScanFilter | FullKeyHashLookup | …trie)(P4 live; P7 extends real hash/trie)
Region field identity SymbolicFieldId promoted only when EVERY producer agrees(P6.2 LIVE — routing)
Recursive structure   recursive_components (SCC)                             (P6.1 LIVE)
RequestEdge   exact ownership, ACYCLIC forest        RuleActivationEdge  derivation dep, MAY cycle (P6.4 — DORMANT)
```

## §3. The path forward as diffs — THE FORK (RANKED: P7 first; confirm with the owner at the [OWNER STOP])

> **RANKING (session-22 close): P7 #1, the runtime cut (P6.3–P6.6) #2.** Reasoning in the
> "Recommendation" block at the end of this section. In short: P7 is closer to a PREREQUISITE than an
> alternative — P6.3 fusion's payoff (co-recursive relations sharing ONE frontier keyed on the common
> prefix) is a PHYSICAL arrangement, exactly what P7 builds; doing the runtime cut first fuses onto a
> placeholder full-scan and gets re-touched at P7 anyway. P7 also keeps the structural-gate cadence
> that landed P2–P6.2, is already deeply critiqued (s14-15), and finally makes a narrow `@key` SEEK
> (closing the branch's namesake arc). The ranking is the owner's to confirm.

### (A) The RUNTIME cut — P6.3–P6.6 (real evaluation; TOUCHES codegen/runtime; answer-equality gates begin) [RANKED #2]
Authority: `keyed-rewrite-reconstruction-diffs.md §3-P6.3..P6.6` (lines ~579+) + `next-session-prompt.md`
Phase 6 steps 3-6 + memory `mobius-differential-dataflow` / `free-termination-paper`. The LARGEST,
riskiest cut — the honest end state (replace M3) but MUST be heavily sub-sliced. Sketch:
```
# P6.3 PlanRecursiveComponent(scc): P = CommonPreservedPrefix(RuleRoutingProjection of rules in scc)
#      over the PROMOTED SymbolicFieldId classes (P6.2 is the input, AFTER its fixpoint — F28);
#      P nonempty & all routes preserve P -> FusedFixpoint(binding_prefix=P) else JointFixpoint(scc).
#      Fusion is a PROVEN-SAFE opt: cross-check FusedFixpoint vs JointFixpoint on the SAME dataset
#      (answer-equality, now legitimate) + the STRUCTURAL "one frontier per K" check SEPARATELY.
# P6.4 DeriveActivationEdge(src_state, src_fact, rule, dst): LIFT the P3 acyclic restriction; dst
#      bindings = RuleRoutingProjection(rule) image of src_fact (P6.2), never a raw column copy;
#      intern into activation_edges (MAY cycle; NEVER an AddRequestEdge; F18 no intra-state self-loops).
# P6.5 EvaluateEpoch: TWO passes — (A) within-epoch support-loss DRed per-FACT on RegionalFactId (B4),
#      mirroring Stratum.cpp OVERDELETE->REDERIVE->INSERT; interleaved with (B) joint rooted-reachability
#      + semi-naive worklist as ONE signed-frontier worklist across components (B5).
# P6.6 RetireUnreachableSCCs: liveness RE-DERIVED (never refcount), drain-before-retire, H7 cascade-retract.
```
Exit-gate character: answer-equality vs the M3 backend BECOMES legitimate (cross-check), PLUS structural
(one-frontier-per-K, no-refcount liveness, acyclic-request/cyclic-activation separation). Flag to the
owner: this is where the greenfield structural-only posture ENDS.

### (B) The PHYSICAL cut — P7 (then P8/P9) (keeps the compile-time-observer posture one more layer) [RANKED #1]
Authority: `keyed-rewrite-p7p9-diffs.md` + `keyed-rewrite-p7p9-critique.md` (deeply critiqued s14-15).
```
# P7  AccessPlan its own domain with REAL hash/trie paths — where a narrow @key first PAYS.
#     kRetainedIndexScan/kFullKeyHashLookup/trie become COST decisions; the intra-relation prefix seek
#     is P7 GetOrCreateIndex(subset) (bound-subset hash index; prefix-share + [A,B]/[B,A] convergence FREE
#     via order-free dedup). AccessPlan DUAL-HOMES (interior/join scans). STRUCTURAL gate: a trie/partial
#     plan must NOT execute the old whole-table rescan (the s14-15 blocking corrections: thread plan_kind
#     at EVERY scan mint; intern trie nodes on BindingStateId; re-source P9 order from a DataFlow signal).
# P8  the cross-relation ORDERED trie / COLT / Free Join — REBUILDS the P5 binding-schema spine
#     region-global over SymbolicFieldSet (the P6.2 atoms), NEVER migrates schema_table.
# P9  access-path inference (additive, logical-only, PRE-Optimize) + Minimize/DeterminedBy.
```
Exit-gate character: STILL structural (greenfield discipline intact) — a trie plan proven to not rescan;
codegen changes are honest-plan reads (the P4 precedent), answer-invariant vs the M3 backend.

**Recommendation (RANKED, session-22 close): P7 #1, the runtime cut #2. Confirm with the owner.**

**Why P7 first (four arguments, strongest first):**
1. **Dependency ordering — the clincher.** P6.3 fusion is CHOSEN from the common binding prefix over the
   promoted `SymbolicFieldId` classes (P6.2). But the PAYOFF of fusing — co-recursive relations sharing
   ONE fixpoint frontier keyed on that prefix — is a PHYSICAL arrangement (a keyed index/trie), which is
   exactly what P7 builds. Do the runtime cut first and the fused plan fuses onto the placeholder
   full-scan (the fusion is NOMINAL) and gets re-touched at P7 anyway. P7 is closer to a PREREQUISITE
   for the runtime cut to be worth doing than an alternative to it. (Precise: P6.3 *detection* is
   P7-independent, but P6.3+ *realization* + P6.5 evaluation over shared arrangements need P7's physical
   layer to be non-nominal.)
2. **Cadence preservation.** The structural-gate compile-time-observer loop has landed 10+ phases cleanly
   (P2→P6.2). P7 stays inside it (structural gate = "a trie/partial plan must NOT execute the old
   whole-table rescan"; codegen changes are honest-plan reads, the proven P4 pattern). The runtime cut
   deliberately BREAKS the cadence (answer-equality gates begin; the dormant activation/derivation half
   goes live for the first time on real programs). Don't break a working process before the substrate it
   needs is real.
3. **Risk-adjusted + already grounded.** P7 is one bounded layer, deeply critiqued s14-15
   (`keyed-rewrite-p7p9-diffs.md`/`-critique.md`: 15 survivors, 3 blocking corrections folded) — only the
   anchors need RE-verification at tip. High probability of a clean 1–2 session landing. The runtime cut
   is unbounded, regime-changing, realistically multi-session with intermediate non-green states.
4. **Closes the branch's namesake arc.** `keyed-instances` is about `@key` becoming a real specialization:
   P5 = logical meaning, P6.2 = routing, **P7 = a narrow `@key` finally SEEKS instead of scans** — the
   first time the accumulated model yields FASTER generated code. Demonstrable, satisfying, loop-closing.

**The one condition that flips it (record it):** if the owner's priority is explicitly "prove the
architecture EVALUATES end-to-end" over "make it faster / finish the @key arc," do NOT take the whole
runtime cut — spike a **compile-time-only P6.3 fusion-DETECTION slice** first (choose `FusedFixpoint` vs
`JointFixpoint` at compile time, render it in `-region-out`, codegen unchanged, STRUCTURAL gate). That
stays in the winning observer cadence, de-risks the big cut, and confirms the P6.2 routing model is
sufficient before touching runtime — the safe hybrid. Otherwise: **P7 next, then the runtime cut once the
physical layer is real (and even then, sub-slice it hard, starting with P6.3 detection).**

## §4. What session 23 should do (the grounding loop)
1. **Ground the whole program at POST-P6.2** (this seed is the backbone — keep it current). Re-verify
   every anchor for the CHOSEN cut at tip (the P6.3+ reconstruction-diffs anchors PREDATE P1–P6.2; the
   p7p9-diffs anchors were last verified s15 at a different tip — RE-GROUND before trusting).
2. **CONFIRM the ranking with the owner** at the [OWNER STOP]: the seed recommends **P7 #1** (§3
   Recommendation — the dependency argument is the clincher: P7 is closer to a prerequisite than an
   alternative). Present it as a recommendation to ratify/override, not an open fork; note the one
   flip condition (a "prove it evaluates" priority → a compile-time-only P6.3 detection spike first).
3. For the chosen cut: run the **build-pseudocode → design-goal diffs → adversarial EMPIRICAL critique →
   IR-desired-states** loop via WORKFLOWS (opus for diffs/critique/judgment; sonnet for mechanical
   census/carrier-dumps/anchor re-verification). VERIFY EMPIRICALLY by compiling throwaway carriers.
4. Execute only if green-lit, gate GREEN, one-or-few coherent commits; update CLAUDE.md + memory +
   write the session-24 seed/prompt.

METHOD: WORKFLOWS. Docs-only unless the owner ranks the fork AND green-lights execution (the [OWNER STOP]).
All P6.2 anchors are grounded at tip; the P6.3+/P7-P9 anchors need RE-grounding (they predate the recent
phases). Watch the `(await parallel(...)).filter(...)` precedence (session 22's one workflow bug).
