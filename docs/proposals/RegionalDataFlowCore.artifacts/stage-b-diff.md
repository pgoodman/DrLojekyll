# Stage B as diff hunks — the regional representation becomes canonical

Design session, 2026-08-02, branch keyed-instances, tip f0c913e0. This
document expands `regional-arch-pseudocode.md` §6 "Stage B" into an
implementation-grain, hunk-by-hunk diff against that pseudocode's §1–§5.
It is the sole deliverable of this session: no production code, no golden
changes, no edits to existing files.

Scope (proposal §13 Stage B, pseudocode §6 Stage B):

1. Add `PlanningRegionalProgram`, `FrozenRegionalProgram`, and the
   structural typed ids.
2. Represent the existing whole program as `ProgramRoot` + ONE
   observation-root region template, WITHOUT child extraction.
3. Move local optimization / stratification / formatting / validation
   *through* the regional representation (a facade rename at Stage B —
   the passes run once, in the existing order, byte-preserving).
4. Make Rel consume only `FrozenRegionalProgram`.

Exit gate (the whole stage): all 180 goldens byte-identical (pure
representation refactor); a NEW regional dump surface pinned deterministic
for the witness set. This is provable because Stage B is a THIN wrapper —
see H3 for why "consumes only FrozenRegionalProgram" is realized as an
accessor over the identical `QueryView` graph, not a re-plumbed traversal.

The four load-bearing design questions the charge names are resolved as:
(a) `ExtractPrimaryProcedure` — PRESERVED unchanged (H8);
(b) regional dump surface — flag + slot chosen (H6), grammar left
    OWNER-GATED with three candidates (H7);
(c) thin vs thick — THIN chosen and justified (H3);
(d) validators here vs Stage C — three land as near-vacuous scaffolds,
    the purity/lifecycle census validators are Stage C (H9, ESC-1).

---

## Terminology contract for the hunks

Two placements were available for the planning/freeze build. Pseudocode
§6 shows it at the **Query::Build tail, returning `frozen`**; the
alternative is a **new top-level step in `main`** between `Query::Build`
and `Program::Build`, leaving `Query::Build`'s return type alone. H1
picks the pseudocode's placement and H1-ALT records the ripple-minimizing
alternative as an OWNER-GATED sub-decision, because the choice changes
which files a golden-neutral refactor touches (return-type ripple across
every `Query::Build` caller and test vs a localized `main` edit).

Throughout, "the single region" = `RegionId(0)`, the one observation-root
`FrozenRegionTemplate` whose `body` is the whole finalized `QueryView`
graph. At Stage B there is exactly one region, zero child calls.

---

## H1 — Query::Build tail: build planning, freeze, return frozen

**Anchor:** pseudocode §1 `Query::Build` tail (`lib/DataFlow/Build.cpp`
:2631-2637, verified this session — real tail is `BuildEquivalenceSets →
Stratify → return Query(impl)`).

```diff
   BuildEquivalenceSets(impl.get())                 # :2631
   impl->Stratify(log)                              # :2632
   if num_errors != log.Size(): return nullopt      # :2633-2635
-  return Query(std::move(impl))                    # :2637
+  Query query(std::move(impl))
+  planning = BuildPlanningRegionalProgram(query)   # NEW — degenerate:
+      # one observation-root region, body = the finalized local graph,
+      # NO extraction, NO re-optimization (see H2 + ESC-2)
+  frozen   = FreezeAndValidate(planning, log)      # NEW — distinct TYPE;
+      # no mutator from frozen back to open; runs the H9 freeze validators
+  if num_errors != log.Size(): return nullopt
+  return frozen                                    # return type CHANGES
+                                                   #   Query -> FrozenRegionalProgram
```

**Ordering constraint (preserved):** the planning build slots strictly
AFTER `Stratify`, so a (still-fabricated, Stage-B) demand relation's
stratum is settled before it is frozen. This matches pseudocode §6's
NOTE and §1's warning that a rewrite moving the demand mint relative to
`Stratify` is forbidden — Stage B moves NOTHING relative to `Stratify`;
it appends after it.

**Return-type ripple:** `std::optional<Query>` → `std::optional<Frozen
RegionalProgram>`. Every caller (`main` at Main.cpp:69-70, and any test
that calls `Query::Build` directly) must adapt. H1-ALT avoids this.

### H1-ALT (OWNER-GATED) — placement variant

- **Variant 1 (pseudocode, H1 as written):** `Query::Build` returns
  `FrozenRegionalProgram`. Pro: matches the normative pseudocode; the
  frozen program is the *only* thing that leaves the DataFlow layer.
  Con: return-type ripple across all callers/tests.
- **Variant 2 (localized):** `Query::Build` unchanged (returns `Query`);
  add a free `FrozenRegionalProgram::Build(const Query&, const ErrorLog&)`
  called from `main` between the two Builds. Pro: zero ripple, smaller
  golden-neutral diff, `Query` stays a usable intermediate for the -df/
  -dot/-ir drains (H6). Con: `Query` remains a public post-DataFlow
  object, softening "the frozen program is canonical."

Owner picks. The rest of this document is written against Variant 1's
types but every hunk is placement-agnostic except H6's dump-slot detail.

---

## H2 — BuildPlanningRegionalProgram: the degenerate (no-extraction) planner

**Anchor:** NEW function; conceptually the Stage-B instance of proposal §8
`BuildPlanningRegionalProgram`. Lives in a new `lib/Regional/Planning.cpp`
(new compiler-internal static library `lib/Regional`, peer of `lib/Rel`).

```
+ BuildPlanningRegionalProgram(query) -> PlanningRegionalProgram:
+     # Stage-B DEGENERATE FORM. The proposal §8 planner loop
+     # (OptimizeLocalGraphToFixpoint per region + FirstStableAdmissible
+     #  Child extraction) is Stage C/D. Here:
+     root   = BuildProgramRootShell(query)     # sealed query/message ABIs
+     region = OpenRegionTemplate{
+                 id: RegionId(0),
+                 owner: ProgramRoot,
+                 parameters: (none),
+                 request_port:  the observation root's declared query ABI,
+                 input_ports:   the program's received-message ABIs,
+                 result_ports:  the program's published-message ABIs,
+                 body: PureRegionalGraph wrapping `query` (the WHOLE local
+                       graph — see ESC-1: it is NOT yet effect-pure),
+                 children: (none),          # <- WITHOUT child extraction
+                 row_contracts: from Stage A's InferConservativeRowContracts
+                                (identity pass-through if Stage A not landed),
+                 requirements: (none solved; no ports negotiated)
+              }
+     return PlanningRegionalProgram{
+                root, {RegionId(0): region}, logical_origins = identity }
```

**Critical resolution (ESC-2, latent conflict):** proposal §8 runs
`OptimizeLocalGraphToFixpoint(region)` and `SolveBackwardRequirements`
*inside* `BuildPlanningRegionalProgram`. The current pipeline already ran
`Optimize` mid-`Query::Build` (before `Stratify`). Re-running Optimize
inside the planner would perturb the graph and BREAK the byte-identical
gate. **Stage B therefore does NOT re-optimize.** The single region's
body is the ALREADY-finalized (`Optimize` + `Link` + `Identify
Inductions` + `Finalize*` + `Stratify`) graph, wrapped as-is. The
proposal's per-region optimize loop is deferred to Stage C/D where
extraction first creates a child whose local graph legitimately re-
optimizes. This is a stated deviation from proposal §8's literal
control flow, forced by the Stage B exit gate; it is recorded as ESC-2
for owner ratification.

`logical_origins` is the identity map at Stage B (`LocalNodeId` ==
the wrapped view's own id); it exists so Stage C/D clones carry origin.

---

## H3 — FrozenRegionalProgram + the Query→frozen adapter (THIN Stage B)

**Anchor:** NEW types (proposal §3); the mechanical answer to charge (c),
"what does 'Rel consumes only FrozenRegionalProgram' mean given
`BuildDRInventory` walks the `QueryView` graph today."

```
+ FreezeAndValidate(planning, log) -> optional<FrozenRegionalProgram>:
+     run the H9 freeze validators over `planning`
+     if any fail (log.Append clean diagnostic): return nullopt
+     freeze each OpenRegionTemplate -> FrozenRegionTemplate (ports become
+        Frozen*Port; NO mutator back to open exists)
+     return FrozenRegionalProgram{ root, roots=[RegionId(0)],
+                regions, logical_origins }

+ FrozenRegionalProgram::RootRegion() -> const FrozenRegionTemplate&
+ FrozenRegionTemplate::LocalGraph() -> const Query&      # THE THIN SEAM
+ FrozenRegionalProgram::Query()     -> const Query&      # = RootRegion().LocalGraph()
```

**Chosen shape: THIN.** `FrozenRegionTemplate::body` OWNS the wrapped
`Query` (which owns the `shared_ptr<QueryImpl>`). `LocalGraph()` hands
back a `const Query&` over the IDENTICAL `QueryView` graph. Rel's
`BuildDataModel` / `BuildDRInventory` traverse `query.ForEachView()` /
`query.Inserts()` / `query.Merges()` / `query.Joins()` / … through this
accessor — the SAME `DefList` walks, in the SAME order, over the SAME
node objects.

**Why the 180-golden gate stays provable:** the .rel / .ir / .df / .dot
dumps and all codegen are pure functions of the `QueryView` graph and its
traversal order. A thin accessor changes neither. `git diff` on the
generated text is empty by construction because no node is renumbered,
reordered, or re-optimized. The proof obligation reduces to "the accessor
returns the same `Query` the old `Program::Build` received" — a one-line
identity, mechanically checkable.

**Thick Stage B (REJECTED, recorded):** a re-plumbed traversal where the
frozen region exposes its own `FrozenPureRegionalGraph` node vocabulary
that Rel walks instead of `QueryView`. This is the eventual Stage D
shape, but at Stage B it would require re-deriving the DR inventory over
a new graph type and re-proving byte-equality of a DIFFERENT traversal —
a proof that does not reduce to an identity and would almost certainly
perturb some tie-broken order. Rejected for Stage B precisely because the
byte-identical gate becomes unprovable-by-inspection. The thin/thick
boundary is the honest Stage-B/Stage-D line.

---

## H4 — Program::Build: consume FrozenRegionalProgram

**Anchor:** pseudocode §1 `Program::Build`
(`lib/ControlFlow/Build/Build.cpp`:1308-1311, verified: signature is
`Program::Build(const ::hyde::Query &query, const ErrorLog&, unsigned
first_id, const PassPolicy&, bool demand_instance)`).

```diff
-  std::optional<Program> Program::Build(const ::hyde::Query &query,
-                                        const ErrorLog &log, unsigned first_id,
-                                        const PassPolicy &policy,
-                                        bool demand_instance)
+  std::optional<Program> Program::Build(const FrozenRegionalProgram &frozen,
+                                        const ErrorLog &log, unsigned first_id,
+                                        const PassPolicy &policy,
+                                        bool demand_instance)
   {
+    const Query &query = frozen.Query();   # THE THIN SEAM (H3)
     # ... the C-2 pre-pass, the demand_instance guard-bucket fences,
     #     BuildDataModel(query, program), the whole §7 rel-arch pipeline
     #     — ALL UNCHANGED, reached through `query` exactly as before.
```

Everything after the first line of the body is byte-preserved: the C-2
V-ALGEBRA pre-pass (:1332-1411), the `demand_instance` guard fences
(:1428-1473), `BuildDataModel` (:1499), the §7 `BuildDRInventory →
validators → Lower*` pipeline. `Program::Build` still receives a
`Query`; it just fishes it out of `frozen` instead of taking it directly.

**Main.cpp call-site edit** (pseudocode §1 :81-83):

```diff
-  auto program_opt = Program::Build(*query_opt, error_log, gFirstId,
-                                    gPassPolicy, gDemandInstance);
+  auto program_opt = Program::Build(*frozen_opt, error_log, gFirstId,
+                                    gPassPolicy, gDemandInstance);
```

where `frozen_opt` is `Query::Build`'s return (H1 Variant 1) or the H1-ALT
Variant 2 free build. `gPassPolicy.bisect_counter` (pseudocode §1 :67, the
one cross-Build mutable state) still threads through the SAME
`gPassPolicy` global — the frozen wrapper carries no policy state and does
not intercept the thread. Exit gate for this hunk explicitly re-asserts
the bisect thread survives (a `-bisect`-driven run must still cut at the
same op count).

---

## H5 — "moved through the regional representation": Optimize/Stratify/Format/Validate facade

**Anchor:** pseudocode §1 `Query::Build` body passes + the format/validate
surfaces. Charge item 3 ("Move local optimization, stratification,
formatting, and validation through the regional representation").

At Stage B this is a REPRESENTATIONAL move, not an operational one. The
existing passes run exactly once, in the existing order; the region merely
becomes the nominal owner of the graph they act on.

```
  # OPTIMIZE / STRATIFY — NO reordering (ESC-2). The region-scoped facade
  # is documentary at Stage B: OptimizeLocalGraphToFixpoint(RegionId(0))
  # == the impl->Optimize already called mid-Query::Build; Stratify stays
  # the existing impl->Stratify. Neither is re-invoked by the planner.

  # FORMATTING — three surfaces today (pseudocode §1):
  #   -ir-out  : operator<<(*program)        AFTER Program::Build  (needs TableId)
  #   -df-out  : QueryDF{query}              AFTER Program::Build  (needs TableId)
  #   -dot-out : DOT of query                AFTER Program::Build  (TableId only)
  #   -rel-out : SetRelDumpStream sink       BEFORE Program::Build (built inside)
  # Stage B ADDS a fourth (H6) and does NOT touch these four — they still
  # take `query` (= frozen.Query()); their goldens stay byte-identical.

  # VALIDATION — the §7 rel-arch validators (V-XOVER-ONE … V-PRED-XCHECK,
  # the 29-kind census) run UNCHANGED inside Program::Build over the same
  # graph. Stage B ADDS the H9 freeze validators at FreezeAndValidate; it
  # deletes NONE.
```

No diff hunk deletes or reorders a pass here — the "move" is that the
region template is now the declared home of the local graph these passes
consume. Recorded so an implementer does not mistake charge item 3 for a
license to relocate `Optimize`/`Stratify` (which ESC-2 forbids at Stage B).

---

## H6 — the regional dump surface: flag + slot (grammar deferred to H7)

**Anchor:** pseudocode §1 dump-timing split (:48-53) + Main.cpp
:57-60/:90-93/:130-133/:315/:364/:381 (verified this session:
`gRelStream` installed via `SetRelDumpStream` at :79 BEFORE
`Program::Build`; `gIRStream`/`gDFStream` drain at :90-93/:130-133 AFTER;
`-rel-out`/`-ir-out`/`-df-out` argv wiring at :381/:315/:364).

**Chosen flag + slot (grammar-independent):**

```diff
   # Main.cpp globals (:57-60)
+  static OutputStream *gRegionStream = nullptr;   # -region-out (name: H6-ALT)

   # Main.cpp drain — NEW, a THIRD timing slot:
+  if (gRegionStream) {                            # AFTER Query::Build,
+    (*gRegionStream) << FrozenRegionalDump{*frozen_opt};   # BEFORE Program::Build
+    gRegionStream->Flush();
+  }
```

**Why a third slot, distinct from both existing shapes:** the frozen
program is COMPLETE at `Query::Build` return and references NO `TableId()`
(tables are minted in `Program::Build`). So it needs neither the
-rel-out BEFORE-Program shape (which exists because the Rel flow is built
and drained *inside* `Program::Build`) NOR the -ir/-df AFTER-Program shape
(which exists because those dumps read `TableId()`). The regional dump is
correctly placed AFTER `Query::Build` / BEFORE `Program::Build` — a slot
neither existing surface occupies. This is the direct answer to charge
(b)'s "where does the regional dump slot."

Under H1-ALT Variant 2 the drain sits at the same point (right after the
free `FrozenRegionalProgram::Build`), unchanged.

### H6-ALT (OWNER-GATED) — flag name

`-region-out` (matches the object) vs `-frozen-out` (matches the type
`FrozenRegionalProgram`, emphasizes the freeze boundary) vs `-rdf-out`
(Regional DataFlow, parallels `-df-out`/`-rel-out` naming). Owner picks;
the tag-struct name (`FrozenRegionalDump` above) tracks whatever is
chosen.

---

## H7 — regional dump GRAMMAR (OWNER-GATED / Phase-4 — three candidates, NO pick)

**Anchor:** the new `operator<<(OutputStream&, FrozenRegionalDump)` in
`lib/Regional/Format.cpp`. Charge (b) explicitly: "you propose candidate
grammars as ALTERNATIVES with trade-offs, you do NOT pick." The dump must
be DETERMINISTIC (id-ordered, no pointer-derived ordering — the HP-9
rule from pseudocode §2: order re-derived from a DefList view walk, never
from opaque handles).

At Stage B every candidate renders the SAME degenerate content: one
`ProgramRoot`, one region `RegionId(0)`, its ports = the declared ABIs,
its body a reference to the local graph (NOT re-printed — the -df/-rel
surfaces already own the graph text; the regional dump prints the
SKELETON: root, region tree, ports, contracts). The witness set for the
pin is small and structural (see exit gate).

### Candidate G1 — indented region/port/contract block syntax (`.ir`-like)

```
program-root {
  query-abi   q_bf(A:bound, B:free)     -> region R0 via request-port P0
  input-abi   edge_2(A, B)              -> region R0 via input-port P1
  output-abi  <none>
}
region R0  owner=program-root  parents=()  children=() {
  request-port P0  fields=(A)
  input-port   P1  fields=(A, B)
  result-port  P2  fields=(A, B)
  row-contract E0  member-key=(A, B)  support=monotone
}
```

Trade-offs: most human-readable; closest to the existing `.ir` region
dump idiom so reviewers pattern-match instantly; verbose; block nesting
must stay deterministic (children printed in RegionId order). Best if the
dump's primary consumer is a human reviewer blessing goldens.

### Candidate G2 — flat BB-with-args / tail-call form (`.df`-like)

```
root(P0: q_bf, P1: edge_2, P2: <none>):
  call R0(P0, P1) -> (P2)
R0[owner=root](in P0(A), in P1(A,B), out P2(A,B)):
  contract E0: key(A,B) support(monotone)
  ; body -> see .df / .rel
```

Trade-offs: matches the MEMORY directive "BB-with-args/tail-call form"
for the DataFlow/DeltaRel dumps, so all four textual IR surfaces share
one grammar family; ports-as-block-args makes the Stage C request/result
edges natural to add later (they become extra call args); slightly less
obvious to a first-time reader than G1. Best if grammar UNIFORMITY across
`.df`/`.rel`/`.region` is the priority.

### Candidate G3 — typed-record / S-expression form keyed by RegionId

```
(frozen-regional-program
  (program-root (query-abi q_bf (bound A) (free B) (-> R0 P0))
                (input-abi edge_2 A B (-> R0 P1)))
  (region R0 (owner program-root) (children)
    (request-port P0 A)
    (input-port P1 A B)
    (result-port P2 A B)
    (row-contract E0 (member-key A B) (support monotone))))
```

Trade-offs: machine-parseable with zero ambiguity (a future test could
parse it back and structurally diff, sidestepping byte-golden brittleness
under benign reformatting); least human-friendly; introduces a paren
grammar no other DrL dump uses. Best if a STRUCTURED oracle (parse-and-
compare, like `permcheck.py` but for regions) is anticipated for Stage C.

**No pick.** The choice interacts with Stage C (request/result edges must
extend whatever grammar lands) and with the referee strategy (byte-golden
vs parse-and-compare), so it is a Phase-4/owner decision. Recorded as
OWNER-GATED. Whichever lands, the exit-gate pin (H-EXIT) is a
byte-golden of the chosen grammar over the witness set.

---

## H8 — ExtractPrimaryProcedure: PRESERVED (charge (a))

**Anchor:** pseudocode §4b Step 9 (`Procedure.cpp`:777-928). The
Phase-1 adjudication finding: the two-procedure split (thin
`kEntryDataFlowFunc` wrapper tail-CALLing a `kPrimaryDataFlowFunc`) is
absent from all pre-2026-08-02 pseudocode.

**Decision: PRESERVE, unchanged. Do NOT fold.**

Rationale: `ExtractPrimaryProcedure` runs AFTER the whole ControlFlow
tree is built and ControlFlow `Optimize` completes — entirely DOWNSTREAM
of the `Query`→`FrozenRegionalProgram` boundary. It splits the
*ControlFlow* `Program`, not the regional program. `FrozenRegionalProgram`
is a DataFlow-layer object; it never sees, and must not encode, the two-
procedure ControlFlow shape. The thin wrapper (H3/H4) guarantees the
ControlFlow build is byte-identical, so the split fires identically and
every `.ir` golden with the two-procedure shape stays byte-exact.

```
  # NO HUNK. ExtractPrimaryProcedure is not touched by Stage B.
  # Documentation obligation only: §4b Step 9 now records the split so a
  # successor doc cannot mistake "the entry procedure" for one C++ function.
```

**Stage C forward-note (not a Stage B action):** Stage C's lifecycle
lowering (`request_edge_add/remove`, `seal_epoch`, …) lowers into the
ControlFlow tree that `ExtractPrimaryProcedure` later splits. Stage C
must ensure the RootRequestLease drain and the injector replacement land
in the correct half (entry wrapper vs primary). That is Stage C's
obligation; Stage B's obligation is only to not disturb the split, which
PRESERVE satisfies. Recorded so the fold-vs-preserve question is closed
here and not silently reopened.

---

## H9 — validators that land at Stage B (charge (d))

**Anchor:** proposal §11 validator list; pseudocode §3 validator
placement. Charge (d): which of §11's validators land here vs Stage C.

Three land at `FreezeAndValidate` (H3) as freeze-time gates. Two of the
three are NEAR-VACUOUS at Stage B (one region, zero child calls) but are
built now as scaffolding so Stage C/D extend rather than introduce them.
The rest are Stage C (they presuppose request edges, pure regions, or
lifecycle ops that do not exist until the cutover).

```
+ V-FROZEN-NO-OPEN-PORT   (proposal §11 "open port reaching Frozen
                            RegionalProgram")
    LANDS. FreezeAndValidate proves no OpenRequestPort / OpenInputDelta
    Port / OpenResultDeltaPort survives into the frozen program. At
    Stage B the single region's ports are the sealed declared ABIs
    (trivially closed), so this is the freeze BOUNDARY gate — real,
    though it cannot yet fail on corpus input. Failure = fprintf+abort
    internal invariant (a freeze bug), NOT a clean diagnostic.

+ V-OWNERSHIP-ACYCLIC     (proposal §11 "cycle in the region ownership/
                            call graph" + "region call not targeting a
                            direct child")
    LANDS, NEAR-VACUOUS. The ownership forest has one node, zero edges;
    the check is a no-op assertion today. Built now so Stage C/D's
    child-call insertion extends an existing validator. fprintf+abort.

+ V-REGION-CENSUS-IDENTITY  (proposal §11 "FrozenRegionalProgram/Rel
                            operation census mismatch")
    LANDS, DEGENERATE. At Stage B there is exactly one region and it
    owns the entire Rel flow; the census tie asserts (region count == 1)
    ∧ (every Rel op belongs to RegionId(0)). This is the Stage-B stub of
    the full per-region census that Stage C/D fills in. Placement: a new
    check called from ValidateDROps' tail (pseudocode §3, Rel.cpp
    :3999-4021 neighborhood) OR from a new post-Program regional
    reconciliation — see ESC-3. fprintf+abort.
```

OWNER DIRECTIVE 2026-08-03 (recorded at adjudication-record §DOT twins):
the Stage-B `-region-out` dump gains a GraphViz DOT TWIN — one
`subgraph cluster_region_<RegionId>` per region, ports rendered at the
cluster boundary, Stage-C request edges later rendering as inter-cluster
edges. Advisory surface only (never byte-goldened; G1 text stays the
referee).

**Do NOT land at Stage B (Stage C, with reasons):**

AMENDED 2026-08-02 (owner-ratified D2.1; closes brief Errata-5): the Stage-C
side of this handoff is now EXPLICIT — stage-c-diff.md H-I lands
`V-PURE-REGION` and the parent/child frozen-port agreement check
(`V-PORT-AGREE`) by name; §11 lines 3/6 are the stated
declines-extraction softening, not validators.

```
- V-PURE-REGION            ("effectful operator inside a region")
    STAGE C. At Stage B the single observation-root region CO-HOSTS the
    program's effects (impure maps, publications) because there is no
    extraction to separate ProgramRoot's effects from pure children.
    Enforcing purity now would FAIL on every corpus program with a
    message publication or impure map. See ESC-1 — this is the honest
    thin-Stage-B concession: the Stage-B region is not yet "pure" in the
    §3.1 sense. V-PURE-REGION co-arrives with real extraction (Stage C).

- open-port / parent-child frozen-port disagreement (multi-region form),
  ambiguous RequestOwnerId/RequestEdgeId, request-edge-without-caller-
  qualified-result, result-removal-without-routed-identity, inactive-
  state-cleared-before-removals, sequestered-key-in-InstancePath,
  Rel/ControlFlow LIFECYCLE census mismatch:
    ALL STAGE C. Each presupposes request edges, multiple owners, or the
    lifecycle op vocabulary (request_edge_add/… ) that Stage C introduces.
    Naming them here only to certify they were considered and deferred
    with cause, per the "state a gate or escalate" rule.
```

**Every §7 rel-arch validator is CARRIED FORWARD unchanged** (V-XOVER-ONE,
V-PROD-MONO/CLASS, V-JOIN-ONE, the census V-ONE-FOLD…V-RETIRE-AFTER,
V-LINEAR/LOOP/READY/BAND-HAZARD, V-PRED-XCHECK, V-JOIN-EMIT-XCHECK, and —
under `-demand-instance` — the CheckInstance*/census-recount family). Stage
B deletes NO validator. The keyed-instance validators die at Stage C with
the machinery they guard.

---

## Exit gate (golden-master terms, per hunk)

**H-EXIT (the stage gate):** all 180 goldens byte-identical across all 4
optimization modes, adjudicated by the existing byte-compare referee
(`runall.sh` must end `SUITE: PASS`; per-case `diffrun.sh`). No golden is
blessed. The `.oracle`/`.monotone` sidecar oracles and the eqgate family
run unchanged and must all pass. Rationale per hunk:

| Hunk | What stays byte-identical | What legitimately changes | Referee |
| --- | --- | --- | --- |
| H1/H1-ALT | all 180 `.stdout`, `.rel`/`.ir`/`.df`/`.dot` dumps, all codegen | `Query::Build`/`Program::Build` signatures (source only) | byte-compare (runall.sh) |
| H2 | ditto (no re-optimize, ESC-2) | nothing observable | byte-compare |
| H3 | ditto — thin accessor over identical `QueryView` graph | nothing observable | byte-compare + identity-of-`Query` inspection |
| H4 | ditto — `query = frozen.Query()`, body unchanged | `Program::Build` param type | byte-compare; `-bisect` re-cut at same op count (bisect-thread check) |
| H5 | `.ir`/`.df`/`.dot`/`.rel` byte-identical (passes unmoved) | nothing (facade is documentary) | byte-compare |
| H6 | the four existing dump surfaces untouched | NEW `-region-out` surface appears | byte-compare on the four; new dump has its own pin |
| H7 | n/a (grammar not yet chosen) | NEW regional dump content | byte-golden of chosen grammar over the witness set (OWNER-GATED grammar) |
| H8 | all `.ir` goldens incl. two-procedure shape | nothing (untouched) | byte-compare |
| H9 | all 180 (validators are abort-on-violation, silent on pass) | nothing observable unless a freeze bug fires | byte-compare; freeze validators are internal-invariant belts |

**Regional dump witness set (H7 pin):** the eleven existing `.rel`-golden
carriers are the natural regional-dump witnesses — they already span the
structural surface (`demand_tc_witness`, `symrec_tie_1`, `map_3`,
`merge_2`, `booleans`, `elim-cond-cycle-simple`, `negate_1`, `negate_6`,
`d5_recursive_negate`, `join_1`, `optimize_2`). At Stage B every one
renders the SAME one-region skeleton differing only in port ABIs and
row-contracts, so a SMALL witness subset (propose: `demand_tc_witness` —
the demand ABI shape; `merge_2` — a multi-input-port shape; `join_1` — a
join body's row-contract) suffices to pin the grammar deterministically.
The full set is available if the owner wants breadth. These are NEW
`.regiongold` (or grammar-named) sidecars, blessed once at Stage B via the
existing `runall.sh --bless` referee, then held byte-exact.

**No missing oracle at Stage B.** Every hunk has a byte-compare gate. The
one place a NEW referee is owed (structured parse-and-compare for the
region dump) is only IF grammar G3 is chosen and byte-golden brittleness
is deemed unacceptable — recorded under H7, not required for Stage B.

---

## Escalations (design decisions the owner must ratify — not picked here)

- **ESC-1 (the thin-Stage-B purity concession):** at Stage B the single
  observation-root region is NOT effect-pure — it co-hosts the program's
  impure maps and publications because no extraction separates ProgramRoot's
  effects. This CONTRADICTS proposal §3.1 ("Pure regions do not contain or
  transport effects") and §15.13 ("Pure regions contain no observable
  effect") read as invariants that hold from Stage B. Resolution proposed:
  those invariants become enforceable (V-PURE-REGION) only at Stage C when
  extraction first creates a pure child; at Stage B the single region is
  explicitly the "observation root that still co-hosts effects." Owner must
  ratify that §15.13 is a Stage-C-onwards invariant, not a Stage-B one.

- **ESC-2 (no re-optimize inside the planner):** proposal §8's
  `BuildPlanningRegionalProgram` runs `OptimizeLocalGraphToFixpoint`
  per region; Stage B must NOT (byte-identical gate). Stage B's planner
  wraps the already-finalized graph and defers per-region optimize to
  Stage C/D. Owner ratifies the deviation from §8's literal control flow.

- **ESC-3 (V-REGION-CENSUS-IDENTITY placement):** the degenerate census
  tie (H9) can live in `ValidateDROps`' tail (reuses the pseudocode §3
  census-recount site, Rel.cpp:3999-4021) OR in a new post-`Program::Build`
  regional reconciliation step in `main`. The former keeps all census
  checks in one function; the latter keeps the regional layer's validators
  out of the Rel file. Owner picks; low-stakes but affects where Stage C's
  full per-region census grows.

- **ESC-4 (H1-ALT placement) and ESC-5 (H6-ALT flag name, H7 grammar):**
  the OWNER-GATED sub-decisions above — return-type ripple vs localized
  `main` edit; `-region-out`/`-frozen-out`/`-rdf-out`; grammar G1/G2/G3.

- **Ledger:** per pseudocode §7 item 5, the §20(AW) regional-epoch-open
  ledger entry is still unwritten (the ledger stops at (AV)). Stage B
  landing should open it. Not a Stage B code action; a documentation debt
  recorded for the owner.

---

## Mechanisms enumeration (the necessity-audit input)

INTRODUCED by Stage B:

- `PlanningRegionalProgram` type — open, pre-freeze regional IR (one region at Stage B).
- `FrozenRegionalProgram` type — the frozen regional IR; Rel's sole input (H3/H4).
- `OpenRegionTemplate` / `FrozenRegionTemplate` — region template (observation-root only, zero children at Stage B).
- `OpenProgramRoot` / `FrozenProgramRoot` — the effect/ABI-owning root shell (thin at Stage B; co-hosts effects per ESC-1).
- Structural typed ids: `RegionId`, `LocalNodeId`, `LogicalNodeId`, `PortId`, `EdgeId` — the subset §4.1 needs for the region skeleton (request/lease/instance/support ids are Stage C).
- `logical_origins` map — `(RegionId, LocalNodeId) -> LogicalNodeId`; identity at Stage B, carries origin for Stage C/D clones.
- `BuildPlanningRegionalProgram` — degenerate no-extraction planner (H2).
- `FreezeAndValidate` — planning→frozen transition + the H9 freeze validators; no reverse mutator.
- `FrozenRegionTemplate::LocalGraph()` / `FrozenRegionalProgram::Query()` — THE thin seam (H3); the accessor Rel reads the identical `QueryView` graph through.
- `lib/Regional` static library (`Planning.cpp`, `Format.cpp`) — new compiler-internal target, peer of `lib/Rel`.
- Regional dump surface: `-region-out` flag (H6-ALT), `gRegionStream`, `FrozenRegionalDump` tag struct, `operator<<` in a chosen grammar (H7, OWNER-GATED) — third dump-timing slot (after Query::Build, before Program::Build).
- `V-FROZEN-NO-OPEN-PORT` — freeze boundary validator (H9).
- `V-OWNERSHIP-ACYCLIC` — near-vacuous ownership-forest validator scaffold (H9).
- `V-REGION-CENSUS-IDENTITY` — degenerate one-region ⇔ whole-Rel-flow census tie (H9, placement ESC-3).

CARRIED FORWARD UNCHANGED through Stage B (each dies at its named later stage or persists):

- `gDemand` / `gDemandInstance` / `gDemandRetract` flags — Stage C deletes.
- `gPassPolicy.bisect_counter` cross-Build thread — persists (not demand-specific); H4 re-asserts it survives the wrapper.
- `ApplyDemandTransform` + its two failure classes (clean rejects vs tripwire/OWN-3 aborts) — Stage C deletes.
- `FabricateDemandMessage`/`FabricateDemandLocal` + fabrication registry + `kMessageHandler` suppression + `IsDemandMessage` — Stage C deletes.
- `GuardAnnotation` / `RecognizedSubgraph` / `QueryDemandForcing` records + `guard_annotation_index` carrier + the `DemandForcings`/`GuardAnnotations`/`RecognizedSubgraphs` read seam — Stage C deletes.
- `ResolveLiveRecognition` / `BuildSubgraphInstanceOps` + `demand_instance_enabled` fork + census recount + `CheckInstance*` validators + `CheckInstanceOrder` — Stage C deletes.
- `BuildQueryInjectorProcedure` / `retract_proc` + `ForcingMessage` fallback — Stage C deletes (replaced by RootRequestLease).
- `InstanceStore<Key,RowT>` runtime + the a0/a1/a2/a2'/b band lowering (`LowerSubgraphInstances`) + `StateCellStore` (agg/KV — survives as the aggregate mechanism; only the demand-instance use retires) — Stage C deletes the demand-instance path.
- `ExtractPrimaryProcedure` two-procedure split — PRESERVED (H8); not a demand mechanism, persists into and beyond Stage C.
- `SetRelDumpStream` / `-rel-out` / `.rel` goldens — persist (the local-graph dump).
- `QueryDF` / `-df-out`, IR dump / `-ir-out`, DOT / `-dot-out` — persist (still take `frozen.Query()`).
- The entire §7 rel-arch pipeline: `BuildDataModel`, `BuildDRInventory`, `DeriveDRStrata`, `LinearizeAndValidateDRFlow`, the 29-kind census, `Lower*`, ingest folds, eager markers, claim gates, commit sweeps, join emission — persist as the per-region local-graph lowering (survives the whole regional replacement per the Fable-review "greenfield at the demand layer only" note).
- `Query::Build` tail passes: `TrackDifferentialUpdates`, `TrackConstAfterInit`, `BuildEquivalenceSets`, `Stratify`, `IdentifyInductions`, `Finalize*` — persist; the planner slots strictly after them (H1).

---

## Structured summary

- **Stage:** B — the regional representation becomes canonical (pure representation refactor).
- **Artifact file:** `/Users/pag/Code/DrLojekyll/docs/proposals/RegionalDataFlowCore.artifacts/stage-b-diff.md`
- **Exit gate:** all 180 goldens byte-identical across all 4 optimization modes (byte-compare referee, `runall.sh` → `SUITE: PASS`; `.oracle`/`.monotone`/eqgate sidecars unchanged and passing); a NEW `-region-out` regional dump pinned deterministic for a small structural witness subset (proposed: `demand_tc_witness`, `merge_2`, `join_1`) via new grammar-named sidecars, blessed once. Provable byte-identity rests on the THIN wrapper (H3): Rel consumes `frozen.Query()`, an accessor over the numerically-identical `QueryView` graph, so every dump and all codegen are unchanged functions of an unchanged traversal. The `gPassPolicy` bisect thread is re-asserted across the wrapped boundary (H4).
- **Charge resolutions:** (a) `ExtractPrimaryProcedure` PRESERVED unchanged, downstream of the Query→frozen boundary (H8); (b) regional dump = third timing slot after `Query::Build`/before `Program::Build`, flag OWNER-GATED among `-region-out`/`-frozen-out`/`-rdf-out`, grammar OWNER-GATED among G1 indented-block / G2 BB-with-args / G3 S-expression (H6/H7); (c) THIN Stage B chosen — frozen region is a wrapper exposing the identical local `QueryView` graph, thick re-plumbing deferred to Stage D (H3); (d) three validators land as freeze-time scaffolds (`V-FROZEN-NO-OPEN-PORT`, `V-OWNERSHIP-ACYCLIC` near-vacuous, `V-REGION-CENSUS-IDENTITY` degenerate); `V-PURE-REGION` and all lifecycle validators are Stage C (H9).
- **Escalations:** ESC-1 purity is a Stage-C-onward invariant (Stage-B region co-hosts effects, contra §3.1/§15.13); ESC-2 planner does NOT re-optimize at Stage B (deviation from proposal §8 forced by the gate); ESC-3 `V-REGION-CENSUS-IDENTITY` placement (ValidateDROps tail vs post-Program `main`); ESC-4 H1-ALT placement (return-type ripple vs localized `main` edit); ESC-5 flag name + dump grammar; ledger §20(AW) regional-epoch-open entry still unwritten.
</content>
</invoke>
