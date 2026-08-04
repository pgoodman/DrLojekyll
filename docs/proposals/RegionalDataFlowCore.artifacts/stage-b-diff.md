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

---
## AMENDMENTS (2026-08-03, session 4) — the DELTA-1..6 fold, re-anchored at tip 8a4520d9

House rule: the H1–H9 hunks and the exit-gate/ESC/mechanisms sections above are NOT edited. This dated section is NORMATIVE ON CONFLICT. It folds `stage-b-seed.md` Part 2 (DELTA-1..6) into the hunks and re-anchors every claim against `regional-arch-pseudocode.md` **Part B** (fleet-verified at tip **8a4520d9**) + its **DRIFT LEDGER (DB-1..DB-13)**. Where a base-hunk anchor drifted from the f0c913e0 tip it was authored at, the correcting DB-N is cited; line numbers drift, structure holds — the Stage-B implementer re-verifies before building (SINGLE-PASS RULE).

### H1-AMEND (DELTA-1) — the freeze point, re-anchored + the return-shape resolved

**Anchor correction (DB-1).** H1 placed the freeze insertion at `return Query(...)` on `:2637`. REALITY at 8a4520d9 (Part B B.2): the tail is `InferConservativeRowContracts` (`Build.cpp:2646`, Stage A — real, landed) → `ValidateRowContracts` (`:2647-2649`, a BOOLEAN `!Validate…` guard, NOT the `num_errors != log.Size()` idiom used by the other six guards) → `return Query(std::move(impl))` (`:2651`). Under the return-shape variants (i)/(ii) the `BuildPlanningRegionalProgram` + `FreezeAndValidate` DIFF inserts at **:2647→:2651** — strictly AFTER the contract pass (contracts are planning input, DELTA-1) and BEFORE the return; under the recommended (iii) the freeze is a `main`-level side computation in `CompileModule`'s THIRD-SLOT and `Query::Build` is UNTOUCHED. The ordering invariant — freeze strictly after `ValidateRowContracts` — holds in all three (F1). The two interleaved `num_errors` guards (`:2632-2634`, `:2639-2641`) the seed collapsed are unchanged and untouched.

**The Variant-1 return-shape ambiguity — RESOLVED to a three-way enumeration.** The base docs disagreed: `stage-b-diff.md:71-72` H1 wrote `return frozen` with "return type CHANGES Query -> FrozenRegionalProgram"; `stage-b-seed.md:101` DELTA-1 (and Part B B.2's DIFF line) wrote `return Query(std::move(impl), frozen)`. These are DIFFERENT shapes. Enumerated, with the NEW ripple fact from Part B **B.3** (`Query::Build` has exactly TWO live callers — `Main.cpp:71` and `bin/Oracle/Main.cpp:745-746`, the latter reading only the `Query` public surface, `NumStrata`/`ForEachView`/… , and NEVER `Program::Build`):

- **(i) `optional<FrozenRegionalProgram>`** — H1 as originally written; the frozen program is the only thing leaving the DataFlow layer. **Ripples BOTH callers**, including the Oracle, which must now fish its `Query` out of the frozen wrapper (B.3) — a change to a demand-blind reference tool that touches no frozen semantics. Rejected on that ground.
- **(ii) `Query` augmented to CARRY `frozen`** (DELTA-1 spelling `return Query(impl, frozen)` — the ctor gains a second arg, return type STAYS `optional<Query>`, Main recovers the frozen via a `query.Frozen()` accessor). **Oracle byte-untouched** (still receives `optional<Query>`, ignores the new member — B.3). One Build-tail freeze authority (the pseudocode §6 placement). Cost: a `Query` ctor/member edit + one Main accessor read.
- **(iii) H1-ALT — a `main`-level free build** in `CompileModule`'s `<<<THIRD-SLOT>>>` gap (Part B B.1, the `Main.cpp:74→:76` gap): a free `FrozenRegionalProgram::Build(const Query&, const ErrorLog&)` runs after the `Query::Build` null-check, `Query::Build`'s signature UNTOUCHED. **Neither caller's `Build` call changes** (B.3); the only FREEZE-BUILD edit is in `CompileModule` — H4's `Program::Build` signature change (consume `frozen`, first statement `const Query &query = frozen.Query();`) applies under EVERY variant, (iii) included (base H4 already covers this: its `frozen_opt` is "the H1-ALT Variant 2 free build"'s output; TC-2). Con: `Query` stays a freely-built post-DataFlow object, softening "the frozen program is canonical."

**Recommendation (OWNER-GATED, ESC-4).** Prefer **(iii)** on strict smaller-not-larger + golden-neutrality-provability grounds: zero change to either `Build` call site, the freeze is a pure `main`-level side computation whose only consumers are `-region-out` (H6) and `Program::Build`'s `frozen.Query()` seam (H4), and byte-neutrality reduces to the H3 one-line identity (`Program::Build` receives the numerically-identical `QueryView` graph). **(ii)** is the equally-Oracle-neutral runner-up when the owner wants the single Build-tail freeze authority the normative §6 pseudocode shows, at the small ctor+Main cost. **(i)** is rejected — the B.3 Oracle ripple buys nothing. The rest of this doc's hunks are placement-agnostic except H6's third-slot detail (which is identical under (ii) and (iii)).

**Library-placement CONSTRAINT (NEC-1).** Any in-`Query::Build` freeze (variants (i)/(ii)) is INCOMPATIBLE with `lib/Regional` as a separate peer of `lib/Rel`: `lib/DataFlow` would then call into a library that itself owns a `Query` by value (H3, `FrozenRegionTemplate::body`), forcing a circular static-library dependency `lib/DataFlow ⇄ lib/Regional` (the Ninja target graph goes cyclic; the link needs `--start-group` hackery). So under (ii) the `FrozenRegionalProgram` type + `Planning.cpp` MUST live in `lib/DataFlow` (restricting `lib/Regional`, if it survives at all, to the dump-only `Format.cpp`) — which H8's own "FrozenRegionalProgram is a DataFlow-layer object" (`stage-b-diff.md:423-424`) already argues for. Only the recommended (iii) keeps `lib/Regional` a clean acyclic peer (`bin/drlojekyll → lib/Regional → lib/DataFlow`), one more reason to prefer it. The ESC-4 (ii) cost line above (ctor+Main only) MUST be read together with this placement obligation.

### H2-AMEND (DELTA-1) — the planner input list is Stage-A-real

`BuildPlanningRegionalProgram`'s `row_contracts` input is now the REAL landed Stage-A artifact: `impl->row_contracts`, set at `Build.cpp:2646` by `InferConservativeRowContracts`. **The H2 clause "identity pass-through if Stage A not landed" (`stage-b-diff.md:127`) is DELETED** — Stage A landed (ledger §20(AX)); the pass-through fallback is dead. The satellite input list beyond the query body is DELTA-1's enriched set, every member a §1.2-seed / B.3-read-surface object already threaded downstream: `row_contracts` (contracts), the EquivalenceSet models, `view->stratum` (strata), `guard_annotations`, `RecognizedSubgraphs()`, `DemandForcings()`. The base `views + column edges` and the node-intrinsic `projection_role` (build-stamped, immutable, folded into TUPLE `Equals`) travel INSIDE the wrapped query graph, not as threaded satellites (F2) — so §1.2's eight-member list and this six-member satellite list agree. All consumed to build ONE `ProgramRoot` + ONE observation-root template; ESC-2 (no re-optimize) stands.

### H3-AMEND — the thin seam is grounded on Part B B.4's read-surface table

H3's "consumes only `FrozenRegionalProgram` is an accessor over the identical `QueryView` graph" is now MECHANICALLY GROUNDED, not asserted. Part B **B.4** walked the whole `Query`-public-method → consumer table at 8a4520d9: **no downstream library names `QueryImpl*`** (one stray comment at `Build.cpp:187`) or includes the private `Query.h`. ControlFlow and Rel take `Query`/`const Query&` by value; CodeGen recovers a `Query` ONLY via `Program::Query()` (`Program.cpp:260-262`, `return impl->query;`) and calls exactly one method off it (`IsDemandMessage`, `Database.cpp:1522/:3692`). The boundary is ALREADY clean; the frozen wrapper starts from a clean substrate.

**What `FrozenRegionalProgram::Query()` MUST pass through** (B.4 budget): the 15 `DefinedNodeRange` view accessors + `ForEachView` that FillDataModel/BuildDataModel and the Format dumps walk; the four demand satellites downstream reads BY NAME (`DemandForcings`/`GuardAnnotations`/`RecognizedSubgraphs`/`IsDemandMessage`); and `Program::Query()` preserved verbatim as the ONE gateway CodeGen recovers a `Query` through. The single genuine `.impl->` leak is Format-local (`QueryContracts`→`row_contracts`, `Format.cpp:1531`, a declared friend), driven from `Main.cpp:139` — so the frozen accessor NEED NOT carry `row_contracts` unless a future frozen dump replaces `-contract-out`.

### H4-AMEND — Program::Build fresh anchors + the bisect thread corrected

**Fresh anchors (DB-2..DB-5, replacing H4's f0c913e0 ranges):** signature `Build.cpp:1331-1334` (DB-2, H4's `:1308-1311` drifted +~23 lines, text verified); the C-2 V-ALGEBRA + feature-gap pre-pass `:1342-1435` (DB-3, H4's `:1332-1411` ran short — it stops at the Joins loop, `:1424-1435`); the `demand_instance` guard fences `:1451-1499` (DB-4, H4's `:1428-1473` stale — this RESOLVES the H4-vs-Part-R disagreement in Part R's favor; fence (iii) differential-summarized-input is named in the comment but is NO LONGER a live reject, lifted by R-a2 band-(a2)); the `num_errors` gate `:1501-1503`; `BuildDataModel(query, program)` `:1522` (DB-5, H4's `:1499` predates the Context/impl setup `:1505-1520`). The hunk body is otherwise byte-preserved: `const Query &query = frozen.Query();` is the first statement, everything after it reached through `query` exactly as before.

**Bisect thread — the H4 claim CORRECTED.** H4's exit-gate line ("a `-bisect`-driven run must still cut at the same op count") names a flag that **does not exist**. Per Part B B.1: there is NO `-bisect` CLI flag. The only bisect surface is `-opt-bisect-limit=<N>` (`Main.cpp:485-495`), which sets `gPassPolicy.bisect_limit` — NOT the counter. The cross-Build mutable index is `PassPolicy::bisect_counter` (`mutable uint64_t{0}`, `Util/PassPolicy.h:53`), reset once per module at `CompileModule:68`, incremented in `PassPolicy::Gate` (`PassPolicy.cpp:85`), one tick per gateable application. The single `gPassPolicy` (`Main.cpp:51`) threads by identity into BOTH Builds (`:71` arg 3, `:83` arg 4). **Re-worded exit check:** a run under a fixed `-opt-bisect-limit=N` must cut at the SAME op count across the wrapped boundary; the frozen wrapper carries no policy state and does not intercept the thread (both (ii) and (iii) leave `gPassPolicy` threaded verbatim).

### H5-AMEND — five dump surfaces, fresh anchors, facade extended

H5 named FOUR dump surfaces; at 8a4520d9 there are FIVE (H5's f0c913e0 text predates `-contract-out`, the Stage-A H-A8 surface). Fresh anchors (Part B B.1):

- `-ir-out` — `(*gIRStream) << *program_opt`, `Main.cpp:91-94`, drains PROGRAM (needs `TableId()`), AFTER `Program::Build`.
- `-dot-out` — `(*gDOTStream) << *query_opt`, `:123-126`, drains QUERY, AFTER.
- `-df-out` — `(*gDFStream) << QueryDF{*query_opt}`, `:131-134`, drains QUERY, AFTER.
- `-contract-out` — `(*gContractStream) << QueryContracts{*query_opt}`, `:139-142`, drains QUERY (via the Format-local `.impl->row_contracts` friend), AFTER. **NEW in the facade.**
- `-rel-out` — `SetRelDumpStream(gRelStream)` call at `:80`, BEFORE `Program::Build` (Rel is built AND drained inside it; no top-level drain).

Facade claim EXTENDED to all five: each still takes `frozen.Query()` (or, for `-contract-out`, reads `row_contracts` through the unchanged friend), the passes are unmoved (ESC-2), goldens byte-identical. The move is representational only — a design stance, not a required correctness rewire; under (ii)/(iii) `query_opt` stays a live `Query` and these five drains could equivalently be left routed through it byte-for-byte.

### H6-AMEND (DELTA-3) — flag RESOLVED to -region-out; slot pinned; frozen-rooted render; DOT twin added

**Flag name RESOLVED: `-region-out`** (the owner's session prompt and every desired-states doc use it; `regional-dump-stage-b-desired-states.md`, `region-model-desired-states.md` §2). **ESC-5's flag-name half is CLOSED** (supersedes H6-ALT's `-region-out`/`-frozen-out`/`-rdf-out` menu). **Slot pinned** to Part B B.1's `<<<THIRD-SLOT>>>` marker — the `Main.cpp:74→:76` gap, after the `Query::Build` null-check's closing brace and before the `SetRelDumpStream` comment.

**Render root RESOLVED to FROZEN, not `query_opt` (F-REGION-DEAD).** The base H1-AMEND(iii) claim that the frozen program's "only consumers are `-region-out` (H6) and `Program::Build`'s `frozen.Query()` seam" is INCOMPATIBLE with a `*query_opt`-rooted region dump — if the dump reads `query_opt`, it does not consume `frozen`, and a `BuildPlanningRegionalProgram` stubbed to a `regions==1` shell with empty ports/contracts would still render a correct-looking `.region` block from the Query + demand satellites, shipping dead planning code green. RESOLUTION: the `-region-out` drain is a `FrozenRegionalDump{*frozen}` (base-H6 spelling) that renders FROM the frozen object's STORED region template — roots/ports/census/row-contracts as populated by H2's planner — NOT re-derived from `*query_opt`. The "PURE DataFlow graph, `TableId()` not yet annotated" phrasing describes only the slot TIMING (before `Program::Build`'s `TableId()` side effect), never the render root. This RESTORES the truth of H1-AMEND(iii)'s "only consumers" claim and makes a stubbed/empty planner yield a divergent `.region` dump the golden catches. Under (ii)/(iii) the drain sits at the same point, gated on `gRegionStream` (a global near `:56-61`, an argv case in the `:315-410` band, the `if (gStream){ (*gStream)<<…; Flush(); }` shape).

**DOT twin ADDED to H6's deliverable** (owner directive 2026-08-03, `owner-adjudication-record.md:133-145`): a `subgraph cluster_region_<RegionId>` GraphViz twin, ports at the cluster boundary, ADVISORY — never byte-goldened (the G1 text remains the referee), `vNNNN` node ids pointer-derived and drift per run. (The region twin renders no `role=`/`KEY(...)` annotation — that annotation is the orthogonal Stage-A dataflow `-dot-out` surface, not the region DOT.)

### H7-AMEND (DELTA-3) — grammar RATIFIED G1; four witnesses; seven-field census; region as a first-class `.irgold` surface

**H7's "no pick / three candidates" is SUPERSEDED.** Grammar is **RATIFIED G1** (D2.2, `owner-adjudication-record.md:48`) — the indented region/port/contract block. G2/G3 are retired as historical alternatives. Two D2.8 ratifications APPLY at Stage B (ADJ-2/ADJ-3, `owner-adjudication-record.md:72-74`): the fabricated `demand__…` input renders **region-internal** (portless, NOT a program-root input-abi); an all-free query renders as a portless **permanent-root** with NO request port (request-port count = "has demand"). The concrete four-witness G1 blocks are `regional-dump-stage-b-desired-states.md` §9.1 (fresh-dump grounded at 8a4520d9); row-contracts follow **Rule R-STORE** (§9.0: one contract per DECLARED relation whose terminal production carries a model table — an `insert … into %table:N` sink or a `merge … table=%table:N` union; join-operand arrangements, `class=table-less` merges, and `@inline`-fused locals are NOT materialized).

**Witness set = FOUR carriers** (H-EXIT proposed three): `demand_tc_witness` (one request port, the demand ABI), `join_1` (zero-field / ADJ-3 permanent-roots), `merge_2` (the `@inline`-collapse / multi-input-port shape), and **`demand_multi_adorn_witness`** — the two-request / one-shared-pub shape session-1 §7 flagged as otherwise missed (the closest Stage-B foreshadow of Stage C's multi-`RequestEdgeId`).

**Golden production + bless wiring (F-REGION-BLESS).** Goldens are NEW `region`-surface `.irgold` pins. Stage B MUST extend `run_irgold`'s produce compile (`runall.sh:318-324`) with `-region-out "$iout/region.$mode.out"`, making `region` a first-class `.irgold` surface pinned as `region <mode>` in the witness sidecars (e.g. `demand_tc_witness.irgold`); the generic `.irgold` bless while-loop then captures it unchanged and inherits the loud missing-surface FATAL guard. Do NOT introduce a bespoke `.regiongold` sidecar type — NO `runall.sh` code reads one, so its pin would be a silent no-op (exactly the under-bless the harness's T3 comment forbids). The earlier phrasing that the pins ride "the existing `runall.sh --bless` referee" with no wiring is inaccurate and is superseded: the referee works only after this produce-line extension lands.

**Mode set (F-REGION-CROSSMODE).** The `.region` dump sits at the same after-`Query::Build`/before-`Program::Build` slot as `.rel`, which is provably NOT cross-mode identical across the df axis (`demand_neighborhood_mono_witness.rel.opt` != `.rel.nodf`). The earlier "opt-orthogonal (cross-mode byte-identical)" wording is STRICKEN as unbacked and contradicted. The four `.region` sidecars ENUMERATE ALL FOUR MODES (`region opt/nodf/nocf/none`) per the `demand_neighborhood_mono_witness` four-mode `.rel` precedent: `opt==nocf` (controlflow opt is downstream of the dump) while `nodf`/`none` may legitimately differ from `opt` (dataflow opt precedes the dump). Byte-identity is asserted PER-MODE against four blessed goldens, NOT claimed across modes. (The demand-witness `.rel` precedent is deliberately MIXED — `demand_tc_witness` opt-only vs `mono_witness` four-mode — which is exactly why the mode set is specified here rather than left to author discretion.) Blessed ONCE after review via the generic `--bless` path, then held byte-exact; demand-flag-dependent (the demand witnesses' `.region` goldens live beside their `.drflags`, like their `.rel` goldens).

**Census stays SEVEN fields** — `regions, child-calls, program-roots, request-ports, input-ports, result-ports, row-contracts` (§9.2/§9.5). **NO `request-edges` eighth field at Stage B**: that is `region-model-desired-states.md` hazard **H7** (request-edge as census field vs permcheck-only block), and the H1 (key-in-output vs sequestered convention) and H2 (`kRequestEdgeAdd`/mono-collapse) desired-states owner items, are ALL Stage-C surfaces NOT prejudged here — the G1 census field-order choice must not foreclose them. **Determinism:** a pure IDENTITY referee at Stage B (fully positional, zero order-free tokens — the first order-free multiset arrives with Stage C's request-edges); block order per §9.2 (request-ports → input-ports → result-ports → region-internal → permanent-root → row-contracts), region-internal in ascending `forcing_index`, permanent-root in `#query` decl order, `PortId` dense across (request,input,result) with all-free excluded and fabricated `demand__` omitted, `EdgeId`/row-contracts in materialized-relation decl order; no `%table:N` (HP-9). **Generator-friendliness obligation (DELTA-5):** the grammar must not foreclose DIFF-R3's adornment-placement fuzzer — G1's positional per-line shape is compatible; keep the bracket/port rendering enumerable so the placement harness can diff by class.

### H8-AMEND — ExtractPrimaryProcedure's real position corrected

PRESERVE (charge (a)) is unchanged. The RATIONALE anchor is corrected per Part B **B.5**: `ExtractPrimaryProcedure` (`Procedure.cpp:777-928`, sole call `Build.cpp:1615`) is not simply "after Optimize" — it is **SANDWICHED between TWO `impl->Optimize(policy)` rounds**: 1st CF-optimize `:1611-1613`, extract `:1615`, `FixupContainingProcedure` `:1617`, 2nd CF-optimize `:1620-1622`. It splits the ControlFlow `Program` (a `kEntryDataFlowFunc` wrapper tail-CALLing a `kPrimaryDataFlowFunc`), entirely downstream of the Query→frozen boundary; the thin wrapper (H3/H4) leaves the ControlFlow build byte-identical, so the split fires identically and every two-procedure `.ir` golden stays byte-exact. No hunk; the sandwich position is recorded so a successor does not relocate the split relative to either optimize round.

### H9-AMEND (DELTA-4) — scaffolds re-homed; census strengthened; two Stage-B owner slices added

**Two `FreezeAndValidate` freeze-time scaffolds stand** (H3): `V-FROZEN-NO-OPEN-PORT` (freeze boundary) and `V-OWNERSHIP-ACYCLIC` (near-vacuous, one-node forest) — both range over the frozen structure, which exists pre-`Program::Build`. **The third, `V-REGION-CENSUS-IDENTITY` (degenerate `regions==1 ∧ every Rel op ∈ RegionId(0)`), is NOT a `FreezeAndValidate` scaffold** (TC-1): the `regions==1` conjunct is freeze-checkable but the `every Rel op ∈ RegionId(0)` conjunct is not — no Rel op exists until `Program::Build` — so the check HOMES at the `ValidateDROps`-tail recount (`Rel.cpp:3999-4021`, per ESC-3), never inside `FreezeAndValidate`. (Base H9's dual framing — listing it under both the FreezeAndValidate gates and the ValidateDROps tail — is inherited, not a new contradiction; this amendment pins the single true home.)

**ESC-3 evidence updated:** the ValidateDROps-tail home is Part B B.5's quoted census recount at **`Rel.cpp:3999-4021`** — and it ALREADY carries the exact per-forcing recount shape ESC-3 wants to extend (`exp_instance`/`exp_death` via `ResolveLiveRecognition(impl, query)` off DataFlow's `RecognizedSubgraphs()`, independent of the mint's own output; `expect(kSubgraphInstantiate, exp_instance, …)` at `:4019`). The Stage-B degenerate tie is a stub in that same tail.

**V-REGION-CENSUS-IDENTITY STRENGTHENED beyond self-consistency (F-REGION-DEAD).** Because `Program::Build` reads only `frozen.Query()` (H4) — so ControlFlow/codegen are byte-identical whether or not the planning fields were populated — the degenerate self-consistency check alone would let a `BuildPlanningRegionalProgram` stubbed to a `regions==1` shell with EMPTY ports/contracts ship green (all 190 stdout + 63 oracle + 63 monotone + 59 behavioral + 6 eqgate + the `.region` golden PASS). The DIFF-R1-oracle-1 positive-presence referee for the code Stage B ACTUALLY adds is therefore an ALWAYS-ON `fprintf`+`abort` recount in the `ValidateDROps` tail (the ESC-3 home) that re-derives the EXPECTED Stage-B census fields FROM the Query graph — request-ports = demand-forcing count, input-ports = real received-message count, row-contracts = materialized-model count per R-STORE — and asserts them EQUAL to the frozen object's STORED census fields, mirroring the `exp_instance`/`exp_death` independent-recount shape already at `:4006-4021`. A `regions==1` shell with unpopulated ports/contracts then ABORTS. Combined with the frozen-rooted `-region-out` render (H6-AMEND), the freeze/planning code Stage B introduces is refereed both by the golden (representation) and by the recount (presence).

**DIFF-R5 Stage-B hook (owner-gated slice item, DELTA-4/DELTA-5).** `Minimize` / `FieldExpression` classes (real minimal keys, beyond the flat-key AllFields floor) were **DEFERRED-TO-B by D3.4 as an OWNER OPTION** (`owner-adjudication-record.md:99-102`); DIFF-R5 records them as a **LIFT-candidate** (`region-model-diffs.md` DIFF-R5, Hunk R5-5 / STOP-R5-A). The in-session decision is binary: land `Minimize` in this slice vs stay deferred. Load-bearing consequence: **`V-NO-COLLAPSE`'s real (non-belt) reject is BLOCKED on it** — Stage-A adjudication #2 made V-NO-COLLAPSE belt-only because AllFields producer keys make benign drops indistinguishable from real collapses; real minimal keys are exactly what unblocks the hard reject. Recommend DEFERRED unless the owner wants the V-NO-COLLAPSE teeth this slice (the flat-key AllFields floor is the current, sound contract; Minimize is answer-neutral so its timing is pure precision, not correctness).

**Two DELTA-5 owner items recorded as NON-actions (reserve, do not build):**

- **Logical-origin provenance on planning table nodes** — DIRECTION, unratified (`owner-adjudication-record.md:177-192`; carry logical-origin sets on physical models as compile-time provenance; candidate home = the Stage-B planning tier's table nodes). Stage B RESERVES the field-slot question for the owner (the H-A1 "reserve the domain, no producer" pattern); it does not add the field.
- **Proxy role-inheritance** — the D2.9 default-rule question / Stage-A adjudication #4 NEW OWNER ITEM (`owner-adjudication-record.md:170-175`): should a proxy mint INHERIT its source's `ProjectionRole` so set-boundary provenance survives onto the final graph? Zero behavioral risk (role is inert on survivors, consumes nothing at Stage B) but it changes the dump semantics. Recorded, not built.

### H-EXIT-AMEND (DELTA-2) — the exit gate widened and strengthened

H-EXIT's "180 goldens byte-identical" is SUPERSEDED. The Stage-B exit gate is the FULL session-3 referee stack, ALL byte-identical, suite run WITH `run_refinterp` live:

- **190** bespoke cases × 4 optimization modes (byte golden; `runall.sh` → `SUITE: PASS`).
- **63** oracle + **63** monotone goldens (the `.batches` derivation-counter oracle + monotone projection).
- **59** behavioral goldens (the I0 CBF == `goldens/<c>.behavioral.stdout` == behavioral binary, all 4 modes).
- **6** eqgate carriers (flat == nested == golden, live), incl. the R-DIFF diff-pub flagship `demand_diff_pub_1` (differential published answer over a standing demand; landed `bd694541` AFTER this doc's authoring — F-EQGATE-COUNT). Re-derive the count as `ls tests/OptDiff/cases/*.eqgate | wc -l`, NOT the CLAUDE.md "family to five" narrative constant (itself stale at 8a4520d9 and due the same bump).
- the `.irgold` pins (`.df`/`.rel`/`.contract`) byte-identical; plus the NEW four-witness `region`-surface `.irgold` pins (H7), four modes each, demand-flag-dependent, blessed once via the extended `run_irgold` produce line.

**Any behavioral-golden divergence during Stage B is a HARD STOP** — the behavioral goldens are FROZEN through the Stage-C cutover (stage-i0 §8). **I0 independence argument (DELTA-2):** the I0 referee shares NO code with `Query::Build` — it is a definitional evaluator over RAW PARSED CLAUSES (OG1-parsed, `bin/RefInterp`; `Main.cpp:5` "no Query::Build, no Rel, no codegen"; B.3 confirms it is NOT a `Query::Build` caller), so it referees the builder-tail refactor INDEPENDENTLY — the pure-representation claim is externally checkable for the first time. No golden is blessed to make a red case green; blessing is only the one-time `.region` pin.

### ESC disposition (updated)

| ESC | Status at session 4 | Note |
| --- | --- | --- |
| ESC-1 (thin-Stage-B purity concession) | STILL OWNER-RATIFY | §15.13/§3.1 as Stage-C-onward invariants; the single region co-hosts effects. Unchanged. |
| ESC-2 (no re-optimize inside the planner) | STILL OWNER-RATIFY | Deviation from proposal §8 forced by the byte gate; Stage B wraps the already-finalized graph. Unchanged. |
| ESC-3 (V-REGION-CENSUS-IDENTITY placement) | EVIDENCE UPDATED + STRENGTHENED | ValidateDROps-tail home is the quoted `Rel.cpp:3999-4021` recount (B.5), which ALREADY has the per-forcing `exp_instance`/`exp_death` shape; V-REGION-CENSUS-IDENTITY is NOT a FreezeAndValidate scaffold (TC-1) and is upgraded to an always-on independent recount of the frozen census vs a Query-derived expectation (F-REGION-DEAD). |
| ESC-4 (H1-ALT placement) | 3-WAY ENUMERATION (replaces the 2-way) + LIB CONSTRAINT | (i) `optional<FrozenRegionalProgram>` ripples the Oracle (B.3) — rejected; (ii) `Query` carries frozen / (iii) `main`-level free build — neither ripples the Oracle. Recommend (iii), (ii) runner-up. NEC-1: (ii) with a SEPARATE `lib/Regional` is a circular static-lib dep — under (ii) the frozen types + `Planning.cpp` MUST live in `lib/DataFlow`; only (iii) keeps `lib/Regional` an acyclic peer. |
| ESC-5 (flag name + dump grammar) | CLOSED, BOTH HALVES | Flag = `-region-out` (owner prompt + desired-states); grammar = G1 (D2.2). |
| Ledger §20(AW) | LANDED (was "unwritten") | (AW)+(AX) appended to `KeyedInstances.md` §20 by owner ratification (`ledger-entry-AW-draft.md:3` records the land); the ESC "still unwritten" note is superseded — Stage-B landing opens the NEXT entry, not (AW). |

### DELTA-6 (method note)

This amendment set is authored under the DELTA-6 discipline: it was panel-critiqued against the **~22% survival calibration** (Stage A's rate, not session-3's over-permissive ~95%). Refuters produced CONCRETE code evidence per confirmation, re-triaged severities, and treated a finding without a failure scenario as not a finding. The panel record follows.

### PANEL RECORD (session 4)

**Total findings raised:** 12. **Survivors (refuter-verified):** 8, plus 1 orchestrator-adjudicated (TC-2 — its refuter agent died on a structured-output failure; adjudicated at the orchestrator against base-H4's text). **Refuted:** 3 (NEC-2 — verbatim rediscovery of the already-adjudicated necessity-audit A-nec-4 verdict INHERENT; NEC-3 — false premise, the owner DOT directive explicitly specifies the Stage-B twin; NEC-4 — V-OWNERSHIP-ACYCLIC's near-vacuity is self-labeled and ESC-3's placement is an explicit owner fork).

| id | lens | hunk | final severity | disposition (one line) |
| --- | --- | --- | --- | --- |
| F-REGION-DEAD | testability-oracle | H6-AMEND / H9-AMEND | MAJOR (from BLOCKING) | No positive-presence referee for the freeze/planning code + H1(iii)-vs-H6 render-root contradiction; folded FROZEN-rooted `-region-out` render and an always-on independent census recount at the ValidateDROps tail. |
| F1 | correctness-lifecycle | H1-AMEND / ESC-4 | MINOR (from MAJOR) | DB-1's `:2647→:2651` code site was stated flat, not variant-scoped; folded a variant-scoping qualifier (i/ii → Build tail, iii → THIRD-SLOT; ordering invariant holds in all three). |
| F-REGION-BLESS | testability-oracle | H7 / H-EXIT | MINOR (from MAJOR) | `.region` pins have no `runall.sh` producer; folded the `run_irgold` produce-line `-region-out` extension and forbade a bespoke `.regiongold` sidecar. |
| F-REGION-CROSSMODE | testability-oracle | H7-AMEND | MINOR | "opt-orthogonal (cross-mode byte-identical)" unbacked/contradicted by the `.rel` sibling; folded four-mode per-golden pinning (`region opt/nodf/nocf/none`). |
| F-EQGATE-COUNT | testability-oracle | H-EXIT-AMEND | MINOR | Stale "5 eqgate carriers" — 6 at tip; folded corrected count naming `demand_diff_pub_1` + `ls … \| wc -l` re-derivation note. |
| NEC-1 | necessity | ESC-4 / H2 | MINOR (from MAJOR) | Variant (ii) + a separate `lib/Regional` = circular static-lib dependency; folded the placement constraint (frozen types in `lib/DataFlow` under i/ii; `lib/Regional` acyclic only under iii). |
| F2 | correctness-lifecycle | H2-AMEND | INFO (from MINOR) | "full input list" imprecise vs §1.2's eight; folded fix (a) — relabel as the satellite list, note `views+column-edges`/`projection_role` travel inside the query body. |
| TC-1 | termination-confluence | H9-AMEND | INFO (from MINOR) | V-REGION-CENSUS-IDENTITY mis-classed as a FreezeAndValidate scaffold (no Rel op exists pre-`Program::Build`); folded its re-home to the ValidateDROps-tail recount. |
| TC-2 | termination-confluence | H1-AMEND / H4-AMEND | MINOR (orchestrator-adjudicated; refuter agent died) | "The only edit is in CompileModule" under (iii) was overbroad — H4's `Program::Build` signature change (consume `frozen`) applies under every variant; the placement-agnostic seam claim itself HOLDS (base H4's `frozen_opt` clause names the H1-ALT free build). Folded as the (iii) wording qualifier. |

**DELTA-6 calibration statement.** 9 of 12 findings survived (8 refuter-verified + TC-2 orchestrator-adjudicated) — a **75% survival rate**, well ABOVE the ~22% Stage-A calibration target. The excess is diagnostic, not comfortable: the first-pass amendment shipped a real internal contradiction (F-REGION-DEAD: `-region-out` rooted at `query_opt` while H1(iii) claims `frozen` is the only consumer) and two harness-wiring gaps (F-REGION-BLESS, F-REGION-CROSSMODE) that a stricter authoring pass would have caught before critique. All survivors are folded above; NONE remain BLOCKING (the lone BLOCKING candidate, F-REGION-DEAD, softened to MAJOR because the H4 `frozen.Query()==query` identity keeps behavioral correctness fully refereed — the gap was oracle coverage of the new planning code, not a miscompile), and the two INFO survivors are doc-precision nits. The three refuted findings failed on either a false premise (region DOT rendering `role=`; the owner DOT directive explicitly specifying the Stage-B twin) or rediscovery of an already-adjudicated verdict (necessity-audit A-nec-4 INHERENT).
### IMPLEMENTATION RECORD (2026-08-03, session 4 — LANDED)

Owner RATIFIED TO RECOMMENDED (ESC-4 variant (iii); Minimize DEFERRED;
ESC-1/ESC-2; the 4-witness pin set; no `key-invariant=` at B). Landed same
session: `lib/Regional` (acyclic peer; `FrozenRegionalProgram::Build` at the
Main.cpp third slot; G1 `-region-out` + advisory `-region-dot-out`; freeze
scaffolds), H4 (`Program::Build(const FrozenRegionalProgram&,…)`, thin
`frozen.Query()` seam, `context.frozen_census`), the ALWAYS-ON
V-REGION-CENSUS recount at the ValidateDROps tail (stored ==
`DeriveRegionalCensus(query)`), the `run_irgold` `region` surface, and 16
`.region.<mode>` goldens blessed once after review. EXIT GATE MET:
SUITE: PASS (190) with run_refinterp + the 16 region pins + V-REGION-CENSUS
live; ctest 7/7; ZERO pre-existing goldens changed. One predict-verify
divergence adjudicated toward derivability (the Stage-A-#4 pattern):
R-STORE narrowed to insert-materialized relations — see
`regional-dump-stage-b-desired-states.md` §9.7 (the merge-materialized
interior is the logical-origin-provenance necessity witness). Ledger:
KeyedInstances.md §20(AY). Next entry point: `next-session-prompt.md`,
seed `stage-b-landed-seed.md`.
