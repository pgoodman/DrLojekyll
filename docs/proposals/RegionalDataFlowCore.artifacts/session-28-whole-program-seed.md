# Session 28 whole-program seed — POST-F16-close: make the model do SOMETHING REAL

> Cold-start START-HERE for session 28. Branch `keyed-instances`, tip `faef28e7` (F16-close),
> OptDiff **SUITE: PASS (227)**, ctest **5/5**. Every anchor below read at tip or carried from the
> session-27 P6.5 grounding (`p6.5-archA-grounding.md`) whose sonnet anchor mapped the M3
> fixpoint-emission path in full. clangd in this repo is NOISE — trust the real build.
>
> **THE MANDATE (owner, session 27): the next step must DO SOMETHING REAL.** Not another
> compile-time observer, `-region-out` token, or ctest-only model. The generated C++ must change
> in a way the bench harness MEASURES (answer-equal to M3, but observably different/faster) — or the
> answers must change. A cut whose exit gate is "bench-measured delta + answer byte-equality" cannot
> be a shadow by construction. That gate is non-negotiable for session 28.

---

## §0 STATUS + the bedrock finding

- **LANDED:** P1–P7 + P7b + P7c (the `AccessPlan` physical-access arc — partial-key seeks, the ONLY
  place the model drives codegen), P6.1 (`recursive_components`), P6.2 (`rules` +
  `inherited_symbolic_fields`), P6.3-detection (`fusion`/`binding_prefix`), **F16-close** (the fusion
  prefix is now a TRUSTWORTHY functional binding-preservation claim — session 27).
- **THE BEDROCK FINDING (session 27, three grounding loops):** the model-drives-codegen *payoff* is
  gated behind real prerequisites, so **every incremental step short of them is observer/shadow code**
  — the P9 → P6.4 → P6.5 pattern. Specifically:
  - P9 (access-path inference): consumer-less. DEFERRED. (`p9-inference-grounding.md`)
  - P6.4 (activation-edge derivation): the `RegionInstanceRelations` runtime-semantics half
    (`EvaluateEpoch`/`AddDerivation`/`activation_edges`) has **ZERO codegen consumers** — it's a
    parallel in-memory model in a different substrate than the product. SKIPPED as ctest-only
    scaffolding. (`p6.4-activation-edge-grounding.md`)
  - P6.5-arch-A (model drives the fixpoint codegen): grounded, **unanimous 3-refuter
    REFUTE-AS-FRAMED**. The smallest achievable slice (A0) is a certification *shadow*; the real
    divergence (A1) has **no pre-built codegen arm to flip** and is possibly *unrealizable* on the
    current all-hash runtime. (`p6.5-archA-grounding.md`)
- **F16-close (session 27) was the first NON-shadow step** — it fixed a *wrong* classification (a
  join-bound key was mis-called FUSED) rather than certifying a right one. But it is still
  compile-time/dump-only. Session 28 must go further: **change the generated code, measurably.**

---

## §1 WHOLE-PROGRAM PSEUDOCODE (as-is, grounded at tip)

### §1.0 Pipeline (`bin/drlojekyll/Main.cpp`)
```
module  = ParseAndCombineModules(...)                          // lib/Lex + lib/Parse
query   = Query::Build(module, log, gPassPolicy)               // :73  DataFlow IR (+Optimize+Stratify)
frozen  = FrozenRegionalProgram::Build(query, log)             // :89  Regional model (compile-time)
program = Program::Build(frozen, log, gFirstId, gPassPolicy)   // :110 ControlFlow(+Rel) — reads frozen.DataFlowGraph()
GenerateDatabaseCode(program, h_os, cc_os, ...)                // :142 C++ codegen over lib/Runtime
```
Four IRs (DataFlow `.df`, Rel `.rel`, ControlFlow `.ir`, Regional `.region`); codegen → `datalog.h`/`.cpp`.

### §1.1 The Regional model (compile-time) — what it knows, what drives codegen
```
FrozenRegionalProgram::Build(query):                          // lib/Regional/Planning.cpp
  relation_schemas  (R-STORE + Tier-2 origin; member_key contracts, declared_access_paths P5)
  request_ports / permanent_roots (P3; bound #query → RequestPortRecord{plan: AccessPlan})   // P4/P7
  recursive_components (P6.1: SCC projection via OriginDecls; members)
  rules + inherited_symbolic_fields (P6.2: clause-source RuleRoutingProjection + promotion fixpoint)
  RecursiveComponent.fusion/binding_prefix (P6.3 + F16-close: kFused{prefix} | kJoint)
```
**Only `AccessPlan` reaches codegen** — `frozen->PlanFor(decl)` at `Build.cpp:447/504` drives
`withhold_index` for a bound `#query`'s scan. `recursive_components`/`rules`/`fusion` are read by
NOTHING in `lib/ControlFlow`/`lib/CodeGen` (verified) — pure `-region-out` observers.

### §1.2 The M3 recursive-fixpoint emission (the target A1 would fork) — grounded map
```
DATAFLOW:  view.InductionGroupId() tags each recursive-SCC anchor view (== QueryView::Stratum()
           partition; Stratify.cpp:391-437 bijection assert proves it == P6.1's grouping, distinct id space).
TABLE-SCC: ComputeRecursiveSCCs (Stratum.cpp:185-243) → RecursiveSccMap (TABLE* → group id);
           copied into DR-IR at Rel.cpp:2122; drives RuleClass (kRecursive) + the round-shell mint.
ROUND-SHELL MINT (Rel.cpp:2351-2394, BuildDRInventory): per SCC GROUP, mint TWO DRRound shells
           {kOverdelete, kInsert}, each carrying scc_group + phase + test_vecs (the claimed-frontier
           vec of EVERY table in the group). ONE round-shell pair PER GROUP — self-recursion (1 table)
           and co-recursion (N tables) get the IDENTICAL shape.
EMISSION (Stratum.cpp):
  LowerDRRounds (:1799): walk OVERDELETE rounds; per group drained at this stratum → LowerRoundBody
    (OD) → REDERIVE → LowerRoundBody (INSERT, the paired round) → two deferred FRONTIER_FILTERs.
  LowerRoundBody (:1691): mint ONE INDUCTION region (:1699) whose cyclic body, per round:
      for table in scc_tables:  VECTORCLEAR its Δ-frontier               // fixpoint-test/break vec set
      for table in scc_tables:  EmitClaimDrain(table, sign)              // in-round CLAIM_DRAIN
      for op in kFixpointFire(this group,sign): EmitJoinFire(op)         // recursive JOIN, semi-naive Δ-over-Δ
      for op in kChainFold(this group,sign):    EmitSeedLoop(op)         // same-SCC internal projection re-fire
      for table in scc_tables:  EmitRetireFrontier(table, sign)          // clear Δ same-round bit
```
**Shape is determined PURELY by SCC-group membership — never by `fusion`.** `EmitJoinFire`
(`Stratum.cpp:781-814`) for a LINEAR recursion already emits an INDEXED delta scan (`scan edge where
edge.Y=Y`) on the lower non-SCC side; a NONLINEAR/same-table recursion uses a position-relative
claim-matrix dispatch. There is **NO alternate "fused" fixpoint shape anywhere** — `LowerRoundBody`/
`EmitJoinFire` are 100% agnostic to P6.3.

### §1.3 Runtime (`include/drlojekyll/Runtime/Table.h`, `hyde::rt`, all HASH)
`Table<Row>` (row store + membership predicates), `Index<Key>` (secondary hash, `First/Next`
FULL-KEY-EXACT), `StateCellStore` (agg/KV). NO ordered/range/trie structure (P8 would add one). The
recursive fixpoint fully materializes each SCC before any `#query` is served.

---

## §2 THE FOUR AUTHORITIES vs "does something real"

| Authority (RegionInstance.h) | Populated by | Drives codegen? | Real-cut relevance |
|---|---|---|---|
| `AccessPlan` (physical) | P4/P7/P7b/P7c | **YES** (query + interior scan) | the ONLY proven-real lever; P8 extends it (ordered trie) |
| `DeclaredAccessPath` / `BindingStateId` (P5) | P5 | no (hint) | P8 node interning; P9 (deferred) |
| `recursive_components`/`rules`/`fusion` | P6.1/6.2/6.3+F16 | no | A1 would make `fusion` drive the fixpoint arm — but no arm exists yet |

The lesson: a REAL cut either (a) extends the AccessPlan/physical lever (P7-style — proven real), or
(b) builds a genuinely NEW emission/runtime capability with a bench win, or (c) makes `fusion` drive a
real, measurably-different fixpoint emission. (a)/(b) are more certain than (c).

---

## §3 PATH FORWARD AS DIFFS — the candidate REAL cuts (session 28 triages, then builds ONE)

Every candidate's exit gate INCLUDES a **bench-measured delta** (`bench/runbench.sh`, methodology in
`bench/BASELINE.md`) proving the generated code observably changed, PLUS answer byte-equality
(`.stdout`/oracle/monotone/behavioral). No candidate lands on a structural gate alone.

### §3-C1 (PRIMARY candidate) — the FUSED-FIXPOINT KEYED DRAIN/SEEK
Use the now-F16-trustworthy `binding_prefix` to drive the recursive round's drain/fire to be KEYED on
the preserved prefix, reusing P7's `Index::First/Next` seek machinery — a P7-style "no new runtime
structure, selector-driven" win INSIDE the fixpoint.
```
  // as-is (§1.2): LowerRoundBody drains/fires each SCC table generically (full-frontier), fusion-agnostic.
+ // FIRST GROUNDING GATE (does-something-real triage): find a kFused carrier where M3 currently
+ //   FULL-SCANS/full-drains where a binding_prefix-KEYED seek would be answer-equal AND fewer rows.
+ //   Candidates: key_corecursion_1 (kFused (K), co-recursion — the joint round over p/q), corecursion_1.
+ //   If M3 is ALREADY keyed everywhere on the prefix (EmitJoinFire linear case is index-driven) →
+ //   NO real win here → PIVOT to §3-C2. This triage is the FIRST deliverable; do not build blind.
+ // IF a real win exists: thread a RecursiveEvaluationPlan (peer of AccessPlan, kFused{prefix}) from
+ //   frozen (context.frozen already reaches LowerRoundBody) via the RelationId↔TABLE*/scc_group BRIDGE
+ //   (build once per LowerDRRounds; sound per the Stratify.cpp bijection; the ONE genuinely-new helper),
+ //   and in LowerRoundBody select a KEYED drain/fire arm (GetOrCreateIndex(prefix) + First/Next) for the
+ //   kFused case. Answer-equal to M3; codegen goldens (.ir/.h/.rel) MOVE; bench shows fewer scanned rows.
  // GATE: SUITE answers byte-equal ×4 modes + oracle/behavioral; .ir/.h goldens move; BENCH delta measured.
```
Risk (from the P6.5 grounding): `EmitJoinFire`'s linear case is already index-driven, so the win may
only exist for the DRAIN or the co-recursive/nonlinear case — the triage must FIND it or pivot. This
is the smallest cut that could do something real AND builds directly on F16.

### §3-C2 (STRONG alternative) — P8: the ordered-trie / WCOJ join (the grounding's "real unblocker")
A genuinely NEW runtime capability with independent, bench-measurable value (worst-case-optimal joins),
and the memory's flagged perf direction (`seekable-iterators-wcoj`, `gpu-datalog-papers-assessment`:
LFTJ-as-interface). Heavier, but REAL by construction.
```
+ // Minimal first slice: ONE ordered/range runtime structure (hyde::rt, peer of Index<Key>) + ONE
+ //   WCOJ/leapfrog carrier where M3's binary hash-join is measurably beaten (a triangle/cyclic join),
+ //   answer-equal. BindingStateId node interning (P5 provides the schema spine). EmitJoin new arm.
  // GATE: answer byte-equal + a bench WIN on the WCOJ carrier vs M3's binary join.
```
This is the honest big-lever real cut; slice it minimally (one structure + one carrier + one bench win).

### §3-C3 (fallback) — a bench-triaged physical win elsewhere
If neither C1 nor C2 shows a clean minimal real win, let `bench/` FIND the corpus's worst generated-code
bottleneck and fix THAT with a landed analysis. "Real" is then grounded in the measurement itself.

### NOT next (settled this session)
- The `EvaluateEpoch` interpreter / `activation_edges` / DRed — different substrate, zero codegen
  consumers. NOT on any codegen path. (P6.4 skipped.)
- A0 fixed shadow (plan token on the INDUCTION line, codegen byte-unchanged). The mandate forbids it.
- P9 (deferred — firewall-relaxation prerequisite).

---

## §4 THE "DOES SOMETHING REAL" EXIT GATE (enforced, all candidates)

1. **Answer byte-equality** — every `.stdout`/`.oracle`/`.monotone`/`.behavioral` golden byte-identical
   across all 4 modes (the correctness net; a fixpoint/join re-emission that changes an answer is a
   MISCOMPILE, not a golden move — this is the FIRST answer-equality gate in the arc, treat it as such).
2. **Codegen goldens MOVE** — `.ir`/`.h`/(`.rel`) change (the generated code genuinely differs). A
   byte-unchanged codegen = a shadow = FAIL the mandate.
3. **Bench-measured delta** — `bench/runbench.sh` shows an observable difference on a directed carrier
   (ideally a win: fewer scanned rows / faster). Never time bench concurrently with the suite; bench
   builds are `-O2 -DNDEBUG`. Methodology: `bench/BASELINE.md`, `bench/README.md`.
4. **ctest 5/5**, OptDiff **SUITE: PASS**.

---

## §5 OPEN QUESTIONS the session-28 grounding must settle
- **C1:** does a real keyed-drain/seek win EXIST on a kFused carrier, or is M3 already keyed on the
  prefix everywhere (→ pivot)? What does the RelationId↔TABLE*/scc_group bridge cost, and is the
  `GetOrCreateIndex(prefix)` seek answer-equal in the differential (OVERDELETE/INSERT) rounds?
- **C2:** what is the minimal ordered/range ABI (`hyde::rt`) that doesn't disturb the hash-only
  compaction contract? Which corpus join is a genuine WCOJ win (a cyclic/triangle join)? Does P5's
  `BindingStateId` provide enough of the node-interning spine?
- **all:** what is the smallest directed bench carrier that MEASURES the win, and is the win robust
  across the `.cost` scenario family (memory `cost-scenario-family`) — not over-fit to one distribution?
- **triage first:** run C1's find-the-win probe (and a quick C2 join-shape scan) BEFORE committing —
  the mandate is to build something real, so prove the win exists before writing the emission.
